#include <cstdio>
#include <linux/limits.h>
#include <unistd.h>

ssize_t get_filename_from_fd(int fd, char result[PATH_MAX]) {
    char path[PATH_MAX];
    sprintf(path, "/proc/self/fd/%d", fd);
    return readlink(path, result, PATH_MAX);
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
    if (argc != 3) {
        printf("usage: %s FILE MODE\n", argv[0]);
        return 1;
    }

    FILE* f = fopen(argv[1], argv[2]);
    if (f == nullptr) {
        printf("fopen error\n");
        return 1;
    }

    at_exit hnd{[=] {
        fclose(f);
    }};

    int fd = fileno(f);
    char real_path[PATH_MAX];
    if (get_filename_from_fd(fd, real_path) == -1) {
        printf("readlink error\n");
        return 1;
    }

    printf("%s\n", real_path);
}
