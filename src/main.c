#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define CLAY_IMPLEMENTATION
#include "../inc/external/clay.h"
#include "external/clay_renderer_raylib.c"
#include "../inc/test.h"
#include "../inc/board.h"

static bool PD_arr = true;
static bool preview = false;

void HandleClayErrors(Clay_ErrorData errorData) {
    printf("%s", errorData.errorText.chars);
}

const static int FONT_ID = 0;
const static int TILE_FONT_ID = 1;
static int test_state = -1;
enum page {
	EXPLORE,
	GENERATE,
	PRACTICE,
	SETTINGS
};
static enum page current_page;
static Clay_Color theme[16] = {
	{255, 255, 255, 255}, // text color
	{140, 133, 133, 255},
	{127, 116, 114, 255},
	{114, 101, 97 , 255},
	{191, 127, 95 , 255},
	{178, 121, 80 , 255},
	{165, 116, 66 , 255},
	{153, 111, 53 , 255},
	{153, 61 , 53 , 255},
	{140, 58 , 42 , 255},
	{127, 55 , 31 , 255},
	{114, 53 , 22 , 255},
	{114, 22 , 38 , 255},
	{114, 21 , 83 , 255},
	{102, 14 , 65 , 255},
	{89 , 8  , 49 , 255}
};
static void RenderSidebarButton(Clay_String text, void (*HoverCallback)(Clay_ElementId, Clay_PointerData, intptr_t), Clay_Color background, Clay_Color hover_background){
	CLAY(
		CLAY_LAYOUT({ .padding = {16, 16}, .sizing = {CLAY_SIZING_GROW(0), 50},
					.childAlignment = {
						.x = CLAY_ALIGN_X_CENTER,
						.y = CLAY_ALIGN_Y_CENTER
					}}
			),
		Clay_OnHover(HoverCallback, (intptr_t)NULL),
		CLAY_RECTANGLE({
			.color = Clay_Hovered() ? hover_background : background,
			.cornerRadius = 15
		})
	){
		CLAY_TEXT(text, CLAY_TEXT_CONFIG({
			.fontId = FONT_ID,
			.fontSize = 16,
			.textColor = { 0, 0, 0, 255 }
		}));
	}
}

static void emptyCallback(Clay_ElementId id, Clay_PointerData mouse, intptr_t userData){}
static void selfTestButton(Clay_ElementId id, Clay_PointerData mouse, intptr_t userData){
	if(mouse.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME){
		test_state = test();
	}
}
static void exploreButton(Clay_ElementId id, Clay_PointerData mouse, intptr_t userData){
	if(mouse.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME){
		current_page = EXPLORE;
	}
}
static void practiceButton(Clay_ElementId id, Clay_PointerData mouse, intptr_t userData){
	if(mouse.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME){
		current_page = PRACTICE;
	}
}
static void generateButton(Clay_ElementId id, Clay_PointerData mouse, intptr_t userData){
	if(mouse.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME){
		current_page = GENERATE;
	}
}
static void settingsButton(Clay_ElementId id, Clay_PointerData mouse, intptr_t userData){
	if(mouse.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME){
		current_page = SETTINGS;
	}
}
static void showTile(int t){
	Clay_String s;
	switch (t){
		default:
		case 0:
			s = CLAY_STRING("0");
			break;
		case 1:
			s = CLAY_STRING("2");
			break;
		case 2:
			s = CLAY_STRING("4");
			break;
		case 3:
			s = CLAY_STRING("8");
			break;
		case 4:
			s = CLAY_STRING("16");
			break;
		case 5:
			s = CLAY_STRING("32");
			break;
		case 6:
			s = CLAY_STRING("64");
			break;
		case 7:
			s = CLAY_STRING("128");
			break;
		case 8:
			s = CLAY_STRING("256");
			break;
		case 9:
			s = CLAY_STRING("512");
			break;
		case 10:
			s = CLAY_STRING("1024");
			break;
		case 11:
			s = CLAY_STRING("2048");
			break;
		case 12:
			s = CLAY_STRING("4096");
			break;
		case 13:
			s = CLAY_STRING("8192");
			break;
		case 14:
			s = CLAY_STRING("16384");
			break;
		case 15:
			s = CLAY_STRING("32768");
			break;
	}
	CLAY(
		CLAY_LAYOUT({ 
			.sizing = {CLAY_SIZING_FIXED(80), CLAY_SIZING_FIXED(80)},
			.childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER }
		}),
		CLAY_RECTANGLE({
			.color = ((t == 0) ? (Clay_Color){114,111,107,255} : theme[t]),
			.cornerRadius = 15
		})
	){
		CLAY_TEXT(s, CLAY_TEXT_CONFIG({
			.fontId = TILE_FONT_ID,
			.fontSize = 24,
			.textColor = ((t == 0) ? (Clay_Color){114,111,107,255} : theme[0]),
		}));
	};
}
static void showRow(int a, int b, int c, int d){
	CLAY(
		CLAY_LAYOUT({ 
			.sizing = {CLAY_SIZING_GROW(0), 50},
			.layoutDirection = CLAY_LEFT_TO_RIGHT,
			.padding = {20, 20, 0, 0},
			.childGap = 20
		})
	){
		showTile(a);
		showTile(b);
		showTile(c);
		showTile(d);
	};
}
static void showBoard(uint64_t board){
	CLAY(
		CLAY_LAYOUT({ 
			.padding = {0, 0, 20, 20}, 
			.childGap = 20,
			.sizing = {420, 420},
			.layoutDirection = CLAY_TOP_TO_BOTTOM,
		}),
		CLAY_RECTANGLE({
			.color = {200,200,200,255},
			.cornerRadius = 15
		})
	){
		for(int i = 0; i < 4; i++){
			showRow(GET_TILE(board, (i*4)), GET_TILE(board, (i*4 + 1)), GET_TILE(board, (i*4 + 2)), GET_TILE(board, (i*4 + 3)));
		}
	};
}
static void generatePage(){

		showBoard(0x0123456789abcdef);
}
static void explorePage(){

}
static void checkboxCallback(Clay_ElementId id, Clay_PointerData mouse, intptr_t userData){
	if(mouse.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME){
		if(*(bool*)userData){
			*(bool*)userData = 0;
		}
		else{
			*(bool*)userData = 1;
		}
	}
}
static void checkbox(bool *data, Clay_String textOn, Clay_String textOff){
	const Clay_Color ON = {230, 255, 230, 255};
	const Clay_Color OFF = {255, 230, 230, 255};
	CLAY(
		CLAY_LAYOUT({
			.sizing = {CLAY_SIZING_GROW(300), 50},
			.childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
			.padding = {16, 16, 0, 0}
		}),
		CLAY_RECTANGLE({
			.color = (*data) ? ON : OFF,
			.cornerRadius = 15
		}),
		Clay_OnHover(checkboxCallback, (intptr_t)data)
	){
	CLAY_TEXT((*(bool*)data) ? textOn : textOff, CLAY_TEXT_CONFIG({
		.fontId = FONT_ID,
		.fontSize = 16,
		.textColor = {0, 0, 0, 255}
	}));};
}
static void truefalsesetting(Clay_String desc, Clay_String on, Clay_String off, bool* data){
	CLAY(
		CLAY_LAYOUT({
			.layoutDirection = CLAY_LEFT_TO_RIGHT,
			.childGap = 16
		})
		){
		CLAY_TEXT(desc, CLAY_TEXT_CONFIG({
			.fontId = FONT_ID,
			.fontSize = 16,
			.textColor = { 0, 0, 0, 255 }
		}));
		checkbox(data, on, off);
	}
}
static void settingsPage(){
	static bool test = false;
	CLAY(
		CLAY_LAYOUT({
			.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
			.padding = {16, 16, 16, 16},
			.childAlignment = {.x = CLAY_ALIGN_X_CENTER},
			.childGap = 16,
			.layoutDirection = CLAY_TOP_TO_BOTTOM
		})
	){
		truefalsesetting(CLAY_STRING("Use Potential Duplicate Array: Use a new experimental method to check for duplicate boards instead of sorting, may increase performance in generation"), 
				CLAY_STRING("Use PD Arr: True"), CLAY_STRING("Use PD Arr: False"), &PD_arr);
		truefalsesetting(CLAY_STRING("Show a random board from each layer during generation, may *slightly* impact performance"),
				CLAY_STRING("Preview: True"), CLAY_STRING("Preview: False"), &preview);
	}
}
static void practicePage(){
}
void ui(){
	const int 		 CHILD_RADIUS = 15;
	const Clay_Color SIDEBAR_COLOR = {230,230,230,255};
	const int 		 SIDEBAR_WIDTH = 200;
	const Clay_Color SIDEBAR_CHILD_COLOR = {225,220,220,255};
	const Clay_Color SIDEBAR_CHILD_COLOR_HOVERED = {220,215,215,255};
	const Clay_Color CONTAINER_COLOR = {245,245,250,255};
	const Clay_Color MAIN_CONTENT_COLOR = {255,255,255,255};
	const Clay_Color TEST_PASSED = {230, 255, 230, 255};
	const Clay_Color TEST_FAILED = {255, 230, 230, 255};
	size_t totalMemorySize = Clay_MinMemorySize();
	Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
	Clay_Initialize(arena, (Clay_Dimensions) { GetScreenWidth(), GetScreenHeight() }, (Clay_ErrorHandler) { HandleClayErrors });
	Clay_Raylib_Initialize(1920,1080,"Cablegen v0.0\0", FLAG_WINDOW_RESIZABLE);
	Clay_SetMeasureTextFunction(Raylib_MeasureText, (uintptr_t)NULL);
	Raylib_fonts[FONT_ID] = (Raylib_Font){
		.fontId = FONT_ID,
		.font = LoadFontEx("resources/Roboto-Regular.ttf", 16, 0, 400)
	};
	Raylib_fonts[TILE_FONT_ID] = (Raylib_Font){
		.fontId = TILE_FONT_ID,
		.font = LoadFontEx("resources/clear-sans.bold.ttf", 24, 0, 400)
	};

	while(!WindowShouldClose()){
		Clay_SetLayoutDimensions((Clay_Dimensions) { GetScreenWidth(), GetScreenHeight() });
		Clay_SetPointerState((Clay_Vector2){ GetMouseX(), GetMouseY() }, IsMouseButtonDown(0) );
		Clay_UpdateScrollContainers(true, (Clay_Vector2){ GetMouseWheelMoveV().x, GetMouseWheelMoveV().y }, GetFrameTime());

		Clay_BeginLayout();

		CLAY(CLAY_ID("OuterContainer"), CLAY_LAYOUT({ .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 16, .layoutDirection = CLAY_LEFT_TO_RIGHT}), 
										CLAY_RECTANGLE({.color = CONTAINER_COLOR})) {
			CLAY(
				CLAY_ID("Sidebar"),
				CLAY_LAYOUT({
					.sizing = {
						.width = CLAY_SIZING_FIXED(SIDEBAR_WIDTH), 
						.height = CLAY_SIZING_GROW(0)
					}, 
					.layoutDirection = CLAY_TOP_TO_BOTTOM,
					.padding = { 16, 16, 16, 16 },
					.childGap = 16,
				}), 
				CLAY_RECTANGLE({
					.color = SIDEBAR_COLOR, 
					.cornerRadius = CHILD_RADIUS
				})
				){
#ifndef PROD
					if(test_state == -1)
						RenderSidebarButton(CLAY_STRING("Run self test"), selfTestButton, SIDEBAR_CHILD_COLOR, SIDEBAR_CHILD_COLOR_HOVERED);
					else if (test_state == 0)
						RenderSidebarButton(CLAY_STRING("Test Failed"), emptyCallback, TEST_FAILED, TEST_FAILED);
					else if (test_state == 1)
						RenderSidebarButton(CLAY_STRING("Test Passed"), emptyCallback, TEST_PASSED, TEST_PASSED);
#endif
					RenderSidebarButton(CLAY_STRING("Explore Tables"), exploreButton, SIDEBAR_CHILD_COLOR, SIDEBAR_CHILD_COLOR_HOVERED);
					RenderSidebarButton(CLAY_STRING("Generate Tables"), generateButton, SIDEBAR_CHILD_COLOR, SIDEBAR_CHILD_COLOR_HOVERED);
					RenderSidebarButton(CLAY_STRING("Practice"), practiceButton, SIDEBAR_CHILD_COLOR, SIDEBAR_CHILD_COLOR_HOVERED);
					RenderSidebarButton(CLAY_STRING("Settings"), settingsButton, SIDEBAR_CHILD_COLOR, SIDEBAR_CHILD_COLOR_HOVERED);

			}
			CLAY(CLAY_ID("Main"),
					CLAY_LAYOUT({
						.sizing = {
							.width = CLAY_SIZING_GROW(0), 
							.height = CLAY_SIZING_GROW(0)
						}, 
						.layoutDirection = CLAY_TOP_TO_BOTTOM
					}), 
					CLAY_RECTANGLE({
						.color = MAIN_CONTENT_COLOR,
						.cornerRadius = CHILD_RADIUS
					})){
					switch (current_page){
						case EXPLORE:
							explorePage();
							break;
						case GENERATE:
							generatePage();
							break;
						case PRACTICE:
							practicePage();
							break;
						case SETTINGS:
							settingsPage();
							break;
				}
			}
		}; 
		// All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
		BeginDrawing();
		ClearBackground(BLACK);
		Clay_Raylib_Render(Clay_EndLayout());
		EndDrawing();
	}
}

int main(){
	ui();
	test();
	return 0;
}
