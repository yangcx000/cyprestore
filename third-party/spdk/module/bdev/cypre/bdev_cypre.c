/*-
 *   BSD LICENSE
 *
 *   Copyright (c) Intel Corporation. All rights reserved.
 *   Copyright (c) 2019 Mellanox Technologies LTD. All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "spdk/stdinc.h"

#include "spdk/bdev.h"
#include "spdk/conf.h"
#include "spdk/env.h"
#include "spdk/thread.h"
#include "spdk/json.h"
#include "spdk/string.h"
#include "spdk/likely.h"

#include "spdk/bdev_module.h"
#include "spdk_internal/log.h"

#include "bdev_cypre.h"

struct cypre_bdev {
	struct spdk_bdev	bdev;
	TAILQ_ENTRY(cypre_bdev)	tailq;
};

struct cypre_io_channel {
	struct spdk_poller		*poller;
	TAILQ_HEAD(, spdk_bdev_io)	io;
};

static TAILQ_HEAD(, cypre_bdev) g_cypre_bdev_head;
static void *g_cypre_read_buf;

static int bdev_cypre_initialize(void);
static void bdev_cypre_finish(void);
static void bdev_cypre_get_spdk_running_config(FILE *fp);

static struct spdk_bdev_module cypre_if = {
	.name = "cypre",
	.module_init = bdev_cypre_initialize,
	.module_fini = bdev_cypre_finish,
	.config_text = bdev_cypre_get_spdk_running_config,
	.async_fini = true,
};

SPDK_BDEV_MODULE_REGISTER(cypre, &cypre_if)

static int
bdev_cypre_destruct(void *ctx)
{
	struct cypre_bdev *bdev = ctx;

	TAILQ_REMOVE(&g_cypre_bdev_head, bdev, tailq);
	free(bdev->bdev.name);
	free(bdev);

	return 0;
}

static void
bdev_cypre_submit_request(struct spdk_io_channel *_ch, struct spdk_bdev_io *bdev_io)
{
	struct cypre_io_channel *ch = spdk_io_channel_get_ctx(_ch);
	struct spdk_bdev *bdev = bdev_io->bdev;
	struct spdk_dif_ctx dif_ctx;
	struct spdk_dif_error err_blk;
	int rc;

	if (SPDK_DIF_DISABLE != bdev->dif_type &&
	    (SPDK_BDEV_IO_TYPE_READ == bdev_io->type ||
	     SPDK_BDEV_IO_TYPE_WRITE == bdev_io->type)) {
		rc = spdk_dif_ctx_init(&dif_ctx,
				       bdev->blocklen,
				       bdev->md_len,
				       bdev->md_interleave,
				       bdev->dif_is_head_of_md,
				       bdev->dif_type,
				       bdev->dif_check_flags,
				       bdev_io->u.bdev.offset_blocks & 0xFFFFFFFF,
				       0xFFFF, 0, 0, 0);
		if (0 != rc) {
			SPDK_ERRLOG("Failed to initialize DIF context, error %d\n", rc);
			spdk_bdev_io_complete(bdev_io, SPDK_BDEV_IO_STATUS_FAILED);
			return;
		}
	}

	switch (bdev_io->type) {
	case SPDK_BDEV_IO_TYPE_READ:
		if (bdev_io->u.bdev.iovs[0].iov_base == NULL) {
			assert(bdev_io->u.bdev.iovcnt == 1);
			if (spdk_likely(bdev_io->u.bdev.num_blocks * bdev_io->bdev->blocklen <=
					SPDK_BDEV_LARGE_BUF_MAX_SIZE)) {
				bdev_io->u.bdev.iovs[0].iov_base = g_cypre_read_buf;
				bdev_io->u.bdev.iovs[0].iov_len = bdev_io->u.bdev.num_blocks * bdev_io->bdev->blocklen;
			} else {
				SPDK_ERRLOG("Overflow occurred. Read I/O size %" PRIu64 " was larger than permitted %d\n",
					    bdev_io->u.bdev.num_blocks * bdev_io->bdev->blocklen,
					    SPDK_BDEV_LARGE_BUF_MAX_SIZE);
				spdk_bdev_io_complete(bdev_io, SPDK_BDEV_IO_STATUS_FAILED);
				return;
			}
		}
		if (SPDK_DIF_DISABLE != bdev->dif_type) {
			rc = spdk_dif_generate(bdev_io->u.bdev.iovs, bdev_io->u.bdev.iovcnt,
					       bdev_io->u.bdev.num_blocks, &dif_ctx);
			if (0 != rc) {
				SPDK_ERRLOG("IO DIF generation failed: lba %lu, num_block %lu\n",
					    bdev_io->u.bdev.offset_blocks,
					    bdev_io->u.bdev.num_blocks);
				spdk_bdev_io_complete(bdev_io, SPDK_BDEV_IO_STATUS_FAILED);
				return;
			}
		}
		TAILQ_INSERT_TAIL(&ch->io, bdev_io, module_link);
		break;
	case SPDK_BDEV_IO_TYPE_WRITE:
		if (SPDK_DIF_DISABLE != bdev->dif_type) {
			rc = spdk_dif_verify(bdev_io->u.bdev.iovs, bdev_io->u.bdev.iovcnt,
					     bdev_io->u.bdev.num_blocks, &dif_ctx, &err_blk);
			if (0 != rc) {
				SPDK_ERRLOG("IO DIF verification failed: lba %lu, num_blocks %lu, "
					    "err_type %u, expected %u, actual %u, err_offset %u\n",
					    bdev_io->u.bdev.offset_blocks,
					    bdev_io->u.bdev.num_blocks,
					    err_blk.err_type,
					    err_blk.expected,
					    err_blk.actual,
					    err_blk.err_offset);
				spdk_bdev_io_complete(bdev_io, SPDK_BDEV_IO_STATUS_FAILED);
				return;
			}
		}
		TAILQ_INSERT_TAIL(&ch->io, bdev_io, module_link);
		break;
	case SPDK_BDEV_IO_TYPE_WRITE_ZEROES:
	case SPDK_BDEV_IO_TYPE_RESET:
		TAILQ_INSERT_TAIL(&ch->io, bdev_io, module_link);
		break;
	case SPDK_BDEV_IO_TYPE_FLUSH:
	case SPDK_BDEV_IO_TYPE_UNMAP:
	default:
		spdk_bdev_io_complete(bdev_io, SPDK_BDEV_IO_STATUS_FAILED);
		break;
	}
}

static bool
bdev_cypre_io_type_supported(void *ctx, enum spdk_bdev_io_type io_type)
{
	switch (io_type) {
	case SPDK_BDEV_IO_TYPE_READ:
	case SPDK_BDEV_IO_TYPE_WRITE:
	case SPDK_BDEV_IO_TYPE_WRITE_ZEROES:
	case SPDK_BDEV_IO_TYPE_RESET:
		return true;
	case SPDK_BDEV_IO_TYPE_FLUSH:
	case SPDK_BDEV_IO_TYPE_UNMAP:
	default:
		return false;
	}
}

static struct spdk_io_channel *
bdev_cypre_get_io_channel(void *ctx)
{
	return spdk_get_io_channel(&g_cypre_bdev_head);
}