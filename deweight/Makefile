appname := deweight

CXX := gcc
CXXFLAGS := -std=c++14 -O3 -DNDEBUG -I. -pedantic
LDLIBS := -lstdc++ -lm

srcfiles := $(shell find . -name "*.cc" -or -name "*.cpp")
objects  := $(patsubst ./%.cpp, ./%.o, $(patsubst ./%.cc, ./%.o, $(srcfiles)))

all: build/$(appname)

build/$(appname): $(objects)
	$(CXX) $(CXXFLAGS) -o build/$(appname) $(objects) $(LDLIBS) 

build/.depend: $(srcfiles)
	mkdir -p build
	rm -f ./build/.depend
	$(CXX) $(CXXFLAGS) -MM $^>>./build/.depend;

clean:
	rm -f $(objects)
	rm -f ./build/.depend
	rm -f ./build/$(appname)

dist-clean: clean
	rm -f *~ ./build/.depend

include build/.depend