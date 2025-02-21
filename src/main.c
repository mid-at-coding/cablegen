#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#define CLAY_IMPLEMENTATION
#include "../inc/external/clay.h"
#include "external/clay_renderer_raylib.c"
#include "../inc/test.h"
#include "../inc/generate.h"
#include "../inc/board.h"

bool PD_arr = true;
bool preview = false;
bool flat_movement = false;
static char *table_fmt_dir = "tables/%d.tables\0";

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

struct {
	int *data;
	bool up;
	bool down;
} typedef scrollableData;

static int numPlaces (int n) {
    if (n < 0) return numPlaces ((n == INT_MIN) ? INT_MAX: -n) + 1;
    if (n < 10) return 1;
    return 1 + numPlaces (n / 10);
}
static Clay_String itocs(int val){
	Clay_String res = {0, 0};
	char* buf = NULL;
	size_t buf_size = numPlaces(val) + 1; // one more for null term
	buf = malloc(buf_size * sizeof(char));
	if(buf == NULL)
		printf("Failed to allocate space for string!\n");
	else{
		snprintf(buf, buf_size, "%d", val);
		res = (Clay_String){buf_size, buf};
	}
	return res;
}
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
void showBoard(uint64_t board){
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

typedef struct {
	uint64_t *board;
	int offset;
} tileCallbackData;

static void editableTileCallback(Clay_ElementId id, Clay_PointerData mouse, intptr_t data){
	tileCallbackData *d = (tileCallbackData*)data;
	if(mouse.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME){
		SET_TILE((*(d->board)), (d->offset), (GET_TILE((*(d->board)), (d->offset)) + 1));
	}
	free(d);
}

static void showEditableTile(int offset, uint64_t *board){
	Clay_String s;
	int t = GET_TILE((*board), offset);
	tileCallbackData *d;
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
	d = malloc(sizeof(tileCallbackData));
	if(d == NULL)
		printf("Couldn't allocate tile callback data!\n");
	*d = (tileCallbackData){board, offset};
	CLAY(
		CLAY_LAYOUT({ 
			.sizing = {CLAY_SIZING_FIXED(80), CLAY_SIZING_FIXED(80)},
			.childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER }
		}),
		CLAY_RECTANGLE({
			.color = ((t == 0) ? (Clay_Color){114,111,107,255} : theme[t]),
			.cornerRadius = 15
		}),
		Clay_OnHover(editableTileCallback, (intptr_t)(d))
	){
		CLAY_TEXT(s, CLAY_TEXT_CONFIG({
			.fontId = TILE_FONT_ID,
			.fontSize = 24,
			.textColor = ((t == 0) ? (Clay_Color){114,111,107,255} : theme[0]),
		}));
	};
}
static void showEditableRow(uint64_t *board, int offset){
	CLAY(
		CLAY_LAYOUT({ 
			.sizing = {CLAY_SIZING_GROW(0), 50},
			.layoutDirection = CLAY_LEFT_TO_RIGHT,
			.padding = {20, 20, 0, 0},
			.childGap = 20
		})
	){
		for(int i = 0; i < 4; i++)
			showEditableTile(offset + i, board);
	};
}
static void showEditableBoard(uint64_t *board){
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
			showEditableRow(board, 4 * i);
		}
	};
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
static bool scrollable(Clay_String desc, Clay_String num, Clay_String on, Clay_String off, scrollableData *data, int mag){
	CLAY(
		CLAY_LAYOUT({
			.layoutDirection = CLAY_LEFT_TO_RIGHT,
			.childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
			.childGap = 16
		})
		){
		CLAY_TEXT(desc, CLAY_TEXT_CONFIG({
			.fontId = FONT_ID,
			.fontSize = 16,
			.textColor = { 0, 0, 0, 255 }
		}));
		CLAY_TEXT(num, CLAY_TEXT_CONFIG({
			.fontId = FONT_ID,
			.fontSize = 16,
			.textColor = { 0, 0, 0, 255 }
		}));
		CLAY(CLAY_LAYOUT({.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}})){CLAY_RECTANGLE();}; // TODO: why doesn't this work
		CLAY(
		CLAY_LAYOUT({
			.layoutDirection = CLAY_TOP_TO_BOTTOM,
			.childGap = 16
		})
		){
			checkbox(&data->up, on, on);
			checkbox(&data->down, off, off);
		}
	}
	if(!data->up){
		(*data->data) += mag;
		data->up = true;
		return true;
	}
	if(!data->down){
		(*data->data) -= mag;
		data->down = true;
		return true;
	}
	return false;
}
static void settingsPage(){
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
		truefalsesetting(CLAY_STRING("Use flat movement: instead of using a look up table, calculate moves directly. Might be slightly faster on CPUs with small L1 cache, or not"),
				CLAY_STRING("Use flat movement"), CLAY_STRING("Use LUT"), &flat_movement);
	}
}
static void generatePage(){
	static int end = 0;
	static int core_count = 1;
	static bool init = 0;
	static scrollableData s = {&end, 1, 1};
	static scrollableData s2 = {&core_count, 1, 1};
	static Clay_String testAsStr;
	static Clay_String ccAsStr;
	static uint64_t initialBoard = 1;
	static bool free_formation = true;
	static bool is_generating = true; // flipped so it's green when it's ready
	static bool gen_started = false; 
	static bool prespawn = false; 
	if(!init){
		testAsStr = itocs(end);
		ccAsStr = itocs(core_count);
		init = true;
	}
	if(scrollable(CLAY_STRING("Ending tile sum:"), testAsStr, CLAY_STRING("+"), CLAY_STRING("-"), &s, 2)){
		free((void*)testAsStr.chars);
		testAsStr = itocs(end);
	};
	if(scrollable(CLAY_STRING("Core count:"), ccAsStr, CLAY_STRING("+"), CLAY_STRING("-"), &s2, 1)){
		free((void*)ccAsStr.chars);
		ccAsStr = itocs(core_count);
	};
	truefalsesetting(CLAY_STRING("Free formation: 32768 tile moves are considered"),
			CLAY_STRING("Free formation: True"), CLAY_STRING("Free formation: False"), &free_formation);
	truefalsesetting(CLAY_STRING("Prespawn: generate spawns on starting board"),
			CLAY_STRING("Prespawn: True"), CLAY_STRING("Prespawn: False"), &prespawn);
	CLAY_TEXT(CLAY_STRING("Starting Board:"), CLAY_TEXT_CONFIG({
		.fontId = FONT_ID,
		.fontSize = 16,
		.textColor = { 0, 0, 0, 255 }
	}));
	showEditableBoard(&initialBoard);
	truefalsesetting(CLAY_STRING(""),
			CLAY_STRING("Generate"), CLAY_STRING("Generating.."), &is_generating);
	if(!is_generating){
		if(!gen_started){
			// ignore flat_move for now
			dynamic_arr_info n = init_darr(0,1);
			push_back(&n, initialBoard);
			generate_lut(free_formation);
			generate(get_sum(initialBoard), end, "tables/%d", n.bp, 1, core_count, prespawn);
		}
		gen_started = true;
	}
}
static void explorePage(){
	static int tableSum = 0;
	static Clay_String tableSumAsStr;
	static scrollableData s = {&tableSum, 1, 1};
	static bool init = false;
	static static_arr_info table = {NULL,0};
	static char *tableFile;
	static const int tableFileNameSize = 100;
	if(!init){
		tableSumAsStr = itocs(tableSum);
		tableFile = malloc(tableFileNameSize);
		if(tableFile == NULL)
			printf("Couldn't allocate filename!\n");
		init = true;
	}
	if(scrollable(CLAY_STRING("Current Table:"), tableSumAsStr, CLAY_STRING("+"), CLAY_STRING("-"), &s, 2)){
		free((void*)tableSumAsStr.chars);
		snprintf(tableFile, tableFileNameSize, "tables/%d", tableSum);
		if(table.bp != NULL){
			table.bp = NULL;
			free(table.bp);
			table.size = 0;
		}
		struct stat buf;
		if (stat(tableFile, &buf) == 0) // file exists
			table = read_table(tableFile);
		tableSumAsStr = itocs(tableSum);
	};
	CLAY(
		CLAY_SCROLL({.vertical = true}),
		CLAY_LAYOUT({
			.layoutDirection = CLAY_TOP_TO_BOTTOM,
			.childGap = 16
		})
	){
		for(uint64_t *curr = table.bp; curr < table.bp + (table.size > 100 ? 100 : table.size); curr++){
			showBoard(*curr);
		}
	}
}
static void practicePage(){
	static uint64_t board = 0;
	showEditableBoard(&board);
	if(IsKeyPressed(KEY_A)){
		movedir(&board, left);
	}
	if(IsKeyPressed(KEY_S)){
		movedir(&board, down);
	}
	if(IsKeyPressed(KEY_D)){
		movedir(&board, right);
	}
	if(IsKeyPressed(KEY_W)){
		movedir(&board, up);
	}
	if(IsKeyPressed(KEY_C)){
		rotate_counterclockwise(&board);
	}
	if(IsKeyPressed(KEY_V)){
		rotate_clockwise(&board);
	}
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
	const int SCROLL_MULT = 5;
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
		Clay_UpdateScrollContainers(true, (Clay_Vector2){ GetMouseWheelMoveV().x * SCROLL_MULT, GetMouseWheelMoveV().y * SCROLL_MULT }, GetFrameTime());

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
						.layoutDirection = CLAY_TOP_TO_BOTTOM,
						.childGap = 16
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
