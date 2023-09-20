#define GEOGRID_C 1
#include "nano_grid.h"
#include "ext_interface.h"

//crustify
f32 GetSplitPixel()        { return (1                                                              ) * gPixelScale; }
f32 GetSplitScrollPad()    { return (2                                                              ) * gPixelScale; }
f32 GetSplitSplitW()       { return (2                                                              ) * gPixelScale; }
f32 GetSplitRoundR()       { return (2                                                              ) * gPixelScale; }
f32 GetSplitGrabDist()     { return (4                                                              ) * gPixelScale; }
f32 GetSplitTextPadding()  { return (4                                                              ) * gPixelScale; }
f32 GetSplitText()         { return (12                                                             ) * gPixelScale; }
f32 GetSplitScrollWidth()  { return (14                                                             ) * gPixelScale; }
f32 GetSplitCtxmDist()     { return (32                                                             ) * gPixelScale; }
f32 GetSplitClamp()        { return (((GetSplitBarHeight() + GetSplitSplitW() * 1.25) * 2)          ); }
f32 GetSplitTextH()        { return ((GetSplitTextPadding() * 2 + GetSplitText()) ); }
f32 GetSplitIconH()        { return ((gPixelScale * 6 + GetSplitText()) ); }
f32 GetSplitBarHeight()    { return ((gPixelScale * 16 + GetSplitText()) ); }
f32 GetSplitElemXPadding() { return ((GetSplitText() * 0.5f)                                        ); }
f32 GetSplitElemYPadding() { return ((GetSplitTextH() + GetSplitElemXPadding())                     ); }
//uncrustify

static void NanoGrid_RemoveDuplicates(NanoGrid* nano);
static void Split_UpdateRect(Split* split);
void Element_SetContext(NanoGrid* setGeo, Split* setSplit);
void DragItem_Draw(NanoGrid* nano);

Vec2f gZeroVec2f;
Vec2s gZeroVec2s;
Rect gZeroRect;

static s32 sDebugMode;

#if 0
static void ggDebug(const char* fmt, ...) {
	if (sDebugMode == false)
		return;
	
	va_list va;
	
	va_start(va, fmt);
	vprintf(fmt, va);
	va_end(va);
	printf("\n");
}
#endif

/* ───────────────────────────────────────────────────────────────────────── */

static SplitDir NanoGrid_GetDir_Opposite(SplitDir dir) {
	return wrap(dir + 2, DIR_L, DIR_B + 1);
}

static SplitDir NanoGrid_GetDir_MouseToPressPos(Split* split) {
	Vec2s pos = Vec2s_Sub(split->cursorPos, split->cursorPressPos);
	
	if (abs(pos.x) > abs(pos.y)) {
		if (pos.x < 0) {
			return DIR_L;
		}
		
		return DIR_R;
	} else {
		if (pos.y < 0) {
			return DIR_T;
		}
		
		return DIR_B;
	}
}

static SplitVtx* NanoGrid_AddVtx(NanoGrid* nano, f64 x, f64 y) {
	SplitVtx* head = nano->vtxHead;
	SplitVtx* vtx;
	
	while (head) {
		if (fabs(head->pos.x - x) < 0.25 && fabs(head->pos.y - y) < 0.25)
			return head;
		head = head->next;
	}
	
	vtx = new(SplitVtx);
	info("New Vtx: %s", addr_name(vtx));
	
	vtx->pos.x = x;
	vtx->pos.y = y;
	Node_Add(nano->vtxHead, vtx);
	
	return vtx;
}

static SplitEdge* NanoGrid_AddEdge(NanoGrid* nano, SplitVtx* v1, SplitVtx* v2) {
	SplitEdge* head = nano->edgeHead;
	SplitEdge* edge = NULL;
	
	osAssert(v1 != NULL && v2 != NULL);
	
	if (v1->pos.y == v2->pos.y) {
		if (v1->pos.x > v2->pos.x)
			Swap(v1, v2);
	} else {
		if (v1->pos.y > v2->pos.y)
			Swap(v1, v2);
	}
	
	while (head) {
		if (head->vtx[0] == v1 && head->vtx[1] == v2) {
			edge = head;
			break;
		}
		head = head->next;
	}
	
	if (edge == NULL) {
		edge = new(SplitEdge);
		info("New Edge: %s", addr_name(edge));
		
		edge->vtx[0] = v1;
		edge->vtx[1] = v2;
		Node_Add(nano->edgeHead, edge);
	}
	
	if (edge->vtx[0]->pos.y == edge->vtx[1]->pos.y) {
		edge->state |= EDGE_HORIZONTAL;
		edge->pos = edge->vtx[0]->pos.y;
		if (edge->pos < nano->workRect.y + 1) {
			edge->state |= EDGE_STICK_T;
		}
		if (edge->pos > nano->workRect.y + nano->workRect.h - 1) {
			edge->state |= EDGE_STICK_B;
		}
	} else {
		edge->state |= EDGE_VERTICAL;
		edge->pos = edge->vtx[0]->pos.x;
		if (edge->pos < nano->workRect.x + 1) {
			edge->state |= EDGE_STICK_L;
		}
		if (edge->pos > nano->workRect.x + nano->workRect.w - 1) {
			edge->state |= EDGE_STICK_R;
		}
	}
	
	return edge;
}

static s32 Split_CursorDistToFlagPos(SplitState flag, Split* split) {
	/*
	 * Manipulate distance check based on the flag so that we get the
	 * distance always to the point or line we need. For points we
	 * utilize both cursor positions, for sides (edge) we utilize only
	 * one axis that makes sense for the direction of that side.
	 */
	Vec2s cursor[] = {
		{ split->cursorPos.x, split->cursorPos.y, }, /* SPLIT_POINT_BL */
		{ split->cursorPos.x, split->cursorPos.y, }, /* SPLIT_POINT_TL */
		{ split->cursorPos.x, split->cursorPos.y, }, /* SPLIT_POINT_TR */
		{ split->cursorPos.x, split->cursorPos.y, }, /* SPLIT_POINT_BR */
		{ split->cursorPos.x, 0,                  }, /* SPLIT_SIDE_L   */
		{ 0,                  split->cursorPos.y, }, /* SPLIT_SIDE_T   */
		{ split->cursorPos.x, 0,                  }, /* SPLIT_SIDE_R   */
		{ 0,                  split->cursorPos.y, }, /* SPLIT_SIDE_B   */
	};
	Vec2s pos[] = {
		{ 0,                 split->dispRect.h, }, /* SPLIT_POINT_BL */
		{ 0,                 0,                 }, /* SPLIT_POINT_TL */
		{ split->dispRect.w, 0,                 }, /* SPLIT_POINT_TR */
		{ split->dispRect.w, split->dispRect.h, }, /* SPLIT_POINT_BR */
		{ 0,                 0,                 }, /* SPLIT_SIDE_L   */
		{ 0,                 0,                 }, /* SPLIT_SIDE_T   */
		{ split->dispRect.w, 0,                 }, /* SPLIT_SIDE_R   */
		{ 0,                 split->dispRect.h, }, /* SPLIT_SIDE_B   */
	};
	s32 i;
	
	i = bitfield_lzeronum(flag);
	
	return Vec2s_DistXZ(cursor[i], pos[i]);
}

static SplitState Split_GetCursorPosState(Split* split, s32 range) {
	for (int i = 0; (1 << i) <= SPLIT_SIDE_B; i++) {
		if (Split_CursorDistToFlagPos((1 << i), split) <= range) {
			SplitEdge* edge = NULL;
			EdgeState stick = 0;
			
			switch (1 << i) {
				case SPLIT_SIDE_L:
					edge = split->edge[EDGE_L];
					stick = EDGE_STICK_L;
					break;
				case SPLIT_SIDE_T:
					edge = split->edge[EDGE_T];
					stick = EDGE_STICK_T;
					break;
				case SPLIT_SIDE_R:
					edge = split->edge[EDGE_R];
					stick = EDGE_STICK_R;
					break;
				case SPLIT_SIDE_B:
					edge = split->edge[EDGE_B];
					stick = EDGE_STICK_B;
					break;
			}
			
			if (!edge || !(edge->state & stick))
				return (1 << i);
		}
	}
	
	return SPLIT_POINT_NONE;
}

static const char* Split_TaskListElemName(Arli* list, size_t pos) {
	return *((char**)Arli_At(list, pos));
}

static void Split_SetupTaskEnum(NanoGrid* nano, Split* this) {
	this->taskList = new(Arli);
	*this->taskList = Arli_New(void*);
	this->taskCombo = new(ElCombo);
	this->taskCombo->controller = true;
	this->taskCombo->align = NVG_ALIGN_CENTER;
	
	osAssert(this->taskList != NULL);
	osAssert(this->taskCombo != NULL);
	
	for (int i = 0; i < nano->numTaskTable; i++)
		Arli_Add(this->taskList, &nano->taskTable[i]->taskName);
	
	Arli_Set(this->taskList, this->id);
	Arli_SetElemNameCallback(this->taskList, Split_TaskListElemName);
	
	Element_Combo_SetArli(this->taskCombo, this->taskList);
}

static Split* Split_Alloc(NanoGrid* nano, s32 id) {
	Split* split = new(Split);
	
	split->prevId = -1; // Forces init
	split->id = id;
	
	Split_SetupTaskEnum(nano, split);
	
	return split;
}

static void Edge_SetSlideClamp(NanoGrid* nano) {
	SplitEdge* tempEdge = nano->edgeHead;
	SplitEdge* setEdge = nano->actionEdge;
	SplitEdge* actionEdge = nano->actionEdge;
	u32 align = ((actionEdge->state & EDGE_VERTICAL) != 0);
	f64 posMin = actionEdge->vtx[0]->pos.axis[align];
	f64 posMax = actionEdge->vtx[1]->pos.axis[align];
	
	setEdge->state |= EDGE_EDIT;
	
	// Get edge with vtx closest to TOPLEFT
	while (tempEdge) {
		osAssert(!(tempEdge->state & EDGE_VERTICAL && tempEdge->state & EDGE_HORIZONTAL));
		
		if ((tempEdge->state & EDGE_ALIGN) == (setEdge->state & EDGE_ALIGN)) {
			if (tempEdge->vtx[1] == setEdge->vtx[0]) {
				setEdge = tempEdge;
				tempEdge->state |= EDGE_EDIT;
				tempEdge = nano->edgeHead;
				posMin = setEdge->vtx[0]->pos.axis[align];
				continue;
			}
		}
		
		tempEdge = tempEdge->next;
	}
	
	tempEdge = nano->edgeHead;
	
	// Set all below setEdgeA
	while (tempEdge) {
		if ((tempEdge->state & EDGE_ALIGN) == (setEdge->state & EDGE_ALIGN)) {
			if (tempEdge->vtx[0] == setEdge->vtx[1]) {
				tempEdge->state |= EDGE_EDIT;
				setEdge = tempEdge;
				tempEdge = nano->edgeHead;
				posMax = setEdge->vtx[1]->pos.axis[align];
				continue;
			}
		}
		
		tempEdge = tempEdge->next;
	}
	
	tempEdge = nano->edgeHead;
	
	while (tempEdge) {
		if ((tempEdge->state & EDGE_ALIGN) == (actionEdge->state & EDGE_ALIGN)) {
			if (tempEdge->pos == actionEdge->pos) {
				if (tempEdge->vtx[1]->pos.axis[align] <= posMax && tempEdge->vtx[0]->pos.axis[align] >= posMin) {
					tempEdge->state |= EDGE_EDIT;
				}
			}
		}
		tempEdge = tempEdge->next;
	}
	
	if (nano->actionEdge->state & EDGE_VERTICAL) {
		nano->slide.clampMin = nano->workRect.x;
		nano->slide.clampMax = nano->workRect.x + nano->workRect.w;
	} else {
		nano->slide.clampMin = nano->workRect.y;
		nano->slide.clampMax = nano->workRect.y + nano->workRect.h;
	}
}

static void Split_ClearActionSplit(NanoGrid* nano) {
	osAssert(nano->actionSplit);
	nano->actionSplit->state &= ~(SPLIT_POINTS | SPLIT_SIDES);
	
	nano->actionSplit = NULL;
}

static void Split_Split(NanoGrid* nano, Split* split, SplitDir dir) {
	Split* newSplit;
	f64 splitPos = (dir == DIR_L || dir == DIR_R) ? nano->input->cursor.pos.x : nano->input->cursor.pos.y;
	
	newSplit = Split_Alloc(nano, split->id);
	Node_Add(nano->splitHead, newSplit);
	
	if (dir == DIR_L) {
		osLog("SplitTo DIR_L");
		newSplit->vtx[0] = NanoGrid_AddVtx(nano, splitPos, split->vtx[0]->pos.y);
		newSplit->vtx[1] = NanoGrid_AddVtx(nano, splitPos, split->vtx[1]->pos.y);
		newSplit->vtx[2] =  NanoGrid_AddVtx(nano, split->vtx[2]->pos.x, split->vtx[2]->pos.y);
		newSplit->vtx[3] = NanoGrid_AddVtx(nano, split->vtx[3]->pos.x, split->vtx[3]->pos.y);
		split->vtx[2] = NanoGrid_AddVtx(nano, splitPos, split->vtx[2]->pos.y);
		split->vtx[3] = NanoGrid_AddVtx(nano, splitPos, split->vtx[3]->pos.y);
	}
	
	if (dir == DIR_R) {
		osLog("SplitTo DIR_R");
		newSplit->vtx[0] = NanoGrid_AddVtx(nano, split->vtx[0]->pos.x, split->vtx[0]->pos.y);
		newSplit->vtx[1] = NanoGrid_AddVtx(nano, split->vtx[1]->pos.x, split->vtx[1]->pos.y);
		newSplit->vtx[2] = NanoGrid_AddVtx(nano, splitPos, split->vtx[2]->pos.y);
		newSplit->vtx[3] = NanoGrid_AddVtx(nano, splitPos, split->vtx[3]->pos.y);
		split->vtx[0] = NanoGrid_AddVtx(nano, splitPos, split->vtx[0]->pos.y);
		split->vtx[1] = NanoGrid_AddVtx(nano, splitPos, split->vtx[1]->pos.y);
	}
	
	if (dir == DIR_T) {
		osLog("SplitTo DIR_T");
		newSplit->vtx[0] = NanoGrid_AddVtx(nano, split->vtx[0]->pos.x, split->vtx[0]->pos.y);
		newSplit->vtx[1] = NanoGrid_AddVtx(nano, split->vtx[1]->pos.x, splitPos);
		newSplit->vtx[2] = NanoGrid_AddVtx(nano, split->vtx[2]->pos.x, splitPos);
		newSplit->vtx[3] = NanoGrid_AddVtx(nano, split->vtx[3]->pos.x, split->vtx[3]->pos.y);
		split->vtx[3] = NanoGrid_AddVtx(nano, split->vtx[3]->pos.x, splitPos);
		split->vtx[0] = NanoGrid_AddVtx(nano, split->vtx[0]->pos.x, splitPos);
	}
	
	if (dir == DIR_B) {
		osLog("SplitTo DIR_B");
		newSplit->vtx[0] = NanoGrid_AddVtx(nano, split->vtx[0]->pos.x, splitPos);
		newSplit->vtx[1] = NanoGrid_AddVtx(nano, split->vtx[1]->pos.x, split->vtx[1]->pos.y);
		newSplit->vtx[2] = NanoGrid_AddVtx(nano, split->vtx[2]->pos.x, split->vtx[2]->pos.y);
		newSplit->vtx[3] = NanoGrid_AddVtx(nano, split->vtx[3]->pos.x, splitPos);
		split->vtx[1] = NanoGrid_AddVtx(nano, split->vtx[1]->pos.x, splitPos);
		split->vtx[2] = NanoGrid_AddVtx(nano, split->vtx[2]->pos.x, splitPos);
	}
	
	osLog("Form Edges A");
	split->edge[EDGE_L] = NanoGrid_AddEdge(nano, split->vtx[VTX_BOT_L], split->vtx[VTX_TOP_L]);
	split->edge[EDGE_T] = NanoGrid_AddEdge(nano, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
	split->edge[EDGE_R] = NanoGrid_AddEdge(nano, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
	split->edge[EDGE_B] = NanoGrid_AddEdge(nano, split->vtx[VTX_BOT_R], split->vtx[VTX_BOT_L]);
	
	osLog("Form Edges B");
	newSplit->edge[EDGE_L] = NanoGrid_AddEdge(nano, newSplit->vtx[VTX_BOT_L], newSplit->vtx[VTX_TOP_L]);
	newSplit->edge[EDGE_T] = NanoGrid_AddEdge(nano, newSplit->vtx[VTX_TOP_L], newSplit->vtx[VTX_TOP_R]);
	newSplit->edge[EDGE_R] = NanoGrid_AddEdge(nano, newSplit->vtx[VTX_TOP_R], newSplit->vtx[VTX_BOT_R]);
	newSplit->edge[EDGE_B] = NanoGrid_AddEdge(nano, newSplit->vtx[VTX_BOT_R], newSplit->vtx[VTX_BOT_L]);
	
	osLog("Clean");
	nano->actionEdge = newSplit->edge[dir];
	NanoGrid_RemoveDuplicates(nano);
	Edge_SetSlideClamp(nano);
}

static void Split_Free(Split* this) {
	info("Kill Split: %s", addr_name(this));
	Arli_Free(this->taskList);
	delete(this->taskList, this->taskCombo, this->instance);
}

static void Split_Kill(NanoGrid* nano, Split* split, SplitDir dir) {
	SplitEdge* sharedEdge = split->edge[dir];
	Split* killSplit = nano->splitHead;
	SplitDir oppositeDir = NanoGrid_GetDir_Opposite(dir);
	
	while (killSplit) {
		if (killSplit->edge[oppositeDir] == sharedEdge) {
			break;
		}
		
		killSplit = killSplit->next;
	}
	
	if (killSplit == NULL)
		return;
	
	split->edge[oppositeDir]->vtx[0]->killFlag = 5;
	split->edge[oppositeDir]->vtx[1]->killFlag = 5;
	split->edge[dir]->vtx[0]->killFlag = 5;
	split->edge[dir]->vtx[1]->killFlag = 5;
	
	if (dir == DIR_L) {
		osLog("Kill DIR_L");
		split->vtx[VTX_BOT_L] = killSplit->vtx[VTX_BOT_L];
		split->vtx[VTX_TOP_L] = killSplit->vtx[VTX_TOP_L];
		split->edge[EDGE_T] = NanoGrid_AddEdge(nano, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
		split->edge[EDGE_B] = NanoGrid_AddEdge(nano, split->vtx[VTX_BOT_L], split->vtx[VTX_BOT_R]);
	}
	
	if (dir == DIR_T) {
		osLog("Kill DIR_T");
		split->vtx[VTX_TOP_L] = killSplit->vtx[VTX_TOP_L];
		split->vtx[VTX_TOP_R] = killSplit->vtx[VTX_TOP_R];
		split->edge[EDGE_L] = NanoGrid_AddEdge(nano, split->vtx[VTX_TOP_L], split->vtx[VTX_BOT_L]);
		split->edge[EDGE_R] = NanoGrid_AddEdge(nano, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
	}
	
	if (dir == DIR_R) {
		osLog("Kill DIR_R");
		split->vtx[VTX_BOT_R] = killSplit->vtx[VTX_BOT_R];
		split->vtx[VTX_TOP_R] = killSplit->vtx[VTX_TOP_R];
		split->edge[EDGE_T] = NanoGrid_AddEdge(nano, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
		split->edge[EDGE_B] = NanoGrid_AddEdge(nano, split->vtx[VTX_BOT_L], split->vtx[VTX_BOT_R]);
	}
	
	if (dir == DIR_B) {
		osLog("Kill DIR_B");
		split->vtx[VTX_BOT_L] = killSplit->vtx[VTX_BOT_L];
		split->vtx[VTX_BOT_R] = killSplit->vtx[VTX_BOT_R];
		split->edge[EDGE_L] = NanoGrid_AddEdge(nano, split->vtx[VTX_TOP_L], split->vtx[VTX_BOT_L]);
		split->edge[EDGE_R] = NanoGrid_AddEdge(nano, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
	}
	
	split->edge[dir] = killSplit->edge[dir];
	if (nano->taskTable[killSplit->id]->destroy)
		nano->taskTable[killSplit->id]->destroy(nano->passArg, killSplit->instance, killSplit);
	
	nano->killSplit = killSplit;
	Split_Free(killSplit);
	Node_Kill(nano->splitHead, killSplit);
	NanoGrid_RemoveDuplicates(nano);
}

/* ───────────────────────────────────────────────────────────────────────── */

static void Vtx_RemoveDuplicates(NanoGrid* nano, SplitVtx* vtx) {
	SplitVtx* vtx2 = nano->vtxHead;
	
	while (vtx2) {
		if (vtx2 == vtx) {
			vtx2 = vtx2->next;
			continue;
		}
		
		if (!veccmp(&vtx->pos, &vtx2->pos)) {
			SplitVtx* kill = vtx2;
			Split* s = nano->splitHead;
			SplitEdge* e = nano->edgeHead;
			
			while (s) {
				for (int i = 0; i < 4; i++) {
					if (s->vtx[i] == vtx2) {
						s->vtx[i] = vtx;
					}
				}
				s = s->next;
			}
			
			while (e) {
				for (int i = 0; i < 2; i++) {
					if (e->vtx[i] == vtx2) {
						e->vtx[i] = vtx;
					}
				}
				e = e->next;
			}
			
			vtx2 = vtx2->next;
			Node_Kill(nano->vtxHead, kill);
			continue;
		}
		
		vtx2 = vtx2->next;
	}
}

static void Vtx_Update(NanoGrid* nano) {
	SplitVtx* vtx = nano->vtxHead;
	
	if (nano->actionEdge != NULL)
		nano->state.cleanVtx = true;
	
	while (vtx) {
		if (nano->state.cleanVtx == true && nano->actionEdge == NULL)
			Vtx_RemoveDuplicates(nano, vtx);
		
		if (vtx->killFlag) {
			if (vtx->killFlag == 1) {
				fornode(s, nano->splitHead) {
					for (int i = 0; i < 4; i++)
						if (s->vtx[i] == vtx)
							vtx->killFlag = false;
				}
				
				fornode(e, nano->edgeHead) {
					for (int i = 0; i < 2; i++)
						if (e->vtx[i] == vtx)
							vtx->killFlag = false;
				}
				
				if (vtx->killFlag) {
					info("Kill Vtx: %s", addr_name(vtx));
					
					Node_Kill(nano->vtxHead, vtx);
					vtx = nano->vtxHead;
					continue;
				}
			} else Decr(vtx->killFlag);
		}
		
		vtx = vtx->next;
		if (vtx == NULL && nano->actionEdge == NULL)
			nano->state.cleanVtx = false;
	}
}

/* ───────────────────────────────────────────────────────────────────────── */

static void Edge_RemoveDuplicates(NanoGrid* nano, SplitEdge* edge) {
	SplitEdge* cur = nano->edgeHead;
	
	while (cur) {
		if (cur == edge) {
			cur = cur->next;
			continue;
		}
		
		if (cur->vtx[0] == edge->vtx[0] && cur->vtx[1] == edge->vtx[1]) {
			SplitEdge* kill = cur;
			Split* s = nano->splitHead;
			
			if (nano->actionEdge == cur)
				nano->actionEdge = edge;
			
			while (s) {
				for (int i = 0; i < 4; i++)
					if (s->edge[i] == cur)
						s->edge[i] = edge;
				s = s->next;
			}
			
			cur = cur->next;
			Node_Kill(nano->edgeHead, kill);
			continue;
		}
		
		cur = cur->next;
	}
}

static void Edge_SetSlide(NanoGrid* nano) {
	SplitEdge* edge = nano->edgeHead;
	f64 diffCentX = (f64)nano->workRect.w / nano->prevWorkRect.w;
	f64 diffCentY = (f64)nano->workRect.h / nano->prevWorkRect.h;
	
	while (edge) {
		bool clampFail = false;
		bool isEditEdge = (edge == nano->actionEdge || edge->state & EDGE_EDIT);
		bool isCornerEdge = ((edge->state & EDGE_STICK) != 0);
		int axis = ((edge->state & EDGE_VERTICAL) == 0) ? 1 : 0;
		
		osAssert(!(edge->state & EDGE_HORIZONTAL && edge->state & EDGE_VERTICAL));
		
		if (isCornerEdge) {
			if (edge->state & EDGE_STICK_L)
				edge->pos = nano->workRect.x;
			
			if (edge->state & EDGE_STICK_T)
				edge->pos = nano->workRect.y;
			
			if (edge->state & EDGE_STICK_R)
				edge->pos = nano->workRect.x + nano->workRect.w;
			
			if (edge->state & EDGE_STICK_B)
				edge->pos = nano->workRect.y + nano->workRect.h;
		} else {
			if (edge->state & EDGE_HORIZONTAL) {
				edge->pos -= nano->workRect.y;
				edge->pos *= diffCentY;
				edge->pos += nano->workRect.y;
			}
			
			if (edge->state & EDGE_VERTICAL)
				edge->pos *= diffCentX;
		}
		
		if (isEditEdge && isCornerEdge == false) {
			SplitEdge* temp = nano->edgeHead;
			s32 align = wrap(edge->state & EDGE_ALIGN, 0, 2);
			
			while (temp) {
				for (int i = 0; i < 2; i++) {
					if (((temp->state & EDGE_ALIGN) != (edge->state & EDGE_ALIGN)) && temp->vtx[1] == edge->vtx[i]) {
						if (temp->vtx[0]->pos.axis[align] > nano->slide.clampMin) {
							nano->slide.clampMin = temp->vtx[0]->pos.axis[align];
						}
					}
				}
				
				for (int i = 0; i < 2; i++) {
					if (((temp->state & EDGE_ALIGN) != (edge->state & EDGE_ALIGN)) && temp->vtx[0] == edge->vtx[i]) {
						if (temp->vtx[1]->pos.axis[align] < nano->slide.clampMax) {
							nano->slide.clampMax = temp->vtx[1]->pos.axis[align];
						}
					}
				}
				
				temp = temp->next;
			}
			
			if (nano->slide.clampMax - SPLIT_CLAMP > nano->slide.clampMin + SPLIT_CLAMP) {
				if (edge->state & EDGE_HORIZONTAL) {
					edge->pos = nano->input->cursor.pos.y;
				}
				if (edge->state & EDGE_VERTICAL) {
					edge->pos = nano->input->cursor.pos.x;
				}
				edge->pos = clamp_min(edge->pos, nano->slide.clampMin + SPLIT_CLAMP);
				edge->pos = clamp_max(edge->pos, nano->slide.clampMax - SPLIT_CLAMP);
			} else {
				clampFail = true;
			}
		}
		
		if (diffCentX == 1.0f && diffCentY == 1.0f && !isCornerEdge) {
			edge->pos = AccuracyF(edge->pos, 0.25);
			edge->pos = AccuracyF(edge->pos, 0.25);
		}
		
		if (isCornerEdge) {
			edge->vtx[0]->pos.axis[axis] = edge->pos;
			edge->vtx[1]->pos.axis[axis] = edge->pos;
		} else {
			if (isEditEdge && isCornerEdge == false && clampFail == false) {
				edge->vtx[0]->pos.axis[axis] = edge->pos;
				edge->vtx[1]->pos.axis[axis] = edge->pos;
			} else {
				edge->vtx[0]->pos.axis[axis] = edge->pos;
				edge->vtx[1]->pos.axis[axis] = edge->pos;
			}
		}
		
		edge = edge->next;
	}
}

static void Edge_Update(NanoGrid* nano) {
	SplitEdge* edge = nano->edgeHead;
	
	if (!Input_GetCursor(nano->input, CLICK_ANY)->hold)
		nano->actionEdge = NULL;
	
	while (edge) {
		SplitEdge* next = edge->next;
		
		if (edge->killFlag == true) {
			info("Kill Edge: %s", addr_name(edge));
			Node_Kill(nano->edgeHead, edge);
			
		} else {
			edge->killFlag = true;
			
			if (nano->actionEdge == NULL)
				edge->state &= ~EDGE_EDIT;
			
			Edge_RemoveDuplicates(nano, edge);
		}
		
		edge = next;
	}
	
	Edge_SetSlide(nano);
}

/* ───────────────────────────────────────────────────────────────────────── */

static void Split_UpdateActionSplit(NanoGrid* nano) {
	Split* split = nano->actionSplit;
	
	if ( Input_GetCursor(nano->input, CLICK_ANY)->press) {
		SplitState tempStateA = Split_GetCursorPosState(split, SPLIT_GRAB_DIST);
		SplitState tempStateB = Split_GetCursorPosState(split, SPLIT_GRAB_DIST * 3);
		
		if (tempStateB & SPLIT_POINTS)
			split->state |= tempStateB;
		
		else if (tempStateA & SPLIT_SIDES)
			split->state |= tempStateA;
		
		if (split->state & SPLIT_SIDES) {
			s32 i;
			
			for (i = 0; i < 4; i++)
				if (split->state & (1 << (4 + i)))
					break;
			
			osAssert(split->edge[i] != NULL);
			
			nano->actionEdge = split->edge[i];
			Edge_SetSlideClamp(nano);
			Split_ClearActionSplit(nano);
		}
	}
	
	if ( Input_GetCursor(nano->input, CLICK_ANY)->hold) {
		if (split->state & SPLIT_POINTS) {
			s32 dist = Vec2s_DistXZ(split->cursorPos, split->cursorPressPos);
			SplitDir dir = NanoGrid_GetDir_MouseToPressPos(split);
			
			if (dist > 1) {
				CursorIndex cid = dir + 1;
				Cursor_SetCursor(cid);
			}
			
			if (dist > SPLIT_CLAMP * 1.05) {
				osLog("Point Action");
				Split_ClearActionSplit(nano);
				
				if (split->cursorInDisp)
					Split_Split(nano, split, dir);
				
				else
					Split_Kill(nano, split, dir);
				
				nano->state.splittedHold = true;
				nano->state.blockElemInput++;
			} else {
				if (!split->cursorInDisp) {
					osLog("Display Kill Arrow");
					SplitEdge* sharedEdge = split->edge[dir];
					Split* killSplit = nano->splitHead;
					SplitDir oppositeDir = NanoGrid_GetDir_Opposite(dir);
					
					while (killSplit) {
						if (killSplit->edge[oppositeDir] == sharedEdge) {
							break;
						}
						
						killSplit = killSplit->next;
					}
					
					if (killSplit)
						killSplit->state |= SPLIT_KILL_DIR_L << dir;
				}
			}
		}
		
		if (split->state & SPLIT_SIDE_H)
			Cursor_SetCursor(CURSOR_ARROW_H);
		
		if (split->state & SPLIT_SIDE_V)
			Cursor_SetCursor(CURSOR_ARROW_V);
	}
}

static void Split_UpdateRect(Split* split) {
	split->dispRect = Rect_New(
		floorf(split->vtx[1]->pos.x),
		floorf(split->vtx[1]->pos.y),
		floorf(split->vtx[3]->pos.x) - floorf(split->vtx[1]->pos.x),
		floorf(split->vtx[3]->pos.y) - floorf(split->vtx[1]->pos.y)
	);
	
	split->rect = Rect_ShrinkY(split->dispRect, SPLIT_BAR_HEIGHT + SPLIT_PIXEL * 2);
	split->headRect = Rect_ShrinkY(split->dispRect, -split->rect.h);
}

static void Split_InitOrSwap(NanoGrid* nano, Split* split) {
	SplitScrollBar* scroll = &split->scroll;
	
	if (split->instance) {
		osLog("" PRNT_REDD "SplitSwapInstance" PRNT_RSET "( %d -> %d )", split->prevId, split->id);
		if (nano->taskTable[split->prevId]->destroy)
			nano->taskTable[split->prevId]->destroy(nano->passArg, split->instance, split);
		scroll->offset = scroll->enabled = 0;
		delete(split->instance);
	}
	
	split->prevId = split->id;
	split->instance = calloc(nano->taskTable[split->id]->size);
	if (nano->taskTable[split->id]->init)
		nano->taskTable[split->id]->init(nano->passArg, split->instance, split);
}

static void Split_UpdateScroll(NanoGrid* nano, Split* split, Input* input) {
	SplitScrollBar* scroll = &split->scroll;
	
	if (nano->state.blockElemInput)
		return;
	
	if (split->scroll.enabled) {
		if (split->cursorInSplit && !split->splitBlockScroll) {
			s32 val = Input_GetScroll(input);
			
			scroll->offset = scroll->offset + SPLIT_ELEM_Y_PADDING * -val;
		}
		
		scroll->offset = clamp(scroll->offset, 0, scroll->max);
	} else
		split->scroll.offset = 0;
	split->splitBlockScroll = 0;
}

static void Split_UpdateInstance(NanoGrid* nano, Split* split) {
	Cursor* cursor = &nano->input->cursor;
	
	osLog("Split %s", addr_name(split));
	if (!split->isHeader)
		Split_UpdateRect(split);
	
	Vec2s rectPos = { split->rect.x, split->rect.y };
	SplitTask** table = nano->taskTable;
	
	split->cursorPos = Vec2s_Sub(cursor->pos, rectPos);
	split->cursorInSplit = Rect_PointIntersect(&split->rect, cursor->pos.x, cursor->pos.y);
	split->cursorInDisp = Rect_PointIntersect(&split->dispRect, cursor->pos.x, cursor->pos.y);
	split->cursorInHeader = Rect_PointIntersect(&split->headRect, cursor->pos.x, cursor->pos.y);
	split->blockCursor = false;
	Split_UpdateScroll(nano, split, nano->input);
	
	if ( Input_GetCursor(nano->input, CLICK_ANY)->press)
		split->cursorPressPos = split->cursorPos;
	
	if (!nano->state.blockSplitting && !split->isHeader) {
		if (split->cursorInDisp) {
			SplitState splitRangeState = Split_GetCursorPosState(split, SPLIT_GRAB_DIST * 3) & SPLIT_POINTS;
			SplitState slideRangeState = Split_GetCursorPosState(split, SPLIT_GRAB_DIST) & SPLIT_SIDES;
			
			if (splitRangeState)
				Cursor_SetCursor(CURSOR_CROSSHAIR);
			
			else if (slideRangeState & SPLIT_SIDE_H)
				Cursor_SetCursor(CURSOR_ARROW_H);
			
			else if (slideRangeState & SPLIT_SIDE_V)
				Cursor_SetCursor(CURSOR_ARROW_V);
			
			split->blockCursor = splitRangeState + slideRangeState;
		}
		
		if (nano->actionSplit == NULL && split->cursorInDisp && cursor->cursorAction) {
			if (cursor->clickAny.press)
				nano->actionSplit = split;
		}
		
		if (nano->actionSplit != NULL && nano->actionSplit == split)
			Split_UpdateActionSplit(nano);
	}
	
	if (split->state != 0)
		split->blockCursor = true;
	
	split->inputAccess = split->cursorInSplit && !nano->state.blockElemInput && !split->blockCursor;
	
	Element_SetContext(nano, split);
	Element_RowY(SPLIT_ELEM_X_PADDING * 2);
	Element_ShiftX(0);
	
	if (!split->isHeader) {
		split->id = split->taskList->cur;
		
		if (split->id != split->prevId)
			Split_InitOrSwap(nano, split);
		
		osLog("SplitUpdate( %d %s )", split->id, nano->taskTable[split->id]->taskName);
		osAssert(table[split->id]->update != NULL);
		table[split->id]->update(nano->passArg, split->instance, split);
		
		if (Element_Box(BOX_INDEX) != 0)
			errr("" PRNT_YELW "Element_Box Overflow" PRNT_RSET "( %d %s )", split->id, table[split->id]->taskName);
		
		for (int i = 0; i < 4; i++) {
			osAssert(split->edge[i] != NULL);
			split->edge[i]->killFlag = false;
		}
	} else if (split->headerFunc)
		split->headerFunc(nano->passArg, split->instance, split);
	osLog("OK");
}

static void Split_CullUpdate(NanoGrid* nano) {
	Split* split = nano->splitHead;
	
	for (; split != NULL; split = split->next) {
		if (!split->isHeader) {
			for (int i = 0; i < 4; i++) {
				osAssert(split->edge[i] != NULL);
				split->edge[i]->killFlag = false;
			}
		}
	}
}

static void Split_Update(NanoGrid* nano) {
	Split* split = nano->splitHead;
	Cursor* cursor = &nano->input->cursor;
	
	if (nano->actionSplit != NULL && cursor->cursorAction == false)
		Split_ClearActionSplit(nano);
	
	for (; split != NULL; split = split->next)
		Split_UpdateInstance(nano, split);
}

/* ───────────────────────────────────────────────────────────────────────── */

static void Split_DrawDebug(NanoGrid* nano) {
	SplitVtx* vtx = nano->vtxHead;
	Split* split = nano->splitHead;
	s32 num = 0;
	Vec2s* wdim = nano->wdim;
	
	glViewport(0, 0, wdim->x, wdim->y);
	nvgBeginFrame(nano->vg, wdim->x, wdim->y, gPixelRatio); {
		while (split) {
			nvgBeginPath(nano->vg);
			nvgLineCap(nano->vg, NVG_ROUND);
			nvgStrokeWidth(nano->vg, 1.0f);
			nvgMoveTo(
				nano->vg,
				split->vtx[0]->pos.x + 2,
				split->vtx[0]->pos.y - 2
			);
			nvgLineTo(
				nano->vg,
				split->vtx[1]->pos.x + 2,
				split->vtx[1]->pos.y + 2
			);
			nvgLineTo(
				nano->vg,
				split->vtx[2]->pos.x - 2,
				split->vtx[2]->pos.y + 2
			);
			nvgLineTo(
				nano->vg,
				split->vtx[3]->pos.x - 2,
				split->vtx[3]->pos.y - 2
			);
			nvgLineTo(
				nano->vg,
				split->vtx[0]->pos.x + 2,
				split->vtx[0]->pos.y - 2
			);
			nvgStrokeColor(nano->vg, nvgHSLA(0.111 * num, 1.0f, 0.6f, 255));
			nvgStroke(nano->vg);
			
			split = split->next;
			num++;
		}
		
		num = 0;
		
		while (vtx) {
			char buf[128];
			Vec2f pos = {
				vtx->pos.x + clamp(
					wdim->x * 0.5 - vtx->pos.x,
					-150.0f,
					150.0f
				) *
				0.1f,
				vtx->pos.y + clamp(
					wdim->y * 0.5 - vtx->pos.y,
					-150.0f,
					150.0f
				) *
				0.1f
			};
			
			sprintf(buf, "%d", num);
			nvgFontSize(nano->vg, SPLIT_TEXT);
			nvgFontFace(nano->vg, "default");
			nvgTextAlign(nano->vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_CENTER);
			nvgFillColor(nano->vg, Theme_GetColor(THEME_BASE, 255, 1.0f));
			nvgFontBlur(nano->vg, 1.5f);
			nvgText(nano->vg, pos.x, pos.y, buf, 0);
			nvgFontBlur(nano->vg, 0);
			nvgFillColor(nano->vg, Theme_GetColor(THEME_TEXT, 255, 1.0f));
			nvgText(nano->vg, pos.x, pos.y, buf, 0);
			
			vtx = vtx->next;
			num++;
		}
	} nvgEndFrame(nano->vg);
}

static void Split_Draw_KillArrow(Split* this, void* vg) {
	if (this->state & SPLIT_KILL_TARGET) {
		Vec2f arrow[] = {
			{ 0,    13    }, { 11,   1   }, { 5.6, 1 }, { 5.6, -10  },
			{ -5.6, -10   }, { -5.6, 1   }, { -10, 1 }, { 0,   13   }
		};
		f32 dir = 0;
		
		switch (this->state & SPLIT_KILL_TARGET) {
			case SPLIT_KILL_DIR_L:
				dir = -90;
				break;
			case SPLIT_KILL_DIR_T:
				dir = -180;
				break;
			case SPLIT_KILL_DIR_R:
				dir = 90;
				break;
			case SPLIT_KILL_DIR_B:
				dir = 0;
				break;
		}
		
		nvgBeginPath(vg);
		nvgRect(vg, -4, -4, this->dispRect.w + 8, this->dispRect.h + 8);
		Gfx_Shape(
			vg,
			Vec2f_New(this->rect.w * 0.5, this->rect.h * 0.5),
			10.0f,
			DegToBin(dir),
			arrow,
			ArrCount(arrow)
		);
		nvgPathWinding(vg, NVG_HOLE);
		
		nvgFillColor(vg, Theme_GetColor(THEME_SHADOW, 125, 1.0f));
		nvgFill(vg);
		
		this->state &= ~(SPLIT_KILL_TARGET);
	}
}

static void Split_DrawInstance(NanoGrid* nano, Split* split) {
	Element_SetContext(nano, split);
	
	osAssert(split != NULL);
	if (nano->taskTable && nano->numTaskTable > split->id && nano->taskTable[split->id])
		osLog("SplitDraw( %d %s )", split->id, nano->taskTable[split->id]->taskName);
	else
		osLog("SplitDraw( %d )", split->id);
	
	if (split->isHeader)
		goto draw_header;
	
	Split_UpdateRect(split);
	
	void* vg = nano->vg;
	Rect r = split->dispRect;
	u32 id = split->id;
	SplitTask** table = nano->taskTable;
	
	osLog("Draw Background");
	glViewportRect(UnfoldRect(split->rect));
	nvgBeginFrame(nano->vg, split->rect.w, split->rect.h, gPixelRatio); {
		r.x = r.y = 0;
		
		nvgBeginPath(vg);
		nvgRect(vg, 0, 0, split->dispRect.w, split->dispRect.h);
		
		if (split->bg.useCustomColor)
			nvgFillColor(vg, nvgRGBA(split->bg.color.r, split->bg.color.g, split->bg.color.b, 255));
		
		else if (split->bg.useCustomPaint)
			nvgFillPaint(vg, split->bg.paint);
		
		else
			nvgFillPaint(vg, nvgBoxGradient(vg, UnfoldRect(r), SPLIT_ROUND_R, 8.0f, Theme_GetColor(THEME_BASE, 255, 1.0f), Theme_GetColor(THEME_BASE, 255, 0.8f)));
		nvgFill(vg);
	} nvgEndFrame(nano->vg);
	
	osLog("Draw Split");
	nvgBeginFrame(nano->vg, split->rect.w, split->rect.h, gPixelRatio); {
		Math_SmoothStepToF(&split->scroll.voffset, split->scroll.offset, 0.25f, fabsf(split->scroll.offset - split->scroll.voffset) * 0.5f, 0.1f);
		if (table[id]->draw)
			table[id]->draw(nano->passArg, split->instance, split);
		Element_Draw(nano, split, false);
		osLog("Split Arrow");
		Split_Draw_KillArrow(split, nano->vg);
	} nvgEndFrame(nano->vg);
	
	osLog("Draw Overlays");
	glViewportRect(UnfoldRect(split->dispRect));
	nvgBeginFrame(nano->vg, split->dispRect.w, split->dispRect.h, gPixelRatio); {
		r = split->dispRect;
		r.x = SPLIT_SPLIT_W * 2;
		r.y = SPLIT_SPLIT_W * 2;
		r.w -= SPLIT_SPLIT_W * 4;
		r.h -= SPLIT_SPLIT_W * 4;
		
		if (!(split->edge[EDGE_L]->state & EDGE_STICK_L)) {
			r.x -= SPLIT_SPLIT_W;
			r.w += SPLIT_SPLIT_W;
		}
		if (!(split->edge[EDGE_R]->state & EDGE_STICK_R)) {
			r.w += SPLIT_SPLIT_W;
		}
		if (!(split->edge[EDGE_T]->state & EDGE_STICK_T)) {
			r.y -= SPLIT_SPLIT_W;
			r.h += SPLIT_SPLIT_W;
		}
		if (!(split->edge[EDGE_B]->state & EDGE_STICK_B)) {
			r.h += SPLIT_SPLIT_W;
		}
		
		nvgBeginPath(nano->vg);
		nvgRect(nano->vg,
			-SPLIT_TEXT_PADDING,
			-SPLIT_TEXT_PADDING,
			split->dispRect.w + SPLIT_TEXT_PADDING * 2,
			split->dispRect.h + SPLIT_TEXT_PADDING * 2);
		nvgRoundedRect(
			nano->vg,
			UnfoldRect(r),
			SPLIT_ROUND_R * 2
		);
		nvgPathWinding(nano->vg, NVG_HOLE);
		
		nvgFillColor(nano->vg, Theme_GetColor(THEME_SPLIT, 255, 1.00f));
		nvgFill(nano->vg);
		
		Rect sc = Rect_SubPos(split->headRect, split->dispRect);
		nvgScissor(vg, UnfoldRect(sc));
		nvgBeginPath(vg);
		nvgRoundedRect(
			nano->vg,
			UnfoldRect(r),
			SPLIT_ROUND_R
		);
		nvgFillColor(nano->vg, Theme_GetColor(THEME_BASE, 255, 1.25f));
		nvgFill(nano->vg);
		nvgResetScissor(vg);
	} nvgEndFrame(nano->vg);
	
	draw_header:
	osLog("Draw Header");
	glViewportRect(UnfoldRect(split->headRect));
	nvgBeginFrame(nano->vg, split->headRect.w, split->headRect.h, gPixelRatio); {
		Element_Draw(nano, split, true);
	} nvgEndFrame(nano->vg);
}

static void Split_Draw(NanoGrid* nano) {
	Split* split = nano->splitHead;
	
	for (; split != NULL; split = split->next)
		Split_DrawInstance(nano, split);
}

/* ───────────────────────────────────────────────────────────────────────── */

static void NanoGrid_RemoveDuplicates(NanoGrid* nano) {
	SplitVtx* vtx = nano->vtxHead;
	SplitEdge* edge = nano->edgeHead;
	
	while (vtx) {
		Vtx_RemoveDuplicates(nano, vtx);
		vtx = vtx->next;
	}
	
	while (edge) {
		Edge_RemoveDuplicates(nano, edge);
		edge = edge->next;
	}
}

static void NanoGrid_UpdateWorkRect(NanoGrid* nano) {
	Vec2s* wdim = nano->wdim;
	
	nano->workRect = (Rect) { 0, 0 + nano->bar[BAR_TOP].rect.h, wdim->x,
							  wdim->y - nano->bar[BAR_BOT].rect.h -
							  nano->bar[BAR_TOP].rect.h };
}

static void NanoGrid_SetTopBarHeight(NanoGrid* nano, s32 h) {
	nano->bar[BAR_TOP].rect.x = 0;
	nano->bar[BAR_TOP].rect.y = 0;
	nano->bar[BAR_TOP].rect.w = nano->wdim->x;
	nano->bar[BAR_TOP].rect.h = h;
	nano->bar[BAR_TOP].headRect = nano->bar[BAR_TOP].rect;
	nano->bar[BAR_TOP].dispRect = nano->bar[BAR_TOP].rect;
	NanoGrid_UpdateWorkRect(nano);
}

static void NanoGrid_SetBotBarHeight(NanoGrid* nano, s32 h) {
	nano->bar[BAR_BOT].rect.x = 0;
	nano->bar[BAR_BOT].rect.y = nano->wdim->y - h;
	nano->bar[BAR_BOT].rect.w = nano->wdim->x;
	nano->bar[BAR_BOT].rect.h = h;
	nano->bar[BAR_BOT].headRect = nano->bar[BAR_BOT].rect;
	nano->bar[BAR_BOT].dispRect = nano->bar[BAR_BOT].rect;
	NanoGrid_UpdateWorkRect(nano);
}

/* ───────────────────────────────────────────────────────────────────────── */

bool Split_CursorInRect(Split* split, Rect* rect) {
	return Rect_PointIntersect(rect, split->cursorPos.x, split->cursorPos.y);
}

bool Split_CursorInSplit(Split* split) {
	Rect r = split->rect;
	
	r.x = r.y = 0;
	
	return Rect_PointIntersect(&r, split->cursorPos.x, split->cursorPos.y);
}

Split* NanoGrid_AddSplit(NanoGrid* nano, Rectf32* rect, s32 id) {
	Split* split = Split_Alloc(nano, id);
	
	split->vtx[VTX_BOT_L] = NanoGrid_AddVtx(nano, rect->x, rect->y + rect->h);
	split->vtx[VTX_TOP_L] = NanoGrid_AddVtx(nano, rect->x, rect->y);
	split->vtx[VTX_TOP_R] = NanoGrid_AddVtx(nano, rect->x + rect->w, rect->y);
	split->vtx[VTX_BOT_R] = NanoGrid_AddVtx(nano, rect->x + rect->w, rect->y + rect->h);
	
	split->edge[EDGE_L] = NanoGrid_AddEdge(nano, split->vtx[VTX_BOT_L], split->vtx[VTX_TOP_L]);
	split->edge[EDGE_T] = NanoGrid_AddEdge(nano, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
	split->edge[EDGE_R] = NanoGrid_AddEdge(nano, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
	split->edge[EDGE_B] = NanoGrid_AddEdge(nano, split->vtx[VTX_BOT_R], split->vtx[VTX_BOT_L]);
	
	Node_Add(nano->splitHead, split);
	
	return split;
}

s32 Split_GetCursor(NanoGrid* nano, Split* split, s32 result) {
	if (
		nano->state.blockElemInput ||
		!split->cursorInSplit ||
		split->blockCursor ||
		split->elemBlockCursor
	)
		return 0;
	
	return result;
}

void NanoGrid_Debug(bool b) {
	sDebugMode = b;
}

void NanoGrid_TaskTable(NanoGrid* nano, SplitTask** taskTable, u32 num) {
	osAssert(num != 0);
	nano->taskTable = taskTable;
	nano->numTaskTable = num;
	
	for (int i = 0; i < num; i++)
		info("%d: %s", i, taskTable[i]->taskName);
}

extern void* ElementState_New(void);
extern void ElementState_Set(void* elemState);
extern void* ElementState_Get();
extern void Element_Flush(NanoGrid* nano);
extern void VectorGfx_InitCommon();
extern void osMsg_Draw(NanoGrid* nano);

void NanoGrid_Init(NanoGrid* this, struct Window* app, void* passArg) {
	this->vg = app->vg;
	this->passArg = passArg;
	
	osLog("Prepare Headers");
	for (var_t i = 0; i < 2; i++)
		this->bar[i].isHeader = true;
	
	osLog("Assign Info");
	this->wdim = &app->dim;
	this->input = app->input;
	this->vg = app->vg;
	
	osLog("Allocate ElemState");
	this->elemState = ElementState_New();
	
	NanoGrid_SetTopBarHeight(this, SPLIT_BAR_HEIGHT);
	NanoGrid_SetBotBarHeight(this, SPLIT_BAR_HEIGHT);
	VectorGfx_InitCommon();
	
	this->prevWorkRect = this->workRect;
}

void NanoGrid_Destroy(NanoGrid* this) {
	osLog("Destroy Splits");
	while (this->splitHead) {
		osLog("Split [%d]", this->splitHead->id);
		if (this->taskTable[this->splitHead->id]->destroy)
			this->taskTable[this->splitHead->id]->destroy(this->passArg, this->splitHead->instance, this->splitHead);
		Split_Free(this->splitHead);
		Node_Kill(this->splitHead, this->splitHead);
	}
	
	osLog("delete Vtx");
	while (this->vtxHead)
		Node_Kill(this->vtxHead, this->vtxHead);
	
	osLog("delete Edge");
	while (this->edgeHead)
		Node_Kill(this->edgeHead, this->edgeHead);
	
	osLog("delete ElemState");
	delete(this->elemState);
}

void NanoGrid_Update(NanoGrid* this) {
	NanoGrid_SetTopBarHeight(this, SPLIT_BAR_HEIGHT);
	NanoGrid_SetBotBarHeight(this, SPLIT_BAR_HEIGHT);
	
	Vtx_Update(this);
	Edge_Update(this);
	
	this->prevWorkRect = this->workRect;
	
	Cursor_SetCursor(CURSOR_DEFAULT);
	
	if (this->state.cullSplits) {
		this->state.cullSplits = 2;
		Split_CullUpdate(this);
		
	} else {
		
		ElementState_Set(this->elemState);
		Element_Flush(this);
		
		Element_UpdateTextbox(this);
		
		Split_Update(this);
	}
	
	Split_UpdateInstance(this, &this->bar[0]);
	Split_UpdateInstance(this, &this->bar[1]);
	
	if (this->state.splittedHold &&  Input_GetCursor(this->input, CLICK_L)->release) {
		this->state.splittedHold = false;
		this->state.blockElemInput--;
	}
}

void NanoGrid_Draw(NanoGrid* this) {
	
	// Draw Bars
	for (int i = 0; i < 2; i++) {
		glViewportRect(UnfoldRect(this->bar[i].rect));
		nvgBeginFrame(this->vg, this->bar[i].rect.w, this->bar[i].rect.h, gPixelRatio); {
			Rect r = this->bar[i].rect;
			
			r.x = r.y = 0;
			
			nvgBeginPath(this->vg);
			nvgRect(this->vg,
				UnfoldRect(r));
			nvgFillColor(this->vg, Theme_GetColor(THEME_BASE, 255, 0.5f));
			nvgFill(this->vg);
		} nvgEndFrame(this->vg);
	}
	
	if (this->state.cullSplits < 2) {
		Split_Draw(this);
		if (sDebugMode) Split_DrawDebug(this);
	}
	
	Split_DrawInstance(this, &this->bar[0]);
	Split_DrawInstance(this, &this->bar[1]);
	
	glViewport(0, 0, this->wdim->x, this->wdim->y);
	osMsg_Draw(this);
	ContextMenu_Draw(this, &this->contextMenu);
	DragItem_Draw(this);
}

////////////////////////////////////////////////////////////////////////////////

void DummyGrid_Push(NanoGrid* this) {
	this->swapState = ElementState_Get();
	ElementState_Set(this->elemState);
	Element_Flush(this);
	Element_UpdateTextbox(this);
}

void DummyGrid_Pop(NanoGrid* this) {
	DragItem_Draw(this);
	ElementState_Set(this->swapState);
}

void DummySplit_Push(NanoGrid* this, Split* split, Rect r) {
	split->dispRect = split->rect = r;
	split->cursorInSplit = true;
	split->cursorPos = this->input->cursor.pos;
	split->cursorPressPos = this->input->cursor.pressPos;
	split->dummy = true;
	
	Element_RowY(r.y + SPLIT_ELEM_X_PADDING * 2);
	Element_ShiftX(0);
	Element_SetContext(this, split);
}

void DummySplit_Pop(NanoGrid* this, Split* split) {
	Element_Draw(this, split, false);
}

////////////////////////////////////////////////////////////////////////////////

void NanoGrid_SaveLayout(NanoGrid* this, Toml* toml, const char* file) {
	Split* split = this->splitHead;
	
	Toml_RmArr(toml, "geo_grid.split");
	
	for (int i = 0; split; split = split->next, i++) {
		Toml_SetVar(toml, x_fmt("geo_grid.split[%d].index", i), "%d", split->id);
		
		Rectf32 r = {
			split->vtx[VTX_TOP_L]->pos.x,
			split->vtx[VTX_TOP_L]->pos.y,
			split->vtx[VTX_BOT_R]->pos.x - split->vtx[VTX_TOP_L]->pos.x,
			split->vtx[VTX_BOT_R]->pos.y - split->vtx[VTX_TOP_L]->pos.y,
		};
		
		for (int k = 0; k < 4; k++) {
			Toml_SetVar(toml,
				x_fmt("geo_grid.split[%d].vtx[%d]", i, k),
				"%g",
				((f32*)(&r.x))[k]
			);
		}
		
		if (this->taskTable[split->id]->saveConfig)
			this->taskTable[split->id]->saveConfig(split->instance, split, toml, x_fmt("geo_grid.split[%d]", i));
	}
	
	Toml_Save(toml, file);
}

int NanoGrid_LoadLayout(NanoGrid* this, Toml* toml) {
	int arrNum = Toml_ArrCount(toml, "geo_grid.split[]");
	s8* buffer = new(s8[this->wdim->y][this->wdim->x]);
	bool valid = true;
	
	for (int i = 0; i < arrNum; i++) {
		Rectf32 r;
		f32* f = (f32*)(&r);
		
		for (int k = 0; k < 4; k++)
			f[k] = Toml_GetFloat(toml, "geo_grid.split[%d].vtx[%d]", i, k);
		
		for (int y = r.y; y < r.y + r.h; y++) {
			memset(&buffer[this->wdim->x * y + (int)r.x], 0xFF, r.w);
		}
	}
	
	int ymin = SPLIT_BAR_HEIGHT;
	int ymax = this->wdim->y - SPLIT_BAR_HEIGHT;
	
	for (int y = ymin; y < ymax; y++) {
		for (int x = 0; x < this->wdim->x; x++) {
			if (!buffer[this->wdim->x * y + x]) {
				valid = false;
				info("" PRNT_REDD "INVALID!");
				
				break;
			}
		}
	}
	
	delete(buffer);
	
	if (arrNum && valid) {
		for (int i = 0; i < arrNum; i++) {
			Rectf32 r;
			f32* f = (f32*)(&r);
			int id = Toml_GetInt(toml, "geo_grid.split[%d].index", i);
			
			for (int k = 0; k < 4; k++)
				f[k] = Toml_GetFloat(toml, "geo_grid.split[%d].vtx[%d]", i, k);
			
			NanoGrid_AddSplit(this, &r, id);
		}
		
		int id = 0;
		for (Split* split = this->splitHead; split; split = split->next, id++) {
			Split_InitOrSwap(this, split);
			
			if (this->taskTable[split->id]->loadConfig)
				this->taskTable[split->id]->loadConfig(split->instance, split, toml, x_fmt("geo_grid.split[%d]", id));
		}
		
		return arrNum;
	}
	
	return 0;
}
