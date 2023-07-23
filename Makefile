CXX := g++
CXXFLAGS := ${CXXFLAGS} -fPIC -std=c++17

libmergerfs_io_hook.so: mergerfs_io_hook.o
	$(CXX) -shared -fPIC $< -o $@ $(LDFLAGS)

test: test.o
	$(CXX) $< -o $@ $(LDFLAGS)
