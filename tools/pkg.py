#!/usr/bin/python3

import sys

pkg_info = {
	"linux/src/gui/interface.o": "-lGL -lglfw",
	"win32/src/gui/interface.o": "-lglfw3 -lopengl32 -lgdi32",
	
	"win32/libreproc.a":       "-lws2_32",
	
	"src/ext_lib.o":           "-lm -pthread",
	"linux/src/ext_lib.o":     "-ldl -lcurl",
	
	"win32/src/ext_lib.o":     "-lwininet",
}

for a in sys.argv:
	for suffix, pkg in pkg_info.items():
		if a.endswith(suffix):
			print(pkg, end = " ")
