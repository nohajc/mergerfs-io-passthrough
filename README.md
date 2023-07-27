# mergerfs-io-passthrough

A library for direct [mergerfs](https://github.com/trapexit/mergerfs) file access.

Experimental - use at your own risk.

When injected into a process (using `LD_PRELOAD`), it intercepts calls to [open(2)](https://man7.org/linux/man-pages/man2/open.2.html) and redirects them to the underlying filesystem.

For example, if you want to open `/mnt/pool/foo.txt`, it will open `/mnt/disk1/foo.txt` instead.
The path mapping is obtained using the `IOCTL_FILE_INFO` request provided by mergerfs.

In practice, this means you can mount your pool with `cache.files=off` (better performance) and still use `mmap`.
This is particularly useful for [rtorrent](https://github.com/rakshasa/rtorrent) which may otherwise be unstable when accessing mergerfs files (I experienced random crashes during hash checking).

A wrapper script `rtorrent-mfs` for launching `rtorrent` is included.

## Install
```bash
make
sudo make install
```
