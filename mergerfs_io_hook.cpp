#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <mutex>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <syslog.h>
#include <unistd.h>

typedef char IOCTL_BUF[4096];
#define IOCTL_APP_TYPE  0xDF
#define IOCTL_FILE_INFO _IOWR(IOCTL_APP_TYPE,0,IOCTL_BUF)

namespace {
    template <typename F>
    class func_ptr {
        F* ptr;
        const char* func_name;
        std::once_flag flag;

    public:
        func_ptr(F*, const char* name) : ptr(nullptr), func_name(name) {}

        F* get() {
            std::call_once(flag, [this] {
                ptr = reinterpret_cast<F*>(dlsym(RTLD_NEXT, func_name));
            });
            return ptr;
        }

        template <typename... Args>
        decltype(auto) operator()(Args&&... args) {
            return get()(std::forward<Args>(args)...);
        }
    };

    #define HOOK(func) func_ptr hooked_ ## func(func, #func)

    std::once_flag log_init;

    static void log(int priority, const char* format, ...) {
        std::call_once(log_init, [] {
            openlog("mergerfs-io-passthrough", LOG_PID, LOG_USER);
        });

        va_list args;
        va_start(args, format);
        vsyslog(priority, format, args);
        va_end(args);
    }
}

extern "C" {
    int open(const char *path, int flags, ...);
    int openat(int dirfd, const char* path, int flags, ...);
    int creat(const char *path, mode_t mode);
}

HOOK(open);
HOOK(openat);

// result will be a null-terminated string
static int get_real_path(int fd, char result[PATH_MAX]) {
    strcpy(result, "fullpath");
    return ioctl(fd, IOCTL_FILE_INFO, result);
    // auto len = fgetxattr(fd, "user.mergerfs.fullpath", result, PATH_MAX);
    // ssize_t last_idx = PATH_MAX - 1;
    // auto null_idx = std::min(len, last_idx);
    // result[null_idx] = 0;

    // return len;
}

int openat(int dirfd, const char* path, int flags, ...) {
    mode_t mode = 0;
    int fd = -1;

    bool creat_flag_present = (flags & O_CREAT) != 0;

    if (creat_flag_present) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);

        // We're possibly creating a file, so we need to
        // first open it using the caller supplied path.
        // That way we ensure mergerfs has established
        // the path mapping before we try to query it.
        fd = hooked_openat(dirfd, path, flags, mode);
    } else {
        fd = hooked_openat(dirfd, path, flags);
    }

    if (fd == -1) {
        return fd;
    }

    char real_path[PATH_MAX];
    bool real_path_available = get_real_path(fd, real_path) != -1;

    if (!real_path_available) {
        // probably not mergerfs, return the fd without change
        return fd;
    }

    log(LOG_NOTICE, "path %s resolved as %s\n", path, real_path);
    close(fd);

    if (creat_flag_present) {
        return hooked_open(real_path, flags, mode);
    }

    return hooked_open(real_path, flags);
}

int creat(const char *path, mode_t mode) {
    return open(path, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

int open(const char *path, int flags, ...) {
    if ((flags & O_CREAT) != 0) {
        va_list args;
        va_start(args, flags);
        mode_t mode = va_arg(args, mode_t);
        va_end(args);
        return openat(AT_FDCWD, path, flags, mode);
    }
    return openat(AT_FDCWD, path, flags);
}
