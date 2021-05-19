
#include <brpc/channel.h>
#include <brpc/server.h>

#include <iostream>

#include "common/extent_id_generator.h"
#include "libcypre.h"
#include "libcypre_common.h"
#include "mock/mock_service.h"

using namespace std;
using namespace cyprestore;
using namespace cyprestore::clients;
using namespace cyprestore::clients::mock;

class MockServer {
public:
    MockServer(const std::string &ip, int port)
            : ip_(ip), mport_(port), mock_(NULL) {}
    ~MockServer() {
        if (mock_ != NULL) {
            mock_->Stop();
        }
        delete mock_;
    }
    int Start(uint64_t dev_size, const std::string &blob_name, bool nullblk);

private:
    std::string ip_;
    int mport_;
    MockInstance *mock_;
};

int MockServer::Start(
        uint64_t dev_size, const std::string &blob_name, bool nullblk) {
    MockExtentManager *mem = NULL;
    MockLogicManager *mlm = new MockLogicManager();
    if (nullblk) {
        mem = new MockEmptyExtentManager();
    } else {
        mem = new MockMemExtentManager();
    }
    mock_ = new MockInstance(mlm, mem);
    int rv = mock_->StartMaster("0.0.0.0", mport_);
    if (rv != 0) {
        cout << "start master failed:, port:" << mport_;
        return rv;
    }
    std::vector<MockInstance::Address> lles = { { ip_, mport_ + 1 },
                                                { ip_, mport_ + 2 },
                                                { ip_, mport_ + 3 },
                                                { ip_, mport_ + 4 } };
    rv = mock_->StartServer(lles);
    if (rv != 0) {
        cout << "start server failed:, port:" << mport_ + 1 << "-" << mport_ + 4
             << endl;
        return rv;
    }
    MockMasterClient client;
    brpc::Controller cntl;
    char addr[100];
    snprintf(addr, sizeof(addr) - 1, "%s:%d", ip_.data(), mport_);
    rv = client.Init(addr);
    if (rv != 0) {
        cout << "connect to manager failed:" << addr;
        return rv;
    }
    // create user & blob
    std::string uid1;
    rv = client.CreateUser("sk1", "sk1@jd.com", uid1);
    if (rv != 0 || uid1.empty()) {
        cout << "create user failed.";
        return rv;
    }
    // create blob
    std::string bid1;
    rv = client.CreateBlob(
            uid1, blob_name, blob_name + ".test",
            common::pb::BlobType::BLOB_TYPE_EXCELLENT, dev_size, bid1);
    if (rv != 0 || bid1.empty()) {
        cout << "createblob failed";
        return rv;
    }
    cout << "create blob done, blob id: " << bid1 << endl;
    return rv;
}

static uint64_t param2size(const char *param) {
    uint64_t s = 0;
    int len = strlen(param);
    if (len == 0) {
        return 0;
    }
    s = atoi(param);
    if (param[len - 1] == 'M' || param[len - 1] == 'm') {
        s *= 1024 * 1024;
    } else if (param[len - 1] == 'G' || param[len - 1] == 'g') {
        s *= 1024 * 1024 * 1024;
    }
    return s;
}
static int help() {
    cout << "usage: <addr> <port> <device size> [nulblk]" << endl;
    cout << "size can use M/G" << endl;
    return -1;
}
const std::string DEVICE_NAME = "mockblock123";
int main(int argc, char **argv) {
    if (argc != 4 && argc != 5) {
        return help();
    }
    if (argc == 5 && strcmp(argv[4], "nulblk") != 0) {
        return help();
    }
    bool nulblk = false;
    if (argc == 5) {
        nulblk = true;
        cout << "use null block for test" << endl;
    }
    brpc::StartDummyServerAt(80);
    int64_t devsize = param2size(argv[3]);
    cout << "starting service, addr=" << argv[1] << ":" << argv[2] << endl;
    cout << "test device size: " << (devsize / 1024 / 1024) << "MB, ";
    cout << "device name: " << DEVICE_NAME << endl;
    GlobalConfig::Instance()->SetExtentSize(1024 * 1024 * 256);
    MockServer ms(argv[1], atoi(argv[2]));
    int rv = ms.Start(devsize, DEVICE_NAME, nulblk);
    if (rv != 0) {
        cout << "start server failed, return:" << rv << endl;
        return -1;
    }
    cout << "service start ok." << endl;
    while (true) {
        sleep(1);
    }
    return 0;
}
