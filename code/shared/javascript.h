// Copyright (C) 2023-2026 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "system.h"
#include "engine.h"
#include "core.h"
#ifdef GAME
#include "../game/bg_public.h"
#include "../game/g_public.h"
#include "../game/g_local.h"
#include "../game/ai_main.h"
#endif
#ifdef CGAME
#include "../shared/client/renderer.h"
#include "../game/bg_public.h"
#include "../cgame/cg_public.h"
#include "../shared/client/input.h"
#include "../cgame/cg_local.h"
#endif
#ifdef UI
#include "../shared/client/renderer.h"
#include "../game/bg_public.h"
#include "../ui/ui_public.h"
#include "../shared/client/input.h"
#include "../ui/ui_local.h"
#endif

#define MAX_JS_ARGS 32
#define MAX_JS_STRINGSIZE 4096
#define GETVMCONTEXT 1000
#define VMCALL 1001

typedef enum {
	JS_MENUCHECK = 1,
	JS_MENUINIT = 2,
	JS_MENUDRAW = 3,
	JS_MENUKEY = 4,
	JS_MENUCALLBACK = 5,
} jscall_t;

typedef enum {
    VM_ENTRY = 0,
    VM_UIRESET = 1,
	VM_DRAWSTRING = 2,
	VM_DRAWRECTANGLE = 3,
	VM_DRAWSHADER = 4,
	VM_CREATEBUTTON = 5,
	VM_CREATECHECKBOX = 6,
	VM_CREATESLIDER = 7,
} vmcall_t;

typedef enum { JS_TYPE_NONE, JS_TYPE_INT, JS_TYPE_FLOAT, JS_TYPE_STRING } js_type_t;

typedef struct {
    int i;
    float f;
    char s[MAX_JS_STRINGSIZE];
} js_value_t;

typedef struct {
    js_type_t t[MAX_JS_ARGS];
    js_value_t v[MAX_JS_ARGS];
} js_args_t;

typedef struct {
    js_type_t t;
    js_value_t v;
} js_result_t;

void JS_StackClean(void);
void JS_Function(int func_id);

// JS Functions
int JS_MenuCheck(void);
void JS_MenuInit(void);
void JS_MenuDraw(void);
void JS_MenuKey(int key);
void JS_MenuCallback(int id);

// JS Definitions
#define JSCOLOR_DISABLED 101
#define JSCOLOR_SELECTED 102
#define JSCOLOR_ENABLED 103
#define JSCOLOR_TIP 104
#define JSCOLOR_TIPSHADOW 105
#define JSCOLOR_TIPTEXT 106

extern js_args_t vmargs;
extern js_result_t vmresult;
extern js_args_t jsargs;
extern js_result_t jsresult;

// syscalls
void VMContext(js_args_t *args, js_result_t *result);
qboolean JSOpenFile(const char *filename);
void JSLoadScripts(const char *path, const char *name);
qboolean JSEval(const char *code, js_result_t *result);
qboolean JSCall(int id, js_args_t *args, js_result_t *result);
