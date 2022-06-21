#include "Global.h"

InputContext* __pInput;
AppInfo* __pAppInfo;

void Input_Init(InputContext* input) {
	__pInput = input;
}

void Input_Update(InputContext* input, AppInfo* app) {
	MouseInput* mouse = &input->mouse;
	f64 x, y;
	static u32 timer;
	
	glfwGetCursorPos(app->mainWindow, &x, &y);
	mouse->pos.x = x;
	mouse->pos.y = y;
	
	__pAppInfo = app;
	{
		static double last = 0;
		double cur = glfwGetTime();
		gDeltaTime = (cur - last) * 50;
		last = cur;
	}
	
	for (s32 i = 0; i < KEY_MAX; i++) {
		input->key[i].press = (input->key[i].prev == 0 && input->key[i].hold);
		input->key[i].release = (input->key[i].prev && input->key[i].hold == 0);
		input->key[i].prev = input->key[i].hold;
	}
	
	for (s32 i = 0; i < 3; i++) {
		mouse->clickArray[i].press = (mouse->clickArray[i].prev == 0 && mouse->clickArray[i].hold);
		mouse->clickArray[i].release = (mouse->clickArray[i].prev && mouse->clickArray[i].hold == 0);
		mouse->clickArray[i].prev = mouse->clickArray[i].hold;
	}
	
	mouse->click.press = (
		mouse->clickL.press ||
		mouse->clickR.press ||
		mouse->clickMid.press
	);
	
	mouse->click.hold = (
		mouse->clickL.hold ||
		mouse->clickR.hold ||
		mouse->clickMid.hold
	);
	
	mouse->click.release = (
		mouse->clickL.release ||
		mouse->clickR.release ||
		mouse->clickMid.release
	);
	
	mouse->cursorAction = (
		mouse->click.press ||
		mouse->click.hold ||
		mouse->scrollY
	);
	
	if (mouse->click.press) {
		mouse->pressPos.x = mouse->pos.x;
		mouse->pressPos.y = mouse->pos.y;
	}
	
	mouse->doubleClick = false;
	if (Decr(timer) > 0) {
		if (Vec_Vec2s_DistXZ(&mouse->pos, &mouse->pressPos) > 4) {
			timer = 0;
		} else if (mouse->clickL.press) {
			mouse->doubleClick = true;
			timer = 0;
		}
	}
	
	if (mouse->clickL.press) {
		timer = 30;
	}
	
	if (glfwGetKey(app->mainWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(app->mainWindow, true);
	
	Vec2s* mPos = &mouse->pos;
	Vec2s* mVel = &mouse->vel;
	Vec2s* mPrev = &mouse->prevPos;
	
	mVel->x = (mPos->x - mPrev->x) + mouse->jumpVelComp.x;
	mVel->y = (mPos->y - mPrev->y) + mouse->jumpVelComp.y;
	*mPrev = *mPos;
	
	if (mouse->click.press) {
		mVel->x = mVel->y = 0;
	}
	
	mouse->jumpVelComp.x = 0;
	mouse->jumpVelComp.y = 0;
}

void Input_KeyCallback(GLFWwindow* window, s32 key, s32 scancode, s32 action, s32 mods) {
	InputContext* input = __pInput;
	int hold = action != GLFW_RELEASE;
	
	input->key[key].hold = hold;
}

void Input_TextCallback(GLFWwindow* window, u32 scancode) {
	InputContext* input = __pInput;
	
	if (!(scancode & 0xFFFFFF00)) {
		if (scancode > 0x7F) {
			printf("\a");
		} else {
			strcat(input->buffer, xFmt("%c", (char)scancode));
		}
	}
}

void Input_MouseClickCallback(GLFWwindow* window, s32 button, s32 action, s32 mods) {
	InputContext* input = __pInput;
	MouseInput* mouse = &input->mouse;
	s32 hold = action == GLFW_PRESS;
	
	switch (button) {
		case GLFW_MOUSE_BUTTON_RIGHT:
			mouse->clickR.hold = hold;
			break;
			
		case GLFW_MOUSE_BUTTON_LEFT:
			mouse->clickL.hold = hold;
			break;
		case GLFW_MOUSE_BUTTON_MIDDLE:
			mouse->clickMid.hold = hold;
	}
}

void Input_ScrollCallback(GLFWwindow* window, f64 x, f64 y) {
	__pInput->mouse.scrollY = y;
}

void Input_End(InputContext* input) {
	MouseInput* mouse = &input->mouse;
	s32 i = 0;
	
	mouse->scrollY = 0;
	
	while (input->buffer[i] != '\0') {
		input->buffer[i++] = '\0';
	}
}

const char* Input_GetClipboardStr() {
	return glfwGetClipboardString(__pAppInfo->mainWindow);
}

void Input_SetClipboardStr(char* str) {
	glfwSetClipboardString(__pAppInfo->mainWindow, str);
}

InputType* Input_GetKey(KeyMap key) {
	static InputType zero;
	
	if (__pInput->state.keyBlock)
		return &zero;
	
	return &__pInput->key[key];
}

InputType* Input_GetMouse(MouseMap type) {
	return &__pInput->mouse.clickArray[type];
}

s32 Input_GetShortcut(KeyMap mod, KeyMap key) {
	if (__pInput->key[mod].hold && __pInput->key[key].press) {
		return 1;
	}
	
	return 0;
}

void Input_SetMousePos(s32 x, s32 y) {
	if (x == MOUSE_KEEP_AXIS) {
		x = __pInput->mouse.pos.x;
	} else {
		__pInput->mouse.jumpVelComp.x = __pInput->mouse.pos.x - x;
	}
	
	if (y == MOUSE_KEEP_AXIS) {
		y = __pInput->mouse.pos.y;
	} else {
		__pInput->mouse.jumpVelComp.y = __pInput->mouse.pos.y - y;
	}
	
	glfwSetCursorPos(__pAppInfo->mainWindow, x, y);
	__pInput->mouse.pos.x = x;
	__pInput->mouse.pos.y = y;
}

f32 Input_GetPressPosDist() {
	return Vec_Vec2s_DistXZ(&__pInput->mouse.pos, &__pInput->mouse.pressPos);
}