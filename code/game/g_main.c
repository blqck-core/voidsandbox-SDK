// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
gclient_t g_clients[MAX_CLIENTS];
int SourceTechEntityList[MAX_GENTITIES];

int mod_jumpheight;
int mod_gravity;

static void G_InitGame(int levelTime, int randomSeed, int restart);
static void G_RunFrame(int levelTime);
static void G_ShutdownGame(int restart);
static void CheckExitRules(void);

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
intptr_t vmMain(int command, int arg0, int arg1, int arg2) {
	switch(command) {
	case GAME_INIT: G_InitGame(arg0, arg1, arg2); return 0;
	case GAME_SHUTDOWN: G_ShutdownGame(arg0); return 0;
	case GAME_CLIENT_CONNECT: return (intptr_t)ClientConnect(arg0, arg1, arg2);
	case GAME_CLIENT_THINK: ClientThink(arg0); return 0;
	case GAME_CLIENT_USERINFO_CHANGED: ClientUserinfoChanged(arg0); return 0;
	case GAME_CLIENT_DISCONNECT: ClientDisconnect(arg0); return 0;
	case GAME_CLIENT_BEGIN: ClientBegin(arg0); return 0;
	case GAME_CLIENT_COMMAND: ClientCommand(arg0); return 0;
	case GAME_RUN_FRAME: G_RunFrame(arg0); return 0;
	case GAME_CONSOLE_COMMAND: return ConsoleCommand();
	case BOTAI_START_FRAME: return AI_Frame(arg0);
	case GETVMCONTEXT: VMContext(&vmargs, &vmresult); return 0;
	case VMCALL: VMCall(arg0); return 0;
	default: G_Error("game.qvm: unknown command %i", command); break;
	}

	return -1;
}

void QDECL G_Printf(const char *fmt, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	trap_Print(text);
}

void QDECL G_Error(const char *fmt, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	trap_Error(text);
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.
All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
static void G_FindTeams(void) {
	gentity_t *e, *e2;
	int i, j;
	int c, c2;

	c = 0;
	c2 = 0;
	for(i = 1, e = g_entities + i; i < level.num_entities; i++, e++) {
		if(!e->inuse) continue;
		if(!e->team) continue;
		if(e->flags & FL_TEAMSLAVE) continue;
		e->teammaster = e;
		c++;
		c2++;
		for(j = i + 1, e2 = e + 1; j < level.num_entities; j++, e2++) {
			if(!e2->inuse) continue;
			if(!e2->team) continue;
			if(e2->flags & FL_TEAMSLAVE) continue;
			if(!strcmp(e->team, e2->team)) {
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if(e2->targetname) {
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}

	G_Printf("%i teams with %i entities\n", c, c2);
}

/*
=================
G_CheckCvars
=================
*/
static void G_CheckCvars(void) {
	// check some things
	if(cvarInt("g_gametype") < 0 || cvarInt("g_gametype") >= GT_MAX_GAME_TYPE) {
		G_Printf("g_gametype %i is out of range, defaulting to 0\n", cvarInt("g_gametype"));
		cvarSet("g_gametype", "0");
	}
}

/*
============
G_InitGame
============
*/
static void G_InitGame(int levelTime, int randomSeed, int restart) {
	int i;

	G_Printf("------- Game Initialization -------\n");
	G_Printf("gamedate: %s\n", __DATE__);

	srand(randomSeed);

	ST_RegisterCvars();
	G_CheckCvars();
	G_InitMemory();

	// set some level globals
	memset(&level, 0, sizeof(level));
	level.time = levelTime;
	level.startTime = levelTime;

	level.snd_fry = G_SoundIndex("sound/player/fry.wav"); // FIXME standing in lava / slime

	G_InitWorldSession();

	// initialize all entities for this game
	memset(g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]));
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = MAX_CLIENTS;
	memset(g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]));
	level.clients = g_clients;

	// set client fields on player ents
	for(i = 0; i < level.maxclients; i++) {
		g_entities[i].client = level.clients + i;
	}

	// reserved for clients
	level.num_entities = MAX_CLIENTS;

	// let the server system know where the entites are
	trap_LocateGameData(level.gentities, level.num_entities, sizeof(gentity_t), &level.clients[0].ps, sizeof(level.clients[0]));

	// reserve some spots for dead player bodies
	InitBodyQue();

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	// general initialization
	G_FindTeams();

	if(cvarInt("bot_enable")) {
		BotAISetup(restart);
		BotAILoadMap(restart);
	}

	if(strlen(cvarString("g_entitypack"))) {
		trap_Cmd(EXEC_APPEND, va("loadmap maps/%s/%s.ent \n", cvarString("g_entitypack"), cvarString("sv_mapname"))); // load map file
	}
}

/*
=================
G_ShutdownGame
=================
*/
static void G_ShutdownGame(int restart) {
	int i;

	G_Printf("==== ShutdownGame ====\n");

	// drop all bots from game in single player
	for(i = 0; i < MAX_CLIENTS; i++) {
		if(g_entities[i].npcType > NT_PLAYER) {
			DropClientSilently(g_entities[i].client->ps.clientNum);
		}
	}

	// write all the client session data so we can get it back
	G_WriteSessionData();
	BotAIShutdown(restart);
}

void QDECL Com_Error(int level, const char *error, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	G_Error("%s", text);
}

void QDECL Com_Printf(const char *msg, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	G_Printf("%s", text);
}

/*
=============
SortRanks
=============
*/
static int QDECL SortRanks(const void *a, const void *b) {
	gclient_t *ca, *cb;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	// sort special clients last
	if(ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0) {
		return 1;
	}
	if(cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0) {
		return -1;
	}

	// then connecting clients
	if(ca->pers.connected == CON_CONNECTING) {
		return 1;
	}
	if(cb->pers.connected == CON_CONNECTING) {
		return -1;
	}

	// then spectators
	if(ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR) {
		if(ca->sess.spectatorNum > cb->sess.spectatorNum) {
			return -1;
		}
		if(ca->sess.spectatorNum < cb->sess.spectatorNum) {
			return 1;
		}
		return 0;
	}
	if(ca->sess.sessionTeam == TEAM_SPECTATOR) {
		return 1;
	}
	if(cb->sess.sessionTeam == TEAM_SPECTATOR) {
		return -1;
	}

	// then sort by score
	if(ca->ps.persistant[PERS_SCORE] > cb->ps.persistant[PERS_SCORE]) {
		return -1;
	}
	if(ca->ps.persistant[PERS_SCORE] < cb->ps.persistant[PERS_SCORE]) {
		return 1;
	}
	return 0;
}

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
static void SendScoreboardMessageToAllClients(void) {
	int i;

	for(i = 0; i < level.maxclients; i++) {
		if(level.clients[i].pers.connected == CON_CONNECTED) {
			DeathmatchScoreboardMessage(g_entities + i);
		}
	}
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks(void) {
	int i;
	int rank;
	int score;
	int newScore;
	gclient_t *cl;

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
	for(i = 0; i < level.maxclients; i++) {
		if(level.clients[i].pers.connected != CON_DISCONNECTED) {
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

			if(level.clients[i].sess.sessionTeam != TEAM_SPECTATOR) {
				level.numNonSpectatorClients++;

				// decide if this should be auto-followed
				if(level.clients[i].pers.connected == CON_CONNECTED) {
					level.numPlayingClients++;
					if(level.follow1 == -1) {
						level.follow1 = i;
					} else if(level.follow2 == -1) {
						level.follow2 = i;
					}
				}
			}
		}
	}

	qsort(level.sortedClients, level.numConnectedClients, sizeof(level.sortedClients[0]), SortRanks);

	// set the rank value for all clients that are connected and not spectators
	if(cvarInt("g_gametype") >= GT_TEAM) {
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for(i = 0; i < level.numConnectedClients; i++) {
			cl = &level.clients[level.sortedClients[i]];
			if(level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE]) {
				cl->ps.persistant[PERS_RANK] = 2;
			} else if(level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE]) {
				cl->ps.persistant[PERS_RANK] = 0;
			} else {
				cl->ps.persistant[PERS_RANK] = 1;
			}
		}
	} else {
		rank = -1;
		score = 0;
		for(i = 0; i < level.numPlayingClients; i++) {
			cl = &level.clients[level.sortedClients[i]];
			newScore = cl->ps.persistant[PERS_SCORE];
			if(i == 0 || newScore != score) {
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK] = rank;
			} else {
				// we are tied with the previous client
				level.clients[level.sortedClients[i - 1]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
			if(level.numPlayingClients == 1) {
				level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
	if(cvarInt("g_gametype") >= GT_TEAM) {
		trap_SetConfigstring(CS_SCORES1, va("%i", level.teamScores[TEAM_RED]));
		trap_SetConfigstring(CS_SCORES2, va("%i", level.teamScores[TEAM_BLUE]));
	} else {
		if(level.numConnectedClients == 0) {
			trap_SetConfigstring(CS_SCORES1, va("%i", SCORE_NOT_PRESENT));
			trap_SetConfigstring(CS_SCORES2, va("%i", SCORE_NOT_PRESENT));
		} else if(level.numConnectedClients == 1) {
			trap_SetConfigstring(CS_SCORES1, va("%i", level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE]));
			trap_SetConfigstring(CS_SCORES2, va("%i", SCORE_NOT_PRESENT));
		} else {
			trap_SetConfigstring(CS_SCORES1, va("%i", level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE]));
			trap_SetConfigstring(CS_SCORES2, va("%i", level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE]));
		}
	}

	// see if it is time to end the level
	CheckExitRules();

	// if we are at the intermission, send the new info to everyone
	if(level.intermissiontime) {
		SendScoreboardMessageToAllClients();
	}
}

/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
void MoveClientToIntermission(gentity_t *ent) {
	// take out of follow mode if needed
	if(ent->client->sess.spectatorState == SPECTATOR_FOLLOW) {
		StopFollowing(ent);
	}

	// move to the spot
	VectorCopy(level.intermission_origin, ent->s.origin);
	VectorCopy(level.intermission_origin, ent->client->ps.origin);
	VectorCopy(level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	memset(ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups));

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.event = 0;
	ent->r.contents = 0;
}

void FindIntermissionPoint(void) {
	VectorClear(level.intermission_origin);
	level.intermission_origin[2] = 16384;
	VectorClear(level.intermission_angle);
}

/*
==================
BeginIntermission
==================
*/
static void BeginIntermission(void) {
	int i;
	gentity_t *client;

	if(level.intermissiontime) return; // already active

	level.intermissiontime = level.time;
	FindIntermissionPoint();

	// move all clients to the intermission point
	for(i = 0; i < level.maxclients; i++) {
		client = g_entities + i;
		if(!client->inuse) continue;
		// respawn if dead
		if(client->health <= 0) {
			ClientRespawn(client);
		}
		MoveClientToIntermission(client);
	}

	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();
}

/*
=============
LevelRestart
=============
*/
static void LevelRestart(void) {
	int i;
	gclient_t *cl;

	trap_Cmd(EXEC_APPEND, "map_restart\n");
	level.intermissiontime = 0;

	// reset all the scores so we don't enter the intermission again
	level.teamScores[TEAM_RED] = 0;
	level.teamScores[TEAM_BLUE] = 0;
	for(i = 0; i < MAX_CLIENTS; i++) {
		cl = level.clients + i;
		if(cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		cl->ps.persistant[PERS_SCORE] = 0;
	}

	// we need to do this here before chaning to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for(i = 0; i < MAX_CLIENTS; i++) {
		if(level.clients[i].pers.connected == CON_CONNECTED) {
			level.clients[i].pers.connected = CON_CONNECTING;
		}
	}
}

/*
================
LevelExit

End game when fraglimit or timelimit or capturelimit reached.
================
*/
static void LevelExit(void) {
	int i, numSorted;
	gclient_t *cl;
	qboolean won = qtrue;

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap_SetConfigstring(CS_INTERMISSION, "1");

	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedClients;
	if(numSorted > 32) {
		numSorted = 32;
	}

	for(i = 0; i < numSorted; i++) {
		int ping;

		cl = &level.clients[level.sortedClients[i]];

		if(cl->sess.sessionTeam == TEAM_SPECTATOR) {
			continue;
		}
		if(cl->pers.connected == CON_CONNECTING) {
			continue;
		}

		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
	}
}

/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
0 seconds before going on.
=================
*/
static void CheckIntermissionExit(void) {
	int ready, notReady, playerCount;
	int i;
	gclient_t *cl;
	int readyMask;

	// see which players are ready
	ready = 0;
	notReady = 0;
	readyMask = 0;
	playerCount = 0;
	for(i = 0; i < MAX_CLIENTS; i++) {
		cl = level.clients + i;
		if(cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		if(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) {
			continue;
		}

		playerCount++;
		if(cl->readyToExit) {
			ready++;
			if(i < 16) {
				readyMask |= 1 << i;
			}
		} else {
			notReady++;
		}
	}

	// copy the readyMask to each player's stats so
	// it can be displayed on the scoreboard
	for(i = 0; i < MAX_CLIENTS; i++) {
		cl = level.clients + i;
		if(cl->pers.connected != CON_CONNECTED) {
			continue;
		}
	}

	// never exit in less than five seconds
	if(level.time < level.intermissiontime + 5000) {
		return;
	}

	// only test ready status when there are real players present
	if(playerCount > 0) {
		// if nobody wants to go, clear timer
		if(!ready) {
			level.readyToExit = qfalse;
			return;
		}

		// if everyone wants to go, go now
		if(!notReady) {
			LevelRestart();
			return;
		}
	}

	// the first person to ready starts the ten second timeout
	if(!level.readyToExit) {
		level.readyToExit = qtrue;
		level.exitTime = level.time;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if(level.time < level.exitTime + 10000) {
		return;
	}

	LevelRestart();
}

/*
=============
ScoreIsTied
=============
*/
static qboolean ScoreIsTied(void) {
	int a, b;

	if(level.numPlayingClients < 2) {
		return qfalse;
	}

	if(cvarInt("g_gametype") >= GT_TEAM) {
		return level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE];
	}

	a = level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE];
	b = level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE];

	return a == b;
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
static void CheckExitRules(void) {
	int i;
	gclient_t *cl;
	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if(level.intermissiontime) {
		CheckIntermissionExit();
		return;
	} else {
		// sago: Find the reason for this to be neccesary.
		for(i = 0; i < MAX_CLIENTS; i++) {
			cl = level.clients + i;
			if(cl->pers.connected != CON_CONNECTED) {
				continue;
			}
		}
	}

	if(level.intermissionQueued) {
		if(level.time - level.intermissionQueued >= INTERMISSION_DELAY_TIME) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}
		return;
	}

	// check for sudden death
	if(ScoreIsTied()) {
		// always wait for sudden death
		return;
	}

	if(cvarInt("g_timelimit")) {
		if(level.time - level.startTime >= cvarInt("g_timelimit") * 60000) {
			trap_SendServerCommand(-1, "print \"Timelimit hit.\n\"");
			LevelExit();
			return;
		}
	}

	if(level.numPlayingClients < 2) {
		return;
	}

	if(cvarInt("g_gametype") <= GT_TEAM && cvarInt("g_fraglimit")) {
		if(level.teamScores[TEAM_RED] >= cvarInt("g_fraglimit")) {
			trap_SendServerCommand(-1, "print \"Red hit the fraglimit.\n\"");
			LevelExit();
			return;
		}

		if(level.teamScores[TEAM_BLUE] >= cvarInt("g_fraglimit")) {
			trap_SendServerCommand(-1, "print \"Blue hit the fraglimit.\n\"");
			LevelExit();
			return;
		}

		for(i = 0; i < MAX_CLIENTS; i++) {
			cl = level.clients + i;
			if(cl->pers.connected != CON_CONNECTED) {
				continue;
			}
			if(cl->sess.sessionTeam != TEAM_FREE) {
				continue;
			}

			if(cl->ps.persistant[PERS_SCORE] >= cvarInt("g_fraglimit")) {
				LevelExit();
				trap_SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " hit the fraglimit.\n\"", cl->pers.netname));
				return;
			}
		}
	}
}

static void PrintTeam(int team, char *message) {
	int i;

	for(i = 0; i < level.maxclients; i++) {
		if(level.clients[i].sess.sessionTeam != team) continue;
		trap_SendServerCommand(i, message);
	}
}

void SetLeader(int team, int client) {
	if(level.clients[client].pers.connected == CON_DISCONNECTED) {
		PrintTeam(team, va("print \"%s is not connected\n\"", level.clients[client].pers.netname));
		return;
	}
	if(level.clients[client].sess.sessionTeam != team) {
		PrintTeam(team, va("print \"%s is not on the team anymore\n\"", level.clients[client].pers.netname));
		return;
	}

	ClientUserinfoChanged(client);
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink(gentity_t *ent) {
	float thinktime;

	thinktime = ent->nextthink;
	if(thinktime <= 0) {
		return;
	}
	if(thinktime > level.time) {
		return;
	}

	ent->nextthink = 0;
	if(!ent->think) {
		G_Error("NULL ent->think");
	}
	ent->think(ent);
}

/*
================
G_UpdateGameCvars

Sends game cvars to client
================
*/
static void G_UpdateGameCvars(void) {
	mod_jumpheight = cvarInt("g_jumpheight");
	mod_gravity = cvarFloat("g_gravity");
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
static void G_RunFrame(int levelTime) {
	int i;
	gentity_t *ent;
	int start, end;

	// if we are waiting for the level to restart, do nothing
	if(level.restarted) return;

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;

	// go through all allocated objects
	start = trap_Milliseconds();
	ent = &g_entities[0];
	for(i = 0; i < level.num_entities; i++, ent++) {
		if(!ent->inuse) continue;

		// clear events that are too old
		if(level.time - ent->eventTime > EVENT_VALID_MSEC) {
			if(ent->s.event) {
				ent->s.event = 0; // &= EV_EVENT_BITS;
				if(ent->client) {
					ent->client->ps.externalEvent = 0;
				}
			}
			if(ent->freeAfterEvent) {
				// tempEntities or dropped items completely go away after their event
				G_FreeEntity(ent);
				continue;
			} else if(ent->unlinkAfterEvent) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				trap_UnlinkEntity(ent);
			}
		}

		if(!ent->r.linked && ent->neverFree) continue;

		if(ent->s.eType == ET_ITEM && !ent->sandboxObject || ent->physicsObject && !ent->sandboxObject) {
			G_RunItem(ent);
			continue;
		}

		if(ent->s.eType == ET_ITEM && ent->sandboxObject || ent->sandboxObject) {
			Phys_Frame(ent);
			continue;
		}

		if(i < MAX_CLIENTS) {
			G_RunClient(ent);
			continue;
		}

		if(ent->s.eType == ET_MISSILE) {
			G_RunMissile(ent);
		}

		G_RunThink(ent);
	}

	end = trap_Milliseconds();

	start = trap_Milliseconds();
	// perform final fixups on the players
	ent = &g_entities[0];
	for(i = 0; i < level.maxclients; i++, ent++) {
		if(ent->inuse) {
			ClientEndFrame(ent);
		}
	}
	end = trap_Milliseconds();

	// see if it is time to end the level
	CheckExitRules();

	G_UpdateGameCvars();

	level.frameStartTime = trap_Milliseconds();
}
