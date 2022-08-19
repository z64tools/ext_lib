CFLAGS      += -I $(PATH_EXTLIB)
CFLAGS_MAIN += -I $(PATH_EXTLIB)

PRNT_GRAY := \e[0;90m
PRNT_REDD := \e[0;91m
PRNT_GREN := \e[0;92m
PRNT_YELW := \e[0;93m
PRNT_BLUE := \e[0;94m
PRNT_PRPL := \e[0;95m
PRNT_CYAN := \e[0;96m
PRNT_RSET := \e[m

ExtLib_H  = $(PATH_EXTLIB)/ExtLib.h

ExtLib_C  = ExtLib.c

Fonts     = $(shell cd $(PATH_EXTLIB) && find ExtGui/fonts/* -type f -name '*.ttf')
ExtGui_C  = $(shell cd $(PATH_EXTLIB) && find ExtGui/* -type f -name '*.c') \
			glad/glad.c \
			nanovg/src/nanovg.c

ExtGui_H  = $(shell find $(PATH_EXTLIB)/ExtGui/* -type f -name '*.h')

Xm_C      = libxm/src/context.c \
			libxm/src/load.c \
			libxm/src/play.c \
			libxm/src/xm.c \
			ExtXm.c

Zip_C     = zip/src/zip.c \
			ExtZip.c

Audio_C   = miniaudio.c \
			ExtAudio.c

Mp3_C     = mp3.c

ExtLib_Linux_O  = $(foreach f,$(ExtLib_C:.c=.o), bin/linux/$f)
ExtGui_Linux_O  = $(foreach f,$(ExtGui_C:.c=.o), bin/linux/$f)
Zip_Linux_O     = $(foreach f,$(Zip_C:.c=.o), bin/linux/$f)
Audio_Linux_O   = $(foreach f,$(Audio_C:.c=.o), bin/linux/$f)
Mp3_Linux_O     = $(foreach f,$(Mp3_C:.c=.o), bin/linux/$f)
Xm_Linux_O      = $(foreach f,$(Xm_C:.c=.o), bin/linux/$f)

ExtLib_Win32_O  = $(foreach f,$(ExtLib_C:.c=.o), bin/win32/$f)
ExtGui_Win32_O  = $(foreach f,$(ExtGui_C:.c=.o), bin/win32/$f)
Zip_Win32_O     = $(foreach f,$(Zip_C:.c=.o), bin/win32/$f)
Audio_Win32_O   = $(foreach f,$(Audio_C:.c=.o), bin/win32/$f)
Mp3_Win32_O     = $(foreach f,$(Mp3_C:.c=.o), bin/win32/$f)
Xm_Win32_O      = $(foreach f,$(Xm_C:.c=.o), bin/win32/$f)

ExtGui_Linux_O  += $(foreach f,$(Fonts:.ttf=.o), bin/linux/$f)
ExtGui_Win32_O  += $(foreach f,$(Fonts:.ttf=.o), bin/win32/$f)

ExtGui_Linux_Flags = -lGL -lglfw
ExtGui_Win32_Flags = `i686-w64-mingw32.static-pkg-config --cflags --libs glfw3`

DataFileCompiler := $(PATH_EXTLIB)/tools/dfc

# Make build directories
$(shell mkdir -p bin/ $(foreach dir, \
	$(dir $(ExtLib_Linux_O)) \
	$(dir $(ExtGui_Linux_O)) \
	$(dir $(Zip_Linux_O)) \
	$(dir $(Audio_Linux_O)) \
	$(dir $(Mp3_Linux_O)) \
	$(dir $(Xm_Linux_O)) \
	\
	$(dir $(ExtLib_Win32_O)) \
	$(dir $(ExtGui_Win32_O)) \
	$(dir $(Zip_Win32_O)) \
	$(dir $(Audio_Win32_O)) \
	$(dir $(Mp3_Win32_O)) \
	$(dir $(Xm_Win32_O)) \
	, $(dir)))

print:
	@echo $(ExtGui_H)

CFLAGS += -Wno-missing-braces
ExtLib_CFlags = -DNDEBUG

bin/win32/nanovg/%.o: CFLAGS += -Wno-misleading-indentation
bin/win32/zip/%.o: CFLAGS += -Wno-stringop-truncation

ASSET_FILENAME = $(notdir $(basename $<))

# Binary file compiler
$(DataFileCompiler): $(PATH_EXTLIB)/tools/dfc.c bin/linux/ExtLib.o
	@echo "$(PRNT_RSET)[$(PRNT_YELW)$(ASSET_FILENAME)$(PRNT_RSET)]"
	@gcc -o $@ $^ -pthread $(CFLAGS_MAIN) $(ExtLib_CFlags)

bin/win32/%.o: $(PATH_EXTLIB)/%.ttf $(DataFileCompiler)
	@echo "$(PRNT_RSET)[$(PRNT_GREN)g$(ASSET_FILENAME)$(PRNT_RSET)]"
	@$(DataFileCompiler) --cc i686-w64-mingw32.static-gcc --i $< --o $@
	
bin/linux/%.o: $(PATH_EXTLIB)/%.ttf $(DataFileCompiler)
	@echo "$(PRNT_RSET)[$(PRNT_GREN)g$(ASSET_FILENAME)$(PRNT_RSET)]"
	@$(DataFileCompiler) --cc gcc --i $< --o $@

bin/win32/ExtLib.o: $(PATH_EXTLIB)/ExtLib.c $(PATH_EXTLIB)/ExtLib.h
	@echo "$(PRNT_RSET)[$(PRNT_BLUE)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(OPT_WIN32) $(CFLAGS) $(ExtLib_CFlags) -D_WIN32
bin/win32/nanovg/%.o: $(PATH_EXTLIB)/nanovg/%.c
	@echo "$(PRNT_RSET)[$(PRNT_BLUE)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(OPT_WIN32) $(CFLAGS) $(ExtLib_CFlags) -D_WIN32
bin/win32/ExtGui/%.o: $(PATH_EXTLIB)/ExtGui/%.c $(ExtGui_H)
	@echo "$(PRNT_RSET)[$(PRNT_BLUE)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(OPT_WIN32) $(CFLAGS) $(ExtLib_CFlags) -D_WIN32
bin/win32/%.o: $(PATH_EXTLIB)/%.c
	@echo "$(PRNT_RSET)[$(PRNT_BLUE)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(OPT_WIN32) $(CFLAGS) $(ExtLib_CFlags) -D_WIN32
	

bin/linux/ExtLib.o: $(PATH_EXTLIB)/ExtLib.c $(PATH_EXTLIB)/ExtLib.h
	@echo "$(PRNT_RSET)[$(PRNT_BLUE)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(OPT_LINUX) $(CFLAGS) $(ExtLib_CFlags)
bin/linux/nanovg/%.o: $(PATH_EXTLIB)/nanovg/%.c
	@echo "$(PRNT_RSET)[$(PRNT_BLUE)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(OPT_LINUX) $(CFLAGS) $(ExtLib_CFlags)
bin/linux/ExtGui/%.o: $(PATH_EXTLIB)/ExtGui/%.c $(ExtGui_H)
	@echo "$(PRNT_RSET)[$(PRNT_BLUE)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(OPT_LINUX) $(CFLAGS) $(ExtLib_CFlags)
bin/linux/%.o: $(PATH_EXTLIB)/%.c
	@echo "$(PRNT_RSET)[$(PRNT_BLUE)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(OPT_LINUX) $(CFLAGS) $(ExtLib_CFlags)