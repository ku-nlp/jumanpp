#ifndef MMAP_H
#define MMAP_H

#include <errno.h>
#include <string>

#define HAVE_MMAP 1

extern "C" {

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
}

#include "common.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

#if !defined(_WIN32) || defined(__CYGWIN__)
namespace {
int open__(const char *name, int flag) { return open(name, flag); }
int close__(int fd) { return close(fd); }
}
#endif

namespace Morph {

template <class T> class Mmap {
  private:
    T *text;
    size_t length;
    std::string fileName;
// whatlog what_;

#if defined(_WIN32) && !defined(__CYGWIN__)
    HANDLE hFile;
    HANDLE hMap;
#else
    int fd;
    int flag;
#endif

  public:
    T &operator[](size_t n) { return *(text + n); }
    const T &operator[](size_t n) const { return *(text + n); }
    T *begin() { return text; }
    const T *begin() const { return text; }
    T *end() { return text + size(); }
    const T *end() const { return text + size(); }
    size_t size() { return length / sizeof(T); }
    // const char *what()          { return what_.str(); }
    const char *file_name() { return fileName.c_str(); }
    size_t file_size() { return length; }
    bool empty() { return (length == 0); }

// This code is imported from sufary, develoved by
//  TATUO Yamashita <yto@nais.to> Thanks!
#if defined(_WIN32) && !defined(__CYGWIN__)
    bool open(const char *filename, const char *mode = "r") {
        this->close();
        unsigned long mode1, mode2, mode3;
        fileName = std::string(filename);

        if (std::strcmp(mode, "r") == 0) {
            mode1 = GENERIC_READ;
            mode2 = PAGE_READONLY;
            mode3 = FILE_MAP_READ;
        } else if (std::strcmp(mode, "r+") == 0) {
            mode1 = GENERIC_READ | GENERIC_WRITE;
            mode2 = PAGE_READWRITE;
            mode3 = FILE_MAP_ALL_ACCESS;
        } else {
            CHECK_CLOSE_FALSE(false) << "unknown open mode:" << filename;
        }

        hFile = CreateFile(filename, mode1, FILE_SHARE_READ, 0, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, 0);
        CHECK_CLOSE_FALSE(hFile != INVALID_HANDLE_VALUE)
            << "CreateFile() failed: " << filename;

        length = GetFileSize(hFile, 0);

        hMap = CreateFileMapping(hFile, 0, mode2, 0, 0, 0);
        CHECK_CLOSE_FALSE(hMap) << "CreateFileMapping() failed: " << filename;

        text = reinterpret_cast<T *>(MapViewOfFile(hMap, mode3, 0, 0, 0));
        CHECK_CLOSE_FALSE(text) << "MapViewOfFile() failed: " << filename;

        return true;
    }

    void close() {
        if (text) {
            UnmapViewOfFile(text);
        }
        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile);
            hFile = INVALID_HANDLE_VALUE;
        }
        if (hMap) {
            CloseHandle(hMap);
            hMap = 0;
        }
        text = 0;
    }

    Mmap() : text(0), hFile(INVALID_HANDLE_VALUE), hMap(0) {}

#else

    bool open(const char *filename, const char *mode = "r") {
        this->close();
        struct stat st;
        fileName = std::string(filename);

        if (std::strcmp(mode, "r") == 0)
            flag = O_RDONLY;
        else if (std::strcmp(mode, "r+") == 0)
            flag = O_RDWR;
        else {
            std::cerr << "unknown open mode: " << filename;
            return false;
        }

        if ((fd = open__(filename, flag | O_BINARY)) < 0) {
            std::cerr << "open failed: " << filename;
            return false;
        }

        if (fstat(fd, &st) < 0) {
            std::cerr << "failed to get file size: " << filename;
            return false;
        }

        length = st.st_size;

#ifdef HAVE_MMAP
        int prot = PROT_READ;
        if (flag == O_RDWR)
            prot |= PROT_WRITE;
        char *p;
        if ((p = reinterpret_cast<char *>(
                 mmap(0, length, prot, MAP_SHARED, fd, 0))) == MAP_FAILED) {
            std::cerr << "mmap() failed: " << filename;
            return false;
        }

        text = reinterpret_cast<T *>(p);
#else
        text = new T[length];
        CHECK_CLOSE_FALSE(read(fd, text, length) >= 0) << "read() failed: "
                                                       << filename;
#endif
        close__(fd);
        fd = -1;

        return true;
    }

    void close() {
        if (fd >= 0) {
            close__(fd);
            fd = -1;
        }

        if (text) {
#ifdef HAVE_MMAP
            munmap(reinterpret_cast<char *>(text), length);
            text = 0;
#else
            if (flag == O_RDWR) {
                int fd2;
                if ((fd2 = open__(fileName.c_str(), O_RDWR)) >= 0) {
                    write(fd2, text, length);
                    close__(fd2);
                }
            }
            delete[] text;
#endif
        }

        text = 0;
    }

    Mmap() : text(0), fd(-1) {}
#endif

    virtual ~Mmap() { this->close(); }
};
}
#endif
