#include "uuid.h"

namespace cyprestore {
namespace utils {

std::string UUID::Generate() {
    return butil::GenerateGUID();
}

}  // namespace utils
}  // namespace cyprestore
