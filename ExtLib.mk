ExtLib_H  = $(C_INCLUDE_PATH)/ExtLib.h

ExtLib_C  = ExtLib.c
XM_C      = libxm/src/context.c \
			libxm/src/load.c \
			libxm/src/play.c \
			libxm/src/xm.c
ZIP_C     = zip/src/zip.c
MINAUD_C  = miniaudio.c

ExtLib_Linux_O  = $(foreach f,$(ExtLib_C:.c=.o) $(XM_C:.c=.o) $(MINAUD_C:.c=.o) $(ZIP_C:.c=.o), $(C_INCLUDE_PATH)/bin/linux/$f)
ExtLib_Win32_O  = $(foreach f,$(ExtLib_C:.c=.o) $(XM_C:.c=.o) $(MINAUD_C:.c=.o) $(ZIP_C:.c=.o), $(C_INCLUDE_PATH)/bin/win32/$f)

# Make build directories
$(shell mkdir -p bin/ $(foreach dir, \
	$(dir $(ExtLib_Linux_O)) \
	$(dir $(ExtLib_Win32_O)) \
	, $(dir)))
	
extlib_linux: $(ExtLib_Linux_O)
extlib_win32: $(ExtLib_Win32_O)
extlib_clean:
	@rm -f -R  $(C_INCLUDE_PATH)/bin
	
$(C_INCLUDE_PATH)/bin/win32/ExtLib.o: $(C_INCLUDE_PATH)/ExtLib.c $(C_INCLUDE_PATH)/ExtLib.h
$(C_INCLUDE_PATH)/bin/win32/%.o: $(C_INCLUDE_PATH)/%.c
	@echo "$(PRNT_RSET)[$(PRNT_BLUE)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(OPT_WIN32) $(CFLAGS) -Wno-format-truncation -Wno-strict-aliasing -Wno-implicit-function-declaration -DNDEBUG -D_WIN32
	
$(C_INCLUDE_PATH)/bin/linux/ExtLib.o: $(C_INCLUDE_PATH)/ExtLib.c $(C_INCLUDE_PATH)/ExtLib.h
$(C_INCLUDE_PATH)/bin/linux/%.o: $(C_INCLUDE_PATH)/%.c
	@echo "$(PRNT_RSET)[$(PRNT_BLUE)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(OPT_LINUX) $(CFLAGS) -Wno-format-truncation -Wno-strict-aliasing -Wno-implicit-function-declaration -DNDEBUG