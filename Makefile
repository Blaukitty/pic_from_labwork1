CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -I. -isystem /usr/include/gtest
LDFLAGS := -lgtest -lgtest_main -pthread

SRCS := Image.cpp
OBJS := $(SRCS:.cpp=.o)
TEST_SRCS := tests/ImageTest.cpp
TEST_BIN := ImageTest

all: $(TEST_BIN)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_BIN): $(OBJS) $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(TEST_SRCS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJS) $(TEST_BIN)
