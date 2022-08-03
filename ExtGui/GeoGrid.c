#include "GeoGrid.h"
#include "Interface.h"

f32 gPixelRatio = 1.0f;

static void GeoGrid_RemoveDuplicates(GeoGrid* geo);
static void Split_UpdateRect(Split* split);
void Element_SetContext(GeoGrid* setGeo, Split* setSplit);

Vec2f gZeroVec2f;
Vec2s gZeroVec2s;
Rect gZeroRect;

static s32 sDebugMode;

static void ggDebug(const char* fmt, ...) {
	if (sDebugMode == false)
		return;
	
	va_list va;
	
	va_start(va, fmt);
	vprintf(fmt, va);
	va_end(va);
	printf("\n");
}

/* ───────────────────────────────────────────────────────────────────────── */

static SplitDir GeoGrid_GetDir_Opposite(SplitDir dir) {
	return WrapS(dir + 2, DIR_L, DIR_B + 1);
}

static SplitDir GeoGrid_GetDir_MouseToPressPos(Split* split) {
	Vec2s pos = Math_Vec2s_Sub(split->mousePos, split->mousePressPos);
	
	if (Abs(pos.x) > Abs(pos.y)) {
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

static SplitVtx* GeoGrid_AddVtx(GeoGrid* geo, f64 x, f64 y) {
	SplitVtx* head = geo->vtxHead;
	SplitVtx* vtx;
	
	while (head) {
		if (fabs(head->pos.x - x) < 0.25 && fabs(head->pos.y - y) < 0.25)
			return head;
		head = head->next;
	}
	
	Calloc(vtx, sizeof(SplitVtx));
	
	vtx->pos.x = x;
	vtx->pos.y = y;
	Node_Add(geo->vtxHead, vtx);
	
	return vtx;
}

static SplitEdge* GeoGrid_AddEdge(GeoGrid* geo, SplitVtx* v1, SplitVtx* v2) {
	SplitEdge* head = geo->edgeHead;
	SplitEdge* edge = NULL;
	
	Assert(v1 != NULL && v2 != NULL);
	
	if (v1->pos.y == v2->pos.y) {
		if (v1->pos.x > v2->pos.x) {
			Swap(v1, v2);
		}
	} else {
		if (v1->pos.y > v2->pos.y) {
			Swap(v1, v2);
		}
	}
	
	while (head) {
		if (head->vtx[0] == v1 && head->vtx[1] == v2) {
			edge = head;
			break;
		}
		head = head->next;
	}
	
	if (edge == NULL) {
		Calloc(edge, sizeof(SplitEdge));
		
		edge->vtx[0] = v1;
		edge->vtx[1] = v2;
		Node_Add(geo->edgeHead, edge);
	}
	
	if (edge->vtx[0]->pos.y == edge->vtx[1]->pos.y) {
		edge->state |= EDGE_HORIZONTAL;
		edge->pos = edge->vtx[0]->pos.y;
		if (edge->pos < geo->workRect.y + 1) {
			edge->state |= EDGE_STICK_T;
		}
		if (edge->pos > geo->workRect.y + geo->workRect.h - 1) {
			edge->state |= EDGE_STICK_B;
		}
	} else {
		edge->state |= EDGE_VERTICAL;
		edge->pos = edge->vtx[0]->pos.x;
		if (edge->pos < geo->workRect.x + 1) {
			edge->state |= EDGE_STICK_L;
		}
		if (edge->pos > geo->workRect.x + geo->workRect.w - 1) {
			edge->state |= EDGE_STICK_R;
		}
	}
	
	return edge;
}

static s32 Split_CursorDistToFlagPos(SplitState flag, Split* split) {
	/*
	 * Manipulate distance check based on the flag so that we get the
	 * distance always to the point or line we need. For points we
	 * utilize both mouse positions, for sides (edge) we utilize only
	 * one axis that makes sense for the direction of that side.
	 */
	Vec2s mouse[] = {
		/* SPLIT_POINT_BL */ { split->mousePos.x, split->mousePos.y },
		/* SPLIT_POINT_TL */ { split->mousePos.x, split->mousePos.y },
		/* SPLIT_POINT_TR */ { split->mousePos.x, split->mousePos.y },
		/* SPLIT_POINT_BR */ { split->mousePos.x, split->mousePos.y },
		/* SPLIT_SIDE_L   */ { split->mousePos.x, 0 },
		/* SPLIT_SIDE_T   */ { 0,                 split->mousePos.y },
		/* SPLIT_SIDE_R   */ { split->mousePos.x, 0 },
		/* SPLIT_SIDE_B   */ { 0,                 split->mousePos.y }
	};
	Vec2s pos[] = {
		/* SPLIT_POINT_BL */ { 0,                 split->dispRect.h, },
		/* SPLIT_POINT_TL */ { 0,                 0,                 },
		/* SPLIT_POINT_TR */ { split->dispRect.w, 0,                 },
		/* SPLIT_POINT_BR */ { split->dispRect.w, split->dispRect.h, },
		/* SPLIT_SIDE_L   */ { 0,                 0,                 },
		/* SPLIT_SIDE_T   */ { 0,                 0,                 },
		/* SPLIT_SIDE_R   */ { split->dispRect.w, 0,                 },
		/* SPLIT_SIDE_B   */ { 0,                 split->dispRect.h, },
	};
	s32 i;
	
	i = bitscan32(flag);
	
	return Math_Vec2s_DistXZ(mouse[i], pos[i]);
}

static SplitState Split_GetCursorPosState(Split* split, s32 range) {
	for (s32 i = 0; (1 << i) <= SPLIT_SIDE_B; i++) {
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

bool Split_CursorInRect(Split* split, Rect* rect) {
	return Rect_PointIntersect(rect, split->mousePos.x, split->mousePos.y);
}

bool Split_CursorInSplit(Split* split) {
	Rect r = split->rect;
	
	r.x = r.y = 0;
	
	return Rect_PointIntersect(&r, split->mousePos.x, split->mousePos.y);
}

static void Split_SetupTaskEnum(GeoGrid* geo, Split* this) {
	this->taskEnum = PropEnum_Init(this->id);
	CallocX(this->taskCombo);
	
	Assert(this->taskEnum != NULL);
	Assert(this->taskCombo != NULL);
	
	for (s32 i = 0; i < geo->numTaskTable; i++)
		PropEnum_Add(this->taskEnum, geo->taskTable[i]->taskName);
	Element_Combo_SetPropEnum(this->taskCombo, this->taskEnum);
}

static Split* Split_Alloc(GeoGrid* geo, const char* name, s32 id) {
	Split* split;
	
	Calloc(split, sizeof(Split));
	split->name = name;
	split->prevId = -1; // Forces init
	split->id = id;
	
	Split_SetupTaskEnum(geo, split);
	
	return split;
}

Split* GeoGrid_AddSplit(GeoGrid* geo, const char* name, Rectf32* rect, s32 id) {
	Split* split = Split_Alloc(geo, name, id);
	
	split->vtx[VTX_BOT_L] = GeoGrid_AddVtx(geo, rect->x, rect->y + rect->h);
	split->vtx[VTX_TOP_L] = GeoGrid_AddVtx(geo, rect->x, rect->y);
	split->vtx[VTX_TOP_R] = GeoGrid_AddVtx(geo, rect->x + rect->w, rect->y);
	split->vtx[VTX_BOT_R] = GeoGrid_AddVtx(geo, rect->x + rect->w, rect->y + rect->h);
	
	split->edge[EDGE_L] = GeoGrid_AddEdge(geo, split->vtx[VTX_BOT_L], split->vtx[VTX_TOP_L]);
	split->edge[EDGE_T] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
	split->edge[EDGE_R] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
	split->edge[EDGE_B] = GeoGrid_AddEdge(geo, split->vtx[VTX_BOT_R], split->vtx[VTX_BOT_L]);
	
	Node_Add(geo->splitHead, split);
	
	return split;
}

static void Edge_SetSlideClamp(GeoGrid* geo) {
	SplitEdge* tempEdge = geo->edgeHead;
	SplitEdge* setEdge = geo->actionEdge;
	SplitEdge* actionEdge = geo->actionEdge;
	u32 align = ((actionEdge->state & EDGE_VERTICAL) != 0);
	f64 posMin = actionEdge->vtx[0]->pos.axis[align];
	f64 posMax = actionEdge->vtx[1]->pos.axis[align];
	
	setEdge->state |= EDGE_EDIT;
	
	// Get edge with vtx closest to TOPLEFT
	while (tempEdge) {
		Assert(!(tempEdge->state & EDGE_VERTICAL && tempEdge->state & EDGE_HORIZONTAL));
		
		if ((tempEdge->state & EDGE_ALIGN) == (setEdge->state & EDGE_ALIGN)) {
			if (tempEdge->vtx[1] == setEdge->vtx[0]) {
				setEdge = tempEdge;
				tempEdge->state |= EDGE_EDIT;
				tempEdge = geo->edgeHead;
				posMin = setEdge->vtx[0]->pos.axis[align];
				continue;
			}
		}
		
		tempEdge = tempEdge->next;
	}
	
	tempEdge = geo->edgeHead;
	
	// Set all below setEdgeA
	while (tempEdge) {
		if ((tempEdge->state & EDGE_ALIGN) == (setEdge->state & EDGE_ALIGN)) {
			if (tempEdge->vtx[0] == setEdge->vtx[1]) {
				tempEdge->state |= EDGE_EDIT;
				setEdge = tempEdge;
				tempEdge = geo->edgeHead;
				posMax = setEdge->vtx[1]->pos.axis[align];
				continue;
			}
		}
		
		tempEdge = tempEdge->next;
	}
	
	tempEdge = geo->edgeHead;
	
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
	
	if (geo->actionEdge->state & EDGE_VERTICAL) {
		geo->slide.clampMin = geo->workRect.x;
		geo->slide.clampMax = geo->workRect.x + geo->workRect.w;
	} else {
		geo->slide.clampMin = geo->workRect.y;
		geo->slide.clampMax = geo->workRect.y + geo->workRect.h;
	}
}

static void Split_ClearActionSplit(GeoGrid* geo) {
	Assert(geo->actionSplit);
	geo->actionSplit->stateFlag &= ~(SPLIT_POINTS | SPLIT_SIDES);
	
	geo->actionSplit = NULL;
}

static void Split_Split(GeoGrid* geo, Split* split, SplitDir dir) {
	Split* newSplit;
	f64 splitPos = (dir == DIR_L || dir == DIR_R) ? geo->input->mouse.pos.x : geo->input->mouse.pos.y;
	
	newSplit = Split_Alloc(geo, split->name, split->id);
	Node_Add(geo->splitHead, newSplit);
	
	if (dir == DIR_L) {
		ggDebug("Split DIR_L");
		newSplit->vtx[0] = GeoGrid_AddVtx(geo, splitPos, split->vtx[0]->pos.y);
		newSplit->vtx[1] = GeoGrid_AddVtx(geo, splitPos, split->vtx[1]->pos.y);
		newSplit->vtx[2] =  GeoGrid_AddVtx(geo, split->vtx[2]->pos.x, split->vtx[2]->pos.y);
		newSplit->vtx[3] = GeoGrid_AddVtx(geo, split->vtx[3]->pos.x, split->vtx[3]->pos.y);
		split->vtx[2] = GeoGrid_AddVtx(geo, splitPos, split->vtx[2]->pos.y);
		split->vtx[3] = GeoGrid_AddVtx(geo, splitPos, split->vtx[3]->pos.y);
	}
	
	if (dir == DIR_R) {
		ggDebug("Split DIR_R");
		newSplit->vtx[0] = GeoGrid_AddVtx(geo, split->vtx[0]->pos.x, split->vtx[0]->pos.y);
		newSplit->vtx[1] = GeoGrid_AddVtx(geo, split->vtx[1]->pos.x, split->vtx[1]->pos.y);
		newSplit->vtx[2] = GeoGrid_AddVtx(geo, splitPos, split->vtx[2]->pos.y);
		newSplit->vtx[3] = GeoGrid_AddVtx(geo, splitPos, split->vtx[3]->pos.y);
		split->vtx[0] = GeoGrid_AddVtx(geo, splitPos, split->vtx[0]->pos.y);
		split->vtx[1] = GeoGrid_AddVtx(geo, splitPos, split->vtx[1]->pos.y);
	}
	
	if (dir == DIR_T) {
		ggDebug("Split DIR_T");
		newSplit->vtx[0] = GeoGrid_AddVtx(geo, split->vtx[0]->pos.x, split->vtx[0]->pos.y);
		newSplit->vtx[1] = GeoGrid_AddVtx(geo, split->vtx[1]->pos.x, splitPos);
		newSplit->vtx[2] = GeoGrid_AddVtx(geo, split->vtx[2]->pos.x, splitPos);
		newSplit->vtx[3] = GeoGrid_AddVtx(geo, split->vtx[3]->pos.x, split->vtx[3]->pos.y);
		split->vtx[3] = GeoGrid_AddVtx(geo, split->vtx[3]->pos.x, splitPos);
		split->vtx[0] = GeoGrid_AddVtx(geo, split->vtx[0]->pos.x, splitPos);
	}
	
	if (dir == DIR_B) {
		ggDebug("Split DIR_B");
		newSplit->vtx[0] = GeoGrid_AddVtx(geo, split->vtx[0]->pos.x, splitPos);
		newSplit->vtx[1] = GeoGrid_AddVtx(geo, split->vtx[1]->pos.x, split->vtx[1]->pos.y);
		newSplit->vtx[2] = GeoGrid_AddVtx(geo, split->vtx[2]->pos.x, split->vtx[2]->pos.y);
		newSplit->vtx[3] = GeoGrid_AddVtx(geo, split->vtx[3]->pos.x, splitPos);
		split->vtx[1] = GeoGrid_AddVtx(geo, split->vtx[1]->pos.x, splitPos);
		split->vtx[2] = GeoGrid_AddVtx(geo, split->vtx[2]->pos.x, splitPos);
	}
	
	split->edge[EDGE_L] = GeoGrid_AddEdge(geo, split->vtx[VTX_BOT_L], split->vtx[VTX_TOP_L]);
	split->edge[EDGE_T] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
	split->edge[EDGE_R] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
	split->edge[EDGE_B] = GeoGrid_AddEdge(geo, split->vtx[VTX_BOT_R], split->vtx[VTX_BOT_L]);
	
	newSplit->edge[EDGE_L] = GeoGrid_AddEdge(geo, newSplit->vtx[VTX_BOT_L], newSplit->vtx[VTX_TOP_L]);
	newSplit->edge[EDGE_T] = GeoGrid_AddEdge(geo, newSplit->vtx[VTX_TOP_L], newSplit->vtx[VTX_TOP_R]);
	newSplit->edge[EDGE_R] = GeoGrid_AddEdge(geo, newSplit->vtx[VTX_TOP_R], newSplit->vtx[VTX_BOT_R]);
	newSplit->edge[EDGE_B] = GeoGrid_AddEdge(geo, newSplit->vtx[VTX_BOT_R], newSplit->vtx[VTX_BOT_L]);
	
	geo->actionEdge = newSplit->edge[dir];
	GeoGrid_RemoveDuplicates(geo);
	Edge_SetSlideClamp(geo);
	Split_UpdateRect(split);
	Split_UpdateRect(newSplit);
}

static void Split_Kill(GeoGrid* geo, Split* split, SplitDir dir) {
	SplitEdge* sharedEdge = split->edge[dir];
	Split* killSplit = geo->splitHead;
	SplitDir oppositeDir = GeoGrid_GetDir_Opposite(dir);
	
	while (killSplit) {
		if (killSplit->edge[oppositeDir] == sharedEdge) {
			break;
		}
		
		killSplit = killSplit->next;
	}
	
	if (killSplit == NULL) {
		return;
	}
	
	split->edge[dir]->vtx[0]->killFlag = true;
	split->edge[dir]->vtx[1]->killFlag = true;
	
	if (dir == DIR_L) {
		ggDebug("Kill DIR_L");
		split->vtx[VTX_BOT_L] = killSplit->vtx[VTX_BOT_L];
		split->vtx[VTX_TOP_L] = killSplit->vtx[VTX_TOP_L];
		split->edge[EDGE_T] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
		split->edge[EDGE_B] = GeoGrid_AddEdge(geo, split->vtx[VTX_BOT_L], split->vtx[VTX_BOT_R]);
	}
	
	if (dir == DIR_T) {
		ggDebug("Kill DIR_T");
		split->vtx[VTX_TOP_L] = killSplit->vtx[VTX_TOP_L];
		split->vtx[VTX_TOP_R] = killSplit->vtx[VTX_TOP_R];
		split->edge[EDGE_L] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_L], split->vtx[VTX_BOT_L]);
		split->edge[EDGE_R] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
	}
	
	if (dir == DIR_R) {
		ggDebug("Kill DIR_R");
		split->vtx[VTX_BOT_R] = killSplit->vtx[VTX_BOT_R];
		split->vtx[VTX_TOP_R] = killSplit->vtx[VTX_TOP_R];
		split->edge[EDGE_T] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
		split->edge[EDGE_B] = GeoGrid_AddEdge(geo, split->vtx[VTX_BOT_L], split->vtx[VTX_BOT_R]);
	}
	
	if (dir == DIR_B) {
		ggDebug("Kill DIR_B");
		split->vtx[VTX_BOT_L] = killSplit->vtx[VTX_BOT_L];
		split->vtx[VTX_BOT_R] = killSplit->vtx[VTX_BOT_R];
		split->edge[EDGE_L] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_L], split->vtx[VTX_BOT_L]);
		split->edge[EDGE_R] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
	}
	
	split->edge[dir] = killSplit->edge[dir];
	geo->taskTable[killSplit->id]->destroy(geo->passArg, killSplit->instance, killSplit);
	
	PropEnum_Free(killSplit->taskEnum);
	Free(killSplit->taskCombo);
	Node_Kill(geo->splitHead, killSplit);
	GeoGrid_RemoveDuplicates(geo);
	Split_UpdateRect(split);
}

s32 Split_GetCursor(GeoGrid* geo, Split* split, s32 result) {
	if (geo->state.noClickInput ||
		!split->mouseInSplit ||
		split->blockMouse ||
		split->elemBlockMouse)
		return 0;
	
	return result;
}

/* ───────────────────────────────────────────────────────────────────────── */

static void Vtx_RemoveDuplicates(GeoGrid* geo, SplitVtx* vtx) {
	SplitVtx* vtx2 = geo->vtxHead;
	
	while (vtx2) {
		if (vtx2 == vtx) {
			vtx2 = vtx2->next;
			continue;
		}
		
		if (!veccmp(&vtx->pos, &vtx2->pos)) {
			SplitVtx* kill = vtx2;
			Split* s = geo->splitHead;
			SplitEdge* e = geo->edgeHead;
			
			while (s) {
				for (s32 i = 0; i < 4; i++) {
					if (s->vtx[i] == vtx2) {
						s->vtx[i] = vtx;
					}
				}
				s = s->next;
			}
			
			while (e) {
				for (s32 i = 0; i < 2; i++) {
					if (e->vtx[i] == vtx2) {
						e->vtx[i] = vtx;
					}
				}
				e = e->next;
			}
			
			vtx2 = vtx2->next;
			Node_Kill(geo->vtxHead, kill);
			continue;
		}
		
		vtx2 = vtx2->next;
	}
}

static void Vtx_Update(GeoGrid* geo) {
	SplitVtx* vtx = geo->vtxHead;
	SplitVtx* prv;
	
	if (geo->actionEdge != NULL)
		geo->state.cleanVtx = true;
	
	while (vtx) {
		if (geo->state.cleanVtx == true && geo->actionEdge == NULL)
			Vtx_RemoveDuplicates(geo, vtx);
		
		if (vtx->killFlag == true) {
			Split* s = geo->splitHead;
			
			while (s) {
				for (s32 i = 0; i < 4; i++)
					if (s->vtx[i] == vtx)
						vtx->killFlag = false;
				
				s = s->next;
			}
			
			if (vtx->killFlag == true) {
				SplitVtx* killVtx = vtx;
				vtx = prv;
				Node_Kill(geo->vtxHead, killVtx);
				continue;
			}
		}
		
		prv = vtx;
		vtx = vtx->next;
		if (vtx == NULL && geo->actionEdge == NULL)
			geo->state.cleanVtx = false;
	}
}

/* ───────────────────────────────────────────────────────────────────────── */

static void Edge_RemoveDuplicates(GeoGrid* geo, SplitEdge* edge) {
	SplitEdge* cur = geo->edgeHead;
	
	while (cur) {
		if (cur == edge) {
			cur = cur->next;
			continue;
		}
		
		if (cur->vtx[0] == edge->vtx[0] && cur->vtx[1] == edge->vtx[1]) {
			SplitEdge* kill = cur;
			Split* s = geo->splitHead;
			
			if (geo->actionEdge == cur)
				geo->actionEdge = edge;
			
			while (s) {
				for (s32 i = 0; i < 4; i++)
					if (s->edge[i] == cur)
						s->edge[i] = edge;
				s = s->next;
			}
			
			cur = cur->next;
			Node_Kill(geo->edgeHead, kill);
			continue;
		}
		
		cur = cur->next;
	}
}

static void Edge_SetSlide(GeoGrid* geo) {
	SplitEdge* edge = geo->edgeHead;
	f64 diffCentX = (f64)geo->workRect.w / geo->prevWorkRect.w;
	f64 diffCentY = (f64)geo->workRect.h / geo->prevWorkRect.h;
	
	while (edge) {
		bool clampFail = false;
		bool isEditEdge = (edge == geo->actionEdge || edge->state & EDGE_EDIT);
		bool isCornerEdge = ((edge->state & EDGE_STICK) != 0);
		bool isHor = ((edge->state & EDGE_VERTICAL) == 0);
		
		Assert(!(edge->state & EDGE_HORIZONTAL && edge->state & EDGE_VERTICAL));
		
		if (isCornerEdge) {
			if (edge->state & EDGE_STICK_L)
				edge->pos = geo->workRect.x;
			
			if (edge->state & EDGE_STICK_T)
				edge->pos = geo->workRect.y;
			
			if (edge->state & EDGE_STICK_R)
				edge->pos = geo->workRect.x + geo->workRect.w;
			
			if (edge->state & EDGE_STICK_B)
				edge->pos = geo->workRect.y + geo->workRect.h;
		} else {
			if (edge->state & EDGE_HORIZONTAL) {
				edge->pos -= geo->workRect.y;
				edge->pos *= diffCentY;
				edge->pos += geo->workRect.y;
			}
			
			if (edge->state & EDGE_VERTICAL)
				edge->pos *= diffCentX;
		}
		
		if (isEditEdge && isCornerEdge == false) {
			SplitEdge* temp = geo->edgeHead;
			s32 align = WrapS(edge->state & EDGE_ALIGN, 0, 2);
			
			while (temp) {
				for (s32 i = 0; i < 2; i++) {
					if (((temp->state & EDGE_ALIGN) != (edge->state & EDGE_ALIGN)) && temp->vtx[1] == edge->vtx[i]) {
						if (temp->vtx[0]->pos.axis[align] > geo->slide.clampMin) {
							geo->slide.clampMin = temp->vtx[0]->pos.axis[align];
						}
					}
				}
				
				for (s32 i = 0; i < 2; i++) {
					if (((temp->state & EDGE_ALIGN) != (edge->state & EDGE_ALIGN)) && temp->vtx[0] == edge->vtx[i]) {
						if (temp->vtx[1]->pos.axis[align] < geo->slide.clampMax) {
							geo->slide.clampMax = temp->vtx[1]->pos.axis[align];
						}
					}
				}
				
				temp = temp->next;
			}
			
			if (geo->slide.clampMax - SPLIT_CLAMP > geo->slide.clampMin + SPLIT_CLAMP) {
				if (edge->state & EDGE_HORIZONTAL) {
					edge->pos = geo->input->mouse.pos.y;
				}
				if (edge->state & EDGE_VERTICAL) {
					edge->pos = geo->input->mouse.pos.x;
				}
				edge->pos = ClampMin(edge->pos, geo->slide.clampMin + SPLIT_CLAMP);
				edge->pos = ClampMax(edge->pos, geo->slide.clampMax - SPLIT_CLAMP);
			} else {
				clampFail = true;
			}
		}
		
		if (diffCentX == 1.0f && diffCentY == 1.0f && !isCornerEdge) {
			edge->pos = AccuracyF(edge->pos, 0.25);
			edge->pos = AccuracyF(edge->pos, 0.25);
		}
		
		if (isCornerEdge) {
			edge->vtx[0]->pos.axis[isHor] = edge->pos;
			edge->vtx[1]->pos.axis[isHor] = edge->pos;
		} else {
			if (isEditEdge && isCornerEdge == false && clampFail == false) {
				edge->vtx[0]->pos.axis[isHor] = edge->pos;
				edge->vtx[1]->pos.axis[isHor] = edge->pos;
			} else {
				edge->vtx[0]->pos.axis[isHor] = edge->pos;
				edge->vtx[1]->pos.axis[isHor] = edge->pos;
			}
		}
		
		edge = edge->next;
	}
}

static void Edge_Update(GeoGrid* geo) {
	SplitEdge* edge = geo->edgeHead;
	
	if (!Input_GetMouse(geo->input, MOUSE_ANY)->hold)
		geo->actionEdge = NULL;
	
	while (edge) {
		if (edge->killFlag == true) {
			SplitEdge* temp = edge->next;
			
			Node_Kill(geo->edgeHead, edge);
			edge = temp;
			
			continue;
		}
		edge->killFlag = true;
		
		if (geo->actionEdge == NULL) {
			edge->state &= ~EDGE_EDIT;
		}
		
		Edge_RemoveDuplicates(geo, edge);
		edge = edge->next;
	}
	
	Edge_SetSlide(geo);
}

/* ───────────────────────────────────────────────────────────────────────── */

static void Split_UpdateActionSplit(GeoGrid* geo) {
	Split* split = geo->actionSplit;
	
	if (Input_GetMouse(geo->input, MOUSE_ANY)->press) {
		SplitState tempStateA = Split_GetCursorPosState(split, SPLIT_GRAB_DIST);
		SplitState tempStateB = Split_GetCursorPosState(split, SPLIT_GRAB_DIST * 3);
		
		if (tempStateB & SPLIT_POINTS)
			split->stateFlag |= tempStateB;
		
		else if (tempStateA & SPLIT_SIDES)
			split->stateFlag |= tempStateA;
		
		if (split->stateFlag & SPLIT_SIDES) {
			s32 i;
			
			for (i = 0; i < 4; i++)
				if (split->stateFlag & (1 << (4 + i)))
					break;
			
			Assert(split->edge[i] != NULL);
			
			geo->actionEdge = split->edge[i];
			Edge_SetSlideClamp(geo);
			Split_ClearActionSplit(geo);
		}
	}
	
	if (Input_GetMouse(geo->input, MOUSE_ANY)->hold) {
		if (split->stateFlag & SPLIT_POINTS) {
			s32 dist = Math_Vec2s_DistXZ(split->mousePos, split->mousePressPos);
			SplitDir dir = GeoGrid_GetDir_MouseToPressPos(split);
			
			if (dist > 1) {
				CursorIndex cid = dir + 1;
				Cursor_SetCursor(cid);
			}
			
			if (dist > SPLIT_CLAMP * 1.05) {
				Split_ClearActionSplit(geo);
				
				if (split->mouseInDispRect)
					Split_Split(geo, split, dir);
				
				else
					Split_Kill(geo, split, dir);
			} else {
				SplitEdge* sharedEdge = split->edge[dir];
				Split* killSplit = geo->splitHead;
				SplitDir oppositeDir = GeoGrid_GetDir_Opposite(dir);
				
				while (killSplit) {
					if (killSplit->edge[oppositeDir] == sharedEdge) {
						break;
					}
					
					killSplit = killSplit->next;
				}
				
				if (killSplit)
					killSplit->stateFlag |= SPLIT_KILL_DIR_L << dir;
			}
		}
		
		if (split->stateFlag & SPLIT_SIDE_H)
			Cursor_SetCursor(CURSOR_ARROW_H);
		
		if (split->stateFlag & SPLIT_SIDE_V)
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
	
	split->rect = split->dispRect;
	split->rect.h -= SPLIT_ELEM_Y_PADDING + 4;
	
	split->headRect.x = split->rect.x;
	split->headRect.y = split->rect.y + split->rect.h;
	split->headRect.h = SPLIT_ELEM_Y_PADDING + 4;
	split->headRect.w = split->rect.w;
}

#define Split_FreeInstance(id) do { \
		table[id]->destroy(geo->passArg, split->instance, split); \
} while (0)

static void Split_Update(GeoGrid* geo) {
	Split* split = geo->splitHead;
	MouseInput* mouse = &geo->input->mouse;
	
	Cursor_SetCursor(CURSOR_DEFAULT);
	
	if (geo->actionSplit != NULL && mouse->cursorAction == false)
		Split_ClearActionSplit(geo);
	
	while (split) {
		Split_UpdateRect(split);
		Vec2s rectPos = { split->rect.x, split->rect.y };
		
		split->mousePos = Math_Vec2s_Sub(mouse->pos, rectPos);
		split->mouseInSplit = Rect_PointIntersect(&split->rect, mouse->pos.x, mouse->pos.y);
		split->mouseInDispRect = Rect_PointIntersect(&split->dispRect, mouse->pos.x, mouse->pos.y);
		split->mouseInHeader = Rect_PointIntersect(&split->headRect, mouse->pos.x, mouse->pos.y);
		split->blockMouse = false;
		
		if (mouse->click.press)
			split->mousePressPos = split->mousePos;
		
		if (geo->state.noSplit == false) {
			if (Split_GetCursorPosState(split, SPLIT_GRAB_DIST * 3) & SPLIT_POINTS && split->mouseInDispRect) {
				Cursor_SetCursor(CURSOR_CROSSHAIR);
				split->blockMouse = true;
			} else if (Split_GetCursorPosState(split, SPLIT_GRAB_DIST) & SPLIT_SIDE_H && split->mouseInDispRect) {
				Cursor_SetCursor(CURSOR_ARROW_H);
				split->blockMouse = true;
			} else if (Split_GetCursorPosState(split, SPLIT_GRAB_DIST) & SPLIT_SIDE_V && split->mouseInDispRect) {
				Cursor_SetCursor(CURSOR_ARROW_V);
				split->blockMouse = true;
			}
			
			if (geo->actionSplit == NULL && split->mouseInDispRect && mouse->cursorAction) {
				if (mouse->click.press)
					geo->actionSplit = split;
			}
			
			if (geo->actionSplit != NULL && geo->actionSplit == split)
				Split_UpdateActionSplit(geo);
		}
		
		if (split->stateFlag != 0)
			split->blockMouse = true;
		
		u32 id = split->id;
		SplitTask** table = geo->taskTable;
		
		split->id = split->taskEnum->key;
		
		if (split->id != split->prevId) {
			if (split->instance) {
				Log("Swap ID");
				Split_FreeInstance(split->prevId);
				Free(split->instance);
			}
			
			id = split->prevId = split->id;
			Calloc(split->instance, table[id]->size);
			table[id]->init(geo->passArg, split->instance, split);
		}
		
		Log("Update Split [%s]", split->name);
		Element_SetContext(geo, split);
		table[id]->update(geo->passArg, split->instance, split);
		Log("OK");
		
		for (s32 i = 0; i < 4; i++) {
			Assert(split->edge[i] != NULL);
			split->edge[i]->killFlag = false;
		}
		split = split->next;
	}
}

/* ───────────────────────────────────────────────────────────────────────── */

static void Split_DrawDebug(GeoGrid* geo) {
	SplitVtx* vtx = geo->vtxHead;
	Split* split = geo->splitHead;
	s32 num = 0;
	Vec2s* wdim = geo->wdim;
	
	glViewport(0, 0, wdim->x, wdim->y);
	nvgBeginFrame(geo->vg, wdim->x, wdim->y, gPixelRatio); {
		while (split) {
			nvgBeginPath(geo->vg);
			nvgLineCap(geo->vg, NVG_ROUND);
			nvgStrokeWidth(geo->vg, 1.0f);
			nvgMoveTo(
				geo->vg,
				split->vtx[0]->pos.x + 2,
				split->vtx[0]->pos.y - 2
			);
			nvgLineTo(
				geo->vg,
				split->vtx[1]->pos.x + 2,
				split->vtx[1]->pos.y + 2
			);
			nvgLineTo(
				geo->vg,
				split->vtx[2]->pos.x - 2,
				split->vtx[2]->pos.y + 2
			);
			nvgLineTo(
				geo->vg,
				split->vtx[3]->pos.x - 2,
				split->vtx[3]->pos.y - 2
			);
			nvgLineTo(
				geo->vg,
				split->vtx[0]->pos.x + 2,
				split->vtx[0]->pos.y - 2
			);
			nvgStrokeColor(geo->vg, nvgHSLA(0.111 * num, 1.0f, 0.6f, 255));
			nvgStroke(geo->vg);
			
			split = split->next;
			num++;
		}
		
		num = 0;
		
		while (vtx) {
			char buf[128];
			Vec2f pos = {
				vtx->pos.x + Clamp(
					wdim->x * 0.5 - vtx->pos.x,
					-150.0f,
					150.0f
				) *
				0.1f,
				vtx->pos.y + Clamp(
					wdim->y * 0.5 - vtx->pos.y,
					-150.0f,
					150.0f
				) *
				0.1f
			};
			
			sprintf(buf, "%d", num);
			nvgFontSize(geo->vg, SPLIT_TEXT);
			nvgFontFace(geo->vg, "default");
			nvgTextAlign(geo->vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_CENTER);
			nvgFillColor(geo->vg, Theme_GetColor(THEME_BASE, 255, 1.0f));
			nvgFontBlur(geo->vg, 1.5f);
			nvgText(geo->vg, pos.x, pos.y, buf, 0);
			nvgFontBlur(geo->vg, 0);
			nvgFillColor(geo->vg, Theme_GetColor(THEME_TEXT, 255, 1.0f));
			nvgText(geo->vg, pos.x, pos.y, buf, 0);
			
			vtx = vtx->next;
			num++;
		}
	} nvgEndFrame(geo->vg);
}

static void Split_Draw(GeoGrid* geo) {
	Split* split = geo->splitHead;
	Vec2s* wdim = geo->wdim;
	
	for (; split != NULL; split = split->next) {
		Split_UpdateRect(split);
		
		glViewport(
			split->dispRect.x,
			wdim->y - split->dispRect.y - split->dispRect.h,
			split->dispRect.w,
			split->dispRect.h
		);
		nvgBeginFrame(geo->vg, split->dispRect.w, split->dispRect.h, gPixelRatio); {
			void* vg = geo->vg;
			Rect r = split->dispRect;
			
			r.x = r.y = 0;
			
			u32 id = split->id;
			SplitTask** table = geo->taskTable;
			
			nvgBeginPath(vg);
			nvgRect(vg, 0, 0, split->dispRect.w, split->dispRect.h);
			if (split->bg.useCustomBG == true) {
				nvgFillColor(vg, nvgRGBA(split->bg.color.r, split->bg.color.g, split->bg.color.b, 255));
			} else {
				// nvgFillColor(vg, Theme_GetColor(THEME_BASE, 255, 1.0f));
				nvgFillPaint(vg, nvgBoxGradient(vg, UnfoldRect(r), SPLIT_ROUND_R, 8.0f, Theme_GetColor(THEME_BASE, 255, 1.0f), Theme_GetColor(THEME_BASE, 255, 0.8f)));
			}
			nvgFill(vg);
			
			/*
			 * New frame to make it possible to have
			 * something drawn from other sources than nvg
			 */
			nvgEndFrame(geo->vg);
			nvgBeginFrame(geo->vg, split->dispRect.w, split->dispRect.h, gPixelRatio);
			
			Element_SetContext(geo, split);
			table[id]->draw(geo->passArg, split->instance, split);
			Element_Draw(geo, split, false);
			
			if (split->stateFlag & SPLIT_KILL_TARGET) {
				Vec2f arrow[] = {
					{ 0, 13 }, { 11, 1 }, { 5.6, 1 }, { 5.6, -10 },
					{ -5.6, -10 }, { -5.6, 1 }, { -10, 1 }, { 0, 13 }
				};
				f32 dir = 0;
				
				switch (split->stateFlag & SPLIT_KILL_TARGET) {
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
				
				nvgBeginPath(geo->vg);
				nvgRect(geo->vg, -4, -4, split->dispRect.w + 8, split->dispRect.h + 8);
				Gfx_Shape(
					geo->vg,
					Math_Vec2f_New(split->rect.w * 0.5, split->rect.h * 0.5),
					10.0f,
					DegToBin(dir),
					arrow,
					ArrayCount(arrow)
				);
				nvgPathWinding(geo->vg, NVG_HOLE);
				
				nvgFillColor(geo->vg, Theme_GetColor(THEME_SHADOW, 125, 1.0f));
				nvgFill(geo->vg);
				
				split->stateFlag &= ~(SPLIT_KILL_TARGET);
			}
			
			r = split->dispRect;
			r.x = 1;
			r.y = 1;
			r.w -= 3;
			r.h -= 3;
			
			nvgBeginPath(geo->vg);
			nvgRect(geo->vg, -4, -4, split->dispRect.w + 8, split->dispRect.h + 8);
			nvgRoundedRect(
				geo->vg,
				UnfoldRect(r),
				SPLIT_ROUND_R * 2
			);
			nvgPathWinding(geo->vg, NVG_HOLE);
			
			nvgFillColor(geo->vg, Theme_GetColor(THEME_SHADOW, 255, 1.5f));
			nvgFill(geo->vg);
			
			r = split->dispRect;
			r.x = 1;
			r.y = 1;
			r.w -= 3;
			r.h -= 3;
			
			Rect sc = Rect_SubPos(split->headRect, split->dispRect);
			nvgScissor(vg, UnfoldRect(sc));
			nvgBeginPath(vg);
			nvgRoundedRect(
				geo->vg,
				UnfoldRect(r),
				SPLIT_ROUND_R
			);
			nvgFillColor(geo->vg, Theme_GetColor(THEME_BASE, 255, 1.25f));
			nvgFill(geo->vg);
			nvgResetScissor(vg);
		} nvgEndFrame(geo->vg);
		
		glViewport(
			split->headRect.x,
			wdim->y - split->headRect.y - split->headRect.h,
			split->headRect.w,
			split->headRect.h
		);
		nvgBeginFrame(geo->vg, split->headRect.w, split->headRect.h, gPixelRatio); {
			Element_Draw(geo, split, true);
		} nvgEndFrame(geo->vg);
	}
}

/* ───────────────────────────────────────────────────────────────────────── */

static void GeoGrid_RemoveDuplicates(GeoGrid* geo) {
	SplitVtx* vtx = geo->vtxHead;
	SplitEdge* edge = geo->edgeHead;
	
	while (vtx) {
		Vtx_RemoveDuplicates(geo, vtx);
		vtx = vtx->next;
	}
	
	while (edge) {
		Edge_RemoveDuplicates(geo, edge);
		edge = edge->next;
	}
}

static void GeoGrid_UpdateWorkRect(GeoGrid* geo) {
	Vec2s* wdim = geo->wdim;
	
	geo->workRect = (Rect) { 0, 0 + geo->bar[BAR_TOP].rect.h, wdim->x,
				 wdim->y - geo->bar[BAR_BOT].rect.h -
				 geo->bar[BAR_TOP].rect.h };
}

static void GeoGrid_SetTopBarHeight(GeoGrid* geo, s32 h) {
	geo->bar[BAR_TOP].rect.x = 0;
	geo->bar[BAR_TOP].rect.y = 0;
	geo->bar[BAR_TOP].rect.w = geo->wdim->x;
	geo->bar[BAR_TOP].rect.h = h;
	GeoGrid_UpdateWorkRect(geo);
}

static void GeoGrid_SetBotBarHeight(GeoGrid* geo, s32 h) {
	geo->bar[BAR_BOT].rect.x = 0;
	geo->bar[BAR_BOT].rect.y = geo->wdim->y - h;
	geo->bar[BAR_BOT].rect.w = geo->wdim->x;
	geo->bar[BAR_BOT].rect.h = h;
	GeoGrid_UpdateWorkRect(geo);
}

/* ───────────────────────────────────────────────────────────────────────── */

void GeoGrid_Debug(bool b) {
	sDebugMode = b;
}

void GeoGrid_TaskTable(GeoGrid* geo, SplitTask** taskTable, u32 num) {
	Assert(num != 0);
	geo->taskTable = taskTable;
	geo->numTaskTable = num;
}

extern u8 gMenloRegular;
extern u8 gMenloRegularSize;
extern u8 gMenloBold;
extern u8 gMenloBoldSize;

void GeoGrid_Init(GeoGrid* geo, Vec2s* wdim, Input* inputCtx, void* vg) {
	geo->wdim = wdim;
	geo->input = inputCtx;
	geo->vg = vg;
	
	GeoGrid_SetTopBarHeight(geo, SPLIT_BAR_HEIGHT);
	GeoGrid_SetBotBarHeight(geo, SPLIT_BAR_HEIGHT);
	
	nvgCreateFontMem(vg, "default", &gMenloRegular, (int)&gMenloRegularSize, 0);
	nvgCreateFontMem(vg, "default-bold", &gMenloBold, (int)&gMenloBoldSize, 0);
	
	geo->prevWorkRect = geo->workRect;
}

void GeoGrid_Update(GeoGrid* geo) {
	GeoGrid_SetTopBarHeight(geo, geo->bar[BAR_TOP].rect.h);
	GeoGrid_SetBotBarHeight(geo, geo->bar[BAR_BOT].rect.h);
	Element_Update(geo);
	Vtx_Update(geo);
	Edge_Update(geo);
	
	geo->prevWorkRect = geo->workRect;
}

void GeoGrid_Draw(GeoGrid* geo) {
	Vec2s* wdim = geo->wdim;
	
	// Draw Bars
	for (s32 i = 0; i < 2; i++) {
		glViewport(
			geo->bar[i].rect.x,
			wdim->y - geo->bar[i].rect.y - geo->bar[i].rect.h,
			geo->bar[i].rect.w,
			geo->bar[i].rect.h
		);
		nvgBeginFrame(geo->vg, geo->bar[i].rect.w, geo->bar[i].rect.h, gPixelRatio); {
			Rect r = geo->bar[i].rect;
			
			r.x = r.y = 0;
			
			nvgBeginPath(geo->vg);
			nvgRect(
				geo->vg,
				UnfoldRect(r)
			);
			nvgFillColor(geo->vg, Theme_GetColor(THEME_BASE, 255, 0.75f));
			nvgFill(geo->vg);
		} nvgEndFrame(geo->vg);
	}
	
	Split_Update(geo);
	Split_Draw(geo);
	if (sDebugMode)
		Split_DrawDebug(geo);
	DropMenu_Draw(geo);
}