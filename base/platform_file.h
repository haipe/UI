
#ifndef __platform_file_h__
#define __platform_file_h__

#pragma once

#include <windows.h>

#include "time.h"

namespace base
{

    class FilePath;

    typedef HANDLE PlatformFile;
    const PlatformFile kInvalidPlatformFileValue = INVALID_HANDLE_VALUE;

    enum PlatformFileFlags
    {
        PLATFORM_FILE_OPEN = 1,
        PLATFORM_FILE_CREATE = 2,
        PLATFORM_FILE_OPEN_ALWAYS = 4,          // 可能会新建文件.
        PLATFORM_FILE_CREATE_ALWAYS = 8,        // 可能会覆盖老文件.
        PLATFORM_FILE_READ = 16,
        PLATFORM_FILE_WRITE = 32,
        PLATFORM_FILE_EXCLUSIVE_READ = 64,      // EXCLUSIVE和Windows的SHARE含义相反.
        PLATFORM_FILE_EXCLUSIVE_WRITE = 128,
        PLATFORM_FILE_ASYNC = 256,
        PLATFORM_FILE_TEMPORARY = 512,          // 仅在Windows上可用.
        PLATFORM_FILE_HIDDEN = 1024,            // 仅在Windows上可用.
        PLATFORM_FILE_DELETE_ON_CLOSE = 2048,
        PLATFORM_FILE_TRUNCATE = 4096,
        PLATFORM_FILE_WRITE_ATTRIBUTES = 8192   // 仅在Windows上可用.
    };

    // 由于文件系统限制导致的调用失败会返回PLATFORM_FILE_ERROR_ACCESS_DENIED.
    // 由于安全策略导致的禁止操作会返回PLATFORM_FILE_ERROR_SECURITY.
    enum PlatformFileError
    {
        PLATFORM_FILE_OK = 0,
        PLATFORM_FILE_ERROR_FAILED = -1,
        PLATFORM_FILE_ERROR_IN_USE = -2,
        PLATFORM_FILE_ERROR_EXISTS = -3,
        PLATFORM_FILE_ERROR_NOT_FOUND = -4,
        PLATFORM_FILE_ERROR_ACCESS_DENIED = -5,
        PLATFORM_FILE_ERROR_TOO_MANY_OPENED = -6,
        PLATFORM_FILE_ERROR_NO_MEMORY = -7,
        PLATFORM_FILE_ERROR_NO_SPACE = -8,
        PLATFORM_FILE_ERROR_NOT_A_DIRECTORY = -9,
        PLATFORM_FILE_ERROR_INVALID_OPERATION = -10,
        PLATFORM_FILE_ERROR_SECURITY = -11,
        PLATFORM_FILE_ERROR_ABORT = -12,
        PLATFORM_FILE_ERROR_NOT_A_FILE = -13,
        PLATFORM_FILE_ERROR_NOT_EMPTY = -14,
    };

    // 用于保存文件的信息. 如果要给结构体添加新的字段, 必须同时更新cpp中的函数.
    struct PlatformFileInfo
    {
        // 文件的大小(字节). 当is_directory为true时未定义.
        int64 size;

        // 文件是一个目录是为true.
        bool is_directory;

        // 文件最后修改时间.
        base::Time last_modified;

        // 文件最后访问时间.
        base::Time last_accessed;

        // 文件创建时间.
        base::Time creation_time;
    };

    // 创建或者打开文件. 如果使用PLATFORM_FILE_OPEN_ALWAYS, 且传入有效的|created|,
    // 当创建文件时设置|created|为true, 打开文件时设置|created|为false.
    // |error_code|可以为NULL.
    PlatformFile CreatePlatformFile(const FilePath& name,
        int flags, bool* created, PlatformFileError* error_code);

    // 关闭文件句柄. 成功返回|true|, 失败返回|false|.
    bool ClosePlatformFile(PlatformFile file);

    // 从offset偏移位置开始读取一定数量的字节(或者遇到EOF). 返回实际读取的字节数,
    // 错误返回-1.
    int ReadPlatformFile(PlatformFile file, int64 offset, char* data, int size);

    // 在offset偏移位置处写一块缓冲区数据到文件, 会覆盖之前的数据. 返回实际写入的
    // 字节数, 错误返回-1.
    int WritePlatformFile(PlatformFile file, int64 offset, const char* data,
        int size);

    // 调整文件的长度. 如果|length|大于当前文件长度, 扩充部分会填充0. 文件不存在
    // 返回false.
    bool TruncatePlatformFile(PlatformFile file, int64 length);

    // 更新文件的缓冲区数据到磁盘.
    bool FlushPlatformFile(PlatformFile file);

    // 设置文件的最后访问时间和修改时间.
    bool TouchPlatformFile(PlatformFile file, const Time& last_access_time,
        const Time& last_modified_time);

    // 返回文件的基本信息.
    bool GetPlatformFileInfo(PlatformFile file, PlatformFileInfo* info);

    // PassPlatformFile用于传递PlatformFile的所有权给接收者, 类本身不接管
    // 所有权.
    //
    // 示例:
    //
    //  void MaybeProcessFile(PassPlatformFile pass_file) {
    //    if(...) {
    //      PlatformFile file = pass_file.ReleaseValue();
    //      // Now, we are responsible for closing |file|.
    //    }
    //  }
    //
    //  void OpenAndMaybeProcessFile(const FilePath& path) {
    //    PlatformFile file = CreatePlatformFile(path, ...);
    //    MaybeProcessFile(PassPlatformFile(&file));
    //    if(file != kInvalidPlatformFileValue)
    //      ClosePlatformFile(file);
    //  }
    class PassPlatformFile
    {
    public:
        explicit PassPlatformFile(PlatformFile* value) : value_(value) {}

        // 返回对象中存储的PlatformFile, 之后调用者获取所有权, 应该负责文件的关闭.
        // 任何后续的调用都将返回非法的PlatformFile.
        PlatformFile ReleaseValue()
        {
            PlatformFile temp = *value_;
            *value_ = kInvalidPlatformFileValue;
            return temp;
        }

    private:
        PlatformFile* value_;
    };

} //namespace base

#endif //__platform_file_h__