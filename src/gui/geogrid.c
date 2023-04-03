#include "ext_geogrid.h"
#include "ext_interface.h"

f32 gPixelRatio = 1.0f;

static void GeoGrid_RemoveDuplicates(GeoGrid* geo);
static void Split_UpdateRect(Split* split);
void Element_SetContext(GeoGrid* setGeo, Split* setSplit);
void DragItem_Draw(GeoGrid* geo);

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

static SplitDir GeoGrid_GetDir_Opposite(SplitDir dir) {
    return wrapi(dir + 2, DIR_L, DIR_B + 1);
}

static SplitDir GeoGrid_GetDir_MouseToPressPos(Split* split) {
    Vec2s pos = Math_Vec2s_Sub(split->cursorPos, split->mousePressPos);
    
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

static SplitVtx* GeoGrid_AddVtx(GeoGrid* geo, f64 x, f64 y) {
    SplitVtx* head = geo->vtxHead;
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
    Node_Add(geo->vtxHead, vtx);
    
    return vtx;
}

static SplitEdge* GeoGrid_AddEdge(GeoGrid* geo, SplitVtx* v1, SplitVtx* v2) {
    SplitEdge* head = geo->edgeHead;
    SplitEdge* edge = NULL;
    
    _assert(v1 != NULL && v2 != NULL);
    
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
    
    return Math_Vec2s_DistXZ(cursor[i], pos[i]);
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

static void Split_SetupTaskEnum(GeoGrid* geo, Split* this) {
    this->taskList = new(Arli);
    *this->taskList = Arli_New(void*);
    this->taskCombo = new(ElCombo);
    
    _assert(this->taskList != NULL);
    _assert(this->taskCombo != NULL);
    
    for (int i = 0; i < geo->numTaskTable; i++)
        Arli_Add(this->taskList, &geo->taskTable[i]->taskName);
    
    Arli_Set(this->taskList, this->id);
    Arli_SetElemNameCallback(this->taskList, Split_TaskListElemName);
    
    Element_Combo_SetArli(this->taskCombo, this->taskList);
}

static Split* Split_Alloc(GeoGrid* geo, s32 id) {
    Split* split;
    
    split = new(Split);
    split->prevId = -1; // Forces init
    split->id = id;
    
    Split_SetupTaskEnum(geo, split);
    
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
        _assert(!(tempEdge->state & EDGE_VERTICAL && tempEdge->state & EDGE_HORIZONTAL));
        
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
    _assert(geo->actionSplit);
    geo->actionSplit->state &= ~(SPLIT_POINTS | SPLIT_SIDES);
    
    geo->actionSplit = NULL;
}

static void Split_Split(GeoGrid* geo, Split* split, SplitDir dir) {
    Split* newSplit;
    f64 splitPos = (dir == DIR_L || dir == DIR_R) ? geo->input->cursor.pos.x : geo->input->cursor.pos.y;
    
    newSplit = Split_Alloc(geo, split->id);
    Node_Add(geo->splitHead, newSplit);
    
    if (dir == DIR_L) {
        _log("SplitTo DIR_L");
        newSplit->vtx[0] = GeoGrid_AddVtx(geo, splitPos, split->vtx[0]->pos.y);
        newSplit->vtx[1] = GeoGrid_AddVtx(geo, splitPos, split->vtx[1]->pos.y);
        newSplit->vtx[2] =  GeoGrid_AddVtx(geo, split->vtx[2]->pos.x, split->vtx[2]->pos.y);
        newSplit->vtx[3] = GeoGrid_AddVtx(geo, split->vtx[3]->pos.x, split->vtx[3]->pos.y);
        split->vtx[2] = GeoGrid_AddVtx(geo, splitPos, split->vtx[2]->pos.y);
        split->vtx[3] = GeoGrid_AddVtx(geo, splitPos, split->vtx[3]->pos.y);
    }
    
    if (dir == DIR_R) {
        _log("SplitTo DIR_R");
        newSplit->vtx[0] = GeoGrid_AddVtx(geo, split->vtx[0]->pos.x, split->vtx[0]->pos.y);
        newSplit->vtx[1] = GeoGrid_AddVtx(geo, split->vtx[1]->pos.x, split->vtx[1]->pos.y);
        newSplit->vtx[2] = GeoGrid_AddVtx(geo, splitPos, split->vtx[2]->pos.y);
        newSplit->vtx[3] = GeoGrid_AddVtx(geo, splitPos, split->vtx[3]->pos.y);
        split->vtx[0] = GeoGrid_AddVtx(geo, splitPos, split->vtx[0]->pos.y);
        split->vtx[1] = GeoGrid_AddVtx(geo, splitPos, split->vtx[1]->pos.y);
    }
    
    if (dir == DIR_T) {
        _log("SplitTo DIR_T");
        newSplit->vtx[0] = GeoGrid_AddVtx(geo, split->vtx[0]->pos.x, split->vtx[0]->pos.y);
        newSplit->vtx[1] = GeoGrid_AddVtx(geo, split->vtx[1]->pos.x, splitPos);
        newSplit->vtx[2] = GeoGrid_AddVtx(geo, split->vtx[2]->pos.x, splitPos);
        newSplit->vtx[3] = GeoGrid_AddVtx(geo, split->vtx[3]->pos.x, split->vtx[3]->pos.y);
        split->vtx[3] = GeoGrid_AddVtx(geo, split->vtx[3]->pos.x, splitPos);
        split->vtx[0] = GeoGrid_AddVtx(geo, split->vtx[0]->pos.x, splitPos);
    }
    
    if (dir == DIR_B) {
        _log("SplitTo DIR_B");
        newSplit->vtx[0] = GeoGrid_AddVtx(geo, split->vtx[0]->pos.x, splitPos);
        newSplit->vtx[1] = GeoGrid_AddVtx(geo, split->vtx[1]->pos.x, split->vtx[1]->pos.y);
        newSplit->vtx[2] = GeoGrid_AddVtx(geo, split->vtx[2]->pos.x, split->vtx[2]->pos.y);
        newSplit->vtx[3] = GeoGrid_AddVtx(geo, split->vtx[3]->pos.x, splitPos);
        split->vtx[1] = GeoGrid_AddVtx(geo, split->vtx[1]->pos.x, splitPos);
        split->vtx[2] = GeoGrid_AddVtx(geo, split->vtx[2]->pos.x, splitPos);
    }
    
    _log("Form Edges A");
    split->edge[EDGE_L] = GeoGrid_AddEdge(geo, split->vtx[VTX_BOT_L], split->vtx[VTX_TOP_L]);
    split->edge[EDGE_T] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
    split->edge[EDGE_R] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
    split->edge[EDGE_B] = GeoGrid_AddEdge(geo, split->vtx[VTX_BOT_R], split->vtx[VTX_BOT_L]);
    
    _log("Form Edges B");
    newSplit->edge[EDGE_L] = GeoGrid_AddEdge(geo, newSplit->vtx[VTX_BOT_L], newSplit->vtx[VTX_TOP_L]);
    newSplit->edge[EDGE_T] = GeoGrid_AddEdge(geo, newSplit->vtx[VTX_TOP_L], newSplit->vtx[VTX_TOP_R]);
    newSplit->edge[EDGE_R] = GeoGrid_AddEdge(geo, newSplit->vtx[VTX_TOP_R], newSplit->vtx[VTX_BOT_R]);
    newSplit->edge[EDGE_B] = GeoGrid_AddEdge(geo, newSplit->vtx[VTX_BOT_R], newSplit->vtx[VTX_BOT_L]);
    
    _log("Clean");
    geo->actionEdge = newSplit->edge[dir];
    GeoGrid_RemoveDuplicates(geo);
    Edge_SetSlideClamp(geo);
#if 0
    // @notice: Possibly not needed but might be something required
    Split_UpdateRect(split);
    Split_UpdateRect(newSplit);
#endif
    _log("Done");
}

static void Split_Free(Split* this) {
    info("Kill Split: %s", addr_name(this));
    Arli_Free(this->taskList);
    vfree(this->taskList, this->taskCombo, this->instance);
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
        _log("Kill DIR_L");
        split->vtx[VTX_BOT_L] = killSplit->vtx[VTX_BOT_L];
        split->vtx[VTX_TOP_L] = killSplit->vtx[VTX_TOP_L];
        split->edge[EDGE_T] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
        split->edge[EDGE_B] = GeoGrid_AddEdge(geo, split->vtx[VTX_BOT_L], split->vtx[VTX_BOT_R]);
    }
    
    if (dir == DIR_T) {
        _log("Kill DIR_T");
        split->vtx[VTX_TOP_L] = killSplit->vtx[VTX_TOP_L];
        split->vtx[VTX_TOP_R] = killSplit->vtx[VTX_TOP_R];
        split->edge[EDGE_L] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_L], split->vtx[VTX_BOT_L]);
        split->edge[EDGE_R] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
    }
    
    if (dir == DIR_R) {
        _log("Kill DIR_R");
        split->vtx[VTX_BOT_R] = killSplit->vtx[VTX_BOT_R];
        split->vtx[VTX_TOP_R] = killSplit->vtx[VTX_TOP_R];
        split->edge[EDGE_T] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
        split->edge[EDGE_B] = GeoGrid_AddEdge(geo, split->vtx[VTX_BOT_L], split->vtx[VTX_BOT_R]);
    }
    
    if (dir == DIR_B) {
        _log("Kill DIR_B");
        split->vtx[VTX_BOT_L] = killSplit->vtx[VTX_BOT_L];
        split->vtx[VTX_BOT_R] = killSplit->vtx[VTX_BOT_R];
        split->edge[EDGE_L] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_L], split->vtx[VTX_BOT_L]);
        split->edge[EDGE_R] = GeoGrid_AddEdge(geo, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
    }
    
    split->edge[dir] = killSplit->edge[dir];
    if (geo->taskTable[killSplit->id]->destroy)
        geo->taskTable[killSplit->id]->destroy(geo->passArg, killSplit->instance, killSplit);
    
    Split_Free(killSplit);
    Node_Kill(geo->splitHead, killSplit);
    GeoGrid_RemoveDuplicates(geo);
#if 0
    // @notice: Possibly not needed but might be something required
    Split_UpdateRect(split);
#endif
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
            Node_Kill(geo->vtxHead, kill);
            continue;
        }
        
        vtx2 = vtx2->next;
    }
}

static void Vtx_Update(GeoGrid* geo) {
    SplitVtx* vtx = geo->vtxHead;
    
    if (geo->actionEdge != NULL)
        geo->state.cleanVtx = true;
    
    while (vtx) {
        if (geo->state.cleanVtx == true && geo->actionEdge == NULL)
            Vtx_RemoveDuplicates(geo, vtx);
        
        if (vtx->killFlag == true) {
            Split* s = geo->splitHead;
            
            while (s) {
                for (int i = 0; i < 4; i++)
                    if (s->vtx[i] == vtx)
                        vtx->killFlag = false;
                
                s = s->next;
            }
            
            if (vtx->killFlag == true) {
                SplitVtx* killVtx = vtx;
                vtx = geo->vtxHead;
                info("Kill Vtx: %s", addr_name(vtx));
                Node_Kill(geo->vtxHead, killVtx);
                continue;
            }
        }
        
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
                for (int i = 0; i < 4; i++)
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
        
        _assert(!(edge->state & EDGE_HORIZONTAL && edge->state & EDGE_VERTICAL));
        
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
            s32 align = wrapi(edge->state & EDGE_ALIGN, 0, 2);
            
            while (temp) {
                for (int i = 0; i < 2; i++) {
                    if (((temp->state & EDGE_ALIGN) != (edge->state & EDGE_ALIGN)) && temp->vtx[1] == edge->vtx[i]) {
                        if (temp->vtx[0]->pos.axis[align] > geo->slide.clampMin) {
                            geo->slide.clampMin = temp->vtx[0]->pos.axis[align];
                        }
                    }
                }
                
                for (int i = 0; i < 2; i++) {
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
                    edge->pos = geo->input->cursor.pos.y;
                }
                if (edge->state & EDGE_VERTICAL) {
                    edge->pos = geo->input->cursor.pos.x;
                }
                edge->pos = clamp_min(edge->pos, geo->slide.clampMin + SPLIT_CLAMP);
                edge->pos = clamp_max(edge->pos, geo->slide.clampMax - SPLIT_CLAMP);
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
    
    if (!Input_GetMouse(geo->input, CLICK_ANY)->hold)
        geo->actionEdge = NULL;
    
    while (edge) {
        SplitEdge* next = edge->next;
        
        if (edge->killFlag == true) {
            info("Kill Edge: %s", addr_name(edge));
            Node_Kill(geo->edgeHead, edge);
            
        } else {
            edge->killFlag = true;
            
            if (geo->actionEdge == NULL) {
                edge->state &= ~EDGE_EDIT;
            }
            
            Edge_RemoveDuplicates(geo, edge);
        }
        
        edge = next;
    }
    
    Edge_SetSlide(geo);
}

/* ───────────────────────────────────────────────────────────────────────── */

static void Split_UpdateActionSplit(GeoGrid* geo) {
    Split* split = geo->actionSplit;
    
    if (Input_GetMouse(geo->input, CLICK_ANY)->press) {
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
            
            _assert(split->edge[i] != NULL);
            
            geo->actionEdge = split->edge[i];
            Edge_SetSlideClamp(geo);
            Split_ClearActionSplit(geo);
        }
    }
    
    if (Input_GetMouse(geo->input, CLICK_ANY)->hold) {
        if (split->state & SPLIT_POINTS) {
            s32 dist = Math_Vec2s_DistXZ(split->cursorPos, split->mousePressPos);
            SplitDir dir = GeoGrid_GetDir_MouseToPressPos(split);
            
            if (dist > 1) {
                CursorIndex cid = dir + 1;
                Cursor_SetCursor(cid);
            }
            
            if (dist > SPLIT_CLAMP * 1.05) {
                _log("Point Action");
                Split_ClearActionSplit(geo);
                
                if (split->mouseInDispRect)
                    Split_Split(geo, split, dir);
                
                else
                    Split_Kill(geo, split, dir);
            } else {
                if (!split->mouseInDispRect) {
                    _log("Display Kill Arrow");
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
    
    split->rect = split->dispRect;
    split->rect.h -= SPLIT_ELEM_Y_PADDING + 6;
    
    split->headRect.x = split->rect.x;
    split->headRect.y = split->rect.y + split->rect.h;
    split->headRect.h = split->dispRect.h - split->rect.h;
    split->headRect.w = split->rect.w;
}

static void Split_SwapInstance(GeoGrid* geo, Split* split) {
    SplitScrollBar* scroll = &split->scroll;
    
    if (split->instance) {
        _log("" PRNT_REDD "SplitSwapInstance" PRNT_RSET "( %d -> %d )", split->prevId, split->id);
        if (geo->taskTable[split->prevId]->destroy)
            geo->taskTable[split->prevId]->destroy(geo->passArg, split->instance, split);
        scroll->offset = scroll->enabled = 0;
        vfree(split->instance);
    }
    
    split->prevId = split->id;
    split->instance = calloc(geo->taskTable[split->id]->size);
    if (geo->taskTable[split->id]->init)
        geo->taskTable[split->id]->init(geo->passArg, split->instance, split);
}

static void Split_UpdateScroll(GeoGrid* geo, Split* split, Input* input) {
    SplitScrollBar* scroll = &split->scroll;
    
    if (geo->state.blockElemInput)
        return;
    
    if (split->scroll.enabled) {
        if (split->mouseInSplit && !split->splitBlockScroll) {
            s32 val = Input_GetScroll(input);
            
            scroll->offset = scroll->offset + SPLIT_ELEM_Y_PADDING * -val;
        }
        
        scroll->offset = clamp(scroll->offset, 0, scroll->max);
    } else
        split->scroll.offset = 0;
    split->splitBlockScroll = 0;
}

static void Split_UpdateSplit(GeoGrid* geo, Split* split) {
    Cursor* cursor = &geo->input->cursor;
    
    _log("Split %s", addr_name(split));
    if (!split->isHeader)
        Split_UpdateRect(split);
    
    Vec2s rectPos = { split->rect.x, split->rect.y };
    SplitTask** table = geo->taskTable;
    
    split->cursorPos = Math_Vec2s_Sub(cursor->pos, rectPos);
    split->mouseInSplit = Rect_PointIntersect(&split->rect, cursor->pos.x, cursor->pos.y);
    split->mouseInDispRect = Rect_PointIntersect(&split->dispRect, cursor->pos.x, cursor->pos.y);
    split->mouseInHeader = Rect_PointIntersect(&split->headRect, cursor->pos.x, cursor->pos.y);
    split->blockMouse = false;
    Split_UpdateScroll(geo, split, geo->input);
    
    if (Input_GetMouse(geo->input, CLICK_ANY)->press)
        split->mousePressPos = split->cursorPos;
    
    if (!geo->state.blockSplitting && !split->isHeader) {
        if (split->mouseInDispRect) {
            SplitState splitRangeState = Split_GetCursorPosState(split, SPLIT_GRAB_DIST * 3) & SPLIT_POINTS;
            SplitState slideRangeState = Split_GetCursorPosState(split, SPLIT_GRAB_DIST) & SPLIT_SIDES;
            
            if (splitRangeState)
                Cursor_SetCursor(CURSOR_CROSSHAIR);
            
            else if (slideRangeState & SPLIT_SIDE_H)
                Cursor_SetCursor(CURSOR_ARROW_H);
            
            else if (slideRangeState & SPLIT_SIDE_V)
                Cursor_SetCursor(CURSOR_ARROW_V);
            
            split->blockMouse = splitRangeState + slideRangeState;
        }
        
        if (geo->actionSplit == NULL && split->mouseInDispRect && cursor->cursorAction) {
            if (cursor->clickAny.press)
                geo->actionSplit = split;
        }
        
        if (geo->actionSplit != NULL && geo->actionSplit == split)
            Split_UpdateActionSplit(geo);
    }
    
    if (split->state != 0)
        split->blockMouse = true;
    
    split->inputAccess = split->mouseInSplit && !geo->state.blockElemInput && !split->blockMouse;
    
    Element_SetContext(geo, split);
    Element_RowY(SPLIT_ELEM_X_PADDING * 2);
    Element_ShiftX(0);
    
    if (!split->isHeader) {
        split->id = split->taskList->cur;
        
        if (split->id != split->prevId)
            Split_SwapInstance(geo, split);
        
        _log("SplitUpdate( %d %s )", split->id, geo->taskTable[split->id]->taskName);
        _assert(table[split->id]->update != NULL);
        table[split->id]->update(geo->passArg, split->instance, split);
        
        if (Element_Box(BOX_GET_NUM) != 0)
            errr("" PRNT_YELW "Element_Box Overflow" PRNT_RSET "( %d %s )", split->id, table[split->id]->taskName);
        
        for (int i = 0; i < 4; i++) {
            _assert(split->edge[i] != NULL);
            split->edge[i]->killFlag = false;
        }
    } else if (split->headerFunc)
        split->headerFunc(geo->passArg, split->instance, split);
    _log("OK");
}

static void Split_Update(GeoGrid* geo) {
    Split* split = geo->splitHead;
    Cursor* cursor = &geo->input->cursor;
    
    Cursor_SetCursor(CURSOR_DEFAULT);
    
    if (geo->actionSplit != NULL && cursor->cursorAction == false)
        Split_ClearActionSplit(geo);
    
    for (; split != NULL; split = split->next)
        Split_UpdateSplit(geo, split);
    
    Split_UpdateSplit(geo, &geo->bar[0]);
    Split_UpdateSplit(geo, &geo->bar[1]);
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
            Math_Vec2f_New(this->rect.w * 0.5, this->rect.h * 0.5),
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

static void Split_DrawSplit(GeoGrid* geo, Split* split) {
    Element_SetContext(geo, split);
    
    _assert(split != NULL);
    if (geo->taskTable && geo->numTaskTable > split->id && geo->taskTable[split->id])
        _log("SplitDraw( %d %s )", split->id, geo->taskTable[split->id]->taskName);
    else
        _log("SplitDraw( %d )", split->id);
    
    if (split->isHeader)
        goto draw_header;
    
    Split_UpdateRect(split);
    
    void* vg = geo->vg;
    Rect r = split->dispRect;
    u32 id = split->id;
    SplitTask** table = geo->taskTable;
    
    // Draw Background
    glViewportRect(UnfoldRect(split->rect));
    nvgBeginFrame(geo->vg, split->rect.w, split->rect.h, gPixelRatio); {
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
    } nvgEndFrame(geo->vg);
    
    // Draw Split
    nvgBeginFrame(geo->vg, split->rect.w, split->rect.h, gPixelRatio); {
        Math_SmoothStepToF(&split->scroll.voffset, split->scroll.offset, 0.25f, fabsf(split->scroll.offset - split->scroll.voffset) * 0.5f, 0.1f);
        if (table[id]->draw)
            table[id]->draw(geo->passArg, split->instance, split);
        Element_Draw(geo, split, false);
        Split_Draw_KillArrow(split, geo->vg);
    } nvgEndFrame(geo->vg);
    
    // Draw Overlays
    glViewportRect(UnfoldRect(split->dispRect));
    nvgBeginFrame(geo->vg, split->dispRect.w, split->dispRect.h, gPixelRatio); {
        r = split->dispRect;
        r.x = 4;
        r.y = 4;
        r.w -= 8;
        r.h -= 8;
        
        if (!(split->edge[EDGE_L]->state & EDGE_STICK_L)) {
            r.x -= 2;
            r.w += 2;
        }
        if (!(split->edge[EDGE_R]->state & EDGE_STICK_R)) {
            r.w += 2;
        }
        if (!(split->edge[EDGE_T]->state & EDGE_STICK_T)) {
            r.y -= 2;
            r.h += 2;
        }
        if (!(split->edge[EDGE_B]->state & EDGE_STICK_B)) {
            r.h += 2;
        }
        
        nvgBeginPath(geo->vg);
        nvgRect(geo->vg, -4, -4, split->dispRect.w + 8, split->dispRect.h + 8);
        nvgRoundedRect(
            geo->vg,
            UnfoldRect(r),
            SPLIT_ROUND_R * 2
        );
        nvgPathWinding(geo->vg, NVG_HOLE);
        
        nvgFillColor(geo->vg, Theme_GetColor(THEME_SPLIT, 255, 0.85f));
        nvgFill(geo->vg);
        
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
    
draw_header:
    glViewportRect(UnfoldRect(split->headRect));
    nvgBeginFrame(geo->vg, split->headRect.w, split->headRect.h, gPixelRatio); {
        Element_Draw(geo, split, true);
    } nvgEndFrame(geo->vg);
}

static void Split_Draw(GeoGrid* geo) {
    Split* split = geo->splitHead;
    
    for (; split != NULL; split = split->next)
        Split_DrawSplit(geo, split);
    
    Split_DrawSplit(geo, &geo->bar[0]);
    Split_DrawSplit(geo, &geo->bar[1]);
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
    geo->bar[BAR_TOP].headRect = geo->bar[BAR_TOP].rect;
    geo->bar[BAR_TOP].dispRect = geo->bar[BAR_TOP].rect;
    GeoGrid_UpdateWorkRect(geo);
}

static void GeoGrid_SetBotBarHeight(GeoGrid* geo, s32 h) {
    geo->bar[BAR_BOT].rect.x = 0;
    geo->bar[BAR_BOT].rect.y = geo->wdim->y - h;
    geo->bar[BAR_BOT].rect.w = geo->wdim->x;
    geo->bar[BAR_BOT].rect.h = h;
    geo->bar[BAR_BOT].headRect = geo->bar[BAR_BOT].rect;
    geo->bar[BAR_BOT].dispRect = geo->bar[BAR_BOT].rect;
    GeoGrid_UpdateWorkRect(geo);
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

Split* GeoGrid_AddSplit(GeoGrid* geo, Rectf32* rect, s32 id) {
    Split* split = Split_Alloc(geo, id);
    
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

s32 Split_GetCursor(GeoGrid* geo, Split* split, s32 result) {
    if (
        geo->state.blockElemInput ||
        !split->mouseInSplit ||
        split->blockMouse ||
        split->elemBlockMouse
    )
        return 0;
    
    return result;
}

void GeoGrid_Debug(bool b) {
    sDebugMode = b;
}

void GeoGrid_TaskTable(GeoGrid* geo, SplitTask** taskTable, u32 num) {
    _assert(num != 0);
    geo->taskTable = taskTable;
    geo->numTaskTable = num;
    
    for (int i = 0; i < num; i++)
        info("%d: %s", i, taskTable[i]->taskName);
}

extern void* ElementState_New(void);
extern void ElementState_SetElemState(void* elemState);
extern void Element_Flush();
extern void VectorGfx_InitCommon();

void GeoGrid_Init(GeoGrid* this, struct AppInfo* app, void* passArg) {
    this->vg = app->vg;
    this->passArg = passArg;
    
    _log("Prepare Headers");
    for (var i = 0; i < 2; i++)
        this->bar[i].isHeader = true;
    
    _log("Assign Info");
    this->wdim = &app->wdim;
    this->input = app->input;
    this->vg = app->vg;
    
    _log("Allocate ElemState");
    this->elemState = ElementState_New();
    
    GeoGrid_SetTopBarHeight(this, SPLIT_BAR_HEIGHT);
    GeoGrid_SetBotBarHeight(this, SPLIT_BAR_HEIGHT);
    VectorGfx_InitCommon();
    
    this->prevWorkRect = this->workRect;
}

void GeoGrid_Destroy(GeoGrid* this) {
    _log("Destroy Splits");
    while (this->splitHead) {
        _log("Split [%d]", this->splitHead->id);
        if (this->taskTable[this->splitHead->id]->destroy)
            this->taskTable[this->splitHead->id]->destroy(this->passArg, this->splitHead->instance, this->splitHead);
        Split_Free(this->splitHead);
        Node_Kill(this->splitHead, this->splitHead);
    }
    
    _log("vfree Vtx");
    while (this->vtxHead)
        Node_Kill(this->vtxHead, this->vtxHead);
    
    _log("vfree Edge");
    while (this->edgeHead)
        Node_Kill(this->edgeHead, this->edgeHead);
    
    _log("vfree ElemState");
    vfree(this->elemState);
}

void GeoGrid_Update(GeoGrid* geo) {
    GeoGrid_SetTopBarHeight(geo, geo->bar[BAR_TOP].rect.h);
    GeoGrid_SetBotBarHeight(geo, geo->bar[BAR_BOT].rect.h);
    
    Vtx_Update(geo);
    Edge_Update(geo);
    
    geo->prevWorkRect = geo->workRect;
}

void GeoGrid_Draw(GeoGrid* this) {
    ElementState_SetElemState(this->elemState);
    Element_Flush();
    
    Element_UpdateTextbox(this);
    Split_Update(this);
    
    // Draw Bars
    for (int i = 0; i < 2; i++) {
        glViewportRect(UnfoldRect(this->bar[i].rect));
        nvgBeginFrame(this->vg, this->bar[i].rect.w, this->bar[i].rect.h, gPixelRatio); {
            Rect r = this->bar[i].rect;
            
            r.x = r.y = 0;
            
            nvgBeginPath(this->vg);
            nvgRect(this->vg,
                UnfoldRect(r));
            nvgFillColor(this->vg, Theme_GetColor(THEME_SPLIT, 255, 1.0f));
            nvgFill(this->vg);
        } nvgEndFrame(this->vg);
    }
    
    Split_Draw(this);
    if (sDebugMode) Split_DrawDebug(this);
    
    glViewport(0, 0, this->wdim->x, this->wdim->y);
    ContextMenu_Draw(this);
    DragItem_Draw(this);
}

void DummySplit_Push(GeoGrid* geo, Split* split, Rect r) {
    split->dispRect = split->rect = r;
    split->mouseInSplit = true;
    split->cursorPos = geo->input->cursor.pos;
    split->mousePressPos = geo->input->cursor.pressPos;
    
    ElementState_SetElemState(geo->elemState);
    Element_SetContext(geo, split);
}

void DummySplit_Pop(GeoGrid* geo, Split* split) {
    Element_UpdateTextbox(geo);
    Element_Draw(geo, split, false);
}
