/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */

/*
* 支持的对象和命令
* 1. User
*    1.1 创建
*    1.2 删除
*    1.3 查询
*    1.4 列表
*    1.5 重命名
* 2. Blob
*    2.1 创建
*    2.2 删除
*    2.3 查询
*    2.4 列表
*    2.5 重命名
*    2.6 扩容
* 3. Extent
*    3.1 顺序读
*    3.2 随机读
*    3.3 顺序写
*    3.4 随机写
*    3.5 选项 blocksize/jobs/iodepth
*

Extent
std::string extent_id;
int64_t offset;
int64_t size;
int32_t block_size;
int32_t iodepth;
int32_t jobs;

Brpc
std::string protocal;
std::string connection_type;
int32_t timeout_ms;
int32_t connection_timeout_ms;
int32_t max_retry;
int32_t dummy_port;


*/

#include <brpc/channel.h>
#include <butil/logging.h>
#include <gflags/gflags.h>

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

#include "access/pb/access.pb.h"
#include "blobplus.h"
#include "common/pb/types.pb.h"
#include "multhread.h"
#include "plus.h"
#include "rpcclient/rpcclient_access.h"
#include "stringdict.h"

using namespace cyprestore::common::pb;
using namespace cyprestore;

typedef std::map<string, string> StringMap;

StringDict g_config;
std::string g_AccessIp = "172.17.60.29";
int g_AccessPort = 6000;

std::string g_ExtentManagerIp;
int g_ExtentManagerPort = 63000;

RpcClientAccess rpcclientaccess;

bool CheckExtentManager(const std::string &user_id);
void Read(StringDict &args);
void RandRead(StringDict &args);
void Write(StringDict &args);
void RandWrite(StringDict &args);
void AsyncWrite(StringDict &args);

void test();

template <class T>
struct DisableCompare : public std::binary_function<T, T, bool> {
    bool operator()(T lhs, T rhs) const {
        static bool disblecompare = false;
        if (lhs == rhs) {
            return false;
        }
        if (disblecompare) {
            disblecompare = false;
            return false;
        } else {
            disblecompare = true;
            return true;
        }
    }
};

int main(int argc, char **argv) {
    StringDict Usage;
    // std::map<string, string, DisableCompare<string>> Usage;
    Usage["createuser"] = "name= email= comments= ";
    Usage["deleteuser"] = "userid=";
    Usage["renameuser"] = "user_id= new_name=";
    Usage["queryuser"] = "user_id=";
    Usage["listuser"] = "set_name=";
    Usage["createblob"] = "user_id= blob_name= blob_size= blob_type= "
                          "instance_id= blob_desc= pool_id=";
    Usage["deleteblob"] = "user_id= blob_id=";
    Usage["renameblob"] = "user_id=  blob_id=  new_name=";
    Usage["resizeblob"] = "user_id= blob_id= blob_size=";
    Usage["queryblob"] = "user_id= blob_id=";
    Usage["listblobs"] = "user_id= from= count=";
    Usage["read"] = "blob_id= offset= readlen= jobs=";
    Usage["randread"] = "blob_id=  readlen= jobs=";
    Usage["write"] = "blob_id= offset= writelen= jobs=";
    Usage["randwrite"] = "blob_id=  readlen= jobs=";

    if (!g_config.LoadFromFile("cypreclient.ini")) {
        std::cerr << "Load config file  cypreclient.ini failed: " << std::endl;
        return 1;
    }

    g_AccessIp = g_config.Get("access.ip");
    g_AccessPort = g_config.GetInt("access.port");

    if (rpcclientaccess.Init(g_AccessIp, g_AccessPort) != STATUS_OK) return (0);

    // StringMap  args;
    StringDict args;
    string fun, name, value;
    if (argc > 1) fun = argv[1];

    if (argc > 2)
        for (int i = 2; i < argc; i++) {
            SplitStr(argv[i], '=', name, value);
            args[name] = value;
        }

    StringToLower(fun);
    std::string errmsg = "";

    if (argc == 2) {
        std::cout << fun << " " << Usage[fun] << "\r\n" << endl;
        return (0);
    }

    if (fun == "test") {
        test();
        return (0);
    } else if (fun == "queryset") {
        const std::string user_id = args.Get("user_id");
        cyprestore::common::pb::SetRoute router;
        if (STATUS_OK == rpcclientaccess.QuerySet(user_id, router, errmsg))
            printf("QuerySet success user_id=%s, set_id=%d, name=%s, ip=%s,  "
                   "port=%d\r\n",
                   user_id.c_str(), router.set_id(), router.set_name().c_str(),
                   router.ip().c_str(), router.port());
        else
            printf("QuerySet failed %s\r\n", errmsg.c_str());
    } else if (fun == "createuser") {
        std::string user_id;
        int type = args.GetInt("pool_type");
        printf("pool_type=%d\r\n", type);
        cyprestore::common::pb::PoolType pool_type = (PoolType)type;

        if (STATUS_OK
            == rpcclientaccess.CreateUser(
                    args["name"], args["email"], args["comments"],
                    args.GetInt("set_id"), args["pool_id"], pool_type, user_id,
                    errmsg))
            printf("CreateUser success user_id: %s\r\n", user_id.c_str());
        else
            printf("CreateUser failed %s\r\n", errmsg.c_str());
    } else if (fun == "deleteuser") {
        if (STATUS_OK == rpcclientaccess.DeleteUser(args["user_id"], errmsg))
            printf("DeleteUser success user_id: %s\r\n",
                   args["user_id"].c_str());
        else
            printf("DeleteUser failed %s\r\n", errmsg.c_str());
    } else if (fun == "renameuser") {
        if (STATUS_OK
            == rpcclientaccess.RenameUser(
                    args["user_id"], args["new_name"], errmsg))
            printf("RenameUser success user_id: %s\r\n",
                   args["user_id"].c_str());
        else
            printf("RenameUser failed %s\r\n", errmsg.c_str());
    } else if (fun == "queryuser") {
        User userinfo;
        if (STATUS_OK
            == rpcclientaccess.QueryUser(args["user_id"], userinfo, errmsg))
            printf("QueryUser success user_id: %s, name=%s, email=%s, "
                   "comments=%s\r\n",
                   args["user_id"].c_str(), userinfo.name().c_str(),
                   userinfo.email().c_str(), userinfo.comments().c_str());
        else
            printf("QueryUser failed %s\r\n", errmsg.c_str());
    } else if (fun == "listuser") {
        std::list<User> Users;
        if (STATUS_OK
            == rpcclientaccess.ListUsers(
                    args.GetInt("set_id"), 0, 1000, Users, errmsg)) {
            printf("ListUsers success Users count: %d \r\n", (int)Users.size());
            std::list<User>::iterator iter;
            for (iter = Users.begin(); iter != Users.end(); iter++) {
                std::cout << iter->id() + " , " << iter->name() + " , "
                          << iter->email() + " , " << iter->comments() + " , "
                          << " ,status=" + iter->status()
                          << " ,set_id=" + iter->set_id()
                          << " ,pool_id=" + iter->pool_id()
                          << " ,create_date=" + iter->create_date()
                          << " ,update_date=" + iter->update_date() + "\r\n"
                          << std::endl;
            }
        } else
            printf("ListUsers failed  %s\r\n", errmsg.c_str());
    } else if (fun == "createblob") {
        google::protobuf::uint64 blob_size = args.GetInt("blob_size");
        BlobType blob_type = (BlobType)args.GetInt("blob_type");
        std::string blob_id = "";

        if (STATUS_OK
            == rpcclientaccess.CreateBlob(
                    args["user_id"], args["blob_name"], blob_size, blob_type,
                    args["instance_id"], args["blob_desc"], args["pool_id"],
                    blob_id, errmsg))
            printf("CreateBlob success blob_id: %s\r\n", blob_id.c_str());
        else
            printf("CreateBlob failed   %s\r\n", errmsg.c_str());
    } else if (fun == "deleteblob") {
        if (STATUS_OK
            == rpcclientaccess.DeleteBlob(
                    args["user_id"], args["blob_id"], errmsg))
            printf("DeleteBlob success blob_id: %s\r\n",
                   args["blob_id"].c_str());
        else
            printf("DeleteBlob failed   %s\r\n", errmsg.c_str());
    } else if (fun == "renameblob") {
        if (STATUS_OK
            == rpcclientaccess.RenameBlob(
                    args["user_id"], args["blob_id"], args["new_name"], errmsg))
            printf("RenameBlob success blob_id: %s\r\n",
                   args["blob_id"].c_str());
        else
            printf("RenameBlob failed   %s\r\n", errmsg.c_str());
    } else if (fun == "resizeblob") {
        google::protobuf::uint64 blob_size = args.GetInt("blob_size");
        if (STATUS_OK
            == rpcclientaccess.ResizeBlob(
                    args["user_id"], args["blob_id"], blob_size, errmsg))
            printf("ResizeBlob success blob_id: %s\r\n",
                   args["blob_id"].c_str());
        else
            printf("ResizeBlob failed   %s\r\n", errmsg.c_str());
    } else if (fun == "queryblob") {
        Blob blob;
        if (STATUS_OK
            == rpcclientaccess.QueryBlob(
                    args["user_id"], args["blob_id"], blob, errmsg)) {
            string typestr = BlobTypeToString(blob.type());
            string statusstr = BlobStatusToString(blob.status());
            std::cout << "QueryBlob success blob_id: " << args["blob_id"]
                      << "\r\n name: " << blob.name()
                      << "\r\n size: " << blob.size()
                      << "\r\n type: " << typestr
                      << "\r\n desc: " << blob.desc()
                      << "\r\n status:  " << statusstr
                      << "\r\n pool_id:  " << blob.pool_id()
                      << "\r\n user_id:  " << blob.user_id()
                      << "\r\n instance_id:  " << blob.instance_id()
                      << "\r\n create_date:  " << blob.create_date()
                      << "\r\n update_date:  " << blob.update_date() << "\r\n"
                      << std::endl;
        } else
            printf("QueryBlob failed   %s\r\n", errmsg.c_str());
    } else if (fun == "listblobs") {
        int from = args.GetInt("from");
        int count = args.GetInt("count");
        if (count <= 0) count = 100;

        std::list<Blob> Blobs;
        if (STATUS_OK
            == rpcclientaccess.ListBlobs(
                    args["user_id"], from, count, Blobs, errmsg)) {
            printf("ListBlobs success Users count: %d \r\n", (int)Blobs.size());

            for (auto iter = Blobs.begin(); iter != Blobs.end(); iter++) {
                std::cout << iter->id() << " ,name:" + iter->name()
                          << " ,size:" << iter->size()
                          << " ,type:" << iter->type()
                          << " ,desc:" + iter->desc()
                          << " ,status:" << BlobStatusToString(iter->status())
                          << " ,pool_id:" + iter->pool_id()
                          << " ,user_id:" + iter->user_id()
                          << " ,instance_id:" + iter->instance_id()
                          << " ,create_date:" + iter->create_date()
                          << " ,update_date:" + iter->update_date() + "\r\n"
                          << std::endl;
            }
        } else
            printf("ListBlobs failed  %s\r\n", errmsg.c_str());
    } else if (fun == "read") {
        Read(args);
    } else if (fun == "randread") {
        RandRead(args);
    } else if (fun == "write") {
        Write(args);
    } else if (fun == "randwrite") {
        RandWrite(args);
    } else if (fun == "asyncwrite") {
        AsyncWrite(args);
    } else {
        printf("usage:  cypreclient command paramname1 = value1 paramname2 = "
               "value2 ...  \r\n");
        std::cout << Usage.BuildToText() << endl;
    }
}

bool CheckExtentManager(const std::string &user_id) {
    if (g_ExtentManagerIp.length() <= 0 || g_ExtentManagerPort <= 0) {
        // const std::string user_id = g_config.Get("user_id");
        cyprestore::common::pb::SetRoute router;
        std::string errmsg;
        if (STATUS_OK != rpcclientaccess.QuerySet(user_id, router, errmsg))
            return (false);
        g_ExtentManagerIp = router.ip();
        g_ExtentManagerPort = router.port();

        printf("rpcclientaccess.QuerySet g_ExtentManagerIp=%s, "
               "g_ExtentManagerPort=%d\r\n",
               g_ExtentManagerIp.c_str(), g_ExtentManagerPort);
    }

    return (true);
}

void test() {
    g_AccessIp = g_config.Get("access.ip");
    g_AccessPort = g_config.GetInt("access.port");

    if (rpcclientaccess.Init(g_AccessIp, g_AccessPort) != STATUS_OK) return;

    // user
    std::string user_name = "test";
    std::string email = "123@jd.com";
    std::string comments = "说明";
    std::string errmsg = "";
    std::string user_id;
    cyprestore::common::pb::PoolType pool_type = POOL_TYPE_HDD;

    if (STATUS_OK
        == rpcclientaccess.CreateUser(
                user_name, email, comments, 0, "", pool_type, user_id, errmsg))
        printf("CreateUser success user_id: %s\r\n", user_id.c_str());
    else
        printf("CreateUser failed %s\r\n", errmsg.c_str());

    if (STATUS_OK == rpcclientaccess.RenameUser(user_id, "test1", errmsg))
        printf("RenameUser success user_id: %s\r\n", user_id.c_str());
    else
        printf("RenameUser failed %s\r\n", errmsg.c_str());

    User userinfo;
    if (STATUS_OK == rpcclientaccess.QueryUser(user_id, userinfo, errmsg))
        printf("QueryUser success user_id: %s, name=%s\r\n", user_id.c_str(),
               userinfo.name().c_str());
    else
        printf("QueryUser failed %s\r\n", errmsg.c_str());

    std::string set_name = "set_name";
    int set_id = 0;
    std::list<User> Users;
    if (STATUS_OK == rpcclientaccess.ListUsers(set_id, 0, 100, Users, errmsg)) {
        printf("ListUsers success Users count: %d \r\n", (int)Users.size());
        std::list<User>::iterator iter;
        for (iter = Users.begin(); iter != Users.end(); iter++) {
            std::cout << iter->name() << "\r\n" << std::endl;
        }
    } else
        printf("ListUsers failed  %s\r\n", errmsg.c_str());

    if (STATUS_OK == rpcclientaccess.DeleteUser(user_id, errmsg))
        printf("DeleteUser success user_id: %s\r\n", user_id.c_str());
    else
        printf("DeleteUser failed  %s\r\n", errmsg.c_str());

    // blob
    std::string blob_name = "blob_name";
    uint64_t blob_size = 10240;
    BlobType blob_type = BLOB_TYPE_GENERAL;
    std::string instance_id = "";
    std::string blob_desc = "description";
    std::string pool_id = "pool_name";
    std::string blob_id = "";

    if (STATUS_OK
        == rpcclientaccess.CreateBlob(
                user_id, blob_name, blob_size, blob_type, instance_id,
                blob_desc, pool_id, blob_id, errmsg))
        printf("CreateBlob success blob_id: %s\r\n", blob_id.c_str());
    else
        printf("CreateBlob failed   %s\r\n", errmsg.c_str());

    if (STATUS_OK
        == rpcclientaccess.RenameBlob(user_id, blob_id, blob_name, errmsg))
        printf("RenameBlob success blob_id: %s\r\n", blob_id.c_str());
    else
        printf("RenameBlob failed   %s\r\n", errmsg.c_str());

    int64_t size = 8349;
    if (STATUS_OK == rpcclientaccess.ResizeBlob(user_id, blob_id, size, errmsg))
        printf("ResizeBlob success blob_id: %s\r\n", blob_id.c_str());
    else
        printf("ResizeBlob failed   %s\r\n", errmsg.c_str());

    Blob blob;
    if (STATUS_OK == rpcclientaccess.QueryBlob(user_id, blob_id, blob, errmsg))
        printf("QueryBlob success blob_id: %s, name=%s, \r\n", blob_id.c_str(),
               blob.name().c_str());
    else
        printf("QueryBlob failed   %s\r\n", errmsg.c_str());

    if (STATUS_OK == rpcclientaccess.DeleteBlob(user_id, blob_id, errmsg))
        printf("DeleteBlob success blob_id: %s\r\n", blob_id.c_str());
    else
        printf("DeleteBlob failed   %s\r\n", errmsg.c_str());
}

void Read(StringDict &args) {
    if (!CheckExtentManager(args["user_id"])) return;

    string blob_id = args["blob_id"];
    int jobs = args.GetInt("jobs");
    if (jobs <= 0) jobs = 1;

    if (jobs == 1) {
        string errmsg;

        string filename = args["filename"];
        uint64_t offset = args.GetInt("offset");
        uint32_t len = args.GetInt("len");
        if (len <= 0) len = 65536;
        uint32_t readlen = 0;

        char *buf = new char[len];
        memset(buf, 0, len);

        unsigned long BeginTick = GetTickCount();
        StatusCode ret = ReadBlob(
                g_ExtentManagerIp, g_ExtentManagerPort, blob_id, offset,
                (unsigned char *)buf, len, readlen, errmsg);
        unsigned long cost = GetTickCount() - BeginTick;

        if (ret == STATUS_OK)
            printf("ReadBlob success blob_id: %s, size=%d, cost=%ldms\r\n",
                   blob_id.c_str(), readlen, cost);
        else
            printf("ReadBlob failed   %s\r\n", errmsg.c_str());

        if (filename.length() > 0)
            WriteBufToFile(filename.c_str(), buf, readlen, false);

        delete[] buf;
    } else {
        MulThreadTest mulThreadTest;
        mulThreadTest.Start(jobs, jtRead, blob_id);
    }
}

void RandRead(StringDict &args) {
    if (!CheckExtentManager(args["user_id"])) return;

    string blob_id = args["blob_id"];
    int jobs = args.GetInt("jobs");
    if (jobs <= 0) jobs = 1;

    if (jobs == 1) {
        const int bufsize = 1048576 * 4;
        char *buf = new char[bufsize];
        memset(buf, 0, bufsize);
        string errmsg;

        StatusCode ret;
        uint32_t buflen = 4096;
        uint32_t readlen;
        Blob blob;
        ret = rpcclientaccess.QueryBlob(args["user_id"], blob_id, blob, errmsg);
        if (ret != STATUS_OK) return;
        uint64_t BlobSize = blob.size();

        unsigned long BeginTick = GetTickCount();
        srand(BeginTick);
        uint64_t offset = rand() % BlobSize;
        ret = ReadBlob(
                g_ExtentManagerIp, g_ExtentManagerPort, blob_id, offset,
                (unsigned char *)buf, buflen, readlen, errmsg);
        unsigned long cost = GetTickCount() - BeginTick;

        if (ret == STATUS_OK)
            printf("RandRead success blob_id: %s, size=%d, cost=%ldms\r\n",
                   blob_id.c_str(), bufsize, cost);
        else
            printf("RandRead failed   %s\r\n", errmsg.c_str());

        delete[] buf;
    } else {
        MulThreadTest mulThreadTest;
        mulThreadTest.Start(jobs, jtRandRead, blob_id);
    }
}

void Write(StringDict &args) {
    if (!CheckExtentManager(args["user_id"])) return;

    string blob_id = args["blob_id"];
    int jobs = args.GetInt("jobs");
    if (jobs <= 0) jobs = 1;

    if (jobs == 1) {
        string errmsg;

        uint64_t offset = args.GetInt("offset");
        uint32_t bufsize = args.GetInt("len");
        if (bufsize <= 0) bufsize = 65536;
        string filename = args["filename"];
        StatusCode ret;
        char *buf = NULL;
        if (filename.length() > 0) {
            bufsize = GetFileLength(filename.c_str());
            buf = new char[bufsize];
            int readbytes = ReadFileToBuf(filename.c_str(), 0, buf, bufsize);
            if (readbytes != bufsize) {
                printf("ReadFileToBuf failed   %s\r\n", filename.c_str());
                delete[] buf;
                return;
            }
        } else {
            buf = new char[bufsize];
            memset(buf, 'A', bufsize);
        }

        unsigned long BeginTick = GetTickCount();
        ret = WriteBlob(
                g_ExtentManagerIp, g_ExtentManagerPort, blob_id, offset,
                (unsigned char *)buf, bufsize, errmsg);
        unsigned long cost = GetTickCount() - BeginTick;

        if (ret == STATUS_OK)
            printf("WriteBlob success blob_id: %s, size=%d, cost=%ldms\r\n",
                   blob_id.c_str(), bufsize, cost);
        else
            printf("WriteBlob failed   %s\r\n", errmsg.c_str());
        delete[] buf;
    } else {
        MulThreadTest mulThreadTest;
        mulThreadTest.Start(jobs, jtWrite, blob_id);
    }
}

void RandWrite(StringDict &args) {
    if (!CheckExtentManager(args["user_id"])) return;

    string blob_id = args["blob_id"];

    const int bufsize = 1048576 * 4;
    char *buf = new char[bufsize];
    memset(buf, 0, bufsize);
    int jobs = args.GetInt("jobs");
    if (jobs <= 0) jobs = 1;

    if (jobs == 1) {
        string errmsg;

        StatusCode ret;

        Blob blob;
        ret = rpcclientaccess.QueryBlob(args["user_id"], blob_id, blob, errmsg);
        if (ret != STATUS_OK) return;
        uint64_t BlobSize = blob.size();

        unsigned long BeginTick = GetTickCount();
        srand(BeginTick);
        uint64_t offset = rand() % BlobSize;
        ret = WriteBlob(
                g_ExtentManagerIp, g_ExtentManagerPort, blob_id, offset,
                (unsigned char *)buf, 4096, errmsg);
        unsigned long cost = GetTickCount() - BeginTick;

        if (ret == STATUS_OK)
            printf("RandWrite success blob_id: %s, size=%d, cost=%ldms\r\n",
                   blob_id.c_str(), bufsize, cost);
        else
            printf("RandWrite failed   %s\r\n", errmsg.c_str());

        delete[] buf;
    } else {
        MulThreadTest mulThreadTest;
        mulThreadTest.Start(jobs, jtRandWrite, blob_id);
    }
}

void AsyncWrite(StringDict &args) {
    if (!CheckExtentManager(args["user_id"])) return;

    string errmsg;
    string blob_id = args["blob_id"];

    // StatusCode AsyncWriteExtent(const std::string &extent_server_addr, int
    // extent_server_port, const std::vector<string> extent_id_arr);
}
