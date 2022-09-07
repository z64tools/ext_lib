#!/usr/bin/python3

import sys

pkg_info = {
	"linux/src/gui/geogrid.o": "-lGL -lglfw",
	"win32/src/gui/geogrid.o": "-lglfw3 -lopengl32 -lgdi32",
	"win32/libreproc.a":       "-lws2_32",
	"src/ext_lib.o":           "-lm -pthread",
	"linux/src/ext_lib.o":     "-ldl",
}

for a in sys.argv:
	for suffix, pkg in pkg_info.items():
		if a.endswith(suffix):
			print(pkg, end = " ")