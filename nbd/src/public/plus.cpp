#include "plus.h"

#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>

namespace cyprestore {
namespace nbd {

bool ReadFileToStr(const char *FileName, std::string &retstr) {
    std::ifstream file(FileName, std::ios_base::binary | std::ios_base::in);
    if (!file.is_open()) {
        printf("open file error %s\r\n", FileName);
        return false;
    }

    // std::filebuf  *fbuf= file.rdbuf();

    std::stringstream ss;
    ss << file.rdbuf();

    file.close();

    retstr = ss.str();
    return true;
}

int ReadFileToBuf(
        const char *FileName, uint64_t offset, char *retbuf, int buflen) {
    std::ifstream file(FileName, std::ios_base::binary | std::ios_base::in);
    if (!file.is_open()) {
        printf("open file error %s\r\n", FileName);
        return -1;
    }

    file.seekg(offset, std::ios_base::beg);
    file.read(retbuf, buflen);
    int readbytes = file.gcount();
    file.close();

    return readbytes;
}

int ReadFileToBuf(
        const char *FileName, uint64_t offset, int size, std::string &retbuf) {
    if (size <= 0) size = GetFileLength(FileName);
    if (size <= 0) return size;
    char *buf = new char[size];
    int readlen = ReadFileToBuf(FileName, 0, buf, size);
    retbuf.assign(buf, readlen);
    delete[] buf;

    return readlen;
}

bool WriteBufToFile(
        const char *FileName, const void *Buf, int Len, bool Append) {
    std::ios_base::openmode mode = std::ios_base::out | std::ios_base::trunc;
    if (Append) mode = std::ios_base::out | std::ios_base::app;
    std::ofstream outFile(FileName, mode);
    if (!outFile.is_open()) {
        printf("open file error %s\r\n", FileName);
        return false;
    }

    outFile.write((const char *)Buf, Len);
    outFile.close();
    return true;
}

int64_t GetFileLength(const char *filepath) {
    struct stat st;
    if (0
        == stat(filepath,
                &st))  // 执行成功则返回0，失败返回-1，错误代码存于errno
    {                  // rStatus.m_ctime = st.st_ctime;
                       // rStatus.m_atime = st.st_atime;
                       // rStatus.m_mtime = st.st_mtime;
        return st.st_size;
    }

    return -1;
}

int StringSplit(
        const std::string &src, const std::string &separator,
        std::vector<std::string> &dstarr) {
    if (src.empty() || separator.empty()) return 0;

    int nCount = 0;
    size_t pos = 0, offset = 0;
    uint len = src.length();
    int l = 0;

    while (offset < len) {
        pos = src.find_first_of(separator, offset);
        if (pos == std::string::npos)
            l = len - offset;
        else
            l = pos - offset;

        dstarr.push_back(src.substr(offset, l));
        nCount++;

        if (pos == std::string::npos) break;
        offset = pos + separator.length();
    }

    return nCount;
}

std::string StringTrim(std::string str) {
    if (str.empty()) return str;

    char c;
    int p = 0;
    for (; (uint)p < str.length(); p++) {
        c = str[p];
        if (c != ' ' && c != '\r' && c != '\n') break;
    }

    int q = 0;
    for (q = str.length() - 1; q > p; q--) {
        c = str[q];
        if (c != ' ' && c != '\r' && c != '\n') break;
    }

    if (p > 0 || q < (int)str.length() - 1) return str.substr(p, q - p + 1);

    return str;
}

void SplitStr(const std::string &Str, char Split, std::string &Head, std::string &Tail) {
    int P = Str.find(Split);
    if (P <= 0) {
        Head = Str;
        Tail = "";
        return;
    }
    Head = StringTrim(Str.substr(0, P));
    Tail = StringTrim(Str.substr(P + 1, Str.length()));
}

void SplitStr(const std::string &Str, std::string &Split, std::string &Head, std::string &Tail) {
    int P = Str.find(Split);
    if (P <= 0) {
        Head = Str;
        Tail = "";
        return;
    }
    Head = StringTrim(Str.substr(0, P));
    Tail = StringTrim(Str.substr(P + Split.length(), Str.length()));
}

void MakeLower(std::string &str) {
    transform(str.begin(), str.end(), str.begin(), ::tolower);
}

void MakeUpper(std::string &str) {
    transform(str.begin(), str.end(), str.begin(), ::toupper);
}

std::string StringToLower(std::string str)  //字符串转小写
{
    transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

std::string StringToUpper(std::string str) {
    transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

//在Data中查找 Data1
int FindData(unsigned char *Data, int Len, unsigned char *Data1, int Len1) {
    if (Len < Len1) return -1;
    int Last = Len - Len1;
    for (int i = 0; i <= Last; i++) {
        if (memcmp(Data + i, Data1, Len1) == 0) return i;
    }
    return -1;
}

#ifndef _MSC_VER
unsigned long GetTickCount() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
#endif

bool GetModuleFileName(bool FullPath, std::string &ModuleFileName) {
    const int BufSize = 512;
    char Buf[BufSize] = { 0 };
    int ret = readlink("/proc/self/exe", Buf, BufSize - 1);
    if (ret < 0 || (ret >= BufSize - 1)) return false;

    if (!FullPath)
        ModuleFileName = GetFileMain(Buf, false);
    else
        ModuleFileName = Buf;
    return true;
}

std::string GetModuleName() {
    pid_t pid;
    char path[256] = { 0 };

    pid = getpid();
    snprintf(path, 256, "/proc/%d/cmdline", pid);
    std::string modlename;
    ReadFileToStr(path, modlename);
    return modlename;
}

//---------------------------------------------------------------------------

std::string CheckPath(std::string Path) {
    Path = StringTrim(Path);
    if (Path.length() > 0) {
        char c = Path[Path.length() - 1];
        if (c != '\\' && c != '/') Path += "/";
    }
    return Path;
}

std::string GetFileName(const char *filepath) {
    int i, c;

    c = (int)strlen(filepath);
    for (i = c - 1; i >= 0; i--) {
        if (filepath[i] == '\\' || filepath[i] == '/') break;
    }

    if (i < 0) i = 0;
    if (filepath[i] == '\\' || filepath[i] == '/') i++;

    return std::string(filepath + i, c - i);
}

std::string GetFileMain(const char *FilePath, bool WithPath) {
    std::string FileName = FilePath;
    if (!WithPath) FileName = GetFileName(FilePath);

    char c;
    for (int i = FileName.length() - 1; i >= 0; i--) {
        c = FileName[i];
        if (c == '.') return FileName.substr(0, i);
    }
    return FileName;
}

int GetFileList(
        std::string Path, std::vector<std::string> &filearr, unsigned int MaxCount,
        bool Append) {  //支持通配符例如:  /*.blk
    if (!Append) filearr.clear();

    std::string path = Path, ext;
    int p = path.length();
    for (int i = p; i >= 0; i--) {
        if (path[i] == '/') {
            p = i;
            break;
        }
    }

    if (p < (int)path.length() - 3 && path[p + 1] == '*'
        && path[p + 2] == '.') {
        path = Path.substr(0, p);
        ext = StringTrim(Path.substr(p + 2));
        if (ext == ".*") ext = "";
    }

    DIR *dir;
    struct dirent *entry;
    dir = opendir(path.c_str());
    if (dir == NULL) return 0;

    while (filearr.size() < MaxCount) {
        entry = readdir(dir);
        if (entry == NULL) break;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0
            || entry->d_type == DT_DIR)
            continue;

        if (ext.length() > 0) {
            size_t len = strlen(entry->d_name);
            if (len > ext.length()
                && ext == std::string(entry->d_name + len, ext.length()))
                filearr.push_back(entry->d_name);
        } else
            filearr.push_back(entry->d_name);
    }

    closedir(dir);

    return filearr.size();
}

bool DeleteFile(const char *lpFileName) {
    // remove成功则返回0，失败则返回-1，错误原因存于errno。
    return remove(lpFileName) == 0;
}

//文件
bool DirectoryExists(const char *Dir) {
#ifdef _WINDOWS
    DWORD Code = GetFileAttributes(Dir);
    return (Code != -1 && ((FILE_ATTRIBUTE_DIRECTORY & Code) != 0));
#else
    struct stat buf;
    return 0
           == stat(Dir,
                   &buf);  // stat
                           // 执行成功则返回0，失败返回-1，错误代码存于errno
#endif
}

bool CreateDir(const char *Dir) {
#ifdef _WINDOWS
    return CreateDirectory(Dir, NULL);
#else
    return mkdir(Dir, S_IRWXU | S_IRWXG | S_IRWXO) == 0;
#endif
}

bool ForceDir(const char *Dir) {
    if (DirectoryExists(Dir)) return true;
    return CreateDir(Dir);
}

bool ForceDirStrong(const char *Dir) {
    if (ForceDir(Dir)) return true;

    char *p = (char *)Dir;
    while (*p) {
        if ((*p == '/' || *p == '\\') && p > Dir) {
            std::string ParentDir(Dir, p - Dir);
            if (!ForceDir(ParentDir.c_str())) return false;
        }
        p++;
    }

    return ForceDir(Dir);
}

bool FileExists(const char *FileName) {
#ifdef _WINDOWS
    HANDLE hContext;
    WIN32_FIND_DATA NextInfo;
    hContext = FindFirstFile(FileName, &NextInfo);
    if (hContext != INVALID_HANDLE_VALUE) {
        FindClose(hContext);
        return true;
    } else
        return false;
#else

    struct stat buf;
    return 0 == stat(FileName, &buf);
#endif
}
//---------------------------------------------------------------------------
std::string GetCurDateTimeString() {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    if (tm == NULL) return std::string();

    char str[80] = { 0 };
    strftime(str, sizeof(str), "%F %H:%M:%S", tm);
    return str;
}

std::string GetCurTimeString() {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    if (tm == NULL) return std::string();

    char str[80] = { 0 };
    strftime(str, sizeof(str), "%H:%M:%S", tm);
    return str;
}

}  // namespace nbd
}  // namespace cyprestore
