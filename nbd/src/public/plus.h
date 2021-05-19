#ifndef NBD_SRC_PUBLIC_PLUS_H_
#define NBD_SRC_PUBLIC_PLUS_H_

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace cyprestore {
namespace nbd {

typedef const char *LPCSTR;
typedef unsigned char BYTE;

bool ReadFileToStr(const char *FileName, std::string &retstr);
int ReadFileToBuf(
        const char *FileName, uint64_t offset, int size, std::string &retbuf);
int ReadFileToBuf(
        const char *FileName, uint64_t offset, char *retbuf, int buflen);
bool WriteBufToFile(
        const char *FileName, const void *Buf, int Len, bool Append = false);
int64_t GetFileLength(const char *filepath);

int StringSplit(
        const std::string &src, const std::string &separator,
        std::vector<std::string> &dstarr);
std::string StringTrim(std::string str);
void SplitStr(const std::string &Str, char Split, std::string &Head, std::string &Tail);
void SplitStr(const std::string &Str, std::string &Split, std::string &Head, std::string &Tail);

void MakeLower(std::string &str);
void MakeUpper(std::string &str);
std::string StringToLower(std::string str);
std::string StringToUpper(std::string str);
int FindData(unsigned char *Data, int Len, unsigned char *Data1, int Len1);


// 返回自系统开机以来的毫秒数（tick）
unsigned long GetTickCount();

bool GetModuleFileName(bool FullPath, std::string &ModuleFileName);
std::string GetModuleName();
//---------------------------------------------------------------------------
//文件及路径
std::string CheckPath(std::string Path);
std::string GetFileName(const char *filepath);
std::string GetFileMain(const char *FilePath, bool WithPath);
int GetFileList(
        std::string Path, std::vector<std::string> &filearr,
        unsigned int MaxCount = 10000, bool Append = false);
bool DeleteFile(const char *lpFileName);

bool DirectoryExists(const char *Dir);
bool CreateDir(const char *Dir);
bool ForceDir(const char *Dir);
bool ForceDirStrong(const char *Dir);
bool FileExists(const char *FileName);
//---------------------------------------------------------------------------
std::string GetCurDateTimeString();
std::string GetCurTimeString();

}  // namespace nbd
}  // namespace cyprestore

#endif