// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

qboolean G_BotConnect(int clientNum) {
	bot_settings_t settings;
	char userinfo[MAX_INFO_STRING];

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	Q_StringCopy(settings.team, Info_ValueForKey(userinfo, "team"), sizeof(settings.team));

	if(!BotAISetupClient(clientNum, &settings)) {
		trap_DropClient(clientNum, "BotAISetupClient failed");
		return qfalse;
	}

	return qtrue;
}

static void G_BotSandboxCheck(gentity_t *self) {
	if(!self->parent || !self->parent->client || self->parent->client->pers.connected == CON_DISCONNECTED) {
		G_FreeEntity(self);
		return;
	}
	VectorCopy(self->parent->s.pos.trBase, self->s.origin);
	self->think = G_BotSandboxCheck;
	self->nextthink = level.time + 1;
}

void G_AddBot(char *model, char *name, char *team, gentity_t *spawn) {
	int clientNum;
	gentity_t *bot;
	char userinfo[MAX_INFO_STRING];

	// create the bot's userinfo
	userinfo[0] = '\0';

	if(!strlen(name)) name = model;

	Info_SetValueForKey(userinfo, "name", name);
	Info_SetValueForKey(userinfo, "rate", "25000");
	Info_SetValueForKey(userinfo, "snaps", "20");

	if(!*model) model = "sarge/default";
	Info_SetValueForKey(userinfo, "model", model);

	// dynamic limit
	if(level.numConnectedClients >= cvarInt("g_maxClients")) {
		G_Printf(S_COLOR_YELLOW "Server is full, increase g_maxClients.\n");
		return;
	}

	// have the server allocate a client slot
	clientNum = trap_BotAllocateClient();
	if(clientNum == -1) {
		G_Printf(S_COLOR_YELLOW "Server is full, increase engine limit.\n");
		return;
	}

	// initialize the bot settings
	if(!team || !*team) {
		if(cvarInt("g_gametype") >= GT_TEAM) {
			if(PickTeam(clientNum) == TEAM_RED) {
				team = "red";
			} else {
				team = "blue";
			}
		} else {
			team = "red";
		}
	}
	Info_SetValueForKey(userinfo, "team", team);

	bot = &g_entities[clientNum];
	bot->r.svFlags |= SVF_BOT;
	bot->inuse = qtrue;

	// sandbox npc spawn
	if(spawn) {
		Info_SetValueForKey(userinfo, "spawnid", va("%i", spawn->s.number));
		Info_SetValueForKey(userinfo, "npcType", va("%i", spawn->type));
		spawn->parent = bot;
		spawn->think = G_BotSandboxCheck;
		spawn->nextthink = level.time;
	} else {
		Info_SetValueForKey(userinfo, "spawnid", "");
		Info_SetValueForKey(userinfo, "npcType", "");
	}

	// register the userinfo
	trap_SetUserinfo(clientNum, userinfo);

	// have it connect to the game as a normal client
	if(ClientConnect(clientNum, qtrue, qtrue)) return;

	ClientBegin(clientNum);
}

void Svcmd_AddBot_f(void) {
	char model[MAX_TOKEN_CHARS];
	char name[MAX_TOKEN_CHARS];
	char team[MAX_TOKEN_CHARS];

	trap_Argv(1, model, sizeof(model));
	if(!model[0]) {
		trap_Print("Usage: addbot <model/skin> [name] [team]\n");
		return;
	}

	trap_Argv(2, name, sizeof(name));
	trap_Argv(3, team, sizeof(team));
	G_AddBot(model, name, team, NULL);
}

void SandboxBotSpawn(gentity_t *bot, char spawnid[]) {
	gentity_t *t;
	int entityNum;

	if(!bot || !spawnid) return;
	if(!strcmp(spawnid, "")) bot->botspawn = NULL;

	entityNum = atoi(spawnid);

	t = NULL;
	bot->botspawn = NULL;
	while((t = G_Find(t, FOFS(classname), "sandbox_npc")) != NULL) {
		if(t->s.number == entityNum) bot->botspawn = t;
	}
}
