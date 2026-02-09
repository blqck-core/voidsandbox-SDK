// Copyright (C) 2023-2026 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "javascript.h"

int JS_MenuCheck(void) {
    JS_StackClean();
    JS_Function(JS_MENUCHECK);
    return jsresult.v.i;
}

void JS_MenuInit(void) {
    JS_StackClean();
    JS_Function(JS_MENUINIT);
}

void JS_MenuDraw(void) {
    JS_StackClean();
    JS_Function(JS_MENUDRAW);
}

void JS_MenuKey(int key) {
    JS_StackClean();
    jsargs.t[0] = JS_TYPE_INT;
    jsargs.v[0].i = key;
    JS_Function(JS_MENUKEY);
}

void JS_MenuCallback(int id) {
    JS_StackClean();
    jsargs.t[0] = JS_TYPE_INT;
    jsargs.v[0].i = id;
    JS_Function(JS_MENUCALLBACK);
}
