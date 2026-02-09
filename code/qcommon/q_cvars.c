// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../qcommon/js_local.h"

cvar_t cvarStorage[MAX_CVARS];

void ST_RegisterCvars(void) {
	cvarReload();
	ST_UpdateCvars();
}

void ST_UpdateCvars(void) {
	int i;

	for(i = 0; i < MAX_CVARS; i++) {
		cvarUpdate(&cvarStorage[i], i);
	}
}

// Debug: trap_Print(va("%i:%s = %i\n", id, name, cvarStorage[id].integer));

int cvarInt(const char *name) {
	int id = cvarID(name);
	if(id == -1) return 0;
	cvarUpdate(&cvarStorage[id], id);
	return cvarStorage[id].integer;
}

float cvarFloat(const char *name) {
	int id = cvarID(name);
	if(id == -1) return 0.0f;
	cvarUpdate(&cvarStorage[id], id);
	return cvarStorage[id].value;
}

char *cvarString(const char *name) {
	int id = cvarID(name);
	if(id == -1) return "0";
	cvarUpdate(&cvarStorage[id], id);
	return cvarStorage[id].string;
}
