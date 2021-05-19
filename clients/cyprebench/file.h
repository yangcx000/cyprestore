/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_CLIENTS_FILE_H_
#define CYPRESTORE_CLIENTS_FILE_H_

#include <unistd.h>

#include <string>

namespace cyprestore {
namespace clients {

class File {
public:
    File(uint64_t capacity, const std::string &file_path)
            : fd_(-1), capacity_(capacity), file_path_(file_path) {}
    ~File() {
        close(fd_);
    }

    int Open();
    int Write(uint64_t offset, uint64_t size, const char *buf);
    int Read(uint64_t offset, uint64_t size, std::string &output);

private:
    int fd_;
    uint64_t capacity_;
    std::string file_path_;
};

}  // namespace clients
}  // namespace cyprestore

#endif  // CYPRESTORE_CLIENTS_FILE_OPERATOR_H_
