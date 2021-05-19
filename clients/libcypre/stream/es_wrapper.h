

/*
 * Copyright 2021 JDD authors.
 * @zhangliang
 */

#ifndef CYPRESTORE_CLIENTS_STREAM_ES_WRAPPER_H_
#define CYPRESTORE_CLIENTS_STREAM_ES_WRAPPER_H_

#include <google/protobuf/stubs/common.h>

#include <memory>

#include "stream/extent_stream_handle.h"

namespace cyprestore {

namespace common {
class ESInstance;
};

namespace clients {

class ReadRequest;
class WriteRequest;
class RBDStreamOptions;

class EsWrapper {
public:
    virtual int AsyncRead(
            const common::ESInstance &es, ReadRequest *req,
            google::protobuf::Closure *done) = 0;
    virtual int AsyncWrite(
            const common::ESInstance &es, WriteRequest *req,
            google::protobuf::Closure *done) = 0;
};

class NullEsWrapper : public EsWrapper {
public:
    NullEsWrapper(
            const RBDStreamOptions &sopts, const ExtentStreamOptions eopts)
            : sopts_(sopts), eopts_(eopts) {}
    virtual int AsyncRead(
            const common::ESInstance &es, ReadRequest *req,
            google::protobuf::Closure *done);
    virtual int AsyncWrite(
            const common::ESInstance &es, WriteRequest *req,
            google::protobuf::Closure *done);

private:
    const RBDStreamOptions &sopts_;
    const ExtentStreamOptions eopts_;
};

}  // namespace clients
}  // namespace cyprestore
#endif  // CYPRESTORE_CLIENTS_STREAM_ES_WRAPPER_H_
