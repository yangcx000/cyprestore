#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <memory>

const std::string kTestImage = "test:/test";  // NOLINT
const int64_t kGB = 1024 * 1024 * 1024;

void TestRandWrite(const char *nbddevname, const char *filename, int loop) {
    int nbdfd = open(nbddevname, O_RDWR | O_SYNC);
    // int nbdfd =open(nbddevname, O_RDWR|O_DIRECT,606); //
    if (nbdfd <= 0) {
        printf("open failed %s  %s\r\n", nbddevname, strerror(errno));
        return;
    }
    int fd = open(filename, O_RDWR | O_CREAT);
    if (fd <= 0) {
        printf("open   %s  %s\r\n", filename, strerror(errno));
        return;
    }

    const int BufSize = 1048576;
    char buf[BufSize] = { 0 };
    uint64_t offset = 0;
    ssize_t r;

    //   if (lseek(nbdfd, 1024 ,SEEK_SET) == -1 )
    //	printf("lseek failed\r\n",nbddevname,strerror(errno));

    //  r=write(nbdfd, buf, 512);
    r = pwrite(nbdfd, buf, 512, 0);
    if (r <= 0) {
        printf("pwrite nbdfd=%d failed %s,nbddevname=%s\r\n", nbdfd,
               strerror(errno), nbddevname);
        return;
    } else
        printf("pwrite 4k success\r\n");

    printf("write  0 data \r\n");
    for (int i = 0; i < 1024; i++) {
        offset = 1048576 * i;
        r = pwrite(nbdfd, buf, BufSize, offset);
        if (r <= 0) {
            printf("pwrite nbdfd failed \r\n");
            return;
        }

        r = pwrite(fd, buf, BufSize, offset);
        if (r <= 0) {
            printf("pwrite nbdfd failed \r\n");
            return;
        }
        if (i % 100 == 0) printf("write  0 data  %d\r\n", i);
    }

    printf("rand  write loop=%d\r\n", loop);
    uint64_t BlobSize = 1048576 * 1024;
    int len = 0;
    srand(time(NULL));
    char nbdbuf[BufSize] = { 0 };
    char filebuf[BufSize] = { 0 };

    for (int i = 0; i < loop; i++) {
        memset(buf, i, BufSize);
        offset = (rand() * rand()) % BlobSize;
        len = rand() % BufSize;

        r = pwrite(nbdfd, buf, len, offset);
        if (r <= 0 || r != len) {
            printf("pwrite nbdfd failed 1 %s, ret=%d\r\n", strerror(errno), r);
            break;
        }

        r = pread(nbdfd, nbdbuf, len, offset);
        if (r <= 0 || r != len) {
            printf("pread nbdfd failed 1 %s, ret=%d\r\n", strerror(errno), r);
            break;
        }

        r = pwrite(fd, buf, len, offset);
        if (r <= 0 || r != len) {
            printf("pwrite fd failed 1%s, ret=%d\r\n", strerror(errno), r);
            break;
        }
        /*
                r=pread(fd, filebuf, len, offset);
                if(r<=0|| r!=len)
                {
                    printf("pread fd failed 1 %s, ret=%d\r\n",strerror(errno),
           r); break;
                }
        */
        if (memcmp(nbdbuf, buf, len) != 0) {
            printf("memcmp(nbdbuf,filebuf,len)!=0 offset=%lu, len=%d\r\n",
                   offset, len);
            break;
        }

        if (i % 100 == 0) printf("test loop  %d\r\n", i);
    }

    close(nbdfd);
    close(fd);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("useage: verify_test  /dev/nbd1  /mnt/loop.img\r\n");
        return 0;
    }

    char *nbddevname = argv[1];
    char *filename = argv[2];
    int loop = 1000;

    if (argc > 3) loop = atoi(argv[3]);
    TestRandWrite(nbddevname, filename, loop);

    return 0;
}
