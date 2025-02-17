HOME = /opt/ffmpeg
TARGET = libmedia
STATIC_LIB = $(TARGET).a

UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
	CC = clang
	CXX = clang++
	CFLAGS = -Wall -Wextra -g -O0 -std=c++2a -I$(HOME)/include
	LDFLAGS = -L$(HOME)/lib -Wl,-rpath,$(HOME)/lib \
		-lavcodec -lavformat -lavutil -lswscale -lswresample
	SHARED_LDFLAGS = -dynamiclib -install_name @rpath/$(TARGET).dylib
	DYNAMIC_LIB = $(TARGET).dylib
else ifeq ($(UNAME), Linux)
	CC = gcc
	CXX = g++
	CFLAGS = -Wall -Wextra -fPIC -g -O0 -std=c++2a -I$(HOME)/include
	LDFLAGS = -L$(HOME)/lib -Wl,-rpath,$(HOME)/lib \
		-lavcodec -lavformat -lavutil -lswscale -lswresample
	SHARED_LDFLAGS = -shared -fPIC
	DYNAMIC_LIB = $(TARGET).so
else
	$(error Unsupported OS: $(UNAME))
endif

SRCS = ffmpeg.cpp \
	avmanage.cpp \
	avmedia.cpp \
	avformat.cpp \
	avcodec.cpp \
	avutil.cpp \
	swscale.cpp \
	swresample.cpp
OBJS = $(SRCS:.cpp=.o)

TEST_TARGET = tests/main
TEST_SRCS = tests/main.cpp \
	tests/test_ffmpeg.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)

all: $(STATIC_LIB) $(DYNAMIC_LIB) $(TEST_TARGET)

$(STATIC_LIB): $(OBJS)
	ar rcs $@ $^

$(DYNAMIC_LIB): $(OBJS)
	$(CXX) -o $@ $^ $(SHARED_LDFLAGS) $(LDFLAGS)

$(TEST_TARGET): $(TEST_OBJS) $(STATIC_LIB)
	$(CXX) -o $@ -I. -L. $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

test:
	rm -rf nohup.out
	nohup ./$(TEST_TARGET)

install:
	mkdir -p $(HOME)/include/libmedia
	mkdir -p $(HOME)/lib
	cp *.h $(HOME)/include/libmedia
	cp $(STATIC_LIB) $(HOME)/lib
	cp $(DYNAMIC_LIB) $(HOME)/lib

clean:
	rm -rf $(OBJS) $(TEST_OBJS) \
		$(STATIC_LIB) $(DYNAMIC_LIB) \
		$(TARGET) $(TARGET).dSYM $(TEST_TARGET) \
		nohup.out

.PHONY: all clean run
