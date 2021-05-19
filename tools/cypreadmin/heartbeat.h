/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#include <brpc/channel.h>

#include <string>

#include "common/pb/types.pb.h"
#include "extentmanager/pb/heartbeat.pb.h"
#include "options.h"

namespace cyprestore {
namespace tools {

class Heartbeat {
public:
    Heartbeat(const Options &options, brpc::Channel *channel)
            : options_(options) {
        stub_ = new extentmanager::pb::HeartbeatService_Stub(channel);
    }

    ~Heartbeat() {
        delete stub_;
    }

    int Run();
    int Report();

private:
    Options options_;
    extentmanager::pb::HeartbeatService_Stub *stub_;
};

}  // namespace tools
}  // namespace cyprestore
