#include <cstdarg>
#include <cstdio>
#include <dlfcn.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <mutex>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <unistd.h>

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
}

extern "C" {
    int open(const char *path, int flags, ...);
    int openat(int dirfd, const char* path, int flags, ...);
}

HOOK(open);
HOOK(openat);

static ssize_t get_real_path(int fd, char result[PATH_MAX]) {
    return fgetxattr(fd, "user.mergerfs.fullpath", result, PATH_MAX);
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

    printf("real path: %s\n", real_path);
    close(fd);

    if (creat_flag_present) {
        return hooked_open(real_path, flags, mode);
    }

    return hooked_open(real_path, flags);
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
    // mode_t mode = 0;
    // int fd = -1;

    // bool creat_flag_present = (flags & O_CREAT) != 0;

    // if (creat_flag_present) {
    //     va_list args;
    //     va_start(args, flags);
    //     mode = va_arg(args, mode_t);
    //     va_end(args);

    //     // We're possibly creating a file, so we need to
    //     // first open it using the caller supplied path.
    //     // That way we ensure mergerfs has established
    //     // the path mapping before we try to query it.
    //     fd = hooked_open(path, flags, mode);
    //     if (fd == -1) {
    //         return fd;
    //     }
    // }

    // char real_path[PATH_MAX];
    // bool real_path_available = get_real_path(path, real_path) != -1;

    // if (!real_path_available) {
    //     // probably not mergerfs, return the fd without change
    //     if (creat_flag_present) {
    //         return fd;
    //     }
    //     return hooked_open(path, flags);
    // }
    // printf("real path: %s\n", real_path);

    // if (creat_flag_present) {
    //     close(fd);
    //     return hooked_open(real_path, flags, mode);
    // }

    // return hooked_open(real_path, flags);
}

static int test() {
    return open("", 0);
}

static int test2() {
    return openat(0, "", 0);
}

static int test3() {
    return creat("", 0); // not variadic
}
