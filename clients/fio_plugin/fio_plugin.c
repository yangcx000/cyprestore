/*
 * Copyright 2020 JDD authors.
 * @chenjianfei18
 *
 */

#include <math.h>

#include "../libcypre/libcypre_c.h"
#include "config-host.h"
#include "fio.h"
#include "optgroup.h"

struct fio_cs_iou {
    struct thread_data *td;
    struct io_u *io_u;
    aio_completion_t *completion;
};

struct cs_data {
    cyprerbd_t *rbd;
    cypreblob_t *curr_blob;
    struct io_u **aio_events;
    bool connected;
};

// fio configuration options read from the job file
struct cs_options {
    void *pad;  // needed because offset can't be 0 for a option defined used
                // offsetof
    char *host;
    char *blob_id;
    unsigned int port;
    int busy_poll;
};

static struct fio_option options[] = {
    {
            .name = "em_host",
            .lname = "cyprestore extentmanager host",
            .type = FIO_OPT_STR_STORE,
            .off1 = offsetof(struct cs_options, host),
            .help = "ExtentManager host",
            .category = FIO_OPT_C_ENGINE,
            .group = FIO_OPT_G_INVALID,
    },
    {
            .name = "em_port",
            .lname = "cyprestore extentmanager host port",
            .type = FIO_OPT_INT,
            .off1 = offsetof(struct cs_options, port),
            .minval = 1,
            .maxval = 65535,
            .help = "ExtentManager port",
            .category = FIO_OPT_C_ENGINE,
            .group = FIO_OPT_G_INVALID,
    },
    {
            .name = "blob_id",
            .lname = "blob id",
            .type = FIO_OPT_STR_STORE,
            .off1 = offsetof(struct cs_options, blob_id),
            .help = "Blob ID",
            .category = FIO_OPT_C_ENGINE,
            .group = FIO_OPT_G_INVALID,
    },
    {
            .name = "busy_poll",
            .lname = "busy poll mode",
            .type = FIO_OPT_BOOL,
            .help = "Busy poll for completions instead of sleeping, default: 1",
            .off1 = offsetof(struct cs_options, busy_poll),
            .def = "1",
            .category = FIO_OPT_C_ENGINE,
            .group = FIO_OPT_G_INVALID,
    },
    {
            .name = NULL,
    },
};

static int _fio_setup_csio_data(struct thread_data *td, struct cs_data **pcd) {
    struct cs_data *cd;
    if (td->io_ops_data) {
        return 0;
    }
    cd = calloc(1, sizeof(struct cs_data));
    if (!cd) {
        return 1;
    }

    cd->connected = false;
    cd->aio_events = calloc(td->o.iodepth, sizeof(struct io_u *));
    if (!cd->aio_events) {
        free(cd);
        return 1;
    }
    *pcd = cd;
    return 0;
}

static int _cyprestore_setup(struct thread_data *td) {
    struct cs_data *cd = td->io_ops_data;
    struct cs_options *options = td->eo;
    uint64_t blob_size;

    // connetct to cyprestore server.
    cd->rbd = open_cyprerbd(options->host, options->port);
    if (!cd->rbd) {
        log_err("cyprestore: open fail, em_host: %s, em_port: %d\n",
                options->host, options->port);
        return 1;
    }

    // open blob file.
    cd->curr_blob = open_blob(cd->rbd, options->blob_id);
    if (!cd->curr_blob) {
        close_cyprerbd(cd->rbd);
        log_err("cyprestore: open blob fail, blob_id: %s\n", options->blob_id);
        return 1;
    }

    blob_size = get_blob_size(cd->curr_blob);

    /* taken from "net" engine. Pretend we deal with files,
     * even if we do not have any ideas about files.
     * The size of the BLOB is set instead of a artificial file.
     */
    if (!td->files_index) {
        add_file(td, td->o.filename ?: "cs_blob", 0, 0);
        td->o.nr_files = td->o.nr_files ?: 1;
        td->o.open_files++;
    }
    td->files[0]->real_file_size = blob_size;

    return 0;
}

static void fio_csio_cleanup(struct thread_data *td) {
    struct cs_data *cd = td->io_ops_data;
    if (cd) {
        close_blob(cd->rbd, cd->curr_blob);
        close_cyprerbd(cd->rbd);
        free(cd->aio_events);
        free(cd);
    }
}

static enum fio_q_status
fio_csio_queue(struct thread_data *td, struct io_u *io_u) {
    struct cs_data *cd = td->io_ops_data;
    struct fio_cs_iou *fci = io_u->engine_data;
    cypreblob_t *blob = cd->curr_blob;
    int r = -1;

    fio_ro_check(td, io_u);
    if (io_u->ddir == DDIR_WRITE) {
        r = aio_write_blob(
                blob, io_u->xfer_buf, io_u->xfer_buflen, io_u->offset,
                fci->completion);
        if (r < 0) {
            log_err("aio_write_blob failed.\n");
            goto failed;
        }
        return FIO_Q_QUEUED;
    } else if (io_u->ddir == DDIR_READ) {
        r = aio_read_blob(
                blob, io_u->xfer_buf, io_u->xfer_buflen, io_u->offset,
                fci->completion);
        if (r < 0) {
            log_err("aio_read_blob failed.\n");
            goto failed;
        }
        return FIO_Q_QUEUED;
    }
    log_err("WARNING: Only DDIR_READ and DDIR_WRITE are supported! %d",
            io_u->ddir);

failed:
    io_u->error = -r;
    td_verror(td, io_u->error, "xfer");
    return FIO_Q_COMPLETED;
}

static struct io_u *fio_csio_event(struct thread_data *td, int event) {
    struct cs_data *cd = td->io_ops_data;
    return cd->aio_events[event];
}

static int fio_csio_getevents(
        struct thread_data *td, unsigned int min, unsigned int max,
        const struct timespec *t) {
    struct cs_data *cd = td->io_ops_data;
    struct cs_options *o = td->eo;
    int busy_poll = o->busy_poll;
    unsigned int events = 0;
    struct io_u *u;
    struct fio_cs_iou *fci;
    unsigned int i;
    aio_completion_t *first_unfinished;
    bool observed_new = 0;

    // loop through inflight ios until we find 'min' completions
    do {
        first_unfinished = NULL;
        io_u_qiter(&td->io_u_all, u, i) {
            if (!(u->flags & IO_U_F_FLIGHT)) {
                continue;
            }
            fci = u->engine_data;
            if (aio_is_complete(fci->completion)) {
                if (!aio_is_success(fci->completion)) {
                    log_err("aio io fail!\n");
                }
                cd->aio_events[events] = u;
                events++;
                observed_new = 1;
            } else if (first_unfinished == NULL) {
                first_unfinished = fci->completion;
            }
            if (events >= max) {
                break;
            }
        }
        if (events >= min) {
            return events;
        }
        if (first_unfinished == NULL || busy_poll) {
            continue;
        }

        if (!observed_new) {
            aio_wait_for_complete(first_unfinished);
        }
    } while (1);
    return events;
}

static int fio_csio_setup(struct thread_data *td) {
    struct cs_data *cd = NULL;
    int r;

    // allocate engine specific structure.
    r = _fio_setup_csio_data(td, &cd);
    if (r) {
        log_err("_fio_setup_csio_data failed.\n");
        return 1;
    }
    td->io_ops_data = cd;
    td->o.use_thread = 1;

    // connet to cs server.
    r = _cyprestore_setup(td);
    if (r) {
        log_err("cyprestore_init failed.\n");
        return 1;
    }
    cd->connected = true;

    return 0;
}

/* open/invalidate are noops. we set the FIO_DISKLESSIO flag in ioengine_ops to
 * prevent fio from creating the files
 */
static int fio_csio_open(struct thread_data *td, struct fio_file *f) {
    return 0;
}
static int fio_csio_invalidate(struct thread_data *td, struct fio_file *f) {
    return 0;
}

static void fio_csio_io_u_free(struct thread_data *td, struct io_u *io_u) {
    struct fio_cs_iou *fci = io_u->engine_data;
    if (fci != NULL) {
        io_u->engine_data = NULL;
        fci->td = NULL;
        aio_release_completion(fci->completion);
        free(fci);
    }
}

static int fio_csio_io_u_init(struct thread_data *td, struct io_u *io_u) {
    struct fio_cs_iou *fci;
    fci = calloc(1, sizeof(*fci));
    fci->io_u = io_u;
    fci->td = td;
    fci->completion = aio_create_completion();
    io_u->engine_data = fci;
    return 0;
}

// ioengine_ops for get_ioengine()
static struct ioengine_ops ioengine = {
    .name = "cyprestore",
    .version = FIO_IOOPS_VERSION,
    .flags = FIO_DISKLESSIO,
    .setup = fio_csio_setup,
    .queue = fio_csio_queue,
    .getevents = fio_csio_getevents,
    .event = fio_csio_event,
    .cleanup = fio_csio_cleanup,
    .open_file = fio_csio_open,
    .invalidate = fio_csio_invalidate,
    .options = options,
    .io_u_init = fio_csio_io_u_init,
    .io_u_free = fio_csio_io_u_free,
    .option_struct_size = sizeof(struct cs_options),
};

static void fio_init fio_csio_register(void) {
    register_ioengine(&ioengine);
}

static void fio_exit fio_csio_unregister(void) {
    unregister_ioengine(&ioengine);
}
