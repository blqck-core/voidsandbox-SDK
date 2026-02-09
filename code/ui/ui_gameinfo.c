// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

#define DIRLIST_SIZE 16384

static int ui_numBots;
static char *ui_botInfos[MAX_BOTS];

static int ui_numArenas;
static char *ui_arenaInfos[MAX_ARENAS];

static char dirlist[DIRLIST_SIZE];

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

static int UI_ParseInfos(char *buf, int max, char *infos[], qboolean arenas) {
	char *token;
	int count;
	char key[MAX_TOKEN_CHARS], info[MAX_INFO_STRING];

	count = 0;

	while(1) {
		token = COM_Parse(&buf);
		if(!token[0]) break;
		if(strcmp(token, "{")) {
			Com_Printf("Missing { in info file\n");
			break;
		}
		if(count == max) {
			Com_Printf("Max infos exceeded\n");
			break;
		}

		info[0] = '\0';
		while(1) {
			token = COM_ParseExt(&buf, qtrue);
			if(!token[0]) {
				Com_Printf("Unexpected end of info file\n");
				break;
			}
			if(!strcmp(token, "}")) break;
			Q_StringCopy(key, token, sizeof(key));
			token = COM_ParseExt(&buf, qfalse);
			if(!token[0]) strcpy(token, "<NULL>");
			if(!arenas || (!strcmp(key, "map") || !strcmp(key, "type"))) Info_SetValueForKey(info, key, token);
		}
		infos[count] = UI_Alloc(strlen(info) + 1);
		if(infos[count]) {
			strcpy(infos[count], info);
			count++;
		}
	}
	return count;
}

static void UI_LoadArenasFromFile(char *filename) {
	int len;
	fileHandle_t f;
	char buf[MAX_ARENAS_TEXT];

	len = FS_Open(filename, &f, FS_READ);
	if(!f) {
		trap_Print(va(S_COLOR_RED "file not found: %s\n", filename));
		return;
	}
	if(len >= MAX_ARENAS_TEXT) {
		trap_Print(va(S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_ARENAS_TEXT));
		FS_Close(f);
		return;
	}

	FS_Read(buf, len, f);
	buf[len] = 0;
	FS_Close(f);

	ui_numArenas += UI_ParseInfos(buf, MAX_ARENAS - ui_numArenas, &ui_arenaInfos[ui_numArenas], qtrue);
}

void UI_LoadArenas(void) {
	int numdirs, i, dirlen;
	char filename[128], dirlist[1024];
	char *dirptr;

	ui_numArenas = 0;

	numdirs = FS_List("scripts", ".arena", dirlist, 1024);
	dirptr = dirlist;
	for(i = 0; i < numdirs; i++, dirptr += dirlen + 1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		UI_LoadArenasFromFile(filename);
	}
}

static void UI_LoadBotsFromFile(char *filename) {
	int len;
	fileHandle_t f;
	char buf[MAX_BOTS_TEXT];

	len = FS_Open(filename, &f, FS_READ);
	if(!f)

		if(len >= MAX_BOTS_TEXT) {
			trap_Print(va(S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_BOTS_TEXT));
			FS_Close(f);
			return;
		}

	FS_Read(buf, len, f);
	buf[len] = 0;
	FS_Close(f);

	ui_numBots += UI_ParseInfos(buf, MAX_BOTS - ui_numBots, &ui_botInfos[ui_numBots], qfalse);
}

void UI_LoadBots(void) {
	int numdirs;
	char filename[128];
	char *dirptr;
	int i;
	int dirlen;

	ui_numBots = 0;

	numdirs = FS_List("scripts", ".bot", dirlist, DIRLIST_SIZE);
	dirptr = dirlist;
	for(i = 0; i < numdirs; i++, dirptr += dirlen + 1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		UI_LoadBotsFromFile(filename);
	}
}

char *UI_GetBotInfoByName(const char *name) {
	int n;
	char *value;

	for(n = 0; n < ui_numBots; n++) {
		value = Info_ValueForKey(ui_botInfos[n], "name");
		if(!Q_stricmp(value, name)) {
			return ui_botInfos[n];
		}
	}

	return NULL;
}

int UI_GetNumBots(void) { return ui_numBots; }

static qboolean MapHasGametype(char *typeString, const char *mode) {
	char *token;

	while(1) {
		token = COM_ParseExt(&typeString, qfalse);
		if(!typeString || !token[0]) break;
		if(!Q_stricmp(token, mode)) return qtrue;
	}

	return qfalse;
}

void UI_FillListOfMaps(menuelement_s *e, char *gametype, char *names, int namesSize, char **configlist) {
	int i, count = 0;
	char *map;
	char *type;
	char *out = names;
	int remaining = namesSize;

	e->string = "levelshots";
	e->itemnames = (const char **)configlist;

	for(i = 0; i < ui_numArenas; i++) {
		map = Info_ValueForKey(ui_arenaInfos[i], "map");
		type = Info_ValueForKey(ui_arenaInfos[i], "type");

		if(!map[0]) continue;

		if(MapHasGametype(type, gametype) || !Q_stricmp(gametype, "all")) {
			int len = strlen(map);
			if(len + 1 >= remaining) break;

			strcpy(out, map);
			e->itemnames[count] = out;

			out += len + 1;
			remaining -= len + 1;
			count++;

			if(count >= 65536) break;
		}
	}

	if(count == 0) {
		strcpy(names, "Empty");
		e->itemnames[0] = names;
		e->numitems = 1;
	} else {
		e->numitems = count;
	}
}

void UI_FillListOfBots(menuelement_s *e, char *names, int namesSize, char **configlist) {
	int i, len, count = 0;
	char *name;
	char *out = names;
	int remaining = namesSize;

	e->string = "";
	e->itemnames = (const char **)configlist;

	for(i = 0; i < ui_numBots; i++) {
		name = Info_ValueForKey(ui_botInfos[i], "name");
		if(!name[0]) continue;

		len = strlen(name);
		strcpy(out, name);
		e->itemnames[i] = out;
		out += len + 1;
	}

	e->numitems = i;
}

int UI_CountOfMaps(char *gametype) {
	int i, count = 0;
	char *map;
	char *type;

	for(i = 0; i < ui_numArenas; i++) {
		map = Info_ValueForKey(ui_arenaInfos[i], "map");
		type = Info_ValueForKey(ui_arenaInfos[i], "type");

		if(!map[0]) continue;

		if(MapHasGametype(type, gametype) || !Q_stricmp(gametype, "all")) count++;
	}
	return count;
}
