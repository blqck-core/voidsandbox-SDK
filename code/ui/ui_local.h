// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#ifndef __UI_LOCAL_H__
#define __UI_LOCAL_H__

#define LOW_MEMORY (5 * 1024 * 1024)

#define OSUI_STANDARD_Y 145
#define OSUI_SPACING_Y 18
#define OSUI_BIGSPACING_Y 36

#define OSUI_MAX_ELEMENTS 101

#define LST_SIMPLE 0
#define LST_ICONS 1
#define LST_GRID 2

#define RBT_NONE 0
#define RBT_NORMAL 1
#define RBT_INVERSE 2

#define AST_OSLOGO 1
#define AST_MOD 2
#define AST_LINK 3

#define SLIDER_RANGE 8
#define MAX_EDIT_LINE 256

#define MAX_MENUDEPTH 8
#define MAX_MENUITEMS 500

#define MTYPE_NULL 0

#define MTYPE_BUTTON 1
#define MTYPE_CHECKBOX 2
#define MTYPE_SLIDER 3
#define MTYPE_ACTION 4
#define MTYPE_SPINCONTROL 5
#define MTYPE_FIELD 6
#define MTYPE_BITMAP 7
#define MTYPE_SCROLLLIST 8
#define MTYPE_TEXT 9

#define MTYPE_MAX 10

#define QMF_LEFT_JUSTIFY (1U << 0)
#define QMF_CENTER_JUSTIFY (1U << 1)
#define QMF_RIGHT_JUSTIFY (1U << 2)
#define QMF_HIGHLIGHT (1U << 3)
#define QMF_HIGHLIGHT_IF_FOCUS (1U << 4) // steady focus
#define QMF_PULSEIFFOCUS (1U << 5)       // pulse if focus
#define QMF_HASMOUSEFOCUS (1U << 6)
#define QMF_HIDDEN (1U << 7)         // skips drawing
#define QMF_GRAYED (1U << 8)         // grays and disables
#define QMF_INACTIVE (1U << 9)       // disables any input
#define QMF_NODEFAULTINIT (1U << 10) // skip default initialization
#define QMF_PULSE (1U << 11)

#define QM_GOTFOCUS 1
#define QM_LOSTFOCUS 2
#define QM_ACTIVATED 3

// control event handler
typedef void (*callbackFunc)(void *self, int event);

typedef struct {
	int cursor;
	int cursor_prev;

	int nitems;
	void *items[MAX_MENUITEMS];

	void (*draw)(void);
	sfxHandle_t (*key)(int key);
} menuframework_s;

typedef struct {
	int type;
	int style;
	qhandle_t shader;
	qhandle_t model;

	// extended callback
	int general_cbtype;
	char* cmd;
	char* var;
	void (*func)(void);

	int id;
	int callid;
	int x, y, w, h;
	int left;
	int top;
	int right;
	int bottom;
	menuframework_s *parent;
	int menuPosition;
	unsigned flags;

	int mode;
	char *buffer;
	int *value;

	void (*callback)(void *self, int event);
	void (*ownerdraw)(void *self);
} menucommon_s;

typedef struct {
	int cursor;
	int scroll;
	int widthInChars;
	char buffer[MAX_EDIT_LINE];
	int maxchars;
} mfield_t;

#define UI_STRINGLENGTH 128
#define UI_ACTIONLENGTH 256

#define EMODE_NONE 0
#define EMODE_INT 1
#define EMODE_FLOAT 2
typedef struct {
	menucommon_s generic;
	
	int type;
	int id;
	float x, y, w, h;
	char text[UI_STRINGLENGTH];
	int style;
	float size;
	int colortext, colorbg;
	int corner;
	int margin;
	char action[UI_ACTIONLENGTH];
	
	float value;
	
	float min, max;
	int mode;
	
	
	char *string;
	void (*callback)(void *self, int event);
	
	int curColortext, curColorbg;
	
	float *color;
	
	char *focuspic;
	char *errorpic;
	qhandle_t shader;
	qhandle_t focusshader;
	int width;
	int height;

	int curvalue;

	int oldvalue;
	int numitems;
	int top;
	const char **itemnames;
	int columns;
	char *file;
	char *extension;

	float minvalue;
	float maxvalue;
	float range;

	mfield_t field;

	qboolean drawText;
	float padding_x;
	float padding_y;
	unsigned flags;
} menuelement_s;

extern sfxHandle_t menu_move_sound;
extern sfxHandle_t menu_out_sound;
extern sfxHandle_t menu_buzz_sound;
extern sfxHandle_t menu_null_sound;

typedef struct {
	int frametime;
	int realtime;
	int cursorx;
	int cursory;
	menuframework_s *stack[MAX_MENUDEPTH];
	qboolean debug;
	qhandle_t menuWallpapers;
	qhandle_t cursor;
	qhandle_t rb_on;
	qhandle_t rb_off;
	qboolean firstdraw;
	qboolean onmap;
	
	int currentItem;
	menuelement_s items[MAX_MENUITEMS];
	qboolean fullscreen;
} uiStatic_t;

// ui_atoms.c
void QDECL Com_Error(int level, const char *error, ...);
void QDECL Com_Printf(const char *msg, ...);
void UI_PushMenu(menuframework_s *menu);
void UI_PopMenu(void);
void UI_ForceMenuOff(void);
void UI_SetActiveMenu(uiMenuCommand_t menu);
void UI_KeyEvent(int key, int down);
void UI_MouseEvent(int dx, int dy);
qboolean UI_ConsoleCommand(int realTime);
void UI_Shutdown(void);
void UI_Init(void);
void UI_DrawHandlePic(float x, float y, float w, float h, qhandle_t hShader);
void UI_DrawModelElement(float x, float y, float w, float h, const char *model, float scale);

// ui_connect.c
void UI_DrawConnectScreen(qboolean overlay);

// ui_controls.c
void UI_Controls(void);

// ui_gameinfo.c
void *UI_Alloc(int size);
void UI_InitMemory(void);
void UI_LoadArenas(void);
void UI_LoadBots(void);
char *UI_GetBotInfoByName(const char *name);
int UI_GetNumBots(void);
void UI_FillListOfMaps(menuelement_s *e, char *gametype, char *names, int namesSize, char **configlist);
void UI_FillListOfBots(menuelement_s *e, char *names, int namesSize, char **configlist);
int UI_CountOfMaps(char *gametype);

// ui_main.c
void UI_CreateCvars(void);

// ui_menu.c
void MenuDraw(void);
void UI_Menu(void);
void UI_HotReloadJS(void);

// ui_newgame.c
void UI_NewGame(void);

// ui_options.c
void UI_Options(void);

// ui_playermodel.c
void UI_PlayerModelMenu(void);

// ui_qmenu.c
qboolean Menu_CursorOnItem(int id);
sfxHandle_t ScrollList_Key(menuelement_s *l, int key);
void Menu_SetCursor(menuframework_s *m, int cursor);
void Menu_ElementsDraw(void);
void *Menu_CurrentItem(void);
sfxHandle_t Menu_DefaultKey(int key);
void Menu_Cache(void);
void UI_FillList(menuelement_s *e, char *location, char *itemsLocation, char *extension, char *names, int namesSize, char **configlist);
int UI_CountFiles(const char *location, const char *extension);
void UI_FillListFromArray(menuelement_s *e, char **configlist, char **items, int maxItems);
void UI_FillListOfItems(menuelement_s *e, char *names, int namesSize, char **configlist);
void UI_FillListPlayers(menuelement_s *e, char **configlist, char *names, int namesSize);
int UI_ListPlayerCount(void);
void UI_SetHitbox(int id, float x, float y, float w, float h);
int UI_CButton(int id, float x, float y, float w, float h, char *text, int style, float size, int colortext, int colorbg, int corner, int margin, char *action);
int UI_CCheckbox(int id, float x, float y, float w, float h, char *text, float size, int colortext, int colorbg, int corner, int margin, char *action);
int UI_CSlider(int id, float x, float y, float w, float h, char *text, float size, int colortext, int colorbg, int corner, int margin, char *action, float min, float max, int mode);
void UI_CSpinControl(menuelement_s *e, float x, float y, char *text, const char **list, char *var, void (*callback)(void *self, int event), int callid);
void UI_CList(menuelement_s *e, float x, float y, float size, int h, int w, float pad_x, float pad_y, int style, qboolean drawText, int corner, float *color, void (*callback)(void *self, int event), int callid);
void UI_CField(menuelement_s *e, float x, float y, char *text, int w, int maxchars, float *color, char *var, void (*callback)(void *self, int event), int callid);
void UI_CText(menuelement_s *e, float x, float y, char *text, int style, float size);
void UI_CBitmap(menuelement_s *e, float x, float y, float w, float h, int pic, int flags, char *cmd, char *var, void (*func)(void), void (*callback)(void *self, int event), int callid);
void UI_CPicture(menuelement_s *e, float x, float y, float w, float h, int pic, int flags, char *cmd, char *var, void (*func)(void), void (*callback)(void *self, int event), int callid);

// ui_servers.c
void UI_ArenaServersMenu(void);

// ui_settings.c
void UI_Settings(void);

// ui_spawnmenu.c
void UI_SpawnMenu(void);

extern uiStatic_t uis;
extern const char *gametype_items[GT_MAX_GAME_TYPE + 1];

#define CON_MAXLINES 16384
#define CON_MAXLINE 256

typedef struct console_s {
	char lines[CON_MAXLINES][CON_MAXLINE];
	int linescount;
} console_t;

extern console_t console;
extern int consoleLines;

typedef struct {
	menuframework_s menu;
	menuelement_s e[OSUI_MAX_ELEMENTS];
} uimenu_t;

extern uimenu_t ui;

// syscalls
void trap_Key_SetBinding(int keynum, const char *binding);
void trap_Key_ClearStates(void);
void trap_Key_SetCatcher(int catcher);
void trap_GetClipboardData(char *buf, int bufsize);
void trap_GetClientState(uiClientState_t *state);
int trap_GetConfigString(int index, char *buff, int buffsize);
int trap_LAN_GetPingQueueCount(void);
void trap_LAN_ClearPing(int n);
void trap_LAN_GetPing(int n, char *buf, int buflen, int *pingtime);
void trap_LAN_GetPingInfo(int n, char *buf, int buflen);
int trap_MemoryRemaining(void);
int trap_LAN_GetServerCount(int source);
void trap_LAN_GetServerAddressString(int source, int n, char *buf, int buflen);
void trap_LAN_GetServerAddressString(int source, int n, char *buf, int buflen);
void consoleSync(console_t *vmconsole, int currentLines);
#endif
