// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../qcommon/js_local.h"

#define NUM_DATA 2
#define FIRST_DATA 3

static void CG_ParseScores(void) {
	int i;

	cg.numScores = atoi(CG_Argv(1));
	if(cg.numScores > MAX_CLIENTS) cg.numScores = MAX_CLIENTS;

	cg.teamScores[0] = atoi(CG_Argv(2));
	cg.teamScores[1] = atoi(CG_Argv(3));

	memset(cg.scores, 0, sizeof(cg.scores));

	for(i = 0; i < cg.numScores; i++) {
		cg.scores[i].client = atoi(CG_Argv(i * NUM_DATA + FIRST_DATA + 1));
		cg.scores[i].score = atoi(CG_Argv(i * NUM_DATA + FIRST_DATA + 2));

		if(cg.scores[i].client < 0 || cg.scores[i].client >= MAX_CLIENTS) cg.scores[i].client = 0;
	}
}

static void CG_ParseRespawnTime(void) { cg.respawnTime = atoi(CG_Argv(1)); }

static void CG_ParseGameCvars(void) {
	mod_jumpheight = atoi(CG_Argv(1));
	mod_gravity = atoi(CG_Argv(2));
}

static void CG_ParseSweps(void) {
	int i;
	int weaponIndex;
	int numArgs = trap_Argc();

	for(i = 0; i < WEAPONS_NUM; i++) cg.swep_listcl[i] = WS_NONE;

	for(i = 1; i < numArgs; i++) {
		weaponIndex = atoi(CG_Argv(i));

		if(weaponIndex >= -WEAPONS_NUM && weaponIndex < WEAPONS_NUM) {
			if(weaponIndex > 0) {
				cg.swep_listcl[weaponIndex] = WS_HAVE;
			} else if(weaponIndex < 0) {
				cg.swep_listcl[-weaponIndex] = WS_NOAMMO;
			}
		}
	}
}

static void CG_ParseSpawnSweps(void) {
	int i;
	int weaponIndex;
	int numArgs = trap_Argc();

	for(i = 0; i < WEAPONS_NUM; i++) cg.swep_spawncl[i] = WS_NONE;

	for(i = 1; i < numArgs; i++) {
		weaponIndex = atoi(CG_Argv(i));

		if(weaponIndex >= -WEAPONS_NUM && weaponIndex < WEAPONS_NUM) {
			if(weaponIndex > 0) {
				cg.swep_spawncl[weaponIndex] = WS_HAVE;
			} else if(weaponIndex < 0) {
				cg.swep_spawncl[-weaponIndex] = WS_NOAMMO;
			}
		}
	}
}

void CG_ParseServerinfo(void) {
	const char *info;
	char *mapname;

	info = CG_ConfigString(CS_SERVERINFO);
	cgs.gametype = atoi(Info_ValueForKey(info, "g_gametype"));
	cvarSet("g_gametype", va("%i", cgs.gametype));
	cgs.teamflags = atoi(Info_ValueForKey(info, "teamflags"));
	cgs.fraglimit = atoi(Info_ValueForKey(info, "fraglimit"));
	cgs.capturelimit = atoi(Info_ValueForKey(info, "capturelimit"));
	cgs.timelimit = atoi(Info_ValueForKey(info, "timelimit"));
	cgs.maxclients = atoi(Info_ValueForKey(info, "g_maxClients"));
	mapname = Info_ValueForKey(info, "sv_mapname");
	Com_sprintf(cgs.mapname, sizeof(cgs.mapname), "maps/%s.bsp", mapname);
}

void CG_SetConfigValues(void) {
	cgs.scores1 = atoi(CG_ConfigString(CS_SCORES1));
	cgs.scores2 = atoi(CG_ConfigString(CS_SCORES2));
	cgs.levelStartTime = atoi(CG_ConfigString(CS_LEVEL_START_TIME));
}

static void CG_ConfigStringModified(void) {
	const char *str;
	int num;

	num = atoi(CG_Argv(1));

	// get the gamestate from the client system, which will have the
	// new configstring already integrated
	trap_GetGameState(&cgs.gameState);

	// look up the individual string that was modified
	str = CG_ConfigString(num);

	// do something with it if necessary
	if(num == CS_MUSIC) {
		CG_StartMusic();
	} else if(num == CS_SERVERINFO) {
		CG_ParseServerinfo();
	} else if(num == CS_SCORES1) {
		cgs.scores1 = atoi(str);
	} else if(num == CS_SCORES2) {
		cgs.scores2 = atoi(str);
	} else if(num == CS_LEVEL_START_TIME) {
		cgs.levelStartTime = atoi(str);
	} else if(num == CS_INTERMISSION) {
		cg.intermissionStarted = atoi(str);
	} else if(num >= CS_MODELS && num < CS_MODELS + MAX_MODELS) {
		cgs.gameModels[num - CS_MODELS] = trap_R_RegisterModel(str);
	} else if(num >= CS_SOUNDS && num < CS_SOUNDS + MAX_SOUNDS) {
		if(str[0] != '*') cgs.gameSounds[num - CS_SOUNDS] = trap_S_RegisterSound(str, qfalse); // player specific sounds don't register here
	} else if(num >= CS_PLAYERS && num < CS_PLAYERS + MAX_CLIENTS) {
		CG_UpdateClientInfo(num - CS_PLAYERS);
	}
}

static void CG_MapRestart(void) {
	CG_InitLocalEntities();
	CG_InitMarkPolys();
	cg.intermissionStarted = qfalse;
	cg.mapRestart = qtrue;
	CG_StartMusic();
	trap_S_ClearLoopingSounds(qtrue);
}

static void CG_RemoveChatEscapeChar(char *text) {
	int i, l;

	l = 0;
	for(i = 0; text[i]; i++) {
		if(text[i] == '\x19') continue;
		text[l++] = text[i];
	}
	text[l] = '\0';
}

static void CG_ServerCommand(void) {
	const char *cmd;
	char text[MAX_SAY_TEXT];
	int offset;
	const char *arg;

	cmd = CG_Argv(0);

	if(!cmd[0]) return;

	if(!strcmp(cmd, "clcmd")) {
		trap_Cmd(EXEC_INSERT, CG_Argv(1));
		return;
	}

	if(!strcmp(cmd, "undo")) {
		CG_AddNotify(CG_Argv(1), NOTIFY_UNDO, 0);
		return;
	}

	if(!strcmp(cmd, "cs")) {
		CG_ConfigStringModified();
		return;
	}

	if(!strcmp(cmd, "print")) {
		// if the message to print is about a client being dropped after a silent drop, suppress the drop message
		arg = CG_Argv(1);
		offset = strlen(arg) - strlen("DR_SILENT_DROP") - 1;
		if(!strcmp(&arg[offset], "DR_SILENT_DROP\n")) return;

		CG_Printf("%s", CG_Argv(1));
		return;
	}

	if(!strcmp(cmd, "chat")) {
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
		Q_StringCopy(text, CG_Argv(1), MAX_SAY_TEXT);
		CG_RemoveChatEscapeChar(text);
		CG_PrintfChat(qfalse, "%s\n", text);
		return;
	}

	if(!strcmp(cmd, "tchat")) {
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
		Q_StringCopy(text, CG_Argv(1), MAX_SAY_TEXT);
		CG_RemoveChatEscapeChar(text);
		CG_PrintfChat(qtrue, "%s\n", text);
		return;
	}

	if(!strcmp(cmd, "scores")) {
		CG_ParseScores();
		return;
	}

	if(!strcmp(cmd, "map_restart")) {
		CG_MapRestart();
		return;
	}

	if(!strcmp(cmd, "respawn")) {
		CG_ParseRespawnTime();
		return;
	}

	if(!strcmp(cmd, "gCvars")) {
		CG_ParseGameCvars();
		return;
	}

	if(!strcmp(cmd, "sweps")) {
		CG_ParseSweps();
		return;
	}

	if(!strcmp(cmd, "wpspawn")) {
		CG_ParseSpawnSweps();
		return;
	}

	if(!strcmp(cmd, "t_info")) {
		Q_StringCopy(cg.entityInfo, CG_Argv(1), sizeof(cg.entityInfo));
		return;
	}

	CG_Printf("Unknown client game command: %s\n", cmd);
}

void CG_ExecuteNewServerCommands(int latestSequence) {
	while(cgs.serverCommandSequence < latestSequence) {
		if(trap_GetServerCommand(++cgs.serverCommandSequence)) CG_ServerCommand();
	}
}
