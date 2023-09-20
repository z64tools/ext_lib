#include "ext_interface.h"

int gScrollSpeedMod = 4;

void Input_Init(Input* input, Window* window) {
	input->window = window;
	window->input = input;
}

void Input_SetTempLock(Input* this, int frames) {
	this->tempLock = frames;
}

void Input_Update(Input* this) {
	Cursor* cursor = &this->cursor;
	f64 x, y;
	
	glfwGetCursorPos(this->window->glfw, &x, &y);
	cursor->vel.x = x - cursor->pos.x;
	cursor->vel.y = y - cursor->pos.y;
	cursor->pos.x = x;
	cursor->pos.y = y;
	
	Decr(this->tempLock);
	
	this->keyAction = false;
	
	for (int i = 0; i < KEY_MAX; i++) {
		this->key[i].dual = false;
		this->key[i].press = (this->key[i].prev == 0 && this->key[i].hold);
		this->key[i].release = (this->key[i].prev && this->key[i].hold == 0);
		this->key[i].prev = this->key[i].hold;
		
		if (this->key[i].hold) {
			this->keyAction = true;
			
			if (this->key[i].__timer) {
				this->key[i].__timer -= this->window->tick;
				
				if (!this->key[i].__timer) {
					this->key[i].dual = true;
					this->key[i].__timer = 4;
				}
			}
			
			if (this->key[i].press)
				this->key[i].__timer = 30;
		}
	}
	
	for (int i = 0; i < CLICK_ANY; i++) {
		cursor->clickList[i].press = (cursor->clickList[i].prev == 0 && cursor->clickList[i].hold);
		cursor->clickList[i].release = (cursor->clickList[i].prev && cursor->clickList[i].hold == 0);
		cursor->clickList[i].prev = cursor->clickList[i].hold;
	}
	
	for (int i = 0; i < CURSOR_ACTION_MAX; i++) {
		cursor->clickList[CLICK_ANY].__action[i] = (
			cursor->clickList[CLICK_L].__action[i] ||
			cursor->clickList[CLICK_R].__action[i] ||
			cursor->clickList[CLICK_M].__action[i]
		);
	}
	
	if (cursor->clickAny.press) {
		cursor->pressPos.x = cursor->pos.x;
		cursor->pressPos.y = cursor->pos.y;
		cursor->dragDist = 0;
		cursor->postPressMoveDist = 0;
	}
	
	f32 vel = Vec2s_DistXZ((Vec2s) {}, cursor->vel);
	
	if (cursor->clickAny.hold)
		cursor->dragDist += vel;
	cursor->postPressMoveDist += vel;
	
	cursor->cursorAction =
		(cursor->clickAny.press || cursor->clickAny.hold || cursor->scrollY) && !(this->state & INPUT_BLOCK);
	
	int pixelDist = Max(gPixelScale, 1);
	
	for (int i = 0; i < CLICK_ANY; i++) {
		InputType* click = &this->cursor.clickList[i];
		
		click->dual = false;
		if (Decr(click->__timer) > 0) {
			if (cursor->postPressMoveDist >= pixelDist) {
				click->__timer = 0;
			} else if (click->release) {
				click->dual = true;
				click->__timer = 0;
			}
		} else if (click->release && cursor->postPressMoveDist < pixelDist)
			click->__timer = 20;
	}
	
	cursor->clickAny.dual = (
		cursor->clickL.dual ||
		cursor->clickR.dual ||
		cursor->clickMid.dual
	);
	
	if (cursor->clickAny.press)
		cursor->vel = Vec2s_New(0, 0);
}

void Input_End(Input* this) {
	Cursor* cursor = &this->cursor;
	s32 i = 0;
	
	cursor->scrollY = 0;
	cursor->vel = Vec2s_New(0, 0);
	
	while (this->buffer[i] != '\0')
		this->buffer[i++] = '\0';
}

const char* Input_GetClipboardStr(Input* this) {
	return glfwGetClipboardString(this->window->glfw);
}

void Input_SetClipboardStr(Input* this, const char* str) {
	glfwSetClipboardString(this->window->glfw, str);
}

InputType* Input_GetKey(Input* this, int key) {
	static InputType zero;
	
	if (!(this->state & INPUT_PERMISSIVE) && (this->state & INPUT_BLOCK || this->tempLock))
		return &zero;
	
	return &this->key[key];
}

InputType* Input_GetCursor(Input* this, CursorClick type) {
	static InputType zero;
	
	if (!(this->state & INPUT_PERMISSIVE) && (this->state & INPUT_BLOCK || this->tempLock))
		return &zero;
	
	return &this->cursor.clickList[type];
}

f32 Input_GetScrollRaw(Input* this) {
	if (!(this->state & INPUT_PERMISSIVE) && (this->state & INPUT_BLOCK || this->tempLock))
		return 0;
	
	return this->cursor.scrollY;
}

f32 Input_GetScroll(Input* this) {
	gScrollSpeedMod = clamp_min(gScrollSpeedMod, 1);
	
	return Input_GetScrollRaw(this) * gScrollSpeedMod;
}

s32 Input_GetShortcut(Input* this, int mod, int key) {
	if (!(this->state & INPUT_PERMISSIVE) && (this->state & INPUT_BLOCK || this->tempLock))
		return 0;
	if (this->key[mod].hold && this->key[key].press)
		return 1;
	
	return 0;
}

void Input_SetMousePos(Input* this, s32 x, s32 y) {
	if (x == MOUSE_KEEP_AXIS)
		x = this->cursor.pos.x;
	
	if (y == MOUSE_KEEP_AXIS)
		y = this->cursor.pos.y;
	
	for (int i = 0; i < 10; i++)
		glfwSetCursorPos(this->window->glfw, x, y);
	
	this->cursor.pos.x = x;
	this->cursor.pos.y = y;
}

f32 Input_GetPressPosDist(Input* this) {
	return Vec2s_DistXZ(this->cursor.pos, this->cursor.pressPos);
}

int Input_SetState(Input* this, InputState state) {
	int s = (this->state & state);
	
	this->state |= state;
	
	return s ? EXIT_FAILURE : 0;
}

int Input_ClearState(Input* this, InputState state) {
	int s = (this->state & state);
	
	this->state &= ~state;
	
	return !s ? EXIT_FAILURE : 0;
}

int Input_SelectClick(Input* this, CursorClick type) {
	if (!Input_GetCursor(this, type)->release)
		return false;
	if (this->cursor.dragDist > 6 * Max(gPixelScale, 1))
		return false;
	
	return true;
}

// # # # # # # # # # # # # # # # # # # # #
// # Callbacks                           #
// # # # # # # # # # # # # # # # # # # # #

void InputCallback_Key(GLFWwindow* window, s32 key, s32 scancode, s32 action, s32 mods) {
	Input* this = GET_WINDOW()->input;
	
	this->key[key].hold = action != 0;
}

void InputCallback_Text(GLFWwindow* window, u32 scancode) {
	Input* this = GET_WINDOW()->input;
	
	if (!(scancode & 0xFFFFFF00)) {
		if (scancode > 0x7F) {
			printf("\a");
		} else {
			strcat(this->buffer, x_fmt("%c", (char)scancode));
		}
	}
}

void InputCallback_Mouse(GLFWwindow* window, s32 button, s32 action, s32 mods) {
	Input* this = GET_WINDOW()->input;
	Cursor* cursor = &this->cursor;
	
	switch (button) {
		case GLFW_MOUSE_BUTTON_RIGHT:
			cursor->clickR.hold = action != 0;
			break;
			
		case GLFW_MOUSE_BUTTON_LEFT:
			cursor->clickL.hold = action != 0;
			break;
		case GLFW_MOUSE_BUTTON_MIDDLE:
			cursor->clickMid.hold = action != 0;
	}
}

void InputCallback_MousePos(GLFWwindow* window, f64 x, f64 y) {
	// Input* input = GET_WINDOW()->input;
	// Cursor* cursor = &input->cursor;
	
	// cursor->vel.x += x - cursor->pos.x;
	// cursor->vel.y += y - cursor->pos.y;
	// cursor->pos.x = x;
	// cursor->pos.y = y;
}

void InputCallback_Scroll(GLFWwindow* window, f64 x, f64 y) {
	Input* this = GET_WINDOW()->input;
	Cursor* cursor = &this->cursor;
	
	cursor->scrollY += y;
}
