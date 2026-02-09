// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#ifndef __Q_CVARS_H
#define __Q_CVARS_H

#define MAX_CVAR_STRING 256

typedef int cvarHandle_t;

typedef struct {
	float value;
	int integer;
	char string[MAX_CVAR_STRING];
} cvar_t;

void ST_RegisterCvars(void);
void ST_UpdateCvars(void);
int cvarInt(const char *name);
float cvarFloat(const char *name);
char *cvarString(const char *name);

#endif
