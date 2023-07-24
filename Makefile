CXX := g++
CXXFLAGS := ${CXXFLAGS} -pthread -fPIC -std=c++17
LDFLAGS := ${LDFLAGS} -pthread -ldl

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

libmergerfs_io_hook.so: mergerfs_io_hook.o
	$(CXX) -shared -fPIC $< -o $@ $(LDFLAGS)

test: test.o
	$(CXX) $< -o $@ $(LDFLAGS)

install: libmergerfs_io_hook.so rtorrent-mfs
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 755 libmergerfs_io_hook.so $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 755 rtorrent-mfs $(DESTDIR)$(PREFIX)/bin/
