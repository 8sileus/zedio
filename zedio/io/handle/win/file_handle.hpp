#pragma once

// Win
#include <WinSock2.h>

namespace zedio::io::detail {

class FileHandle {
protected:
    explicit FileHandle(HANDLE handle)
        : handle{handle} {}

    ~FileHandle() {
        if (handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
            handle = INVALID_HANDLE_VALUE;
        }
    }

    FileHandle(FileHandle &&other)
        : handle{other.handle} {
        other.handle = INVALID_HANDLE_VALUE;
    }

    auto operator=(this FileHandle &self, FileHandle &&other) noexcept -> FileHandle & {
        if (self.handle != INVALID_HANDLE_VALUE) {
            CloseHandle(self.handle);
        }
        self.handle = other.handle;
        other.handle = INVALID_HANDLE_VALUE;
        return self;
    }

    // uncopyable
    FileHandle(const FileHandle &other) noexcept = delete;
    auto operator=(const FileHandle &other) noexcept -> FileHandle & = delete;

public:
    [[nodiscard]]
    auto handle(this const FileHandle &self) noexcept -> HANDLE {
        return self.handle;
    }

    [[nodiscard]]
    auto take_handle(this FileHandle &self) noexcept -> HANDLE {
        auto result = self.handle;
        self.handle = INVALID_HANDLE_VALUE;
        return result;
    }

protected:
    HANDLE handle;
};

} // namespace zedio::io::detail
