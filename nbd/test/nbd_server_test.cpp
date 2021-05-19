
#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <memory>

#include "fake_safe_io.h"
#include "src/BlobInstance.h"
#include "src/NBDServer.h"

namespace cyprestore {
namespace nbd {

const int64_t kSleepTime = 500;  // ms

class NBDServerTest : public ::testing::Test {
public:
    void SetUp() override {
        ASSERT_NE(-1, socketpair(AF_UNIX, SOCK_STREAM, 0, fd_));

        blob_ = std::make_shared<BlobInstance>("172.17.60.29", 8080);
        safeIO_ = std::make_shared<SafeIO>();
        server_.reset(new NBDServer(fd_[1], blob_, nullptr));
    }

    void TearDown() override {
        ::shutdown(fd_[0], SHUT_RDWR);
    }

protected:
    int fd_[2];
    std::shared_ptr<BlobInstance> blob_;
    std::shared_ptr<SafeIO> safeIO_;
    std::unique_ptr<NBDServer> server_;
    char handle_[8] = { 0 };

    struct nbd_request request_;
    struct nbd_reply reply_;

    const uint32_t NBDRequestSize = sizeof(request_);
    const uint32_t NBDReplySize = sizeof(reply_);
};
/*
TEST_F(NBDServerTest, InvalidCommandTypeTest) {

  ASSERT_NO_THROW(server_->Start());

  request_.from = 0;
  request_.len = htonl(8);
  request_.type = htonl(100); // 设置一个无效的请求类型
  request_.magic = htonl(NBD_REQUEST_MAGIC);
  printf("1\r\n");
  memcpy(&request_.handle, &handle_, sizeof(request_.handle));
  printf("2\r\n");
  ASSERT_EQ(NBDRequestSize, write(fd_[0], &request_, NBDRequestSize));
  printf("2a\r\n");
  std::this_thread::sleep_for(std::chrono::milliseconds(kSleepTime));
  printf("3\r\n");
  if (server_ == nullptr)
    printf("server_==nullptr\r\n");
  ASSERT_TRUE(server_->IsTerminated());
}

TEST_F(NBDServerTest, InvalidRequestMagicTest) {
  ASSERT_NO_THROW(server_->Start());

  request_.from = 0;
  request_.len = htonl(8);
  request_.type = htonl(NBD_CMD_READ);
  request_.magic = htonl(0); // 设置一个错误的magic
  memcpy(&request_.handle, &handle_, sizeof(request_.handle));

  ASSERT_EQ(NBDRequestSize, write(fd_[0], &request_, NBDRequestSize));

  std::this_thread::sleep_for(std::chrono::milliseconds(kSleepTime));

  ASSERT_TRUE(server_->IsTerminated());
}
*/
TEST_F(NBDServerTest, AioReadTest) {
    printf("1\r\n");
    ASSERT_NO_THROW(server_->Start());
    printf("2\r\n");
    request_.from = 0;
    request_.len = htonl(8);
    request_.type = htonl(NBD_CMD_READ);
    request_.magic = htonl(NBD_REQUEST_MAGIC);
    memcpy(&request_.handle, &handle_, sizeof(request_.handle));

    printf("3\r\n");
    IOContext Context;
    blob_->Read(&Context);
    printf("4\r\n");
    ASSERT_EQ(NBDRequestSize, write(fd_[0], &request_, NBDRequestSize));

    printf("5\r\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(kSleepTime));
    printf("4\r\n");
    ASSERT_EQ(Context.bdIoCtx.offset, request_.from);
    ASSERT_EQ(Context.bdIoCtx.length, ntohl(request_.len));
    ASSERT_EQ(Context.bdIoCtx.op, LIBAIO_OP::LIBAIO_OP_READ);

    memcpy(Context.data.get(), handle_, sizeof(handle_));
    Context.bdIoCtx.cb(0, &Context);

    char readbuf[8];
    ASSERT_EQ(NBDReplySize, read(fd_[0], &reply_, NBDReplySize));
    ASSERT_EQ(sizeof(readbuf), read(fd_[0], readbuf, sizeof(readbuf)));

    ASSERT_EQ(0, memcmp(readbuf, handle_, sizeof(handle_)));
}

TEST_F(NBDServerTest, AioWriteTest) {
    ASSERT_NO_THROW(server_->Start());

    request_.from = 0;
    request_.len = htonl(8);
    request_.type = htonl(NBD_CMD_WRITE);
    request_.magic = htonl(NBD_REQUEST_MAGIC);
    memcpy(&request_.handle, &handle_, sizeof(request_.handle));

    IOContext Context;
    blob_->Write(&Context);

    ASSERT_EQ(NBDRequestSize, write(fd_[0], &request_, NBDRequestSize));
    ASSERT_EQ(8, write(fd_[0], "hello, world", 8));

    std::this_thread::sleep_for(std::chrono::milliseconds(kSleepTime));

    ASSERT_EQ(Context.bdIoCtx.offset, request_.from);
    ASSERT_EQ(Context.bdIoCtx.length, ntohl(request_.len));
    ASSERT_EQ(Context.bdIoCtx.op, LIBAIO_OP::LIBAIO_OP_WRITE);

    memcpy(Context.data.get(), handle_, sizeof(handle_));
    Context.bdIoCtx.cb(0, &Context);

    ASSERT_EQ(NBDReplySize, read(fd_[0], &reply_, NBDReplySize));
    ASSERT_EQ(0, reply_.error);
}

TEST_F(NBDServerTest, DisconnectTest) {
    ASSERT_NO_THROW(server_->Start());

    request_.from = 0;
    request_.len = htonl(8);
    request_.type = htonl(NBD_CMD_DISC);
    request_.magic = htonl(NBD_REQUEST_MAGIC);
    memcpy(&request_.handle, &handle_, sizeof(request_.handle));

    ASSERT_EQ(NBDRequestSize, write(fd_[0], &request_, NBDRequestSize));

    std::this_thread::sleep_for(std::chrono::milliseconds(kSleepTime));

    ASSERT_TRUE(server_->IsTerminated());
}

TEST_F(NBDServerTest, ReadWriteDataErrorTest) {
    auto fakeSafeIO = std::make_shared<FakeSafeIO>();
    server_.reset(new NBDServer(fd_[1], nullptr, fakeSafeIO));

    request_.from = 0;
    request_.len = htonl(8);
    request_.type = htonl(NBD_CMD_WRITE);
    request_.magic = htonl(NBD_REQUEST_MAGIC);
    memcpy(&request_.handle, &handle_, sizeof(request_.handle));

    IOContext Context;
    blob_->Write(&Context);

    auto task = [this](int fd, void *buf, size_t count) {
        static int callTime = 1;
        if (callTime++ == 1) {
            *reinterpret_cast<struct nbd_request *>(buf) = request_;
            return 0;
        } else {
            return -1;
        }
    };

    fakeSafeIO->SetReadExactTask(task);

    ASSERT_NO_THROW(server_->Start());

    std::this_thread::sleep_for(std::chrono::milliseconds(kSleepTime));

    ASSERT_TRUE(server_->IsTerminated());
}

}  // namespace nbd
}  // namespace cyprestore
