CFLAGS      += -I $(PATH_EXTLIB)
CC_W32      ?= i686-w64-mingw32.static-gcc

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
Proc_C           = $(shell cd $(PATH_EXTLIB) && find src/proc/* -type f -name '*.c')
Texel_C          = $(shell cd $(PATH_EXTLIB) && find src/xtexel/* -type f -name '*.c')

Fonts            = $(shell cd $(PATH_EXTLIB) && find src/fonts/* -type f -name '*.ttf')
Fonts           += $(shell cd $(PATH_EXTLIB) && find src/fonts/* -type f -name '*.otf')

ExtLib_Linux_O   = $(foreach f,$(ExtLib_C:.c=.o), bin/linux/$f)
ExtGui_Linux_O   = $(foreach f,$(ExtGui_C:.c=.o), bin/linux/$f)
Regex_Linux_O      = $(foreach f,$(Regex_C:.c=.o), bin/linux/$f)
Zip_Linux_O      = $(foreach f,$(Zip_C:.c=.o), bin/linux/$f)
Audio_Linux_O    = $(foreach f,$(Audio_C:.c=.o), bin/linux/$f)
Mp3_Linux_O      = $(foreach f,$(Mp3_C:.c=.o), bin/linux/$f)
Xm_Linux_O       = $(foreach f,$(Xm_C:.c=.o), bin/linux/$f)
Proc_Linux_O     = $(foreach f,$(Proc_C:.c=.o), bin/linux/$f)
Image_Linux_O    = $(foreach f,$(Texel_C:.c=.o), bin/linux/$f)
All_Linux_O      = $(ExtLib_Linux_O) $(ExtGui_Linux_O) $(Zip_Linux_O) \
					$(Audio_Linux_O) $(Mp3_Linux_O) $(Xm_Linux_O) \
					$(Proc_Linux_O) $(Image_Linux_O) $(Regex_Linux_O)

ExtLib_Win32_O   = $(foreach f,$(ExtLib_C:.c=.o), bin/win32/$f)
ExtGui_Win32_O   = $(foreach f,$(ExtGui_C:.c=.o), bin/win32/$f)
Regex_Win32_O      = $(foreach f,$(Regex_C:.c=.o), bin/win32/$f)
Zip_Win32_O      = $(foreach f,$(Zip_C:.c=.o), bin/win32/$f)
Audio_Win32_O    = $(foreach f,$(Audio_C:.c=.o), bin/win32/$f)
Mp3_Win32_O      = $(foreach f,$(Mp3_C:.c=.o), bin/win32/$f)
Xm_Win32_O       = $(foreach f,$(Xm_C:.c=.o), bin/win32/$f)
Proc_Win32_O     = $(foreach f,$(Proc_C:.c=.o), bin/win32/$f)
Image_Win32_O    = $(foreach f,$(Texel_C:.c=.o), bin/win32/$f)
All_Win32_O      = $(ExtLib_Win32_O) $(ExtGui_Win32_O) $(Zip_Win32_O) \
					$(Audio_Win32_O) $(Mp3_Win32_O) $(Xm_Win32_O) \
					$(Proc_Win32_O) $(Image_Win32_O) $(Regex_Win32_O)

ExtGui_Linux_O  += $(foreach f,$(Fonts:%=%.o), bin/linux/$f)
ExtGui_Win32_O  += $(foreach f,$(Fonts:%=%.o), bin/win32/$f)

ReProc_Linux_C   = $(shell cd $(PATH_EXTLIB) && find reproc/reproc/src/* -type f -name '*.c' -not -name '*.windows.c')
ReProc_Win32_C   = $(shell cd $(PATH_EXTLIB) && find reproc/reproc/src/* -type f -name '*.c' -not -name '*.posix.c')
ReProc_Linux_O   = $(foreach f,$(ReProc_Linux_C:.c=.o), bin/linux/$f)
ReProc_Win32_O   = $(foreach f,$(ReProc_Win32_C:.c=.o), bin/win32/$f)

DataFileCompiler := $(PATH_EXTLIB)/tools/dfc.py
ExtPkg           := $(PATH_EXTLIB)/tools/pkg.py
XFLAGS            = $(shell $(ExtPkg) $^)

CFLAGS += -Wno-missing-braces -Wno-unused-local-typedefs

bin/win32/src/xxm/%.o: CFLAGS += -I$(PATH_EXTLIB)/src/xxm
bin/linux/src/xxm/%.o: CFLAGS += -I$(PATH_EXTLIB)/src/xxm
bin/win32/src/gui/%.o: CFLAGS += -Wno-misleading-indentation
bin/linux/src/gui/%.o: CFLAGS += -Wno-misleading-indentation
bin/win32/src/xzip/%.o: CFLAGS += -Wno-stringop-truncation
bin/linux/src/xzip/%.o: CFLAGS += -Wno-stringop-truncation
bin/win32/src/xaudio/__ma__.o: CFLAGS += -Wno-maybe-uninitialized
bin/linux/src/xaudio/__ma__.o: CFLAGS += -Wno-maybe-uninitialized
bin/win32/src/proc/%.o: CFLAGS += -I $(PATH_EXTLIB)/reproc/reproc/include -I$(PATH_EXTLIB)/reproc/reproc/src
bin/linux/src/proc/%.o: CFLAGS += -I $(PATH_EXTLIB)/reproc/reproc/include -I$(PATH_EXTLIB)/reproc/reproc/src
bin/win32/reproc/reproc/src/%.o: CFLAGS = -Ofast -Wno-cpp -I $(PATH_EXTLIB)/reproc/reproc/include -I$(PATH_EXTLIB)/reproc/reproc/src
bin/linux/reproc/reproc/src/%.o: CFLAGS = -Ofast -Wno-cpp -I $(PATH_EXTLIB)/reproc/reproc/include -I$(PATH_EXTLIB)/reproc/reproc/src

bin/linux/libreproc.a: $(ReProc_Linux_O)
	@mkdir -p $(dir $@)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@ar rcs $@ $^
bin/win32/libreproc.a: $(ReProc_Win32_O)
	@mkdir -p $(dir $@)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-ar rcs $@ $^

ASSET_FILENAME = $(notdir $(basename $<))

-include $(All_Linux_O:.o=.d)
-include $(All_Win32_O:.o=.d)

Proc_Linux_O    += bin/linux/libreproc.a
Proc_Win32_O    += bin/win32/libreproc.a

define GD_WIN32
	@echo -n $(dir $@) > $(@:.o=.d)
	@$(CC_W32) -MM $(CFLAGS) -D_WIN32 $< >> $(@:.o=.d)
endef

define GD_LINUX
	@echo -n $(dir $@) > $(@:.o=.d)
	@gcc -MM $(CFLAGS) $< >> $(@:.o=.d)
endef

define GD++_WIN32
	@echo -n $(dir $@) > $(@:.o=.d)
	@i686-w64-mingw32.static-g++ -MM $(CFLAGS) -D_WIN32 $< >> $(@:.o=.d)
endef

define GD++_LINUX
	@echo -n $(dir $@) > $(@:.o=.d)
	@g++ -MM $(CFLAGS) $< >> $(@:.o=.d)
endef

bin/win32/src/fonts/%.o: $(PATH_EXTLIB)/src/fonts/%
	@mkdir -p $(dir $@)
	@echo "$(PRNT_RSET)[$(PRNT_GREN)g$(ASSET_FILENAME)$(PRNT_RSET)]"
	@$(DataFileCompiler) --cc $(CC_W32) --i $< --o $@
	
bin/linux/src/fonts/%.o: $(PATH_EXTLIB)/src/fonts/%
	@mkdir -p $(dir $@)
	@echo "$(PRNT_RSET)[$(PRNT_GREN)g$(ASSET_FILENAME)$(PRNT_RSET)]"
	@$(DataFileCompiler) --cc gcc --i $< --o $@

bin/win32/%.o: $(PATH_EXTLIB)/%.c
	@mkdir -p $(dir $@)
	@echo "$(PRNT_RSET)[$(PRNT_BLUE)$(notdir $@)$(PRNT_RSET)]"
	@$(CC_W32) -c -o $@ $< $(CFLAGS) -D_WIN32
	$(GD_WIN32)
	
bin/linux/%.o: $(PATH_EXTLIB)/%.c
	@mkdir -p $(dir $@)
	@echo "$(PRNT_RSET)[$(PRNT_BLUE)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(CFLAGS)
	$(GD_LINUX)
		
bin/linux/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(CFLAGS)
	$(GD_LINUX)
		
bin/win32/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@$(CC_W32) -c -o $@ $< $(CFLAGS) -D_WIN32
	$(GD_WIN32)
	
bin/win32/src/data/%.o: src/data/%
	@mkdir -p $(dir $@)
	@echo "$(PRNT_RSET)[$(PRNT_GREN)g$(ASSET_FILENAME)$(PRNT_RSET)]"
	@$(DataFileCompiler) --cc $(CC_W32) --i $< --o $@
	
bin/linux/src/data/%.o: src/data/%
	@mkdir -p $(dir $@)
	@echo "$(PRNT_RSET)[$(PRNT_GREN)g$(ASSET_FILENAME)$(PRNT_RSET)]"
	@$(DataFileCompiler) --cc gcc --i $< --o $@

bin/win32/icon.o: src/icon.rc src/icon.ico
	@mkdir -p $(dir $@)
	@i686-w64-mingw32.static-windres -o $@ $<

bin/win32/info.o: src/info.rc
	@mkdir -p $(dir $@)
	@i686-w64-mingw32.static-windres -o $@ $<