// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

static vec3_t playerMins = {-15, -15, -24};
static vec3_t playerMaxs = {15, 15, 32};

#define MAX_SPAWN_POINTS 128

void SP_info_player_deathmatch(gentity_t *ent) {
	int i;

	G_SpawnInt("nobots", "0", &i);
	if(i) ent->flags |= FL_NO_BOTS;
	G_SpawnInt("nohumans", "0", &i);
	if(i) ent->flags |= FL_NO_HUMANS;
}

void InitBodyQue(void) {
	int i;
	gentity_t *ent;

	level.bodyQueIndex = 0;
	for(i = 0; i < BODY_QUEUE_SIZE; i++) {
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

static void BodySink(gentity_t *ent) {
	if(level.time - ent->timestamp > 6500) {
		trap_UnlinkEntity(ent);
		ent->physicsObject = qfalse;
		return;
	}
	ent->nextthink = level.time + 100;
	ent->s.pos.trBase[2] -= 1;
}

/*
=============
CopyToBodyQue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
void CopyToBodyQue(gentity_t *ent) {
	gentity_t *body;
	int contents;

	trap_UnlinkEntity(ent);

	// if client is in a nodrop area, don't leave the body
	contents = trap_PointContents(ent->s.origin, -1);
	if(contents & CONTENTS_NODROP) return;

	// grab a body que and cycle to the next one
	body = level.bodyQue[level.bodyQueIndex];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;

	trap_UnlinkEntity(body);

	body->s = ent->s;
	body->s.eFlags = EF_DEAD; // clear EF_TALK, etc
	body->s.powerups = 0;  // clear powerups
	body->s.loopSound = 0; // clear lava burning
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->phys_bounce = 0; // don't bounce
	if(body->s.groundEntityNum == ENTITYNUM_NONE) {
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy(ent->client->ps.velocity, body->s.pos.trDelta);
	} else {
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.event = 0;

	body->r.svFlags = ent->r.svFlags;
	VectorCopy(ent->r.mins, body->r.mins);
	VectorCopy(ent->r.maxs, body->r.maxs);
	VectorCopy(ent->r.absmin, body->r.absmin);
	VectorCopy(ent->r.absmax, body->r.absmax);

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	body->nextthink = level.time + 5000;
	body->think = BodySink;

	body->die = body_die;

	// don't take more damage if already gibbed
	if(ent->health <= GIB_HEALTH) {
		body->takedamage = qfalse;
	} else {
		body->takedamage = qtrue;
	}

	VectorCopy(body->s.pos.trBase, body->r.currentOrigin);
	trap_LinkEntity(body);
}

void SetClientViewAngle(gentity_t *ent, vec3_t angle) {
	int i;

	// set the delta angle
	for(i = 0; i < 3; i++) {
		int cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy(angle, ent->s.angles);
	VectorCopy(ent->s.angles, ent->client->ps.viewangles);
}

void ClientRespawn(gentity_t *ent) {
	gentity_t *tent;

	if(ent->npcType > NT_PLAYER) {
		DropClientSilently(ent->client->ps.clientNum);
		return;
	}

	CopyToBodyQue(ent);
	ClientSpawn(ent);

	// add a teleportation effect
	tent = G_TempEntity(ent->client->ps.origin, EV_PLAYER_TELEPORT_IN);
	tent->s.clientNum = ent->s.clientNum;
}

/*
================
TeamCount

Returns number of players on a team
================
*/
static team_t TeamCount(int ignoreClientNum, int team) {
	int i;
	int count = 0;

	for(i = 0; i < level.maxclients; i++) {
		if(i == ignoreClientNum) continue;
		if(level.clients[i].pers.connected == CON_DISCONNECTED) continue;
		if(level.clients[i].pers.connected == CON_CONNECTING) continue;
		if(level.clients[i].sess.sessionTeam == team) count++;
	}

	return count;
}

team_t PickTeam(int ignoreClientNum) {
	int counts[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = TeamCount(ignoreClientNum, TEAM_BLUE);
	counts[TEAM_RED] = TeamCount(ignoreClientNum, TEAM_RED);

	if(counts[TEAM_BLUE] > counts[TEAM_RED]) return TEAM_RED;
	if(counts[TEAM_RED] > counts[TEAM_BLUE]) return TEAM_BLUE;
	if(level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED]) return TEAM_RED;
	return TEAM_BLUE;
}

/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo if desired.
============
*/
void ClientUserinfoChanged(int clientNum) {
	gentity_t *ent;
	int team;
	int npcType;
	char *s;
	char playerskin[MAX_QPATH];
	gclient_t *client;
	char userinfo[MAX_INFO_STRING];

	ent = g_entities + clientNum;
	client = ent->client;

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	// check for malformed or illegal info strings
	if(!Info_Validate(userinfo)) strcpy(userinfo, "\\name\\badinfo");

	// set name
	s = Info_ValueForKey(userinfo, "name");
	if(strlen(s) > 0) {
		strcpy(client->pers.netname, s);
	} else {
		strcpy(client->pers.netname, "noname");
	}

	ent->tool_id = atoi(Info_ValueForKey(userinfo, "toolgun_tool"));
	ent->tool_entity = NULL;

	Q_StringCopy(playerskin, Info_ValueForKey(userinfo, "playerskin"), sizeof(playerskin));

	npcType = atoi(Info_ValueForKey(userinfo, "npcType"));
	if(npcType && ent->r.svFlags & SVF_BOT) {
		ent->npcType = npcType;
	} else {
		ent->npcType = NT_PLAYER;
	}

	// set team
	if(cvarInt("g_gametype") >= GT_TEAM && g_entities[clientNum].r.svFlags & SVF_BOT) {
		s = Info_ValueForKey(userinfo, "team");
		if(!Q_stricmp(s, "red") || !Q_stricmp(s, "r")) {
			team = TEAM_RED;
		} else if(!Q_stricmp(s, "blue") || !Q_stricmp(s, "b")) {
			team = TEAM_BLUE;
		} else if(!Q_stricmp(s, "free") && ent->npcType > NT_PLAYER) { // FREE_TEAM
			team = TEAM_FREE;
		} else {
			// pick the team with the least number of players
			team = PickTeam(clientNum);
		}
		client->sess.sessionTeam = team;
	} else {
		team = client->sess.sessionTeam;
	}

	if(ent->r.svFlags & SVF_BOT) {
		s = va("n\\%s\\t\\%i\\p\\%s\\v\\%i\\i\\%i", client->pers.netname, team, playerskin, client->vehicleNum, ent->npcType);
	} else {
		s = va("n\\%s\\t\\%i\\p\\%s\\v\\%i\\i\\%i\\f\\%i", client->pers.netname, client->sess.sessionTeam, playerskin, client->vehicleNum, ent->npcType, ent->flashlight);
	}

	trap_SetConfigstring(CS_PLAYERS + clientNum, s);
}

/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change.
============
*/
char *ClientConnect(int clientNum, qboolean firstTime, qboolean isBot) {
	char *value;
	gclient_t *client;
	char userinfo[MAX_INFO_STRING];
	gentity_t *ent;

	if(clientNum >= cvarInt("g_maxClients")) return "Server is full, increase g_maxClients.";

	ent = &g_entities[clientNum];

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	if(!isBot) {
		// check for a password
		value = Info_ValueForKey(userinfo, "password");
		if(cvarString("g_password")[0] && Q_stricmp(cvarString("g_password"), "none") && strcmp(cvarString("g_password"), value) != 0) {
			return "Invalid password";
		}
	}

	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;

	memset(client, 0, sizeof(*client));

	client->pers.connected = CON_CONNECTING;

	// read or initialize the session data
	if(firstTime || level.newSession) G_InitSessionData(client, userinfo);
	G_ReadSessionData(client);

	if(isBot) {
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if(!G_BotConnect(clientNum)) return "BotConnectfailed";

		SandboxBotSpawn(ent, Info_ValueForKey(userinfo, "spawnid"));
	}

	ClientUserinfoChanged(clientNum);

	// don't do the "xxx connected" messages if they were caried over from previous level
	if(firstTime && (ent->npcType <= NT_PLAYER)) trap_SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " connected\n\"", client->pers.netname));

	if(cvarInt("g_gametype") >= GT_TEAM && client->sess.sessionTeam != TEAM_SPECTATOR) BroadcastTeamChange(client, -1);

	// count current clients and rank for scoreboard
	CalculateRanks();

	return NULL;
}

void ClientBegin(int clientNum) {
	gentity_t *ent;
	gclient_t *client;
	gentity_t *tent;
	int flags;

	ent = g_entities + clientNum;

	client = level.clients + clientNum;

	if(ent->r.linked) trap_UnlinkEntity(ent);
	G_InitGentity(ent);
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	client->pers.connected = CON_CONNECTED;
	client->pers.teamState.state = TEAM_BEGIN;

	flags = client->ps.eFlags;
	memset(&client->ps, 0, sizeof(client->ps));
	client->ps.eFlags = flags;

	// locate ent at a spawn point
	ClientSpawn(ent);

	if(client->sess.sessionTeam != TEAM_SPECTATOR) {
		// send event
		tent = G_TempEntity(ent->client->ps.origin, EV_PLAYER_TELEPORT_IN);
		tent->s.clientNum = ent->s.clientNum;

		trap_SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " entered the game\n\"", client->pers.netname));
	}

	// count current clients and rank for scoreboard
	CalculateRanks();
}

static void SetupCustomBot(gentity_t *bot) {
	if(!bot->botspawn) return;

	// give bot weapons
	if(bot->botspawn->weapon <= 1) {
		Set_Weapon(bot, WP_GAUNTLET, WS_HAVE);
	}
	Set_Weapon(bot, bot->botspawn->weapon, WS_HAVE);
	if(bot->swep_ammo[bot->botspawn->weapon] != -1) {
		Set_Ammo(bot, bot->botspawn->weapon, 9999);
	}

	bot->health = bot->client->ps.stats[STAT_HEALTH] = bot->botspawn->health;

	CopyAlloc(bot->target, bot->botspawn->target); // noire.dev bot->target
}

void SetUnlimitedWeapons(gentity_t *ent) {
	Set_Ammo(ent, WP_GAUNTLET, -1);
	Set_Ammo(ent, WP_TOOLGUN, -1);
	Set_Ammo(ent, WP_PHYSGUN, -1);
	Set_Ammo(ent, WP_GRAVITYGUN, -1);
}

static void SetSandboxWeapons(gentity_t *ent) {
	if(cvarInt("g_gametype") == GT_SANDBOX) {
		Set_Weapon(ent, WP_TOOLGUN, WS_HAVE);
		Set_Weapon(ent, WP_PHYSGUN, WS_HAVE);
		Set_Weapon(ent, WP_GRAVITYGUN, WS_HAVE);
	}
}

static void SetCustomWeapons(gentity_t *ent) {
	int i;
	Set_Ammo(ent, WP_GAUNTLET, -1);
	Set_Ammo(ent, WP_TOOLGUN, -1);
	Set_Ammo(ent, WP_PHYSGUN, -1);
	Set_Ammo(ent, WP_GRAVITYGUN, -1);
	Set_Weapon(ent, WP_GAUNTLET, WS_HAVE);
	Set_Weapon(ent, WP_MACHINEGUN, WS_HAVE);
	Set_Ammo(ent, WP_MACHINEGUN, 100);

	ent->health = ent->client->ps.stats[STAT_ARMOR] = 0;
	ent->health = ent->client->ps.stats[STAT_HEALTH] = 100;
	// Set spawnweapon
	if(cvarInt("g_gametype") == GT_SANDBOX) {
		ent->s.weapon = WP_PHYSGUN;
		ent->client->ps.weapon = WP_PHYSGUN;
		ClientUserinfoChanged(ent->s.clientNum);
		return;
	} else {
		for(i = WEAPONS_NUM; i > 1; i--) {
			if(ent->swep_list[i] == WS_HAVE) {
				ent->s.weapon = i;
				ent->client->ps.weapon = i;
				ClientUserinfoChanged(ent->s.clientNum);
				return;
			}
		}
		ent->s.weapon = 1;
		ent->client->ps.weapon = 1;
		ClientUserinfoChanged(ent->s.clientNum);
	}
}

void ClientSpawn(gentity_t *ent) {
	int index;
	gclient_t *client;
	int i;
	clientPersistant_t saved;
	clientSession_t savedSess;
	int persistant[MAX_PERSISTANT];
	gentity_t *spawnPoint;
	int flags;
	int savedPing;
	int eventSequence;
	char userinfo[MAX_INFO_STRING];

	index = ent - g_entities;
	client = ent->client;

	// find a spawn point
	if(ent->botspawn) spawnPoint = ent->botspawn;
	else if(cvarInt("g_gametype") >= GT_TEAM) spawnPoint = FindRandomTeamSpawn(client->sess.sessionTeam);
	else spawnPoint = FindRandomSpawn();
	client->pers.teamState.state = TEAM_ACTIVE;

	// toggle the teleport bit so the client knows to not lerp
	flags = ent->client->ps.eFlags & (EF_TELEPORT_BIT);
	flags ^= EF_TELEPORT_BIT;

	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
	for(i = 0; i < MAX_PERSISTANT; i++) {
		persistant[i] = client->ps.persistant[i];
	}
	eventSequence = client->ps.eventSequence;

	memset(client, 0, sizeof(*client));

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;

	for(i = 0; i < MAX_PERSISTANT; i++) {
		client->ps.persistant[i] = persistant[i];
	}
	client->ps.eventSequence = eventSequence;
	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;
	client->airOutTime = level.time + 12000;

	trap_GetUserinfo(index, userinfo, sizeof(userinfo));
	// set max health
	client->pers.maxHealth = 100;
	// clear entity values
	client->ps.eFlags = flags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;

	VectorCopy(playerMins, ent->r.mins);
	VectorCopy(playerMaxs, ent->r.maxs);

	client->ps.clientNum = index;

	// health will count down towards max_health
	ent->health = client->ps.stats[STAT_HEALTH] = MAX_PLAYER_HEALTH;

	G_SetOrigin(ent, spawnPoint->s.origin);
	VectorCopy(spawnPoint->s.origin, client->ps.origin);

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	trap_GetUsercmd(client - level.clients, &ent->client->pers.cmd);
	SetClientViewAngle(ent, spawnPoint->s.angles);

	if(ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		G_KillBox(ent);
		trap_LinkEntity(ent);

		client->ps.weaponstate = WEAPON_READY;
		for(i = 1; i < WEAPONS_NUM; i++) {
			ent->swep_list[i] = WS_NONE;
			ent->swep_ammo[i] = 0;
		}
		SetUnlimitedWeapons(ent);
		SetSandboxWeapons(ent);
		if(ent->botspawn) {
			SetupCustomBot(ent);
		} else {
			SetCustomWeapons(ent);
		}
	}

	G_SendSpawnSwepWeapons(ent);

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->latched_buttons = 0;

	if(level.intermissiontime) {
		MoveClientToIntermission(ent);
	} else {
		G_UseTargets(spawnPoint, ent);
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink(ent - g_entities);

	// positively link the client, even if the command times are weird
	if(ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		BG_PlayerStateToEntityState(&client->ps, &ent->s, qtrue);
		VectorCopy(ent->client->ps.origin, ent->r.currentOrigin);
		trap_LinkEntity(ent);
	}

	// run the presend to set anything else
	ClientEndFrame(ent);

	// clear entity state values
	BG_PlayerStateToEntityState(&client->ps, &ent->s, qtrue);

	RespawnTimeMessage(ent, 0);

	G_SendGameCvars(ent);
}

void ClientDisconnect(int clientNum) {
	gentity_t *ent;
	int i;

	ent = g_entities + clientNum;
	if(!ent->client) return;

	// stop any following clients
	for(i = 0; i < level.maxclients; i++) {
		if(level.clients[i].sess.sessionTeam == TEAM_SPECTATOR && level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW && level.clients[i].sess.spectatorClient == clientNum) {
			StopFollowing(&g_entities[i]);
		}
	}

	// send effect if they were completely connected
	if(ent->client->pers.connected == CON_CONNECTED && ent->client->sess.sessionTeam != TEAM_SPECTATOR) TossClientItems(ent);

	trap_UnlinkEntity(ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.sessionTeam = TEAM_FREE;

	trap_SetConfigstring(CS_PLAYERS + clientNum, "");

	CalculateRanks();

	if(ent->r.svFlags & SVF_BOT) BotAIShutdownClient(clientNum, qfalse);
}

void DropClientSilently(int clientNum) {
	if(g_entities[clientNum].npcType > NT_PLAYER) trap_DropClient(clientNum, "DR_SILENT_DROP");
}

qboolean OnSameTeam(gentity_t *ent1, gentity_t *ent2) {
	if(!ent1->client || !ent2->client) return qfalse;
	if(cvarInt("g_gametype") < GT_TEAM) return qfalse;
	if(ent1->client->sess.sessionTeam == ent2->client->sess.sessionTeam) return qtrue;
	return qfalse;
}
