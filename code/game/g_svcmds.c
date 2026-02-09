// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

static void Svcmd_EntityList_f(void) {
	int e;
	gentity_t *check;

	check = g_entities + 1;
	for(e = 1; e < level.num_entities; e++, check++) {
		if(!check->inuse) continue;
		G_Printf("%3i: ", e);

		if(check->classname) G_Printf("%s", check->classname);
		G_Printf("\n");
	}
}

static gclient_t *ClientForString(const char *s) {
	gclient_t *cl;
	int i;
	int idnum;

	// numeric values are just slot numbers
	if(s[0] >= '0' && s[0] <= '9') {
		idnum = atoi(s);
		if(idnum < 0 || idnum >= level.maxclients) {
			Com_Printf("Bad client slot: %i\n", idnum);
			return NULL;
		}

		cl = &level.clients[idnum];
		if(cl->pers.connected == CON_DISCONNECTED) {
			G_Printf("Client %i is not connected\n", idnum);
			return NULL;
		}
		return cl;
	}

	// check for a name match
	for(i = 0; i < level.maxclients; i++) {
		cl = &level.clients[i];
		if(cl->pers.connected == CON_DISCONNECTED) continue;
		if(!Q_stricmp(cl->pers.netname, s)) return cl;
	}

	G_Printf("User %s is not on the server\n", s);
	return NULL;
}

/*
===================
Svcmd_ForceTeam_f
forceteam <player> <team>
===================
*/
static void Svcmd_ForceTeam_f(void) {
	gclient_t *cl;
	char str[MAX_TOKEN_CHARS];

	// find the player
	trap_Argv(1, str, sizeof(str));
	cl = ClientForString(str);
	if(!cl) {
		return;
	}

	// set the team
	trap_Argv(2, str, sizeof(str));
	SetTeam(&g_entities[cl - level.clients], str);
}

/*
===============
Server Command Table
================
*/
struct {
	char *cmd;
	void (*function)(void);
} svcmds[] = {
    {"entityList", Svcmd_EntityList_f},
    {"forceTeam", Svcmd_ForceTeam_f},
    {"addbot", Svcmd_AddBot_f},

    {"savemap", G_WriteMapfile_f},
    {"deletemap", G_DeleteMapfile_f},
    {"clearmap", G_ClearMap_f},
    {"loadmap", G_LoadMapfile_f},
};

/*
=================
ConsoleCommand
=================
*/
qboolean ConsoleCommand(void) {
	char cmd[MAX_TOKEN_CHARS];
	int i;

	trap_Argv(0, cmd, sizeof(cmd));

	for(i = 0; i < sizeof(svcmds) / sizeof(svcmds[0]); i++) {
		if(!Q_stricmp(cmd, svcmds[i].cmd)) {
			svcmds[i].function();
			return qtrue;
		}
	}

	if(cvarInt("g_dedicated")) G_Printf("unknown command: %s\n", cmd);

	return qfalse;
}
