all:
	g++ -o main main.cpp \
		avmedia.cpp \
		avformat.cpp \
		avcodec.cpp \
		swscale.cpp \
		swresample.cpp \
		-std=c++2a \
		-I/opt/ffmpeg/include \
		-L/opt/ffmpeg/lib -Wl,-rpath,/opt/ffmpeg/lib \
		-lavcodec -lavformat -lavutil -lswscale -lswresample
