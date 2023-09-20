#include <nano_grid.h>
#include <ext_interface.h>

struct FileDialog {
	NanoGrid nano;
	Split*   split[4];
	char*    path;
	s8*      searchFilterFlagList;
	int      searchFilterNum;
	s8       multiSelect;
	s8       canMultiSelect;
	s8       done;
	
	List* filter;
	Arli  index;
	
	struct TypeSearch {
		int   index;
		Timer timer;
		char  buffer[PATH_BUFFER_SIZE];
		f32   mod;
		f32   target;
		f32   failMod;
	} type;
	
	struct SidePanel {
		ElPanel     panel;
		ElContainer container;
		Arli list;
	} volumes, bookmarks;
	
	struct SearchPanel {
		ElButton  back;
		ElTextbox path;
		ElTextbox search;
	} search;
	
	struct ConfirmPanel {
		ElTextbox textbox;
		ElButton  confirm, cancel;
		char      actionText[32];
	} confirm;
	
	enum FileDialogAction {
		FILE_DIALOG_OPEN_FILE,
		FILE_DIALOG_SAVE_FILE,
		FILE_DIALOG_FOLDER,
		FILE_DIALOG_MAX,
	} action;
	
	List      files;
	List      folders;
	ScrollBar scroll;
};

typedef struct {
	List* filter;
	enum FileDialogAction action;
	s8 multiSelect;
} FileDialogInit;

enum SelectionMode {
	TEXT_TO_LIST,
	LIST_TO_TEXT
};

static bool IsFolderIndex(FileDialog* this, int64_t index) {
	return (index < this->folders.num);
}

static const char* GetItemString(FileDialog* this, int64_t index) {
	if (IsFolderIndex(this, index))
		return this->folders.item[index];
	else
		return this->files.item[index - this->folders.num];
}

static const char* GetIndexArliItemString(FileDialog* this, int64_t index) {
	int* k = Arli_At(&this->index, index);
	
	return GetItemString(this, *k);
}

static void ClearSelections(FileDialog* this) {
	Arli_Clear(&this->index);
	this->confirm.textbox.txt[0] = '\0';
}

static void UpdateSelections(FileDialog* this, enum SelectionMode mode) {
	if (mode == TEXT_TO_LIST && this->action == FILE_DIALOG_OPEN_FILE) {
		Arli_Clear(&this->index);
		
		List list = List_New();
		
		List_Tokenize(&list, this->confirm.textbox.txt, ',');
		List_Print(&list);
		
		if (list.num > 1) {
			forlist(item, list) {
				strcpy(*pitem, x_filename(item));
			}
		}
		
		forlist(item, list) {
			char* trimmed = x_strtrim(item, "\" ");
			
			forlist(file, this->files) {
				if (streq(file, trimmed)) {
					int i = indexof(this->files.item, pitem, void**);
					
					info("Add: %d - %s", i, GetItemString(this, i));
					Arli_Add(&this->index, pval32(i + this->folders.num));
				}
			}
		}
	} else if (mode == LIST_TO_TEXT) {
		this->confirm.textbox.txt[0] = '\0';
		int add = 0;
		
		for (int i = 0; i < this->index.num; i++) {
			int* k = Arli_At(&this->index, i);
			
			if (IsFolderIndex(this, *k))
				continue;
			
			const char* name = GetItemString(this, *k);
			
			if (add++)
				strcat(this->confirm.textbox.txt, ", ");
			strcat(this->confirm.textbox.txt, x_filename(name));
		}
	}
}

static int TypeSearch_Update(struct TypeSearch* this, Input* input, bool inputAccess) {
	int ret = 0;
	
	if (TimerDecr(&this->timer)) {
		this->target = 1;
		
		if (*input->buffer && inputAccess) {
			strncat(this->buffer, input->buffer, PATH_BUFFER_SIZE);
			this->timer = TimerSet(1.0f);
			ret = true;
		}
		
	} else if (*input->buffer && inputAccess) {
		arrzero(this->buffer);
		strncat(this->buffer, input->buffer, PATH_BUFFER_SIZE);
		this->timer = TimerSet(1.0f);
		this->index = -1;
		this->target = 1;
		ret = true;
	} else {
		this->failMod = 0;
		this->target = 0;
	}
	
	Math_SmoothStepToF(&this->mod, this->target, 0.25f, 0.25f, 0.01f);
	
	return ret;
}

static void TypeSearch_Draw(struct TypeSearch* this, Rect mainRect, void* vg, int search) {
	if (this->mod < EPSILON)
		return;
	
	Rect r = Rect_Scale(mainRect, -SPLIT_TEXT_H, -SPLIT_TEXT_H);
	NVGcolor texcol = Theme_GetColor(THEME_TEXT, this->mod * 255, 1.0f);
	NVGcolor shadowcol = Theme_GetColor(THEME_SHADOW, this->mod * 255, 1.0f);
	NVGcolor lightcol = Theme_GetColor(THEME_HIGHLIGHT, this->mod * 255, 1.0f);
	
	if (search || this->failMod) {
		Math_SmoothStepToF(&this->failMod, 1.0f, 0.25f, 0.25f, 0.01f);
		
		texcol = Theme_Mix(this->failMod, texcol, Theme_GetColor(THEME_DELETE, 255, 1.0f));
		lightcol = Theme_Mix(this->failMod, lightcol, Theme_GetColor(THEME_DELETE, 255, 1.0f));
	}
	
	r = Rect_ShrinkX(r, r.w * 0.75f);
	r.h = SPLIT_TEXT_H;
	
	Gfx_DrawRounderRect(vg, r, shadowcol);
	Gfx_DrawRounderOutline(vg, r, lightcol);
	Gfx_Text(vg, r, NVG_ALIGN_CENTER, texcol, this->buffer);
}

////////////////////////////////////////////////////////////////////////////////

static const char* Travel(FileDialog* this, const char* path) {
	static char buffer[PATH_BUFFER_SIZE] = {};
	
	strncpy(buffer, this->path, PATH_BUFFER_SIZE);
	
	if (!strcmp(path, "../")) {
		int len = strlen(buffer);
		char* p = buffer + strlen("c:/");
		
		if (strend(p, "/")) {
			strend(p, "/")[0] = '\0';
			
			while (!strend(p, "/") && strlen(p))
				p[strlen(p) - 1] = '\0';
		}
		
		if (len != strlen(buffer))
			return buffer;
	} else {
		strncat(buffer, path, PATH_BUFFER_SIZE);
		
		return buffer;
	}
	
	return NULL;
}

static void ReadPath(FileDialog* this, const char* path) {
	if (!path) return;
	if (!sys_isdir(path)) {
		Element_Textbox_SetText(&this->search.path, this->path);
		return;
	}
	
	this->type.timer.ongoing = false;
	
	if (this->path != path)
		strncpy(this->path, path, PATH_BUFFER_SIZE);
	
	List_Free(&this->files);
	List_Free(&this->folders);
	
	if (this->filter) {
		List* list = this->filter;
		
		for (int i = 0; i < list->num; i++) {
			FilterNode* node = new(FilterNode);
			node->type = CONTAIN_END;
			node->txt = strdup(list->item[i]);
			
			Node_Add(this->files.filterNode, node);
		}
	}
	
	List_Walk(&this->files, this->path, 0, LIST_FILES | LIST_RELATIVE);
	List_Walk(&this->folders, this->path, 0, LIST_FOLDERS | LIST_RELATIVE);
	
	List_Sort(&this->files);
	List_Sort(&this->folders);
	
	delete(this->searchFilterFlagList);
	this->searchFilterFlagList = new(s8[this->files.num + this->folders.num + 1]);
	
	Element_Textbox_SetText(&this->search.path, this->path);
	
	Input* input = GET_INPUT();
	
	ClearSelections(this);
	Input_SetTempLock(input, 2);
}

static void SetCullFlag(FileDialog* this, const char* filter) {
	int* index = NULL;
	
	if (this->index.num == 1)
		index = Arli_At(&this->index, 0);
	
	if (!filter || !*filter) {
		memset(this->searchFilterFlagList, 0, this->files.num + this->folders.num);
		this->searchFilterNum = 0;
		
		if (index)
			ScrollBar_FocusSlot(&this->scroll, *index, true);
	} else {
		memset(this->searchFilterFlagList, 1, this->files.num + this->folders.num);
		this->searchFilterNum = this->files.num + this->folders.num;
		
		for (int k = 0; k < this->files.num + this->folders.num; k++) {
			if (k < this->folders.num) {
				int i = k;
				
				info_prog("folder", i + 1, this->folders.num);
				if (stristr(this->folders.item[i], filter)) {
					this->searchFilterFlagList[k] = false;
					this->searchFilterNum--;
				} else if (index && *index == k) {
					Arli_Remove(&this->index, 0, 1);
					ScrollBar_FocusSlot(&this->scroll, 0, true);
				}
			} else {
				int i = k - this->folders.num;
				
				info_prog("file", i + 1, this->files.num);
				if (stristr(this->files.item[i], filter)) {
					this->searchFilterFlagList[k] = false;
					this->searchFilterNum--;
				} else if (index && *index == k) {
					Arli_Remove(&this->index, 0, 1);
					ScrollBar_FocusSlot(&this->scroll, 0, true);
				}
			}
		}
		
		if (index) {
			for (int i = 0, j = 0; i < this->files.num + this->folders.num; i++) {
				if (this->searchFilterFlagList[i])
					continue;
				
				if (i == *index) {
					ScrollBar_FocusSlot(&this->scroll, j, true);
					break;
				}
				
				j++;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

static void Draw_SidePanel(FileDialog* this, ContextMenu* context, Rect mainRect, void* vg) {
	struct SidePanel* volumes = &this->volumes;
	struct SidePanel* bookmarks = &this->bookmarks;
	
	Gfx_DrawRounderRect(vg, mainRect, Theme_GetColor(THEME_BASE, 255, 1.25f));
	
	if (Element_Box(BOX_START, &volumes->panel, "Volumes")) {
		Element_Row(&volumes->container, 1.0f);
		
		switch (Element_Container(&volumes->container)->state) {
			case SET_CHANGE:
				ReadPath(this, Arli_At(&volumes->list, volumes->list.cur));
				break;
				
			default:
				break;
		}
	}
	Element_Box(BOX_END, &volumes->panel);
	
	if (Element_Box(BOX_START, &bookmarks->panel, "Bookmarks")) {
		Element_Row(&bookmarks->container, 1.0f);
		
		switch (Element_Container(&bookmarks->container)->state) {
			case SET_NEW:
				strncpy(Arli_At(&bookmarks->list, bookmarks->container.set.index), this->path, PATH_BUFFER_SIZE);
				break;
				
			case SET_CHANGE:
				ReadPath(this, Arli_At(&bookmarks->list, bookmarks->list.cur));
				break;
				
			default:
				break;
		}
	}
	Element_Box(BOX_END, &bookmarks->panel);
}

static void Draw_FilePanel(FileDialog* this, ContextMenu* context, Rect mainRect, void* vg) {
	Input* input = GET_INPUT();
	Rect scrollRect = Rect_Scale(mainRect, -SPLIT_ELEM_X_PADDING * 2, -SPLIT_ELEM_X_PADDING * 2);
	int num = (this->folders.num + this->files.num) - this->searchFilterNum;
	int j = 0;
	bool inputAccess = DummySplit_InputAcces(&this->nano, this->split[2], &mainRect);
	Rect select = Rect_Vec2x2(input->cursor.pressPos, input->cursor.pos);
	
	ScrollBar_Init(&this->scroll, num, SPLIT_TEXT_H);
	if (ScrollBar_Update(&this->scroll, input, input->cursor.pos, scrollRect, mainRect))
		inputAccess = false;
	
	struct TypeSearch* ts = &this->type;
	const char* travel = NULL;
	int search = TypeSearch_Update(ts, input, inputAccess);
	
	if (!Input_GetCursor(input, CLICK_L)->hold && this->multiSelect) {
		for (int i = 0; i < this->index.num; i++) {
			int* index = Arli_At(&this->index, i);
			
			if (*index < this->folders.num)
				Arli_Remove(&this->index, i--, 1);
		}
		
		this->multiSelect = false;
	}
	
	if (inputAccess && Rect_PointIntersect(&mainRect, UnfoldVec2(input->cursor.pos))) {
		if (Input_GetCursor(input, CLICK_L)->press) {
			if (!Input_GetKey(input, KEY_LEFT_CONTROL)->hold)
				ClearSelections(this);
			
			ts->index = -1;
		}
		
		if (this->canMultiSelect && Input_GetCursor(input, CLICK_L)->hold && input->cursor.dragDist > 8)
			this->multiSelect = true;
		
		if (inputAccess) {
			int* index = Arli_At(&this->index, 0);
			
			if (Input_GetKey(input, KEY_BACKSPACE)->press)
				travel = Travel(this, "../");
			
			if (this->index.num == 1 && *index < this->folders.num && Input_GetKey(input, KEY_ENTER)->press)
				travel = Travel(this, this->folders.item[*index]);
		}
	}
	
	nvgScissor(vg, UnfoldRect(mainRect));
	
	if (this->multiSelect)
		Gfx_DrawRounderRect(vg, select, Theme_GetColor(THEME_PRIM, 180, 1.0f));
	
	for (int k = 0; k < this->files.num + this->folders.num; k++) {
		const char* name = NULL;
		
		if (this->searchFilterFlagList[k])
			continue;
		
		name = GetItemString(this, k);
		
		if (search && k >= ts->index) {
			if (stristart(name, ts->buffer)) {
				ScrollBar_FocusSlot(&this->scroll, j, true);
				Arli_Clear(&this->index);
				Arli_Add(&this->index, pval32(k));
				UpdateSelections(this, LIST_TO_TEXT);
				
				ts->index = j;
				
				search = false;
			}
		}
		
		Rect r = ScrollBar_GetRect(&this->scroll, j++);
		
		if (!Rect_RectIntersect(r, mainRect))
			continue;
		
		if (Rect_PointIntersect(&r, UnfoldVec2(input->cursor.pos))) {
			
			if (Input_GetCursor(input, CLICK_L)->press) {
				Arli_Add(&this->index, pval32(k));
				UpdateSelections(this, LIST_TO_TEXT);
				ts->index = -1;
			}
			
			if (IsFolderIndex(this, k)) {
				if (Input_GetCursor(input, CLICK_L)->dual) {
					travel = Travel(this, this->folders.item[k]);
					ScrollBar_FocusSlot(&this->scroll, 0, true);
				}
				
			} else if (this->action != FILE_DIALOG_FOLDER)
				if (Input_GetCursor(input, CLICK_L)->dual)
					context->state.setCondition = 1;
		}
		
		void* find = Arli_Find(&this->index, pval32(k));
		
		if (this->multiSelect) {
			if (Rect_RectIntersect(r, select)) {
				if (!IsFolderIndex(this, k) && !find) {
					find = Arli_Add(&this->index, pval32(k));
					UpdateSelections(this, LIST_TO_TEXT);
				}
				
			} else if (find) {
				Arli_Remove(&this->index, Arli_IndexOf(&this->index, find), 1);
				UpdateSelections(this, LIST_TO_TEXT);
				find = NULL;
			}
		}
		
		if (find) {
			NVGcolor col;
			
			if (this->multiSelect)
				col = Theme_GetColor(THEME_HIGHLIGHT, 64, 1.0f);
			else
				col = Theme_GetColor(THEME_PRIM, 185, 1.0f);
			
			Gfx_DrawRounderRect(vg, Rect_Scale(r, -1, -1), col);
		}
		
		Rect ir = Rect_ShrinkX(r, SPLIT_ELEM_X_PADDING);
		
		if (k < this->folders.num)
			Gfx_Icon(vg, ir, Theme_GetColor(THEME_TEXT, 225, 1.0f), ICON_FS_FOLDER);
		else
			Gfx_Icon(vg, ir, Theme_GetColor(THEME_PRIM, 225, 1.0f), ICON_FS_FILE);
		
		r = Rect_ShrinkX(r, SPLIT_ELEM_X_PADDING * 2 + SPLIT_TEXT);
		Gfx_Text(vg, r, NVG_ALIGN_LEFT, Theme_GetColor(THEME_TEXT, 255, 1.0f), name);
	}
	
	nvgResetScissor(vg);
	
	ScrollBar_Draw(&this->scroll, vg);
	
	TypeSearch_Draw(&this->type, scrollRect, vg, search);
	ReadPath(this, travel);
}

static void Draw_SearchPanel(FileDialog* this, ContextMenu* context, Rect mainRect, void* vg) {
	struct SearchPanel* search = &this->search;
	
	Gfx_DrawRounderRect(vg, mainRect, Theme_GetColor(THEME_BASE, 255, 1.0f));
	
	Element_Row(&search->back, ElAbs(SPLIT_TEXT_H), &search->path, 0.75f, &search->search, 0.25f);
	
	if (Element_Button(&search->back))
		ReadPath(this, Travel(this, "../"));
	
	if (Element_Textbox(&search->path)) {
		strrep(search->path.txt, "\\", "/");
		if (!strend(search->path.txt, "/")) strcat(search->path.txt, "/");
		ReadPath(this, search->path.txt);
	}
	
	if (Element_Textbox(&search->search) || search->search.modified)
		SetCullFlag(this, search->search.txt);
}

static void Draw_ConfirmPanel(FileDialog* this, ContextMenu* context, Rect mainRect, void* vg) {
	struct ConfirmPanel* confirm = &this->confirm;
	
	Gfx_DrawRounderRect(vg, mainRect, Theme_GetColor(THEME_BASE, 255, 1.0f));
	
	Element_Row(&confirm->textbox, 1.00f, &confirm->confirm, ElAbs(120 * gPixelScale), &confirm->cancel, ElAbs(120 * gPixelScale));
	if (Element_Textbox(&confirm->textbox)) {
		if (sys_stat(confirm->textbox.txt)) {
			strrep(confirm->textbox.txt, "\\", "/");
			
			if (sys_isdir(confirm->textbox.txt)) {
				if (!strend(confirm->textbox.txt, "/"))
					strcat(confirm->textbox.txt, "/");
				ReadPath(this, confirm->textbox.txt);
				ClearSelections(this);
			} else {
				ReadPath(this, x_path(confirm->textbox.txt));
				strcpy(confirm->textbox.txt, x_filename(confirm->textbox.txt));
			}
		}
		
		UpdateSelections(this, TEXT_TO_LIST);
	}
	
	Element_Condition(&confirm->confirm, confirm->textbox.txt[0]);
	
	if (Element_Button(&confirm->confirm))
		context->state.setCondition = 1;
	
	if (Element_Button(&confirm->cancel))
		context->state.setCondition = -1;
}

////////////////////////////////////////////////////////////////////////////////

const char* sFileDialogActionTable[] = {
	[FILE_DIALOG_OPEN_FILE] = "open",
	[FILE_DIALOG_SAVE_FILE] = "save",
	[FILE_DIALOG_FOLDER] = "folder",
};
const char* sFileDialogMessage[FILE_DIALOG_MAX] = {
	[FILE_DIALOG_OPEN_FILE] = "Open",
	[FILE_DIALOG_SAVE_FILE] = "Save",
	[FILE_DIALOG_FOLDER] = "Select",
};

static inline Toml* OpenToml() {
	const char* file = x_fmt("%sfile_dialog.toml", sys_appdata());
	static Toml toml;
	
	toml = Toml_New();
	
	if (sys_stat(file))
		Toml_Load(&toml, file);
	
	return &toml;
}

static inline void SaveToml(Toml* toml) {
	const char* file = x_fmt("%sfile_dialog.toml", sys_appdata());
	
	Toml_Save(toml, file);
}

static void LoadConfig(FileDialog* this) {
	Toml* toml = OpenToml();
	const char* work;
	
	if (Toml_Var(toml, "%s.path", sFileDialogActionTable[this->action])) {
		work = Toml_GetStr(toml, "%s.path", sFileDialogActionTable[this->action]);
		
		strncpy(this->path, work, PATH_BUFFER_SIZE);
		delete(work);
	} else
		strncpy(this->path, sys_appdir(), PATH_BUFFER_SIZE);
	
	Element_Textbox_SetText(&this->search.path, this->path);
	
	for (char i = 'A'; i <= 'Z'; i++) {
		char volume[PATH_BUFFER_SIZE] = {};
		
		xl_snprintf(volume, PATH_BUFFER_SIZE, "%c:/", i);
		
		if (sys_isdir(volume))
			Arli_Add(&this->volumes.list, volume);
	}
	
	int count = Toml_ArrCount(toml, "bookmark");
	
	for (int i = 0; i < count; i++) {
		char buffer[PATH_BUFFER_SIZE] = {};
		
		work = Toml_GetStr(toml, "bookmark[%d]", i);
		strncpy(buffer, work, PATH_BUFFER_SIZE);
		
		Arli_Add(&this->bookmarks.list, buffer);
		
		delete(work);
	}
	
	if (Toml_Var(toml, "volume_slot_num"))
		this->volumes.container.element.slotNum = Toml_GetInt(toml, "volume_slot_num");
	if (Toml_Var(toml, "bookmark_slot_num"))
		this->bookmarks.container.element.slotNum = Toml_GetInt(toml, "bookmark_slot_num");
	
	if (Toml_Var(toml, "volume_panel_state"))
		this->volumes.panel.state = Toml_GetInt(toml, "volume_panel_state");
	if (Toml_Var(toml, "bookmark_panel_state"))
		this->bookmarks.panel.state = Toml_GetInt(toml, "bookmark_panel_state");
	
	Toml_Free(toml);
}

static void SaveConfig(FileDialog* this) {
	Toml* toml = OpenToml();
	
	Toml_SetVar(toml, x_fmt("%s.path", sFileDialogActionTable[this->action]), "\"%s\"", this->path);
	
	Toml_RmArr(toml, "bookmark");
	for (int i = 0; i < this->bookmarks.list.num; i++)
		Toml_SetVar(toml, x_fmt("bookmark[%d]", i), "\"%s\"", Arli_At(&this->bookmarks.list, i));
	
	Toml_SetVar(toml, "volume_slot_num", "%d", this->volumes.container.element.slotNum);
	Toml_SetVar(toml, "bookmark_slot_num", "%d", this->bookmarks.container.element.slotNum);
	Toml_SetVar(toml, "volume_panel_state", "%d", this->volumes.panel.state);
	Toml_SetVar(toml, "bookmark_panel_state", "%d", this->bookmarks.panel.state);
	
	SaveToml(toml);
	Toml_Free(toml);
}

////////////////////////////////////////////////////////////////////////////////

static const char* ArliCallback_FolderName(Arli* this, size_t index) {
	const char* str = Arli_At(this, index);
	
	if (str && str[0]) {
		str = x_pathslot(str, -1);
		if (strend(str, "/"))
			strend(str, "/")[0] = '\0';
	} else
		return "(null)";
	
	return str;
}

static const char* ArliCallback_VolumeName(Arli* this, size_t index) {
	const char* v = Arli_At(this, index);
	
	return x_fmt("%s %s", v, sys_volumename(v));
}

////////////////////////////////////////////////////////////////////////////////

static void Destroy(NanoGrid* mainGrid, ContextMenu* context) {
	FileDialog* this = context->udata;
	struct SidePanel* volumes = &this->volumes;
	struct SidePanel* bookmarks = &this->bookmarks;
	
	SaveConfig(this);
	
	Arli_Free(&volumes->list);
	Arli_Free(&bookmarks->list);
	
	for (int i = 0; i < ArrCount(this->split); i++)
		delete(this->split[i]);
	delete(this->searchFilterFlagList);
	
	if (this->filter) {
		List_Free(this->filter);
		delete(this->filter);
	}
	
	mainGrid->state.cullSplits = false;
	
	this->done = true;
	if (context->state.setCondition < 1)
		this->done = -1;
}

static void Init(NanoGrid* mainGrid, ContextMenu* context) {
	FileDialog* this = context->udata;
	FileDialogInit* init = (void*)context->element;
	struct SidePanel* volumes = &this->volumes;
	struct SidePanel* bookmarks = &this->bookmarks;
	struct SearchPanel* search = &this->search;
	struct ConfirmPanel* confirm = &this->confirm;
	
	context->element = NULL;
	
	this->filter = init->filter;
	this->action = init->action;
	this->canMultiSelect = init->multiSelect;
	this->index = Arli_New(int);
	
	for (int i = 0; i < ArrCount(this->split); i++)
		this->split[i] = new(Split);
	this->path = new(char[PATH_BUFFER_SIZE]);
	
	this->files = List_New();
	this->folders = List_New();
	volumes->list = Arli_New(char[PATH_BUFFER_SIZE]);
	bookmarks->list = Arli_New(char[PATH_BUFFER_SIZE]);
	Arli_SetElemNameCallback(&volumes->list, ArliCallback_VolumeName);
	Arli_SetElemNameCallback(&bookmarks->list, ArliCallback_FolderName);
	
	context->state.offsetOriginRect = false;
	context->state.widthAdjustment = false;
	context->state.distanceCheck = false;
	context->state.maximize = true;
	mainGrid->state.cullSplits = 1;
	
	NanoGrid_Init(&this->nano, GET_WINDOW(), NULL);
	
	Element_SetName(&confirm->confirm, sFileDialogMessage[this->action]);
	Element_SetName(&confirm->cancel, "Cancel");
	
	Element_Button_SetProperties(&confirm->confirm, false, false);
	Element_Button_SetProperties(&confirm->cancel, false, false);
	Element_Container_SetArli(&volumes->container, &volumes->list, 5);
	Element_Container_SetArli(&bookmarks->container, &bookmarks->list, 5);
	volumes->container.align = NVG_ALIGN_LEFT;
	volumes->container.controller = true;
	volumes->container.stretch = true;
	bookmarks->container.align = NVG_ALIGN_LEFT;
	bookmarks->container.controller = true;
	bookmarks->container.stretch = true;
	bookmarks->container.mutable = true;
	
	search->path.align = NVG_ALIGN_LEFT;
	search->search.align = NVG_ALIGN_LEFT;
	confirm->textbox.align = NVG_ALIGN_LEFT;
	
	search->path.size = PATH_BUFFER_SIZE;
	search->search.size = 32;
	
	confirm->confirm.align = confirm->cancel.align = NVG_ALIGN_CENTER;
	confirm->confirm.element.colOvrdBase = THEME_PRIM;
	
	LoadConfig(this);
	ReadPath(this, this->path);
}

static void Draw(NanoGrid* mainGrid, ContextMenu* context) {
	FileDialog* this = context->udata;
	void* vg = mainGrid->vg;
	Rect r;
	int sliceA = context->rect.w * 0.25f;
	int sliceB = context->rect.w - sliceA;
	int height = SPLIT_TEXT_H + SPLIT_ELEM_X_PADDING * 4;
	Input* input = GET_INPUT();
	
	if (DummyGrid_InputCheck(&this->nano)) {
		if (!this->multiSelect && this->index.num) {
			if (Input_GetKey(input, KEY_ENTER)->press) {
				if (this->type.timer.ongoing)
					this->type.timer = TimerSet(0.0f);
				else
					context->state.setCondition = 1;
			}
		}
		if (Input_GetKey(input, KEY_ESCAPE)->press)
			context->state.setCondition = -1;
	}
	
	DummyGrid_Push(&this->nano);
	
	r = Rect_ShrinkX(context->rect, -sliceB);
	osLog("FileDialow: SidePanel");
	DummySplit_Push(&this->nano, this->split[0], r); {
		r.w--;
		Gfx_DrawRounderOutline(vg, r, Theme_GetColor(THEME_BASE, 255, 1.5f));
		
		Draw_SidePanel(this, context, this->split[0]->rect, vg);
	} DummySplit_Pop(&this->nano, this->split[0]);
	
	r = Rect_ShrinkX(context->rect, sliceA);
	r = Rect_ShrinkY(r, (context->rect.h - height));
	osLog("FileDialow: SearchPanel");
	DummySplit_Push(&this->nano, this->split[1], r); {
		r.h--;
		Gfx_DrawRounderOutline(vg, r, Theme_GetColor(THEME_BASE, 255, 1.5f));
		
		Draw_SearchPanel(this, context, this->split[1]->rect, vg);
	} DummySplit_Pop(&this->nano, this->split[1]);
	
	r = Rect_ShrinkX(context->rect, sliceA);
	r = Rect_ShrinkY(r, -height);
	r = Rect_ShrinkY(r, height);
	osLog("FileDialow: FilePanel");
	DummySplit_Push(&this->nano, this->split[2], r); {
		r.h--;
		Gfx_DrawRounderOutline(vg, r, Theme_GetColor(THEME_BASE, 255, 1.5f));
		
		Draw_FilePanel(this, context, this->split[2]->rect, vg);
	} DummySplit_Pop(&this->nano, this->split[2]);
	
	r = Rect_ShrinkX(context->rect, sliceA);
	r = Rect_ShrinkY(r, -(context->rect.h - height));
	osLog("FileDialow: BottomPanel");
	DummySplit_Push(&this->nano, this->split[3], r); {
		Gfx_DrawRounderOutline(vg, r, Theme_GetColor(THEME_BASE, 255, 1.5f));
		
		Draw_ConfirmPanel(this, context, this->split[3]->rect, vg);
	} DummySplit_Pop(&this->nano, this->split[3]);
	
	DummyGrid_Pop(&this->nano);
}

////////////////////////////////////////////////////////////////////////////////

FileDialog* FileDialog_Open(NanoGrid* nano, char action, const char* basename, const char* filter) {
	FileDialog* this = new(FileDialog);
	Rect r = { 0, 0, UnfoldVec2(*nano->wdim) };
	FileDialogInit init = {};
	
	switch (action) {
		case 'm':
		case 'M':
			init.multiSelect = true;
		case 'o':
		case 'O':
			init.action = FILE_DIALOG_OPEN_FILE;
			break;
			
		case 's':
		case 'S':
			init.action = FILE_DIALOG_SAVE_FILE;
			break;
			
		case 'f':
		case 'F':
			init.action = FILE_DIALOG_FOLDER;
			break;
			
		default:
			warn("\aTrying to call FileDialog with action '%c'", action);
			break;
	}
	
	if (filter) {
		List* list = new(List);
		*list = List_New();
		
		List_Tokenize(list, filter, ',');
		List_Print(list);
		
		init.filter = list;
	}
	
	ContextMenu_Custom(nano, this, &init, Init, Draw, Destroy, r);
	
	return this;
}

bool FileDialog_Poll(FileDialog* this) {
	return (this && this->done);
}

List* FileDialog_GetResult(FileDialog** __this) {
	FileDialog* this = *__this;
	List* list = NULL;
	
	info("result: %d", this->index.num);
	
	if (this->index.num && this->done > 0) {
		switch ( this->action ) {
			case FILE_DIALOG_OPEN_FILE:
			case FILE_DIALOG_SAVE_FILE: {
				Arli* index = &this->index;
				list = new(List);
				*list = List_New();
				
				for (int i = 0; i < index->num; i++)
					List_Add(list, x_fmt("%s%s", this->path, GetIndexArliItemString(this, i)));
			} break;
			
			case FILE_DIALOG_FOLDER:
				list = new(List);
				*list = List_New();
				List_Add(list, this->path);
				break;
				
			default:
				break;
		}
	}
	
	Arli_Free(&this->index);
	List_Free(&this->files);
	List_Free(&this->folders);
	delete(this->path, this);
	*__this = NULL;
	
	return list;
}
