CXX = g++
CXXFLAGS = -std=c++11 -Wall -Iinclude -I/usr/local/cuda/include
LDFLAGS = -lcudart -L/usr/local/cuda/lib64
SRCS = src/main.cpp src/memory.cpp
BUILDS = build
OBJS = $(patsubst src/%.cpp,$(BUILDS)/%.o,$(SRCS))
TARGET = $(BUILDS)/benchmark

all: $(BUILDS) $(TARGET)

$(BUILDS):
	mkdir -p $(BUILDS)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDS)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDS)