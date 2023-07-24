# mergerfs-io-passthrough

A library for direct [mergerfs](https://github.com/trapexit/mergerfs) file access.

Experimental - use at your own risk.

When injected into a process (using `LD_PRELOAD`), it intercepts calls to [open(2)](https://man7.org/linux/man-pages/man2/open.2.html) and redirects them to the underlying filesystem. It relies on the `xattr` interface, so mergerfs cannot be mounted with `xattr=noattr` or `xattr=nosys` (it still works with `security_capability=false` though).

For example, if you want to open `/mnt/pool/foo.txt`, it will open `/mnt/disk1/foo.txt` instead.
The path mapping is given by mergerfs extended attribute `user.mergerfs.fullpath`.

In practice, this means you can mount your pool with `cache.files=off` (better performance) and still use `mmap`.
This is particularly useful for [rtorrent](https://github.com/rakshasa/rtorrent) which may otherwise be unstable when accessing mergerfs files (I experienced random crashes during hash checking).

A wrapper script `rtorrent-mfs` for launching `rtorrent` is included.
