// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

#define POOLSIZE (1024 * 1024) // QVM_MEMORY: 1mb
static char memoryPool[POOLSIZE];
static int allocPoint;

void *UI_Alloc(int size) {
	char *p;

	if(allocPoint + size > POOLSIZE) return NULL;

	p = &memoryPool[allocPoint];
	allocPoint += (size + 31) & ~31;
	return p;
}

void UI_InitMemory(void) { allocPoint = 0; }
