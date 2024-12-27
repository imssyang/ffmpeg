all:
	gcc -o m main.cpp -D__STDC_CONSTANT_MACROS -I/opt/ffmpeg/include -L/opt/ffmpeg/lib -Wl,-rpath,/opt/ffmpeg/lib -lavutil -lavcodec -lavformat
	#g++  main.cpp  -D__STDC_CONSTANT_MACROS -I/opt/ffmpeg/include -L/opt/ffmpeg/lib -Wl,-rpath,/opt/ffmpeg/lib -Wl,--start-group -lavfilter -lavformat -lavcodec -lavutil -lm -latomic -lxml2 -lbz2 -lz -lssl -lcrypto -lpthread -Wl,--end-group

avformat:
	g++ -o m avformat_test.cpp avformat.cpp \
		-std=c++2a \
		-I/opt/ffmpeg/include \
		-L/opt/ffmpeg/lib -Wl,-rpath,/opt/ffmpeg/lib \
		-lavcodec -lavformat
