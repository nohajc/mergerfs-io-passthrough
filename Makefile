CXX := g++
CXXFLAGS := ${CXXFLAGS} -pthread -fPIC -std=c++17
LDFLAGS := ${LDFLAGS} -pthread -ldl

libmergerfs_io_hook.so: mergerfs_io_hook.o
	$(CXX) -shared -fPIC $< -o $@ $(LDFLAGS)

test: test.o
	$(CXX) $< -o $@ $(LDFLAGS)
