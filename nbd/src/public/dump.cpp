
#include "dump.h"

#include <execinfo.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <butil/logging.h>

#include "plus.h"

namespace cyprestore {
namespace nbd {


static std::string addr2line(const char *exename, void *addr);

void DumpErrorStack(int signo) {
    void *stack[250];
    int size, i;
    std::string module_name = GetModuleName();
    char exe_file[PATH_MAX];
    auto ret = readlink("/proc/self/exe", exe_file, sizeof(exe_file));
	exe_file[ret] = '\0';
    LOG(ERROR) << sys_siglist[signo];
    LOG(ERROR) << module_name;
    LOG(ERROR) << exe_file;

    size = backtrace(stack, 250);
    if (size <= 0) return;
    char **strings;
    strings = backtrace_symbols(stack, size);
    if (strings == NULL) return;

    for (i = 0; i < size; i++) {
        std::string s;
        if (strstr(strings[i], module_name.c_str()) != NULL)
            s = addr2line(exe_file, stack[i]);
        if (s.empty())
            s = strings[i];
        LOG(ERROR) << s;
    }

    free(strings);
}

std::string addr2line(const char *exename, void *addr) {
    FILE *p;
    char *s, line[1024];
    std::string cmd, str, codeline;

    snprintf(
            line, 1024, "/usr/bin/addr2line -e %s -C -f 0x%lx", exename,
            (unsigned long)addr);
    cmd = line;

    p = popen(cmd.c_str(), "r");
    if (p) {
        s = fgets(line, sizeof(line), p);
        if (s != NULL) {
            // printf("addr2line StringTrim\r\n");
            str = StringTrim(s);
            codeline += str + "  ";
            // printf("addr2line fgets2\r\n");
            s = fgets(line, sizeof(line), p);
            str = StringTrim(s);
            codeline += str;
        }

        pclose(p);
    }

    return codeline;
}

}  // namespace nbd
}  // namespace cyprestore
