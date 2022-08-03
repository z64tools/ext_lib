#include "Interface.h"

void Input_Init(Input* input, AppInfo* app) {
	input->app = app;
}

void Input_Update(Input* input) {
	MouseInput* mouse = &input->mouse;
	ThreadLocal static u32 timer;
	
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
		if (Math_Vec2s_DistXZ(mouse->pos, mouse->pressPos) > 4) {
			timer = 0;
		} else if (mouse->clickL.press) {
			mouse->doubleClick = true;
			timer = 0;
		}
	}
	
	if (mouse->clickL.press) {
		timer = 30;
	}
	
	if (glfwGetKey(input->app->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(input->app->window, true);
	
	if (mouse->click.press)
		mouse->vel = Math_Vec2s_New(0, 0);
}

void Input_End(Input* input) {
	MouseInput* mouse = &input->mouse;
	s32 i = 0;
	
	mouse->scrollY = 0;
	mouse->vel = Math_Vec2s_New(0, 0);
	
	while (input->buffer[i] != '\0')
		input->buffer[i++] = '\0';
}

const char* Input_GetClipboardStr(Input* input) {
	return glfwGetClipboardString(input->app->window);
}

void Input_SetClipboardStr(Input* input, char* str) {
	glfwSetClipboardString(input->app->window, str);
}

InputType* Input_GetKey(Input* input, KeyMap key) {
	static InputType zero;
	
	if (input->state.keyBlock)
		return &zero;
	
	return &input->key[key];
}

InputType* Input_GetMouse(Input* input, MouseMap type) {
	return &input->mouse.clickArray[type];
}

s32 Input_GetShortcut(Input* input, KeyMap mod, KeyMap key) {
	if (input->key[mod].hold && input->key[key].press)
		return 1;
	
	return 0;
}

void Input_SetMousePos(Input* input, s32 x, s32 y) {
	if (x == MOUSE_KEEP_AXIS)
		x = input->mouse.pos.x;
	
	if (y == MOUSE_KEEP_AXIS)
		y = input->mouse.pos.y;
	
	glfwSetCursorPos(input->app->window, x, y);
	input->mouse.pos.x = x;
	input->mouse.pos.y = y;
}

f32 Input_GetPressPosDist(Input* input) {
	return Math_Vec2s_DistXZ(input->mouse.pos, input->mouse.pressPos);
}

// # # # # # # # # # # # # # # # # # # # #
// # Callbacks                           #
// # # # # # # # # # # # # # # # # # # # #

void InputCallback_Key(GLFWwindow* window, s32 key, s32 scancode, s32 action, s32 mods) {
	Input* input = GetAppInfo(window)->input;
	
	input->key[key].hold = action != 0;
}

void InputCallback_Text(GLFWwindow* window, u32 scancode) {
	Input* input = GetAppInfo(window)->input;
	
	if (!(scancode & 0xFFFFFF00)) {
		if (scancode > 0x7F) {
			printf("\a");
		} else {
			strcat(input->buffer, xFmt("%c", (char)scancode));
		}
	}
}

void InputCallback_Mouse(GLFWwindow* window, s32 button, s32 action, s32 mods) {
	Input* input = GetAppInfo(window)->input;
	MouseInput* mouse = &input->mouse;
	
	switch (button) {
		case GLFW_MOUSE_BUTTON_RIGHT:
			mouse->clickR.hold = action != 0;
			break;
			
		case GLFW_MOUSE_BUTTON_LEFT:
			mouse->clickL.hold = action != 0;
			break;
		case GLFW_MOUSE_BUTTON_MIDDLE:
			mouse->clickMid.hold = action != 0;
	}
}

void InputCallback_MousePos(GLFWwindow* window, f64 x, f64 y) {
	Input* input = GetAppInfo(window)->input;
	MouseInput* mouse = &input->mouse;
	
	mouse->vel.x += x - mouse->pos.x;
	mouse->vel.y += y - mouse->pos.y;
	mouse->pos.x = x;
	mouse->pos.y = y;
}

void InputCallback_Scroll(GLFWwindow* window, f64 x, f64 y) {
	Input* input = GetAppInfo(window)->input;
	MouseInput* mouse = &input->mouse;
	
	mouse->scrollY += y;
}