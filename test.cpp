#include <algorithm>
#include <fcntl.h>
#include <linux/limits.h>
#include <string>
#include <unistd.h>

ssize_t get_filename_from_fd(int fd, char result[PATH_MAX]) {
    char path[PATH_MAX];
    sprintf(path, "/proc/self/fd/%d", fd);

    ssize_t len = readlink(path, result, PATH_MAX);
    if (len == -1) {
        return len;
    }

    ssize_t last_idx = PATH_MAX - 1;
    auto null_idx = std::min(len, last_idx);
    result[null_idx] = 0;
    return len;
}

template <typename F>
struct at_exit {
    F func;

    at_exit(F f) : func(f) {}
    ~at_exit() {
        func();
    }
};

int main(int argc, char *argv[]) {
    using namespace std::string_literals;

    if (argc != 3) {
        printf("usage: %s FILE FLAGS\n", argv[0]);
        return 1;
    }

    int flags = 0;
    if (argv[2] == "r"s)  {
        flags = O_RDONLY;
    } else if (argv[2] == "w"s) {
        flags = O_CREAT | O_WRONLY | O_TRUNC;
    } else {
        printf("flag error\n");
        return 1;
    }

    int fd = -1;
    if ((flags & O_CREAT) != 0) {
        fd = open(argv[1], flags, 0644);
    } else {
        fd = open(argv[1], flags);
    }
    if (fd == -1) {
        printf("fopen error\n");
        return 1;
    }

    at_exit hnd{[=] {
        close(fd);
    }};

    char real_path[PATH_MAX];
    if (get_filename_from_fd(fd, real_path) == -1) {
        printf("readlink error\n");
        return 1;
    }

    printf("%s\n", real_path);
}
