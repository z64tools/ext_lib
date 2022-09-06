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

ExtLib_H         = $(shell find . -maxdepth 0 -type f -name '*.h')
ExtGui_H         =
ExtLib_C         = $(shell cd $(PATH_EXTLIB) && find src/* -maxdepth 0 -type f -name '*.c')
ExtGui_C         = $(shell cd $(PATH_EXTLIB) && find src/gui/* -type f -name '*.c')
Regex_C          = $(shell cd $(PATH_EXTLIB) && find src/regex/* -type f -name '*.c')
Xm_C             = $(shell cd $(PATH_EXTLIB) && find src/xxm/* -type f -name '*.c')
Audio_C          = $(shell cd $(PATH_EXTLIB) && find src/xaudio/* -type f -name '*.c')
Zip_C            = $(shell cd $(PATH_EXTLIB) && find src/xzip/* -type f -name '*.c')
Mp3_C            = $(shell cd $(PATH_EXTLIB) && find src/xmp3/* -type f -name '*.c')
Fonts            = $(shell cd $(PATH_EXTLIB) && find src/fonts/* -type f -name '*.ttf')

ExtLib_Linux_O   = $(foreach f,$(ExtLib_C:.c=.o), bin/linux/$f)
ExtGui_Linux_O   = $(foreach f,$(ExtGui_C:.c=.o), bin/linux/$f)
Zip_Linux_O      = $(foreach f,$(Zip_C:.c=.o), bin/linux/$f)
Audio_Linux_O    = $(foreach f,$(Audio_C:.c=.o), bin/linux/$f)
Mp3_Linux_O      = $(foreach f,$(Mp3_C:.c=.o), bin/linux/$f)
Xm_Linux_O       = $(foreach f,$(Xm_C:.c=.o), bin/linux/$f)
All_Linux_O      = $(ExtLib_Win32_O) $(ExtGui_Linux_O) $(Zip_Linux_O) $(Audio_Linux_O) $(Mp3_Linux_O) $(Xm_Linux_O)

ExtLib_Win32_O   = $(foreach f,$(ExtLib_C:.c=.o), bin/win32/$f) $(foreach f,$(Regex_C:.c=.o), bin/win32/$f)
ExtGui_Win32_O   = $(foreach f,$(ExtGui_C:.c=.o), bin/win32/$f)
Zip_Win32_O      = $(foreach f,$(Zip_C:.c=.o), bin/win32/$f)
Audio_Win32_O    = $(foreach f,$(Audio_C:.c=.o), bin/win32/$f)
Mp3_Win32_O      = $(foreach f,$(Mp3_C:.c=.o), bin/win32/$f)
Xm_Win32_O       = $(foreach f,$(Xm_C:.c=.o), bin/win32/$f)
All_Win32_O      = $(ExtLib_Win32_O) $(ExtGui_Win32_O) $(Zip_Win32_O) $(Audio_Win32_O) $(Mp3_Win32_O) $(Xm_Win32_O)

ExtGui_Linux_O  += $(foreach f,$(Fonts:.ttf=.o), bin/linux/$f)
ExtGui_Win32_O  += $(foreach f,$(Fonts:.ttf=.o), bin/win32/$f)

DataFileCompiler := $(PATH_EXTLIB)tools/dfc

# Make build directories
$(shell mkdir -p bin/ $(foreach dir, \
	$(dir $(ExtLib_Linux_O)) \
	$(dir $(ExtGui_Linux_O)) \
	$(dir $(Zip_Linux_O)) \
	$(dir $(Audio_Linux_O)) \
	$(dir $(Mp3_Linux_O)) \
	$(dir $(Xm_Linux_O)) \
	\
	$(dir $(All_Win32_O)) \
	\
	, $(dir)))

print:
	@echo $(ExtGui_H)

CFLAGS += -Wno-missing-braces
ExtGui_Linux_Flags = -lGL -lglfw
ExtGui_Win32_Flags = `i686-w64-mingw32.static-pkg-config --cflags --libs glfw3`

bin/win32/src/gui/%.o: CFLAGS += -Wno-misleading-indentation
bin/win32/src/xzip/%.o: CFLAGS += -Wno-stringop-truncation
bin/linux/src/gui/%.o: CFLAGS += -Wno-misleading-indentation
bin/linux/src/xzip/%.o: CFLAGS += -Wno-stringop-truncation
bin/linux/src/xaudio/__c0.o: CFLAGS += -Wno-maybe-uninitialized

ASSET_FILENAME = $(notdir $(basename $<))

-include $(All_Linux_O:.o=.d)
-include $(All_Win32_O:.o=.d)

define GD_WIN32
	@echo -n $(dir $@) > $(@:.o=.d)
	@i686-w64-mingw32.static-gcc -MM $(CFLAGS) -D_WIN32 $< >> $(@:.o=.d)
endef

define GD_LINUX
	@echo -n $(dir $@) > $(@:.o=.d)
	@gcc -MM $(CFLAGS) $< >> $(@:.o=.d)
endef

# Binary file compiler
$(DataFileCompiler): $(PATH_EXTLIB)tools/dfc.c
	@echo "$(PRNT_RSET)[$(PRNT_YELW)$(ASSET_FILENAME)$(PRNT_RSET)]"
	@gcc -o $@ $< $(ExtLib_C) -pthread -lm $(CFLAGS) -DEXTLIB_PERMISSIVE

bin/win32/%.o: $(PATH_EXTLIB)%.ttf $(DataFileCompiler)
	@echo "$(PRNT_RSET)[$(PRNT_GREN)g$(ASSET_FILENAME)$(PRNT_RSET)]"
	@$(DataFileCompiler) --cc i686-w64-mingw32.static-gcc --i $< --o $@
	
bin/linux/%.o: $(PATH_EXTLIB)%.ttf $(DataFileCompiler)
	@echo "$(PRNT_RSET)[$(PRNT_GREN)g$(ASSET_FILENAME)$(PRNT_RSET)]"
	@$(DataFileCompiler) --cc gcc --i $< --o $@

bin/win32/%.o: $(PATH_EXTLIB)%.c
	@echo "$(PRNT_RSET)[$(PRNT_BLUE)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(CFLAGS) -D_WIN32
	$(GD_WIN32)
	
bin/linux/%.o: $(PATH_EXTLIB)%.c
	@echo "$(PRNT_RSET)[$(PRNT_BLUE)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(CFLAGS)
	$(GD_LINUX)