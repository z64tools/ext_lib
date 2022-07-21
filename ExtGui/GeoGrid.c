#include "GeoGrid.h"
#include "Global.h"

f32 gPixelRatio = 1.0f;

void GeoGrid_RemoveDublicates(GeoGrid* geo);
void GeoGrid_Update_SplitRect(Split* split);

/* ───────────────────────────────────────────────────────────────────────── */

static SplitDir GeoGrid_GetDir_Opposite(SplitDir dir) {
	return WrapS(dir + 2, DIR_L, DIR_B + 1);
}

static SplitDir GeoGrid_GerDir_MouseToPressPos(Split* split) {
	Vec2s pos = Vec2_Substract(s, split->mousePos, split->mousePressPos);
	
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

static s32 GeoGrid_Cursor_GetDistTo(SplitState flag, Split* split) {
	Vec2s mouse[] = {
		{ split->mousePos.x, split->mousePos.y },
		{ split->mousePos.x, split->mousePos.y },
		{ split->mousePos.x, split->mousePos.y },
		{ split->mousePos.x, split->mousePos.y },
		{ split->mousePos.x, 0 },
		{ 0,                 split->mousePos.y },
		{ split->mousePos.x, 0 },
		{ 0,                 split->mousePos.y }
	};
	Vec2s pos[] = {
		{ 0,             split->rect.h, },
		{ 0,             0,            },
		{ split->rect.w, 0,            },
		{ split->rect.w, split->rect.h, },
		{ 0,             0,            },
		{ 0,             0,            },
		{ split->rect.w, 0,            },
		{ 0,             split->rect.h, },
	};
	s32 i;
	
	for (i = 0; (1 << i) <= SPLIT_SIDE_B; i++) {
		if (flag & (1 << i)) {
			break;
		}
	}
	
	return Math_Vec2s_DistXZ(&mouse[i], &pos[i]);
}

static SplitState GeoGrid_GetState_CursorPos(Split* split, s32 range) {
	for (s32 i = 0; (1 << i) <= SPLIT_SIDE_B; i++) {
		if (GeoGrid_Cursor_GetDistTo((1 << i), split) <= range) {
			return (1 << i);
		}
	}
	
	return SPLIT_POINT_NONE;
}

bool GeoGrid_Cursor_InRect(Split* split, Rect* rect) {
	s32 resX = (split->mousePos.x < rect->x + rect->w && split->mousePos.x >= rect->x);
	s32 resY = (split->mousePos.y < rect->y + rect->h && split->mousePos.y >= rect->y);
	
	return (resX && resY);
}

bool GeoGrid_Cursor_InSplit(Split* split) {
	s32 resX = (split->mousePos.x < split->rect.w && split->mousePos.x >= 0);
	s32 resY = (split->mousePos.y < split->rect.h && split->mousePos.y >= 0);
	
	return (resX && resY);
}

Split* GeoGrid_AddSplit(GeoGrid* geo, Rectf32* rect) {
	Split* split;
	
	Calloc(split, sizeof(Split));
	
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

static void GeoGrid_Edge_SetSlideClamp(GeoGrid* geo) {
	SplitEdge* tempEdge = geo->edgeHead;
	SplitEdge* setEdge = geo->actionEdge;
	SplitEdge* actionEdge = geo->actionEdge;
	u32 align = ((actionEdge->state & EDGE_VERTICAL) != 0);
	f64 posMin = actionEdge->vtx[0]->pos.axis[align];
	f64 posMax = actionEdge->vtx[1]->pos.axis[align];
	
	setEdge->state |= EDGE_EDIT;
	
	// Get edge with vtx closest to TOPLEFT
	while (tempEdge) {
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

static void GeoGrid_Reset(GeoGrid* geo) {
	Assert(geo->actionSplit);
	geo->actionSplit->stateFlag &= ~(SPLIT_POINTS | SPLIT_SIDES);
	
	geo->actionSplit = NULL;
}

static void GeoGrid_Split(GeoGrid* geo, Split* split, SplitDir dir) {
	Split* newSplit;
	f64 splitPos = (dir == DIR_L || dir == DIR_R) ? geo->input->mouse.pos.x : geo->input->mouse.pos.y;
	
	Calloc(newSplit, sizeof(Split));
	Node_Add(geo->splitHead, newSplit);
	
	if (dir == DIR_L) {
		newSplit->vtx[0] = GeoGrid_AddVtx(geo, splitPos, split->vtx[0]->pos.y);
		newSplit->vtx[1] = GeoGrid_AddVtx(geo, splitPos, split->vtx[1]->pos.y);
		newSplit->vtx[2] =  GeoGrid_AddVtx(geo, split->vtx[2]->pos.x, split->vtx[2]->pos.y);
		newSplit->vtx[3] = GeoGrid_AddVtx(geo, split->vtx[3]->pos.x, split->vtx[3]->pos.y);
		split->vtx[2] = GeoGrid_AddVtx(geo, splitPos, split->vtx[2]->pos.y);
		split->vtx[3] = GeoGrid_AddVtx(geo, splitPos, split->vtx[3]->pos.y);
	}
	
	if (dir == DIR_R) {
		newSplit->vtx[0] = GeoGrid_AddVtx(geo, split->vtx[0]->pos.x, split->vtx[0]->pos.y);
		newSplit->vtx[1] = GeoGrid_AddVtx(geo, split->vtx[1]->pos.x, split->vtx[1]->pos.y);
		newSplit->vtx[2] = GeoGrid_AddVtx(geo, splitPos, split->vtx[2]->pos.y);
		newSplit->vtx[3] = GeoGrid_AddVtx(geo, splitPos, split->vtx[3]->pos.y);
		split->vtx[0] = GeoGrid_AddVtx(geo, splitPos, split->vtx[0]->pos.y);
		split->vtx[1] = GeoGrid_AddVtx(geo, splitPos, split->vtx[1]->pos.y);
	}
	
	if (dir == DIR_T) {
		newSplit->vtx[0] = GeoGrid_AddVtx(geo, split->vtx[0]->pos.x, split->vtx[0]->pos.y);
		newSplit->vtx[1] = GeoGrid_AddVtx(geo, split->vtx[1]->pos.x, splitPos);
		newSplit->vtx[2] = GeoGrid_AddVtx(geo, split->vtx[2]->pos.x, splitPos);
		newSplit->vtx[3] = GeoGrid_AddVtx(geo, split->vtx[3]->pos.x, split->vtx[3]->pos.y);
		split->vtx[3] = GeoGrid_AddVtx(geo, split->vtx[3]->pos.x, splitPos);
		split->vtx[0] = GeoGrid_AddVtx(geo, split->vtx[0]->pos.x, splitPos);
	}
	
	if (dir == DIR_B) {
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
	GeoGrid_RemoveDublicates(geo);
	GeoGrid_Edge_SetSlideClamp(geo);
	GeoGrid_Update_SplitRect(split);
	GeoGrid_Update_SplitRect(newSplit);
}

static void GeoGrid_KillSplit(GeoGrid* geo, Split* split, SplitDir dir) {
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
		split->vtx[VTX_BOT_L] = killSplit->vtx[VTX_BOT_L];
		split->vtx[VTX_TOP_L] = killSplit->vtx[VTX_TOP_L];
		split->edge[EDGE_T] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
		split->edge[EDGE_B] = GeoGrid_AddEdge(geo, split->vtx[VTX_BOT_L], split->vtx[VTX_BOT_R]);
	}
	
	if (dir == DIR_T) {
		split->vtx[VTX_TOP_L] = killSplit->vtx[VTX_TOP_L];
		split->vtx[VTX_TOP_R] = killSplit->vtx[VTX_TOP_R];
		split->edge[EDGE_L] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_L], split->vtx[VTX_BOT_L]);
		split->edge[EDGE_R] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
	}
	
	if (dir == DIR_R) {
		split->vtx[VTX_BOT_R] = killSplit->vtx[VTX_BOT_R];
		split->vtx[VTX_TOP_R] = killSplit->vtx[VTX_TOP_R];
		split->edge[EDGE_T] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
		split->edge[EDGE_B] = GeoGrid_AddEdge(geo, split->vtx[VTX_BOT_L], split->vtx[VTX_BOT_R]);
	}
	
	if (dir == DIR_B) {
		split->vtx[VTX_BOT_L] = killSplit->vtx[VTX_BOT_L];
		split->vtx[VTX_BOT_R] = killSplit->vtx[VTX_BOT_R];
		split->edge[EDGE_L] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_L], split->vtx[VTX_BOT_L]);
		split->edge[EDGE_R] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
	}
	
	split->edge[dir] = killSplit->edge[dir];
	
	if (killSplit->id > 0) {
		geo->taskTable[killSplit->id].destroy(geo->passArg, killSplit->instance, killSplit);
		free(killSplit->instance);
	}
	Node_Kill(geo->splitHead, killSplit);
	GeoGrid_RemoveDublicates(geo);
	GeoGrid_Update_SplitRect(split);
}

s32 Split_Cursor(GeoGrid* geo, Split* split, s32 result) {
	if (geo->state.noClickInput ||
		!split->mouseInSplit ||
		split->blockMouse ||
		split->elemBlockMouse)
		return 0;
	
	return result;
}

/* ───────────────────────────────────────────────────────────────────────── */

void GeoGrid_Update_Vtx_RemoveDublicates(GeoGrid* geo, SplitVtx* vtx) {
	SplitVtx* vtx2 = geo->vtxHead;
	
	while (vtx2) {
		if (vtx2 == vtx) {
			vtx2 = vtx2->next;
			continue;
		}
		
		if (Vec2_Equal(&vtx->pos, &vtx2->pos)) {
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

void GeoGrid_Update_Vtx(GeoGrid* geo) {
	SplitVtx* vtx = geo->vtxHead;
	static s32 clean;
	
	if (geo->actionEdge != NULL) {
		clean = true;
	}
	
	while (vtx) {
		if (clean == true && geo->actionEdge == NULL) {
			GeoGrid_Update_Vtx_RemoveDublicates(geo, vtx);
		}
		
		if (vtx->killFlag == true) {
			Split* s = geo->splitHead;
			
			while (s) {
				for (s32 i = 0; i < 4; i++) {
					if (s->vtx[i] == vtx) {
						vtx->killFlag = false;
					}
				}
				s = s->next;
			}
			
			if (vtx->killFlag == true) {
				SplitVtx* killVtx = vtx;
				vtx = vtx->prev;
				Node_Kill(geo->vtxHead, killVtx);
				continue;
			}
		}
		
		vtx = vtx->next;
		if (vtx == NULL && geo->actionEdge == NULL) {
			clean = false;
		}
	}
}

/* ───────────────────────────────────────────────────────────────────────── */

void GeoGrid_Update_Edge_RemoveDublicates(GeoGrid* geo, SplitEdge* edge) {
	SplitEdge* edge2 = geo->edgeHead;
	
	while (edge2) {
		if (edge2 == edge) {
			edge2 = edge2->next;
			continue;
		}
		
		if (edge2->vtx[0] == edge->vtx[0] && edge2->vtx[1] == edge->vtx[1]) {
			SplitEdge* kill = edge2;
			Split* s = geo->splitHead;
			
			if (geo->actionEdge == edge2) {
				geo->actionEdge = edge;
			}
			
			while (s) {
				for (s32 i = 0; i < 4; i++) {
					if (s->edge[i] == edge2) {
						s->edge[i] = edge;
					}
				}
				s = s->next;
			}
			
			edge2 = edge2->next;
			Node_Kill(geo->edgeHead, kill);
			continue;
		}
		
		edge2 = edge2->next;
	}
}

void GeoGrid_Update_Edge_SetSlide(GeoGrid* geo) {
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
			if (edge->state & EDGE_STICK_L) {
				edge->pos = geo->workRect.x;
			}
			if (edge->state & EDGE_STICK_T) {
				edge->pos = geo->workRect.y;
			}
			if (edge->state & EDGE_STICK_R) {
				edge->pos = geo->workRect.x + geo->workRect.w;
			}
			if (edge->state & EDGE_STICK_B) {
				edge->pos = geo->workRect.y + geo->workRect.h;
			}
		} else {
			if (edge->state & EDGE_HORIZONTAL) {
				edge->pos -= geo->workRect.y;
				edge->pos *= diffCentY;
				edge->pos += geo->workRect.y;
			}
			if (edge->state & EDGE_VERTICAL) {
				edge->pos *= diffCentX;
			}
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
					edge->pos = __inputCtx->mouse.pos.y;
				}
				if (edge->state & EDGE_VERTICAL) {
					edge->pos = __inputCtx->mouse.pos.x;
				}
				edge->pos = ClampMin(edge->pos, geo->slide.clampMin + SPLIT_CLAMP);
				edge->pos = ClampMax(edge->pos, geo->slide.clampMax - SPLIT_CLAMP);
			} else {
				clampFail = true;
			}
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

void GeoGrid_Update_Edges(GeoGrid* geo) {
	SplitEdge* edge = geo->edgeHead;
	
	if (!Input_GetMouse(MOUSE_ANY)->hold) {
		geo->actionEdge = NULL;
	}
	
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
		
		GeoGrid_Update_Edge_RemoveDublicates(geo, edge);
		edge = edge->next;
	}
	
	GeoGrid_Update_Edge_SetSlide(geo);
}

/* ───────────────────────────────────────────────────────────────────────── */

void GeoGrid_Update_ActionSplit(GeoGrid* geo) {
	Split* split = geo->actionSplit;
	
	if (Input_GetMouse(MOUSE_ANY)->press) {
		SplitState tempStateA = GeoGrid_GetState_CursorPos(split, SPLIT_GRAB_DIST);
		SplitState tempStateB = GeoGrid_GetState_CursorPos(split, SPLIT_GRAB_DIST * 3);
		if (tempStateB & SPLIT_POINTS) {
			split->stateFlag |= tempStateB;
		} else if (tempStateA & SPLIT_SIDES) {
			split->stateFlag |= tempStateA;
		}
		
		if (split->stateFlag & SPLIT_SIDES) {
			s32 i;
			
			for (i = 0; i < 4; i++) {
				if (split->stateFlag & (1 << (4 + i))) {
					break;
				}
			}
			
			Assert(split->edge[i] != NULL);
			
			geo->actionEdge = split->edge[i];
			GeoGrid_Edge_SetSlideClamp(geo);
			GeoGrid_Reset(geo);
		}
	}
	
	if (Input_GetMouse(MOUSE_ANY)->hold) {
		if (split->stateFlag & SPLIT_POINTS) {
			s32 dist = Math_Vec2s_DistXZ(&split->mousePos, &split->mousePressPos);
			
			if (dist > 1) {
				CursorIndex cid = GeoGrid_GerDir_MouseToPressPos(split) + 1;
				Cursor_SetCursor(cid);
			}
			if (dist > SPLIT_CLAMP * 1.05) {
				GeoGrid_Reset(geo);
				if (split->mouseInSplit) {
					GeoGrid_Split(geo, split, GeoGrid_GerDir_MouseToPressPos(split));
				} else {
					GeoGrid_KillSplit(geo, split, GeoGrid_GerDir_MouseToPressPos(split));
				}
			}
		}
		
		if (split->stateFlag & SPLIT_SIDE_H) {
			Cursor_SetCursor(CURSOR_ARROW_H);
		}
		if (split->stateFlag & SPLIT_SIDE_V) {
			Cursor_SetCursor(CURSOR_ARROW_V);
		}
	}
}

void GeoGrid_Update_SplitRect(Split* split) {
	split->rect = (Rect) {
		floor(split->vtx[1]->pos.x),
		floor(split->vtx[1]->pos.y),
		floor(split->vtx[3]->pos.x) - floor(split->vtx[1]->pos.x),
		floor(split->vtx[3]->pos.y) - floor(split->vtx[1]->pos.y)
	};
}

void GeoGrid_Update_Split(GeoGrid* geo) {
	Split* split = geo->splitHead;
	MouseInput* mouse = &geo->input->mouse;
	
	Cursor_SetCursor(CURSOR_DEFAULT);
	
	if (geo->actionSplit != NULL && mouse->cursorAction == false) {
		GeoGrid_Reset(geo);
	}
	
	while (split) {
		GeoGrid_Update_SplitRect(split);
		Vec2s rectPos = { split->rect.x, split->rect.y };
		Rect headerRect = {
			0, split->rect.h - SPLIT_BAR_HEIGHT,
			split->rect.w, SPLIT_BAR_HEIGHT
		};
		split->mousePos = Vec2_Substract(s, mouse->pos, rectPos);
		split->mouseInSplit = GeoGrid_Cursor_InSplit(split);
		split->mouseInHeader = GeoGrid_Cursor_InRect(split, &headerRect);
		split->center.x = split->rect.w * 0.5f;
		split->center.y = (split->rect.h - SPLIT_BAR_HEIGHT) * 0.5f;
		
		split->blockMouse = false;
		
		if (mouse->click.press)
			split->mousePressPos = split->mousePos;
		if (geo->state.noSplit == false) {
			if (GeoGrid_GetState_CursorPos(split, SPLIT_GRAB_DIST * 3) & SPLIT_POINTS && split->mouseInSplit) {
				Cursor_SetCursor(CURSOR_CROSSHAIR);
				split->blockMouse = true;
			} else if (GeoGrid_GetState_CursorPos(split, SPLIT_GRAB_DIST) & SPLIT_SIDE_H && split->mouseInSplit) {
				Cursor_SetCursor(CURSOR_ARROW_H);
				split->blockMouse = true;
			} else if (GeoGrid_GetState_CursorPos(split, SPLIT_GRAB_DIST) & SPLIT_SIDE_V && split->mouseInSplit) {
				Cursor_SetCursor(CURSOR_ARROW_V);
				split->blockMouse = true;
			}
			
			if (geo->actionSplit == NULL && split->mouseInSplit && mouse->cursorAction) {
				if (mouse->click.press) {
					geo->actionSplit = split;
				}
			}
			
			if (geo->actionSplit != NULL && geo->actionSplit == split) {
				GeoGrid_Update_ActionSplit(geo);
			}
		}
		
		if (split->stateFlag != 0) {
			split->blockMouse = true;
		}
		
		u32 id = split->id;
		SplitTask* table = geo->taskTable;
		if (split->id > 0) {
			if (split->id != split->prevId) {
				if (split->prevId != 0) {
					table[split->prevId].destroy(geo->passArg, split->instance, split);
					free(split->instance);
				}
				
				Calloc(split->instance, table[id].size);
				table[id].init(geo->passArg, split->instance, split);
				split->prevId = split->id;
			}
			table[id].update(geo->passArg, split->instance, split);
		} else {
			if (split->prevId != 0) {
				id = split->prevId;
				table[id].destroy(geo->passArg, split->instance, split);
				free(split->instance);
				split->prevId = 0;
			}
		}
		
		for (s32 i = 0; i < 4; i++) {
			Assert(split->edge[i] != NULL);
			split->edge[i]->killFlag = false;
		}
		split = split->next;
	}
}

/* ───────────────────────────────────────────────────────────────────────── */

void GeoGrid_Draw_Debug(GeoGrid* geo) {
	SplitVtx* vtx = geo->vtxHead;
	Split* split = geo->splitHead;
	s32 num = 0;
	Vec2s* winDim = geo->winDim;
	
	glViewport(0, 0, winDim->x, winDim->y);
	nvgBeginFrame(geo->vg, winDim->x, winDim->y, gPixelRatio); {
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
					winDim->x * 0.5 - vtx->pos.x,
					-150.0f,
					150.0f
				) *
				0.1f,
				vtx->pos.y + Clamp(
					winDim->y * 0.5 - vtx->pos.y,
					-150.0f,
					150.0f
				) *
				0.1f
			};
			
			sprintf(buf, "%d", num);
			nvgFontSize(geo->vg, SPLIT_TEXT);
			nvgFontFace(geo->vg, "dejavu");
			nvgTextAlign(geo->vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_CENTER);
			nvgFillColor(geo->vg, Theme_GetColor(THEME_BASE_L1, 255, 1.0f));
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

void GeoGrid_Draw_SplitHeader(GeoGrid* geo, Split* split) {
	Rect rect = split->rect;
	
	rect.x = 0;
	rect.y = ClampMin(rect.h - SPLIT_BAR_HEIGHT, 0);
	
	nvgBeginPath(geo->vg);
	nvgRect(geo->vg, 0, ClampMin(rect.h - SPLIT_BAR_HEIGHT, 0), rect.w, rect.h);
	nvgPathWinding(geo->vg, NVG_HOLE);
	nvgFillColor(geo->vg, Theme_GetColor(THEME_BASE, 255, 1.30f));
	nvgFill(geo->vg);
}

void GeoGrid_Draw_SplitBorder(GeoGrid* geo, Split* split) {
	void* vg = geo->vg;
	Rect* rect = &split->rect;
	Rectf32 adjRect = {
		0 + SPLIT_SPLIT_W,
		0 + SPLIT_SPLIT_W,
		rect->w - SPLIT_SPLIT_W * 2,
		rect->h - SPLIT_SPLIT_W * 2
	};
	
	if (split->id > 0) {
		u32 id = split->id;
		SplitTask* table = geo->taskTable;
		
		nvgBeginPath(vg);
		nvgRect(vg, 0, 0, rect->w, rect->h);
		if (split->bg.useCustomBG == true) {
			nvgFillColor(vg, nvgRGBA(split->bg.color.r, split->bg.color.g, split->bg.color.b, 255));
		} else {
			nvgFillColor(vg, Theme_GetColor(THEME_BASE_SPLIT, 255, 1.0f));
		}
		nvgFill(vg);
		
		nvgEndFrame(geo->vg);
		nvgBeginFrame(geo->vg, split->rect.w, split->rect.h, gPixelRatio);
		
		table[id].draw(geo->passArg, split->instance, split);
		// GeoGrid_Draw_SplitHeader(geo, split);
		Element_Draw(geo, split);
	} else {
		nvgBeginPath(vg);
		nvgRect(vg, 0, 0, rect->w, rect->h);
		nvgFillColor(vg, Theme_GetColor(THEME_BASE_SPLIT, 255, 1.0f));
		nvgFill(vg);
		// GeoGrid_Draw_SplitHeader(geo, split);
	}
	
	if (split->edge[DIR_L]->state & EDGE_STICK_L) {
		adjRect.x += SPLIT_SPLIT_W / 2;
		adjRect.w -= SPLIT_SPLIT_W / 2;
	}
	if (split->edge[DIR_R]->state & EDGE_STICK_R) {
		adjRect.w -= SPLIT_SPLIT_W / 2;
	}
	
	if (split->edge[DIR_T]->state & EDGE_STICK_T) {
		adjRect.y += SPLIT_SPLIT_W / 2;
		adjRect.h -= SPLIT_SPLIT_W / 2;
	}
	if (split->edge[DIR_B]->state & EDGE_STICK_B) {
		adjRect.h -= SPLIT_SPLIT_W / 2;
	}
	
	nvgBeginPath(geo->vg);
	nvgRect(geo->vg, 0, 0, rect->w, rect->h);
	nvgRoundedRect(
		geo->vg,
		adjRect.x,
		adjRect.y,
		adjRect.w,
		adjRect.h,
		SPLIT_ROUND_R
	);
	nvgPathWinding(geo->vg, NVG_HOLE);
	nvgFillColor(geo->vg, Theme_GetColor(THEME_BASE, 255, 0.66f));
	nvgFill(geo->vg);
}

void GeoGrid_Draw_Splits(GeoGrid* geo) {
	Split* split = geo->splitHead;
	Vec2s* winDim = geo->winDim;
	
	for (; split != NULL; split = split->next) {
		GeoGrid_Update_SplitRect(split);
		
		glViewport(
			split->rect.x,
			winDim->y - split->rect.y - split->rect.h,
			split->rect.w,
			split->rect.h
		);
		nvgBeginFrame(geo->vg, split->rect.w, split->rect.h, gPixelRatio); {
			GeoGrid_Draw_SplitBorder(geo, split);
		} nvgEndFrame(geo->vg);
	}
}

/* ───────────────────────────────────────────────────────────────────────── */

void GeoGrid_RemoveDublicates(GeoGrid* geo) {
	SplitVtx* vtx = geo->vtxHead;
	SplitEdge* edge = geo->edgeHead;
	
	while (vtx) {
		GeoGrid_Update_Vtx_RemoveDublicates(geo, vtx);
		vtx = vtx->next;
	}
	
	while (edge) {
		GeoGrid_Update_Edge_RemoveDublicates(geo, edge);
		edge = edge->next;
	}
}

void GeoGrid_UpdateWorkRect(GeoGrid* geo) {
	Vec2s* winDim = geo->winDim;
	
	geo->workRect = (Rect) { 0, 0 + geo->bar[BAR_TOP].rect.h, winDim->x,
				 winDim->y - geo->bar[BAR_BOT].rect.h -
				 geo->bar[BAR_TOP].rect.h };
}

void GeoGrid_SetTopBarHeight(GeoGrid* geo, s32 h) {
	geo->bar[BAR_TOP].rect.x = 0;
	geo->bar[BAR_TOP].rect.y = 0;
	geo->bar[BAR_TOP].rect.w = geo->winDim->x;
	geo->bar[BAR_TOP].rect.h = h;
	GeoGrid_UpdateWorkRect(geo);
}

void GeoGrid_SetBotBarHeight(GeoGrid* geo, s32 h) {
	geo->bar[BAR_BOT].rect.x = 0;
	geo->bar[BAR_BOT].rect.y = geo->winDim->y - h;
	geo->bar[BAR_BOT].rect.w = geo->winDim->x;
	geo->bar[BAR_BOT].rect.h = h;
	GeoGrid_UpdateWorkRect(geo);
}

/* ───────────────────────────────────────────────────────────────────────── */

extern unsigned char gFont_DejaVuBold[705684];
extern unsigned char gFont_DejaVuLight[355380];
extern unsigned char gFont_DejaVu[757076];

void GeoGrid_Init(GeoGrid* geo, Vec2s* winDim, InputContext* inputCtx, void* vg) {
	geo->winDim = winDim;
	geo->input = inputCtx;
	geo->vg = vg;
	GeoGrid_SetTopBarHeight(geo, SPLIT_BAR_HEIGHT);
	GeoGrid_SetBotBarHeight(geo, SPLIT_BAR_HEIGHT);
	
	nvgCreateFontMem(vg, "dejavu", gFont_DejaVu, sizeof(gFont_DejaVu), 0);
	nvgCreateFontMem(vg, "dejavu-bold", gFont_DejaVu, sizeof(gFont_DejaVu), 0);
	nvgCreateFontMem(vg, "dejavu-light", gFont_DejaVu, sizeof(gFont_DejaVu), 0);
	
	geo->prevWorkRect = geo->workRect;
	Element_Init(geo);
}

void GeoGrid_Update(GeoGrid* geo) {
	GeoGrid_SetTopBarHeight(geo, geo->bar[BAR_TOP].rect.h);
	GeoGrid_SetBotBarHeight(geo, geo->bar[BAR_BOT].rect.h);
	Element_Update(geo);
	GeoGrid_Update_Vtx(geo);
	GeoGrid_Update_Edges(geo);
	GeoGrid_Update_Split(geo);
	
	geo->prevWorkRect = geo->workRect;
}

void GeoGrid_Draw(GeoGrid* geo) {
	Vec2s* winDim = geo->winDim;
	
	// Draw Bars
	for (s32 i = 0; i < 2; i++) {
		glViewport(
			geo->bar[i].rect.x,
			winDim->y - geo->bar[i].rect.y - geo->bar[i].rect.h,
			geo->bar[i].rect.w,
			geo->bar[i].rect.h
		);
		nvgBeginFrame(geo->vg, geo->bar[i].rect.w, geo->bar[i].rect.h, gPixelRatio); {
			nvgBeginPath(geo->vg);
			nvgRect(
				geo->vg,
				0,
				0,
				geo->bar[i].rect.w,
				geo->bar[i].rect.h
			);
			nvgFillColor(geo->vg, Theme_GetColor(THEME_BASE_L1, 255, 0.825f));
			nvgFill(geo->vg);
		} nvgEndFrame(geo->vg);
	}
	
	GeoGrid_Draw_Splits(geo);
	if (0)
		GeoGrid_Draw_Debug(geo);
}