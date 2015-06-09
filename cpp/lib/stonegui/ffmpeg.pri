
win32 {
	INCLUDEPATH += c:/msys/1.0/local/include/ffmpeg
	LIBS+=-Lc:/msys/1.0/local/lib -lavcodec -lavformat -lavutil
}

unix {
	LIBS+=-lavcodec -lavutil -lavformat -lswscale
}
 
