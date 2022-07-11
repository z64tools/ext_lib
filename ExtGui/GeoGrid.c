#include "GeoGrid.h"
#include "Global.h"

f32 gPixelRatio = 1.0f;

void GeoGrid_RemoveDublicates(GeoGrid* geoGrid);
void GeoGrid_Update_SplitRect(Split* split);

/* ───────────────────────────────────────────────────────────────────────── */

SplitDir GeoGrid_GetDir_Opposite(SplitDir dir) {
	return WrapS(dir + 2, DIR_L, DIR_B + 1);
}

SplitDir GeoGrid_GerDir_MouseToPressPos(Split* split) {
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

SplitVtx* GeoGrid_AddVtx(GeoGrid* geoGrid, f64 x, f64 y) {
	SplitVtx* head = geoGrid->vtxHead;
	SplitVtx* vtx;
	
	while (head) {
		if (fabs(head->pos.x - x) < 0.25 && fabs(head->pos.y - y) < 0.25)
			return head;
		head = head->next;
	}
	
	Calloc(vtx, sizeof(SplitVtx));
	
	vtx->pos.x = x;
	vtx->pos.y = y;
	Node_Add(geoGrid->vtxHead, vtx);
	
	return vtx;
}

SplitEdge* GeoGrid_AddEdge(GeoGrid* geoGrid, SplitVtx* v1, SplitVtx* v2) {
	SplitEdge* head = geoGrid->edgeHead;
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
		Node_Add(geoGrid->edgeHead, edge);
	}
	
	if (edge->vtx[0]->pos.y == edge->vtx[1]->pos.y) {
		edge->state |= EDGE_HORIZONTAL;
		edge->pos = edge->vtx[0]->pos.y;
		if (edge->pos < geoGrid->workRect.y + 1) {
			edge->state |= EDGE_STICK_T;
		}
		if (edge->pos > geoGrid->workRect.y + geoGrid->workRect.h - 1) {
			edge->state |= EDGE_STICK_B;
		}
	} else {
		edge->state |= EDGE_VERTICAL;
		edge->pos = edge->vtx[0]->pos.x;
		if (edge->pos < geoGrid->workRect.x + 1) {
			edge->state |= EDGE_STICK_L;
		}
		if (edge->pos > geoGrid->workRect.x + geoGrid->workRect.w - 1) {
			edge->state |= EDGE_STICK_R;
		}
	}
	
	return edge;
}

s32 GeoGrid_Cursor_GetDistTo(SplitState flag, Split* split) {
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

SplitState GeoGrid_GetState_CursorPos(Split* split, s32 range) {
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

Split* GeoGrid_AddSplit(GeoGrid* geoGrid, Rectf32* rect) {
	Split* split;
	
	Calloc(split, sizeof(Split));
	
	split->vtx[VTX_BOT_L] = GeoGrid_AddVtx(geoGrid, rect->x, rect->y + rect->h);
	split->vtx[VTX_TOP_L] = GeoGrid_AddVtx(geoGrid, rect->x, rect->y);
	split->vtx[VTX_TOP_R] = GeoGrid_AddVtx(geoGrid, rect->x + rect->w, rect->y);
	split->vtx[VTX_BOT_R] = GeoGrid_AddVtx(geoGrid, rect->x + rect->w, rect->y + rect->h);
	
	split->edge[EDGE_L] = GeoGrid_AddEdge(geoGrid, split->vtx[VTX_BOT_L], split->vtx[VTX_TOP_L]);
	split->edge[EDGE_T] = GeoGrid_AddEdge(geoGrid, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
	split->edge[EDGE_R] = GeoGrid_AddEdge(geoGrid, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
	split->edge[EDGE_B] = GeoGrid_AddEdge(geoGrid, split->vtx[VTX_BOT_R], split->vtx[VTX_BOT_L]);
	
	Node_Add(geoGrid->splitHead, split);
	
	return split;
}

void GeoGrid_Edge_SetSlideClamp(GeoGrid* geoGrid) {
	SplitEdge* tempEdge = geoGrid->edgeHead;
	SplitEdge* setEdge = geoGrid->actionEdge;
	SplitEdge* actionEdge = geoGrid->actionEdge;
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
				tempEdge = geoGrid->edgeHead;
				posMin = setEdge->vtx[0]->pos.axis[align];
				continue;
			}
		}
		
		tempEdge = tempEdge->next;
	}
	
	tempEdge = geoGrid->edgeHead;
	
	// Set all below setEdgeA
	while (tempEdge) {
		if ((tempEdge->state & EDGE_ALIGN) == (setEdge->state & EDGE_ALIGN)) {
			if (tempEdge->vtx[0] == setEdge->vtx[1]) {
				tempEdge->state |= EDGE_EDIT;
				setEdge = tempEdge;
				tempEdge = geoGrid->edgeHead;
				posMax = setEdge->vtx[1]->pos.axis[align];
				continue;
			}
		}
		
		tempEdge = tempEdge->next;
	}
	
	tempEdge = geoGrid->edgeHead;
	
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
	
	if (geoGrid->actionEdge->state & EDGE_VERTICAL) {
		geoGrid->slide.clampMin = geoGrid->workRect.x;
		geoGrid->slide.clampMax = geoGrid->workRect.x + geoGrid->workRect.w;
	} else {
		geoGrid->slide.clampMin = geoGrid->workRect.y;
		geoGrid->slide.clampMax = geoGrid->workRect.y + geoGrid->workRect.h;
	}
}

void GeoGrid_Reset(GeoGrid* geoGrid) {
	Assert(geoGrid->actionSplit);
	geoGrid->actionSplit->stateFlag &= ~(SPLIT_POINTS | SPLIT_SIDES);
	
	geoGrid->actionSplit = NULL;
}

void GeoGrid_Split(GeoGrid* geoGrid, Split* split, SplitDir dir) {
	Split* newSplit;
	f64 splitPos = (dir == DIR_L || dir == DIR_R) ? geoGrid->input->mouse.pos.x : geoGrid->input->mouse.pos.y;
	
	Calloc(newSplit, sizeof(Split));
	Node_Add(geoGrid->splitHead, newSplit);
	
	if (dir == DIR_L) {
		newSplit->vtx[0] = GeoGrid_AddVtx(geoGrid, splitPos, split->vtx[0]->pos.y);
		newSplit->vtx[1] = GeoGrid_AddVtx(geoGrid, splitPos, split->vtx[1]->pos.y);
		newSplit->vtx[2] =  GeoGrid_AddVtx(geoGrid, split->vtx[2]->pos.x, split->vtx[2]->pos.y);
		newSplit->vtx[3] = GeoGrid_AddVtx(geoGrid, split->vtx[3]->pos.x, split->vtx[3]->pos.y);
		split->vtx[2] = GeoGrid_AddVtx(geoGrid, splitPos, split->vtx[2]->pos.y);
		split->vtx[3] = GeoGrid_AddVtx(geoGrid, splitPos, split->vtx[3]->pos.y);
	}
	
	if (dir == DIR_R) {
		newSplit->vtx[0] = GeoGrid_AddVtx(geoGrid, split->vtx[0]->pos.x, split->vtx[0]->pos.y);
		newSplit->vtx[1] = GeoGrid_AddVtx(geoGrid, split->vtx[1]->pos.x, split->vtx[1]->pos.y);
		newSplit->vtx[2] = GeoGrid_AddVtx(geoGrid, splitPos, split->vtx[2]->pos.y);
		newSplit->vtx[3] = GeoGrid_AddVtx(geoGrid, splitPos, split->vtx[3]->pos.y);
		split->vtx[0] = GeoGrid_AddVtx(geoGrid, splitPos, split->vtx[0]->pos.y);
		split->vtx[1] = GeoGrid_AddVtx(geoGrid, splitPos, split->vtx[1]->pos.y);
	}
	
	if (dir == DIR_T) {
		newSplit->vtx[0] = GeoGrid_AddVtx(geoGrid, split->vtx[0]->pos.x, split->vtx[0]->pos.y);
		newSplit->vtx[1] = GeoGrid_AddVtx(geoGrid, split->vtx[1]->pos.x, splitPos);
		newSplit->vtx[2] = GeoGrid_AddVtx(geoGrid, split->vtx[2]->pos.x, splitPos);
		newSplit->vtx[3] = GeoGrid_AddVtx(geoGrid, split->vtx[3]->pos.x, split->vtx[3]->pos.y);
		split->vtx[3] = GeoGrid_AddVtx(geoGrid, split->vtx[3]->pos.x, splitPos);
		split->vtx[0] = GeoGrid_AddVtx(geoGrid, split->vtx[0]->pos.x, splitPos);
	}
	
	if (dir == DIR_B) {
		newSplit->vtx[0] = GeoGrid_AddVtx(geoGrid, split->vtx[0]->pos.x, splitPos);
		newSplit->vtx[1] = GeoGrid_AddVtx(geoGrid, split->vtx[1]->pos.x, split->vtx[1]->pos.y);
		newSplit->vtx[2] = GeoGrid_AddVtx(geoGrid, split->vtx[2]->pos.x, split->vtx[2]->pos.y);
		newSplit->vtx[3] = GeoGrid_AddVtx(geoGrid, split->vtx[3]->pos.x, splitPos);
		split->vtx[1] = GeoGrid_AddVtx(geoGrid, split->vtx[1]->pos.x, splitPos);
		split->vtx[2] = GeoGrid_AddVtx(geoGrid, split->vtx[2]->pos.x, splitPos);
	}
	
	split->edge[EDGE_L] = GeoGrid_AddEdge(geoGrid, split->vtx[VTX_BOT_L], split->vtx[VTX_TOP_L]);
	split->edge[EDGE_T] = GeoGrid_AddEdge(geoGrid, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
	split->edge[EDGE_R] = GeoGrid_AddEdge(geoGrid, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
	split->edge[EDGE_B] = GeoGrid_AddEdge(geoGrid, split->vtx[VTX_BOT_R], split->vtx[VTX_BOT_L]);
	
	newSplit->edge[EDGE_L] = GeoGrid_AddEdge(geoGrid, newSplit->vtx[VTX_BOT_L], newSplit->vtx[VTX_TOP_L]);
	newSplit->edge[EDGE_T] = GeoGrid_AddEdge(geoGrid, newSplit->vtx[VTX_TOP_L], newSplit->vtx[VTX_TOP_R]);
	newSplit->edge[EDGE_R] = GeoGrid_AddEdge(geoGrid, newSplit->vtx[VTX_TOP_R], newSplit->vtx[VTX_BOT_R]);
	newSplit->edge[EDGE_B] = GeoGrid_AddEdge(geoGrid, newSplit->vtx[VTX_BOT_R], newSplit->vtx[VTX_BOT_L]);
	
	geoGrid->actionEdge = newSplit->edge[dir];
	GeoGrid_RemoveDublicates(geoGrid);
	GeoGrid_Edge_SetSlideClamp(geoGrid);
	GeoGrid_Update_SplitRect(split);
	GeoGrid_Update_SplitRect(newSplit);
}

void GeoGrid_KillSplit(GeoGrid* geoGrid, Split* split, SplitDir dir) {
	SplitEdge* sharedEdge = split->edge[dir];
	Split* killSplit = geoGrid->splitHead;
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
		split->edge[EDGE_T] = GeoGrid_AddEdge(geoGrid, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
		split->edge[EDGE_B] = GeoGrid_AddEdge(geoGrid, split->vtx[VTX_BOT_L], split->vtx[VTX_BOT_R]);
	}
	
	if (dir == DIR_T) {
		split->vtx[VTX_TOP_L] = killSplit->vtx[VTX_TOP_L];
		split->vtx[VTX_TOP_R] = killSplit->vtx[VTX_TOP_R];
		split->edge[EDGE_L] = GeoGrid_AddEdge(geoGrid, split->vtx[VTX_TOP_L], split->vtx[VTX_BOT_L]);
		split->edge[EDGE_R] = GeoGrid_AddEdge(geoGrid, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
	}
	
	if (dir == DIR_R) {
		split->vtx[VTX_BOT_R] = killSplit->vtx[VTX_BOT_R];
		split->vtx[VTX_TOP_R] = killSplit->vtx[VTX_TOP_R];
		split->edge[EDGE_T] = GeoGrid_AddEdge(geoGrid, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
		split->edge[EDGE_B] = GeoGrid_AddEdge(geoGrid, split->vtx[VTX_BOT_L], split->vtx[VTX_BOT_R]);
	}
	
	if (dir == DIR_B) {
		split->vtx[VTX_BOT_L] = killSplit->vtx[VTX_BOT_L];
		split->vtx[VTX_BOT_R] = killSplit->vtx[VTX_BOT_R];
		split->edge[EDGE_L] = GeoGrid_AddEdge(geoGrid, split->vtx[VTX_TOP_L], split->vtx[VTX_BOT_L]);
		split->edge[EDGE_R] = GeoGrid_AddEdge(geoGrid, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
	}
	
	split->edge[dir] = killSplit->edge[dir];
	
	if (killSplit->id > 0) {
		geoGrid->taskTable[killSplit->id].destroy(geoGrid->passArg, killSplit->instance, killSplit);
		free(killSplit->instance);
	}
	Node_Kill(geoGrid->splitHead, killSplit);
	GeoGrid_RemoveDublicates(geoGrid);
	GeoGrid_Update_SplitRect(split);
}

static GeoGrid* __geoCtx;

s32 Split_Cursor(Split* split, s32 result) {
	if (__geoCtx->ctxMenu.num != 0 ||
		!split->mouseInSplit ||
		split->blockMouse ||
		split->elemBlockMouse)
		return 0;
	
	return result;
}

/* ───────────────────────────────────────────────────────────────────────── */

void GeoGrid_Update_Vtx_RemoveDublicates(GeoGrid* geoGrid, SplitVtx* vtx) {
	SplitVtx* vtx2 = geoGrid->vtxHead;
	
	while (vtx2) {
		if (vtx2 == vtx) {
			vtx2 = vtx2->next;
			continue;
		}
		
		if (Vec2_Equal(&vtx->pos, &vtx2->pos)) {
			SplitVtx* kill = vtx2;
			Split* s = geoGrid->splitHead;
			SplitEdge* e = geoGrid->edgeHead;
			
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
			Node_Kill(geoGrid->vtxHead, kill);
			continue;
		}
		
		vtx2 = vtx2->next;
	}
}

void GeoGrid_Update_Vtx(GeoGrid* geoGrid) {
	SplitVtx* vtx = geoGrid->vtxHead;
	static s32 clean;
	
	if (geoGrid->actionEdge != NULL) {
		clean = true;
	}
	
	while (vtx) {
		if (clean == true && geoGrid->actionEdge == NULL) {
			GeoGrid_Update_Vtx_RemoveDublicates(geoGrid, vtx);
		}
		
		if (vtx->killFlag == true) {
			Split* s = geoGrid->splitHead;
			
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
				Node_Kill(geoGrid->vtxHead, killVtx);
				continue;
			}
		}
		
		vtx = vtx->next;
		if (vtx == NULL && geoGrid->actionEdge == NULL) {
			clean = false;
		}
	}
}

/* ───────────────────────────────────────────────────────────────────────── */

void GeoGrid_Update_Edge_RemoveDublicates(GeoGrid* geoGrid, SplitEdge* edge) {
	SplitEdge* edge2 = geoGrid->edgeHead;
	
	while (edge2) {
		if (edge2 == edge) {
			edge2 = edge2->next;
			continue;
		}
		
		if (edge2->vtx[0] == edge->vtx[0] && edge2->vtx[1] == edge->vtx[1]) {
			SplitEdge* kill = edge2;
			Split* s = geoGrid->splitHead;
			
			if (geoGrid->actionEdge == edge2) {
				geoGrid->actionEdge = edge;
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
			Node_Kill(geoGrid->edgeHead, kill);
			continue;
		}
		
		edge2 = edge2->next;
	}
}

void GeoGrid_Update_Edge_SetSlide(GeoGrid* geoGrid) {
	SplitEdge* edge = geoGrid->edgeHead;
	f64 diffCentX = (f64)geoGrid->workRect.w / geoGrid->prevWorkRect.w;
	f64 diffCentY = (f64)geoGrid->workRect.h / geoGrid->prevWorkRect.h;
	
	while (edge) {
		bool clampFail = false;
		bool isEditEdge = (edge == geoGrid->actionEdge || edge->state & EDGE_EDIT);
		bool isCornerEdge = ((edge->state & EDGE_STICK) != 0);
		bool isHor = ((edge->state & EDGE_VERTICAL) == 0);
		
		Assert(!(edge->state & EDGE_HORIZONTAL && edge->state & EDGE_VERTICAL));
		
		if (isCornerEdge) {
			if (edge->state & EDGE_STICK_L) {
				edge->pos = geoGrid->workRect.x;
			}
			if (edge->state & EDGE_STICK_T) {
				edge->pos = geoGrid->workRect.y;
			}
			if (edge->state & EDGE_STICK_R) {
				edge->pos = geoGrid->workRect.x + geoGrid->workRect.w;
			}
			if (edge->state & EDGE_STICK_B) {
				edge->pos = geoGrid->workRect.y + geoGrid->workRect.h;
			}
		} else {
			if (edge->state & EDGE_HORIZONTAL) {
				edge->pos -= geoGrid->workRect.y;
				edge->pos *= diffCentY;
				edge->pos += geoGrid->workRect.y;
			}
			if (edge->state & EDGE_VERTICAL) {
				edge->pos *= diffCentX;
			}
		}
		
		if (isEditEdge && isCornerEdge == false) {
			SplitEdge* temp = geoGrid->edgeHead;
			s32 align = WrapS(edge->state & EDGE_ALIGN, 0, 2);
			
			while (temp) {
				for (s32 i = 0; i < 2; i++) {
					if (((temp->state & EDGE_ALIGN) != (edge->state & EDGE_ALIGN)) && temp->vtx[1] == edge->vtx[i]) {
						if (temp->vtx[0]->pos.axis[align] > geoGrid->slide.clampMin) {
							geoGrid->slide.clampMin = temp->vtx[0]->pos.axis[align];
						}
					}
				}
				
				for (s32 i = 0; i < 2; i++) {
					if (((temp->state & EDGE_ALIGN) != (edge->state & EDGE_ALIGN)) && temp->vtx[0] == edge->vtx[i]) {
						if (temp->vtx[1]->pos.axis[align] < geoGrid->slide.clampMax) {
							geoGrid->slide.clampMax = temp->vtx[1]->pos.axis[align];
						}
					}
				}
				
				temp = temp->next;
			}
			
			if (geoGrid->slide.clampMax - SPLIT_CLAMP > geoGrid->slide.clampMin + SPLIT_CLAMP) {
				if (edge->state & EDGE_HORIZONTAL) {
					edge->pos = __inputCtx->mouse.pos.y;
				}
				if (edge->state & EDGE_VERTICAL) {
					edge->pos = __inputCtx->mouse.pos.x;
				}
				edge->pos = ClampMin(edge->pos, geoGrid->slide.clampMin + SPLIT_CLAMP);
				edge->pos = ClampMax(edge->pos, geoGrid->slide.clampMax - SPLIT_CLAMP);
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

void GeoGrid_Update_Edges(GeoGrid* geoGrid) {
	SplitEdge* edge = geoGrid->edgeHead;
	
	if (!Input_GetMouse(MOUSE_ANY)->hold) {
		geoGrid->actionEdge = NULL;
	}
	
	while (edge) {
		if (edge->killFlag == true) {
			SplitEdge* temp = edge->next;
			
			Node_Kill(geoGrid->edgeHead, edge);
			edge = temp;
			
			continue;
		}
		edge->killFlag = true;
		
		if (geoGrid->actionEdge == NULL) {
			edge->state &= ~EDGE_EDIT;
		}
		
		GeoGrid_Update_Edge_RemoveDublicates(geoGrid, edge);
		edge = edge->next;
	}
	
	GeoGrid_Update_Edge_SetSlide(geoGrid);
}

/* ───────────────────────────────────────────────────────────────────────── */

void GeoGrid_Update_ActionSplit(GeoGrid* geoGrid) {
	Split* split = geoGrid->actionSplit;
	
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
			
			geoGrid->actionEdge = split->edge[i];
			GeoGrid_Edge_SetSlideClamp(geoGrid);
			GeoGrid_Reset(geoGrid);
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
				GeoGrid_Reset(geoGrid);
				if (split->mouseInSplit) {
					GeoGrid_Split(geoGrid, split, GeoGrid_GerDir_MouseToPressPos(split));
				} else {
					GeoGrid_KillSplit(geoGrid, split, GeoGrid_GerDir_MouseToPressPos(split));
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

void GeoGrid_Update_Split(GeoGrid* geoGrid) {
	Split* split = geoGrid->splitHead;
	MouseInput* mouse = &geoGrid->input->mouse;
	
	Cursor_SetCursor(CURSOR_DEFAULT);
	
	if (geoGrid->actionSplit != NULL && mouse->cursorAction == false) {
		GeoGrid_Reset(geoGrid);
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
		if (geoGrid->ctxMenu.num == 0) {
			if (GeoGrid_GetState_CursorPos(split, SPLIT_GRAB_DIST * 3) & SPLIT_POINTS &&
				split->mouseInSplit) {
				Cursor_SetCursor(CURSOR_CROSSHAIR);
				split->blockMouse = true;
			} else if (GeoGrid_GetState_CursorPos(split, SPLIT_GRAB_DIST) & SPLIT_SIDE_H &&
				split->mouseInSplit) {
				Cursor_SetCursor(CURSOR_ARROW_H);
				split->blockMouse = true;
			} else if (GeoGrid_GetState_CursorPos(split, SPLIT_GRAB_DIST) & SPLIT_SIDE_V &&
				split->mouseInSplit) {
				Cursor_SetCursor(CURSOR_ARROW_V);
				split->blockMouse = true;
			}
			
			if (geoGrid->actionSplit == NULL && split->mouseInSplit && mouse->cursorAction) {
				if (mouse->click.press) {
					geoGrid->actionSplit = split;
				}
			}
			
			if (geoGrid->actionSplit != NULL && geoGrid->actionSplit == split) {
				GeoGrid_Update_ActionSplit(geoGrid);
			}
		}
		
		if (split->stateFlag != 0) {
			split->blockMouse = true;
		}
		
		u32 id = split->id;
		SplitTask* table = geoGrid->taskTable;
		if (split->id > 0) {
			if (split->id != split->prevId) {
				if (split->prevId != 0) {
					table[split->prevId].destroy(geoGrid->passArg, split->instance, split);
					free(split->instance);
				}
				
				Calloc(split->instance, table[id].size);
				table[id].init(geoGrid->passArg, split->instance, split);
				split->prevId = split->id;
			}
			table[id].update(geoGrid->passArg, split->instance, split);
		} else {
			if (split->prevId != 0) {
				id = split->prevId;
				table[id].destroy(geoGrid->passArg, split->instance, split);
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

void GeoGrid_SetContextMenu(GeoGrid* geoGrid, Split* split, char** optionList, s32 num) {
	GeoCtxMenu* ctxMenu = &geoGrid->ctxMenu;
	
	ctxMenu->optionList = optionList;
	ctxMenu->split = split;
	ctxMenu->num = num;
	ctxMenu->pos = split->mousePos;
	ctxMenu->pos.x += split->vtx[1]->pos.x;
	ctxMenu->pos.y += split->vtx[1]->pos.y;
}

s32 GeoGrid_GetContextMenuResult(GeoGrid* geoGrid, Split* split) {
	if (geoGrid->ctxMenu.split == split && geoGrid->ctxMenu.num < 0) {
		s32 ret = geoGrid->ctxMenu.num;
		
		geoGrid->ctxMenu.num = 0;
		geoGrid->ctxMenu.split = NULL;
		
		return -ret;
	}
	
	return 0;
}

/* ───────────────────────────────────────────────────────────────────────── */

void GeoGrid_Draw_Debug(GeoGrid* geoGrid) {
	SplitVtx* vtx = geoGrid->vtxHead;
	Split* split = geoGrid->splitHead;
	s32 num = 0;
	Vec2s* winDim = geoGrid->winDim;
	
	glViewport(0, 0, winDim->x, winDim->y);
	nvgBeginFrame(geoGrid->vg, winDim->x, winDim->y, gPixelRatio); {
		while (split) {
			nvgBeginPath(geoGrid->vg);
			nvgLineCap(geoGrid->vg, NVG_ROUND);
			nvgStrokeWidth(geoGrid->vg, 1.0f);
			nvgMoveTo(
				geoGrid->vg,
				split->vtx[0]->pos.x + 2,
				split->vtx[0]->pos.y - 2
			);
			nvgLineTo(
				geoGrid->vg,
				split->vtx[1]->pos.x + 2,
				split->vtx[1]->pos.y + 2
			);
			nvgLineTo(
				geoGrid->vg,
				split->vtx[2]->pos.x - 2,
				split->vtx[2]->pos.y + 2
			);
			nvgLineTo(
				geoGrid->vg,
				split->vtx[3]->pos.x - 2,
				split->vtx[3]->pos.y - 2
			);
			nvgLineTo(
				geoGrid->vg,
				split->vtx[0]->pos.x + 2,
				split->vtx[0]->pos.y - 2
			);
			nvgStrokeColor(geoGrid->vg, nvgHSLA(0.111 * num, 1.0f, 0.6f, 255));
			nvgStroke(geoGrid->vg);
			
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
			nvgFontSize(geoGrid->vg, SPLIT_TEXT);
			nvgFontFace(geoGrid->vg, "dejavu");
			nvgTextAlign(geoGrid->vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_CENTER);
			nvgFillColor(geoGrid->vg, Theme_GetColor(THEME_BASE_L1, 255, 1.0f));
			nvgFontBlur(geoGrid->vg, 1.5f);
			nvgText(geoGrid->vg, pos.x, pos.y, buf, 0);
			nvgFontBlur(geoGrid->vg, 0);
			nvgFillColor(geoGrid->vg, Theme_GetColor(THEME_TEXT, 255, 1.0f));
			nvgText(geoGrid->vg, pos.x, pos.y, buf, 0);
			
			vtx = vtx->next;
			num++;
		}
	} nvgEndFrame(geoGrid->vg);
}

void GeoGrid_Draw_SplitHeader(GeoGrid* geoGrid, Split* split) {
	Rect rect = split->rect;
	s32 menuSel;
	ElButton* hdrbtn = (void*)split->header.data;
	u32 id = split->id;
	SplitTask* table = geoGrid->taskTable;
	
	rect.x = 0;
	rect.y = ClampMin(rect.h - SPLIT_BAR_HEIGHT, 0);
	
	nvgBeginPath(geoGrid->vg);
	nvgRect(geoGrid->vg, 0, ClampMin(rect.h - SPLIT_BAR_HEIGHT, 0), rect.w, rect.h);
	nvgPathWinding(geoGrid->vg, NVG_HOLE);
	nvgFillColor(geoGrid->vg, Theme_GetColor(THEME_BASE, 255, 1.30f));
	nvgFill(geoGrid->vg);
	
	rect.x += SPLIT_ELEM_X_PADDING;
	rect.y += 4;
	rect.w = SPLIT_BAR_HEIGHT - SPLIT_SPLIT_W - 8;
	rect.h = SPLIT_BAR_HEIGHT - SPLIT_SPLIT_W - 8;
	
	split->cect = split->rect;
	split->cect.h = rect.y;
	
	hdrbtn->txt = table[id].taskName;
	hdrbtn->rect = rect;
	hdrbtn->autoWidth = true;
	
	if (hdrbtn->txt == NULL) {
		static char* txt =  "Empty";
		hdrbtn->txt = txt;
	}
	
	if (Element_Button(geoGrid, split, hdrbtn)) {
		static char* optionList[64] = { NULL };
		
		for (s32 i = 0; i < geoGrid->taskTableNum; i++) {
			optionList[i] = geoGrid->taskTable[i].taskName;
		}
		
		GeoGrid_SetContextMenu(geoGrid, split, optionList, geoGrid->taskTableNum);
	}
	Element_PushToPost();
	
	if ((menuSel = GeoGrid_GetContextMenuResult(geoGrid, split))) {
		split->id = menuSel - 1;
	}
}

void GeoGrid_Draw_SplitBorder(GeoGrid* geoGrid, Split* split) {
	void* vg = geoGrid->vg;
	Rect* rect = &split->rect;
	Rectf32 adjRect = {
		0 + SPLIT_SPLIT_W,
		0 + SPLIT_SPLIT_W,
		rect->w - SPLIT_SPLIT_W * 2,
		rect->h - SPLIT_SPLIT_W * 2
	};
	
	if (split->id > 0) {
		u32 id = split->id;
		SplitTask* table = geoGrid->taskTable;
		
		nvgBeginPath(vg);
		nvgRect(vg, 0, 0, rect->w, rect->h);
		if (split->bg.useCustomBG == true) {
			nvgFillColor(vg, nvgRGBA(split->bg.color.r, split->bg.color.g, split->bg.color.b, 255));
		} else {
			nvgFillColor(vg, Theme_GetColor(THEME_BASE_SPLIT, 255, 1.0f));
		}
		nvgFill(vg);
		
		nvgEndFrame(geoGrid->vg);
		nvgBeginFrame(geoGrid->vg, split->rect.w, split->rect.h, gPixelRatio); {
			table[id].draw(geoGrid->passArg, split->instance, split);
			Element_Draw(geoGrid, split);
		} nvgEndFrame(geoGrid->vg);
		nvgBeginFrame(geoGrid->vg, split->rect.w, split->rect.h, gPixelRatio);
	} else {
		nvgBeginPath(vg);
		nvgRect(vg, 0, 0, rect->w, rect->h);
		nvgFillColor(vg, Theme_GetColor(THEME_BASE_SPLIT, 255, 1.0f));
		nvgFill(vg);
	}
	
	GeoGrid_Draw_SplitHeader(geoGrid, split);
	
	Element_PostDraw(geoGrid, split);
	
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
	
	nvgBeginPath(geoGrid->vg);
	nvgRect(geoGrid->vg, 0, 0, rect->w, rect->h);
	nvgRoundedRect(
		geoGrid->vg,
		adjRect.x,
		adjRect.y,
		adjRect.w,
		adjRect.h,
		SPLIT_ROUND_R
	);
	nvgPathWinding(geoGrid->vg, NVG_HOLE);
	nvgFillColor(geoGrid->vg, Theme_GetColor(THEME_BASE, 255, 0.66f));
	nvgFill(geoGrid->vg);
}

void GeoGrid_Draw_Splits(GeoGrid* geoGrid) {
	Split* split = geoGrid->splitHead;
	Vec2s* winDim = geoGrid->winDim;
	
	for (; split != NULL; split = split->next) {
		GeoGrid_Update_SplitRect(split);
		
		glViewport(
			split->rect.x,
			winDim->y - split->rect.y - split->rect.h,
			split->rect.w,
			split->rect.h
		);
		nvgBeginFrame(geoGrid->vg, split->rect.w, split->rect.h, gPixelRatio); {
			GeoGrid_Draw_SplitBorder(geoGrid, split);
		} nvgEndFrame(geoGrid->vg);
	}
}

/* ───────────────────────────────────────────────────────────────────────── */

Rect sCtxMenuRect;

void CtxMenu_UpdateRect(GeoGrid* geoGrid) {
	GeoCtxMenu* ctxMenu = &geoGrid->ctxMenu;
	Rectf32 boundary = { 0 };
	void* vg = geoGrid->vg;
	
	sCtxMenuRect = (Rect) {
		ctxMenu->pos.x,
		ctxMenu->pos.y,
		0,
		0
	};
	
	sCtxMenuRect.w = 0;
	for (s32 i = 0; i < ctxMenu->num; i++) {
		if (ctxMenu->optionList[i] != NULL) {
			nvgFontFace(vg, "dejavu");
			nvgFontSize(vg, SPLIT_TEXT);
			nvgFontBlur(vg, 0);
			nvgTextLetterSpacing(vg, 1.10);
			nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
			nvgTextBounds(vg, 0, 0, ctxMenu->optionList[i], NULL, (f32*)&boundary);
			sCtxMenuRect.w = fmax(sCtxMenuRect.w, boundary.w + SPLIT_TEXT_PADDING * 4);
		}
	}
	
	sCtxMenuRect.h = 0;
	for (s32 i = 0; i < ctxMenu->num; i++) {
		if (ctxMenu->optionList[i] != NULL) {
			sCtxMenuRect.h += SPLIT_TEXT_PADDING + SPLIT_TEXT_PADDING + SPLIT_TEXT;
		} else {
			sCtxMenuRect.h += SPLIT_TEXT_PADDING * 1.5;
		}
	}
	
	if (sCtxMenuRect.y > geoGrid->winDim->y * 0.5)
		sCtxMenuRect.y -= sCtxMenuRect.h;
}

void GeoGrid_Update_ContextMenu(GeoGrid* geoGrid) {
	GeoCtxMenu* ctxMenu = &geoGrid->ctxMenu;
	
	if (ctxMenu->num == 0)
		return;
	
	ctxMenu->hoverNum = -1;
	
	CtxMenu_UpdateRect(geoGrid);
	
	sCtxMenuRect.h = 0;
	
	for (s32 i = 0; i < ctxMenu->num; i++) {
		if (ctxMenu->optionList[i] != NULL) {
			Rect pressRect;
			
			sCtxMenuRect.h += SPLIT_TEXT_PADDING;
			
			pressRect.x = sCtxMenuRect.x;
			pressRect.y = sCtxMenuRect.y + sCtxMenuRect.h - SPLIT_TEXT_PADDING;
			pressRect.w = sCtxMenuRect.w;
			pressRect.h = SPLIT_TEXT + SPLIT_TEXT_PADDING * 2;
			
			if (geoGrid->input->mouse.pos.x >= pressRect.x && geoGrid->input->mouse.pos.x < pressRect.x + pressRect.w) {
				if (geoGrid->input->mouse.pos.y >= pressRect.y && geoGrid->input->mouse.pos.y < pressRect.y + pressRect.h) {
					ctxMenu->hoverNum = i;
					ctxMenu->hoverRect = pressRect;
					if (Input_GetMouse(MOUSE_L)->press) {
						ctxMenu->num = -(i + 1);
						
						return;
					}
				}
			}
			
			sCtxMenuRect.h += SPLIT_TEXT;
			sCtxMenuRect.h += SPLIT_TEXT_PADDING;
		} else {
			sCtxMenuRect.h += SPLIT_TEXT_PADDING * 1.5;
		}
	}
	
	if (geoGrid->input->mouse.pos.x < ctxMenu->pos.x - SPLIT_CTXM_DIST ||
		geoGrid->input->mouse.pos.x > ctxMenu->pos.x + sCtxMenuRect.w + SPLIT_CTXM_DIST ||
		geoGrid->input->mouse.pos.y < sCtxMenuRect.y - SPLIT_CTXM_DIST ||
		geoGrid->input->mouse.pos.y > sCtxMenuRect.y + sCtxMenuRect.h + SPLIT_CTXM_DIST
	) {
		ctxMenu->num = 0;
		ctxMenu->split = NULL;
	}
	
	if (geoGrid->input->mouse.pos.x > ctxMenu->pos.x ||
		geoGrid->input->mouse.pos.x < ctxMenu->pos.x + sCtxMenuRect.w ||
		geoGrid->input->mouse.pos.y > sCtxMenuRect.y ||
		geoGrid->input->mouse.pos.y < sCtxMenuRect.y + sCtxMenuRect.h
	) {
		if (Input_GetMouse(MOUSE_ANY)->press) {
			ctxMenu->num = 0;
			ctxMenu->split = NULL;
			Input_GetMouse(MOUSE_ANY)->press = 0;
			Input_GetMouse(MOUSE_L)->press = 0;
			Input_GetMouse(MOUSE_R)->press = 0;
			Input_GetMouse(MOUSE_M)->press = 0;
		}
	}
}

void GeoGrid_Draw_ContextMenu(GeoGrid* geoGrid) {
	GeoCtxMenu* ctxMenu = &geoGrid->ctxMenu;
	void* vg = geoGrid->vg;
	
	CtxMenu_UpdateRect(geoGrid);
	
	if (ctxMenu->num > 0) {
		glViewport(0, 0, geoGrid->winDim->x, geoGrid->winDim->y);
		nvgBeginFrame(vg, geoGrid->winDim->x, geoGrid->winDim->y, gPixelRatio); {
			nvgBeginPath(vg);
			nvgFillColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 255, 1.0f));
			nvgRoundedRect(vg, sCtxMenuRect.x - 0.5, sCtxMenuRect.y - 0.5, sCtxMenuRect.w + 1.0, sCtxMenuRect.h + 1.0, SPLIT_ROUND_R);
			nvgFill(vg);
			
			nvgBeginPath(vg);
			nvgFillColor(vg, Theme_GetColor(THEME_BASE, 255, 0.75f));
			nvgRoundedRect(vg, sCtxMenuRect.x, sCtxMenuRect.y, sCtxMenuRect.w, sCtxMenuRect.h, SPLIT_ROUND_R);
			nvgFill(vg);
			
			if (ctxMenu->hoverNum != -1) {
				nvgBeginPath(vg);
				nvgFillColor(vg, Theme_GetColor(THEME_BASE_SPLIT, 255, 1.0f));
				nvgRoundedRect(
					vg,
					ctxMenu->hoverRect.x,
					ctxMenu->hoverRect.y,
					ctxMenu->hoverRect.w,
					ctxMenu->hoverRect.h,
					SPLIT_ROUND_R
				);
				nvgFill(vg);
			}
			
			sCtxMenuRect.y = ctxMenu->pos.y;
			sCtxMenuRect.x += SPLIT_TEXT_PADDING * 2;
			
			if (sCtxMenuRect.y > geoGrid->winDim->y * 0.5) {
				sCtxMenuRect.y -= sCtxMenuRect.h;
			}
			
			sCtxMenuRect.y += SPLIT_TEXT_PADDING * 0.25;
			
			for (s32 i = 0; i < ctxMenu->num; i++) {
				if (ctxMenu->optionList[i] != NULL) {
					nvgFontFace(vg, "dejavu");
					nvgFontSize(vg, SPLIT_TEXT);
					nvgFontBlur(vg, 0);
					nvgTextLetterSpacing(vg, 1.10);
					nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
					nvgFillColor(geoGrid->vg, Theme_GetColor(THEME_TEXT, 255, 1.0f));
					nvgText(vg, sCtxMenuRect.x, sCtxMenuRect.y + SPLIT_TEXT_PADDING, ctxMenu->optionList[i], NULL);
					sCtxMenuRect.y += SPLIT_TEXT + SPLIT_TEXT_PADDING + SPLIT_TEXT_PADDING;
				} else {
					nvgBeginPath(vg);
					nvgMoveTo(vg, ctxMenu->pos.x + SPLIT_TEXT_PADDING, sCtxMenuRect.y + SPLIT_TEXT_PADDING * 0.5);
					nvgLineTo(vg, ctxMenu->pos.x + sCtxMenuRect.w - SPLIT_TEXT_PADDING, sCtxMenuRect.y + SPLIT_TEXT_PADDING * 0.5);
					nvgStrokeWidth(vg, 0.75f);
					nvgStrokeColor(vg, Theme_GetColor(THEME_TEXT, 255, 1.0f));
					nvgStroke(vg);
					sCtxMenuRect.y += SPLIT_TEXT_PADDING;
				}
			}
		} nvgEndFrame(vg);
	}
}

/* ───────────────────────────────────────────────────────────────────────── */

void GeoGrid_RemoveDublicates(GeoGrid* geoGrid) {
	SplitVtx* vtx = geoGrid->vtxHead;
	SplitEdge* edge = geoGrid->edgeHead;
	
	while (vtx) {
		GeoGrid_Update_Vtx_RemoveDublicates(geoGrid, vtx);
		vtx = vtx->next;
	}
	
	while (edge) {
		GeoGrid_Update_Edge_RemoveDublicates(geoGrid, edge);
		edge = edge->next;
	}
}

void GeoGrid_UpdateWorkRect(GeoGrid* geoGrid) {
	Vec2s* winDim = geoGrid->winDim;
	
	geoGrid->workRect = (Rect) { 0, 0 + geoGrid->bar[BAR_TOP].rect.h, winDim->x,
				     winDim->y - geoGrid->bar[BAR_BOT].rect.h -
				     geoGrid->bar[BAR_TOP].rect.h };
}

void GeoGrid_SetTopBarHeight(GeoGrid* geoGrid, s32 h) {
	geoGrid->bar[BAR_TOP].rect.x = 0;
	geoGrid->bar[BAR_TOP].rect.y = 0;
	geoGrid->bar[BAR_TOP].rect.w = geoGrid->winDim->x;
	geoGrid->bar[BAR_TOP].rect.h = h;
	GeoGrid_UpdateWorkRect(geoGrid);
}

void GeoGrid_SetBotBarHeight(GeoGrid* geoGrid, s32 h) {
	geoGrid->bar[BAR_BOT].rect.x = 0;
	geoGrid->bar[BAR_BOT].rect.y = geoGrid->winDim->y - h;
	geoGrid->bar[BAR_BOT].rect.w = geoGrid->winDim->x;
	geoGrid->bar[BAR_BOT].rect.h = h;
	GeoGrid_UpdateWorkRect(geoGrid);
}

/* ───────────────────────────────────────────────────────────────────────── */

extern unsigned char gFont_DejaVuBold[705684];
extern unsigned char gFont_DejaVuLight[355380];
extern unsigned char gFont_DejaVu[757076];

void GeoGrid_Init(GeoGrid* geoGrid, Vec2s* winDim, InputContext* inputCtx, void* vg) {
	__geoCtx = geoGrid;
	geoGrid->winDim = winDim;
	geoGrid->input = inputCtx;
	geoGrid->vg = vg;
	GeoGrid_SetTopBarHeight(geoGrid, SPLIT_BAR_HEIGHT);
	GeoGrid_SetBotBarHeight(geoGrid, SPLIT_BAR_HEIGHT);
	
	nvgCreateFontMem(vg, "dejavu", gFont_DejaVu, sizeof(gFont_DejaVu), 0);
	nvgCreateFontMem(vg, "dejavu-bold", gFont_DejaVu, sizeof(gFont_DejaVu), 0);
	nvgCreateFontMem(vg, "dejavu-light", gFont_DejaVu, sizeof(gFont_DejaVu), 0);
	
	geoGrid->prevWorkRect = geoGrid->workRect;
	Element_Init(geoGrid);
}

void GeoGrid_Update(GeoGrid* geoGrid) {
	GeoGrid_SetTopBarHeight(geoGrid, geoGrid->bar[BAR_TOP].rect.h);
	GeoGrid_SetBotBarHeight(geoGrid, geoGrid->bar[BAR_BOT].rect.h);
	Element_Update(geoGrid);
	GeoGrid_Update_ContextMenu(geoGrid);
	GeoGrid_Update_Vtx(geoGrid);
	GeoGrid_Update_Edges(geoGrid);
	GeoGrid_Update_Split(geoGrid);
	
	geoGrid->prevWorkRect = geoGrid->workRect;
}

void GeoGrid_Draw(GeoGrid* geoGrid) {
	Vec2s* winDim = geoGrid->winDim;
	
	// Draw Bars
	for (s32 i = 0; i < 2; i++) {
		glViewport(
			geoGrid->bar[i].rect.x,
			winDim->y - geoGrid->bar[i].rect.y - geoGrid->bar[i].rect.h,
			geoGrid->bar[i].rect.w,
			geoGrid->bar[i].rect.h
		);
		nvgBeginFrame(geoGrid->vg, geoGrid->bar[i].rect.w, geoGrid->bar[i].rect.h, gPixelRatio); {
			nvgBeginPath(geoGrid->vg);
			nvgRect(
				geoGrid->vg,
				0,
				0,
				geoGrid->bar[i].rect.w,
				geoGrid->bar[i].rect.h
			);
			nvgFillColor(geoGrid->vg, Theme_GetColor(THEME_BASE_L1, 255, 0.825f));
			nvgFill(geoGrid->vg);
		} nvgEndFrame(geoGrid->vg);
	}
	
	GeoGrid_Draw_Splits(geoGrid);
	if (0)
		GeoGrid_Draw_Debug(geoGrid);
	GeoGrid_Draw_ContextMenu(geoGrid);
}