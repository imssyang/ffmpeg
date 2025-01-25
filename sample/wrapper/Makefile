UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    CC = clang
    CXX = clang++
    CFLAGS = -Wall -Wextra -g -O0 -std=c++2a -I/opt/ffmpeg/include
    LDFLAGS = -L/opt/ffmpeg/lib \
		-Wl,-rpath,/opt/ffmpeg/lib \
		-lavcodec -lavformat -lavutil -lswscale -lswresample
else ifeq ($(UNAME), Linux)
    CC = gcc
    CXX = g++
    CFLAGS = -Wall -Wextra -g -O0 -std=c++2a -I/opt/ffmpeg/include
    LDFLAGS = -L/opt/ffmpeg/lib \
		-Wl,-rpath,/opt/ffmpeg/lib \
		-lavcodec -lavformat -lavutil -lswscale -lswresample
else
    $(error Unsupported OS: $(UNAME))
endif

SRCS = main.cpp \
	avmedia.cpp \
	avformat.cpp \
	avcodec.cpp \
	avutil.cpp \
	swscale.cpp \
	swresample.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = main

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

run:
	rm -rf nohup.out
	nohup ./main

clean:
	rm -rf $(OBJS) $(TARGET) $(TARGET).dSYM

.PHONY: all clean