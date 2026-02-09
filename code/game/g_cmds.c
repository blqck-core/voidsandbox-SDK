// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

void DeathmatchScoreboardMessage(gentity_t *ent) {
	char entry[64];
	char string[1024];
	int stringlength;
	int i, j;
	gclient_t *cl;
	gentity_t *client;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;

	for(i = 0; i < level.numConnectedClients; i++) {

		cl = &level.clients[level.sortedClients[i]];
		client = g_entities + cl->ps.clientNum;

		if(client->npcType > NT_PLAYER) continue;

		Com_sprintf(entry, sizeof(entry), " %i %i", level.sortedClients[i], cl->ps.persistant[PERS_SCORE]);
		j = strlen(entry);
		if(stringlength + j >= sizeof(string)) break;
		strcpy(string + stringlength, entry);
		stringlength += j;
	}

	trap_SendServerCommand(ent - g_entities, va("scores %i %i %i%s", i, level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE], string));
}

void G_SendGameCvars(gentity_t *ent) {
	char string[4096];
	Com_sprintf(string, sizeof(string), "%i %i", mod_jumpheight, mod_gravity);
	trap_SendServerCommand(ent - g_entities, va("gCvars %s", string));
}

void G_SendSwepWeapons(gentity_t *ent) {
	char string[4096] = "";
	int i;
	int len;

	for(i = 1; i < WEAPONS_NUM; i++) {
		if(ent->swep_list[i] >= WS_HAVE) {
			if(ent->swep_ammo[i] > 0 || ent->swep_ammo[i] == -1) {
				ent->swep_list[i] = WS_HAVE; // we have weapon and ammo
			} else {
				ent->swep_list[i] = WS_NOAMMO; // we have weapon only
			}
		}
		if(ent->swep_list[i] == WS_HAVE) {
			Q_strcat(string, sizeof(string), va("%i ", i));
		}
		if(ent->swep_list[i] == WS_NOAMMO) {
			Q_strcat(string, sizeof(string), va("%i ", i * -1)); // use -id for send WS_NOAMMO
		}
	}
	len = strlen(string);
	if(len > 0 && string[len - 1] == ' ') {
		string[len - 1] = '\0';
	}

	trap_SendServerCommand(ent - g_entities, va("sweps %s", string));
}

void G_SendSpawnSwepWeapons(gentity_t *ent) {
	char string[4096] = "";
	int i;
	int len;

	for(i = 1; i < WEAPONS_NUM; i++) {
		if(ent->swep_list[i] >= WS_HAVE) {
			if(ent->swep_ammo[i] > 0 || ent->swep_ammo[i] == -1) {
				ent->swep_list[i] = WS_HAVE; // we have weapon and ammo
			} else {
				ent->swep_list[i] = WS_NOAMMO; // we have weapon only
			}
		}
		if(ent->swep_list[i] == WS_HAVE) {
			Q_strcat(string, sizeof(string), va("%i ", i));
		}
		if(ent->swep_list[i] == WS_NOAMMO) {
			Q_strcat(string, sizeof(string), va("%i ", i * -1)); // use -id for send WS_NOAMMO
		}
	}
	len = strlen(string);
	if(len > 0 && string[len - 1] == ' ') {
		string[len - 1] = '\0';
	}

	trap_SendServerCommand(ent - g_entities, va("wpspawn %s", string));
	ClientUserinfoChanged(ent->s.clientNum);
}

void RespawnTimeMessage(gentity_t *ent, int time) { trap_SendServerCommand(ent - g_entities, va("respawn %i", time)); }

void Cmd_Score_f(gentity_t *ent) { DeathmatchScoreboardMessage(ent); }

static qboolean CheatsOk(gentity_t *ent) {
	if(!cvarInt("sv_cheats")) {
		trap_SendServerCommand(ent - g_entities, va("print \"Cheats are not enabled on this server.\n\""));
		return qfalse;
	}
	return qtrue;
}

char *ConcatArgs(int start) {
	int i, c, tlen;
	static char line[MAX_STRING_CHARS];
	int len;
	char arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for(i = start; i < c; i++) {
		trap_Argv(i, arg, sizeof(arg));
		tlen = strlen(arg);
		if(len + tlen >= MAX_STRING_CHARS - 1) {
			break;
		}
		memcpy(line + len, arg, tlen);
		len += tlen;
		if(i != c - 1) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

static void Cmd_Give_f(gentity_t *ent) {
	char *name;
	item_t *it;
	int i;
	qboolean give_all;
	gentity_t *it_ent;
	trace_t trace;

	if(cvarInt("g_gametype") != GT_SANDBOX && !CheatsOk(ent)) return;

	name = ConcatArgs(1);

	if(Q_strequal(name, "all")) give_all = qtrue;
	else give_all = qfalse;

	if(give_all) {
		for(i = 1; i < WEAPONS_NUM; i++) {
			ent->swep_list[i] = WS_HAVE;
			ent->swep_ammo[i] = 9999;
		}
		SetUnlimitedWeapons(ent);
	}

	// spawn a specific item right on the player
	if(!give_all) {
		it = BG_FindItem(name);
		if(!it) return;

		it_ent = G_Spawn();
		VectorCopy(ent->r.currentOrigin, it_ent->s.origin);
		it_ent->classname = it->classname;
		G_SpawnItem(it_ent, it);
		FinishSpawningItem(it_ent);
		memset(&trace, 0, sizeof(trace));
		Touch_Item(it_ent, ent, &trace);
		if(it_ent->inuse) G_FreeEntity(it_ent);
	}
}

static void Cmd_VehicleExit_f(gentity_t *ent) {
	if(ent->client->vehicleNum) {
		ent->client->vehicleNum = 0;
		ClientUserinfoChanged(ent->s.clientNum);
	}
}

static void Cmd_God_f(gentity_t *ent) {
	char *msg;

	if(!CheatsOk(ent)) return;

	ent->flags ^= FL_GODMODE;
	if(!(ent->flags & FL_GODMODE))
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	trap_SendServerCommand(ent - g_entities, va("print \"%s\"", msg));
}

static void Cmd_Noclip_f(gentity_t *ent) {
	if(cvarInt("g_gametype") != GT_SANDBOX && !CheatsOk(ent)) return;

	ent->client->noclip = !ent->client->noclip;
}

static void Cmd_Kill_f(gentity_t *ent) {
	if(ent->client->sess.sessionTeam == TEAM_SPECTATOR) return;
	if(ent->health <= 0) return;
	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die(ent, ent, ent, 100000, MOD_SUICIDE);
}

void BroadcastTeamChange(gclient_t *client, int oldTeam) {
	if(client->sess.sessionTeam == TEAM_RED) {
		trap_SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " joined the red team.\n\"", client->pers.netname));
	} else if(client->sess.sessionTeam == TEAM_BLUE) {
		trap_SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " joined the blue team.\n\"", client->pers.netname));
	} else if(client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR) {
		trap_SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " joined the spectators.\n\"", client->pers.netname));
	} else if(client->sess.sessionTeam == TEAM_FREE) {
		trap_SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " joined the battle.\n\"", client->pers.netname));
	}
}

void SetTeam(gentity_t *ent, char *s) {
	int team, oldTeam;
	gclient_t *client;
	int clientNum;
	spectatorState_t specState;
	int specClient;

	// see what change is requested
	client = ent->client;

	clientNum = client - level.clients;
	specClient = 0;
	specState = SPECTATOR_NOT;
	if(!Q_stricmp(s, "scoreboard") || !Q_stricmp(s, "score")) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	} else if(!Q_stricmp(s, "follow1")) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	} else if(!Q_stricmp(s, "follow2")) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	} else if(!Q_stricmp(s, "spectator") || !Q_stricmp(s, "s")) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	} else if(cvarInt("g_gametype") >= GT_TEAM) {
		// if running a team game, assign player to one of the teams
		specState = SPECTATOR_NOT;
		if(!Q_stricmp(s, "red") || !Q_stricmp(s, "r")) {
			team = TEAM_RED;
		} else if(!Q_stricmp(s, "blue") || !Q_stricmp(s, "b")) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam(clientNum);
		}
	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}

	// decide if we will allow the change
	oldTeam = client->sess.sessionTeam;
	if(team == oldTeam && team != TEAM_SPECTATOR) return;

	// if the player was dead leave the body
	if(client->ps.stats[STAT_HEALTH] <= 0) CopyToBodyQue(ent);

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if(oldTeam != TEAM_SPECTATOR) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die(ent, ent, ent, 100000, MOD_SUICIDE);
	}

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	BroadcastTeamChange(client, oldTeam);

	// get and distribute relevent paramters
	ClientUserinfoChanged(clientNum);

	ClientBegin(clientNum);
}

void StopFollowing(gentity_t *ent) {
	ent->client->ps.persistant[PERS_TEAM] = TEAM_SPECTATOR;
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
}

static void Cmd_Team_f(gentity_t *ent) {
	char s[MAX_TOKEN_CHARS];

	if(trap_Argc() != 2) {
		trap_SendServerCommand(ent - g_entities, "print \"Enter team name.\n\"");
		return;
	}

	trap_Argv(1, s, sizeof(s));

	SetTeam(ent, s);
}

void Cmd_FollowCycle_f(gentity_t *ent, int dir) {
	int clientnum;
	int original;
	gentity_t *checkNPC;

	// first set them to spectator
	if(ent->client->sess.spectatorState == SPECTATOR_NOT) SetTeam(ent, "spectator");

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;
	checkNPC = g_entities + clientnum;
	do {
		clientnum += dir;

		if(clientnum >= level.maxclients) clientnum = 0;

		if(clientnum < 0) clientnum = level.maxclients - 1;

		// can only follow connected clients
		if(level.clients[clientnum].pers.connected != CON_CONNECTED) continue;

		// can't follow another spectator
		if(level.clients[clientnum].sess.sessionTeam == TEAM_SPECTATOR) continue;

		// can't follow npc
		if(checkNPC->npcType > NT_PLAYER) continue;

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while(clientnum != original);
}

static void Cmd_RunFollowNext_f(gentity_t *ent) { Cmd_FollowCycle_f(ent, 1); }

static void Cmd_RunFollowPrev_f(gentity_t *ent) { Cmd_FollowCycle_f(ent, -1); }

static void G_SayTo(gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message) {
	if(!other) return;
	if(!other->inuse) return;
	if(!other->client) return;
	if(other->client->pers.connected != CON_CONNECTED) return;
	if(mode == SAY_TEAM && (cvarInt("g_gametype") != GT_SANDBOX && !OnSameTeam(ent, other))) return;

	trap_SendServerCommand(other - g_entities, va("%s \"%s%c%c%s\"", mode == SAY_TEAM ? "tchat" : "chat", name, Q_COLOR_ESCAPE, color, message));
}

#define EC "\x19"

static void G_Say(gentity_t *ent, gentity_t *target, int mode, const char *chatText) {
	int j;
	gentity_t *other;
	int color;
	char name[64];
	// don't let text be too long for malicious reasons
	char text[MAX_SAY_TEXT];

	switch(mode) {
	default:
	case SAY_ALL:
		Com_sprintf(name, sizeof(name), "%s%c%c" EC ": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		Com_sprintf(name, sizeof(name), EC "(%s%c%c" EC ")" EC ": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		Com_sprintf(name, sizeof(name), EC "[%s%c%c" EC "]" EC ": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		color = COLOR_MAGENTA;
		break;
	}

	Q_StringCopy(text, chatText, sizeof(text));

	if(target) {
		G_SayTo(ent, target, mode, color, name, text);
		return;
	}

	// echo the text to the console
	if(cvarInt("g_dedicated")) G_Printf("%s%s\n", name, text);

	// send it to all the apropriate clients
	for(j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_SayTo(ent, other, mode, color, name, text);
	}
}

static void Cmd_Say_f(gentity_t *ent, int mode, qboolean arg0) {
	char *p;

	if(trap_Argc() < 2 && !arg0) return;

	if(!gameInfoNPCTypes[ent->npcType].canChat) return;

	if(arg0) {
		p = ConcatArgs(0);
	} else {
		p = ConcatArgs(1);
	}

	G_Say(ent, NULL, mode, p);
}

static void Cmd_Tell_f(gentity_t *ent) {
	int targetNum;
	gentity_t *target;
	char *p;
	char arg[MAX_TOKEN_CHARS];

	if(trap_Argc() < 2) return;

	trap_Argv(1, arg, sizeof(arg));
	targetNum = atoi(arg);
	if(targetNum < 0 || targetNum >= level.maxclients) return;

	target = &g_entities[targetNum];
	if(!target || !target->inuse || !target->client) {
		return;
	}

	p = ConcatArgs(2);

	G_Say(ent, target, SAY_TELL, p);
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if(ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Say(ent, ent, SAY_TELL, p);
	}
}

static void Cmd_RunSay_f(gentity_t *ent) { Cmd_Say_f(ent, SAY_ALL, qfalse); }

static void Cmd_RunSayTeam_f(gentity_t *ent) { Cmd_Say_f(ent, SAY_TEAM, qfalse); }

static void Cmd_Weapon_f(gentity_t *ent) {
	char str[64];
	int id;
	
	trap_Argv(1, str, sizeof(str));
	id = atoi(str);

	if(id >= 0 && id < WEAPONS_NUM) ent->client->ps.weapon = id;
	G_Printf("Selected weapon: %i\n", id);
}

static void Cmd_SpawnList_Item_f(gentity_t *ent) {
	vec3_t end, start, forward, up, right;
	trace_t tr;
	gentity_t *tent;
	char arg01[64];
	char arg02[64];
	char arg03[64];
	char arg04[64];
	char arg05[64];
	char arg06[64];
	char arg07[64];
	char arg08[64];
	char arg09[64];
	char arg10[64];
	char arg11[64];
	char arg12[64];
	char arg13[64];
	char arg14[64];
	char arg15[64];
	char arg16[64];
	char arg17[64];
	char arg18[64];
	char arg19[64];
	char arg20[64];
	char arg21[64];
	char arg22[64];

	if(cvarInt("g_gametype") != GT_SANDBOX) return;

	if(ent->client->sess.sessionTeam == TEAM_SPECTATOR) return;

	// tr.endpos
	trap_Argv(1, arg01, sizeof(arg01));
	trap_Argv(2, arg02, sizeof(arg02));
	trap_Argv(3, arg03, sizeof(arg03));
	trap_Argv(4, arg04, sizeof(arg04));
	trap_Argv(5, arg05, sizeof(arg05));
	trap_Argv(6, arg06, sizeof(arg06));
	trap_Argv(7, arg07, sizeof(arg07));
	trap_Argv(8, arg08, sizeof(arg08));
	trap_Argv(9, arg09, sizeof(arg09));
	trap_Argv(10, arg10, sizeof(arg10));
	trap_Argv(11, arg11, sizeof(arg11));
	trap_Argv(12, arg12, sizeof(arg12));
	trap_Argv(13, arg13, sizeof(arg13));
	trap_Argv(14, arg14, sizeof(arg14));
	trap_Argv(15, arg15, sizeof(arg15));
	trap_Argv(16, arg16, sizeof(arg16));
	trap_Argv(17, arg17, sizeof(arg17));
	trap_Argv(18, arg18, sizeof(arg18));
	trap_Argv(19, arg19, sizeof(arg19));
	trap_Argv(20, arg20, sizeof(arg20));
	trap_Argv(21, arg21, sizeof(arg21));
	trap_Argv(22, arg22, sizeof(arg22));

	// Set Aiming Directions
	AngleVectors(ent->client->ps.viewangles, forward, right, up);
	CalcMuzzlePoint(ent, forward, right, up, start);
	VectorMA(start, gameInfoWeapons[WP_TOOLGUN].range, forward, end);

	// Trace Position
	trap_Trace(&tr, start, NULL, NULL, end, ent->s.number, MASK_SELECT);

	if(!Q_stricmp(arg01, "prop")) {
		tent = G_TempEntity(tr.endpos, EV_PARTICLES_GRAVITY);
		tent->s.constantLight = (((rand() % 256 | rand() % 256 << 8) | rand() % 256 << 16) | (255 << 24));
		tent->s.eventParm = 24; // eventParm is used to determine the number of particles
		tent->s.generic1 = 500; // generic1 is used to determine the speed of the particles
		tent->s.generic2 = 16;  // generic2 is used to determine the size of the particles
		G_BuildProp(arg02, arg03, tr.endpos, ent, arg04, arg05, arg06, arg07, arg08, arg09, arg10, arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18, arg19, arg20, arg21, arg22);
		return;
	}
	if(!Q_stricmp(arg01, "npc")) {
		if(level.numConnectedClients >= cvarInt("g_maxClients")) {
			G_Printf(S_COLOR_YELLOW "Server is full, increase g_maxClients.\n");
			return;
		}

		tent = G_Spawn();
		tent->sb_isnpc = 1;
		VectorCopy(tr.endpos, tent->s.origin);
		tent->s.origin[2] += 25;
		tent->classname = "sandbox_npc";
		CopyAlloc(tent->clientname, arg02);
		tent->type = BG_FindNPCTypeID(arg03);
		tent->skill = atof(arg04);
		tent->health = atoi(arg05);
		CopyAlloc(tent->message, arg06);
		tent->weapon = atoi(arg08);

		if(!Q_stricmp(arg07, "0")) {
			CopyAlloc(tent->target, arg02);
		} else {
			CopyAlloc(tent->target, arg07);
		}

		if(tent->health <= 0) tent->health = 100;

		if(tent->skill <= 0) tent->skill = 1;

		if(!Q_stricmp(tent->message, "0") || !tent->message) CopyAlloc(tent->message, tent->clientname);

		G_AddBot(tent->clientname, tent->message, "Blue", tent);

		if(tent->parent) Undo_AddElement(ent, tent->s.number);
		return;
	}
}

static void Cmd_Modify_Prop_f(gentity_t *ent) {
	vec3_t end, start, forward, up, right;
	trace_t tr;
	gentity_t *tent;
	gentity_t *traceEnt;
	char arg01[64];
	char arg02[64];
	char arg03[64];
	char arg04[64];
	char arg05[64];

	if(cvarInt("g_gametype") != GT_SANDBOX) return;

	if(ent->client->sess.sessionTeam == TEAM_SPECTATOR) return;

	// tr.endpos
	trap_Argv(1, arg01, sizeof(arg01));
	trap_Argv(2, arg02, sizeof(arg02));
	trap_Argv(3, arg03, sizeof(arg03));
	trap_Argv(4, arg04, sizeof(arg04));
	trap_Argv(5, arg05, sizeof(arg05));

	// Set Aiming Directions
	AngleVectors(ent->client->ps.viewangles, forward, right, up);
	CalcMuzzlePoint(ent, forward, right, up, start);
	VectorMA(start, gameInfoWeapons[WP_TOOLGUN].range, forward, end);

	// Trace Position
	trap_Trace(&tr, start, NULL, NULL, end, ent->s.number, MASK_SELECT);

	traceEnt = &g_entities[tr.entityNum]; // entity for modding

	if(!traceEnt->sandboxObject && traceEnt->npcType <= NT_PLAYER && traceEnt->s.eType != ET_ITEM) return;

	tent = G_TempEntity(tr.endpos, EV_PARTICLES_GRAVITY);
	tent->s.constantLight = (((rand() % 256 | rand() % 256 << 8) | rand() % 256 << 16) | (255 << 24));
	tent->s.eventParm = 24; // eventParm is used to determine the number of particles
	tent->s.generic1 = 125; // generic1 is used to determine the speed of the particles
	tent->s.generic2 = 3;   // generic2 is used to determine the size of the particles
	G_ModProp(traceEnt, ent, arg01, arg02, arg03, arg04, arg05);
	return;
}

static void Cmd_Altfire_Physgun_f(gentity_t *ent) {
	if(ent->client->ps.weapon == WP_PHYSGUN) {
		if(ent->client->buttons & BUTTON_ATTACK) {
			if(ent->grabbedEntity) {
				ent->grabbedEntity->grabNewPhys = PHYS_STATIC; // say physgun about freeze option
			}
		}
	}
}

static void Cmd_PhysgunDist_f(gentity_t *ent) {
	char mode[MAX_TOKEN_CHARS];

	trap_Argv(1, mode, sizeof(mode));

	if(ent->client->ps.weapon == WP_PHYSGUN) {
		if(ent->client->buttons & BUTTON_ATTACK) {
			if(ent->grabbedEntity) {
				if(atoi(mode) == 0) {
					ent->grabDist -= 20;
					if(ent->grabbedEntity->sb_coltype) {
						if(ent->grabDist < ent->grabbedEntity->sb_coltype + 1) ent->grabDist = ent->grabbedEntity->sb_coltype + 1;
					} else {
						if(ent->grabDist < 100) ent->grabDist = 100;
					}
				}
				if(atoi(mode) == 1) ent->grabDist += 20;
			}
		}
	}
}

static void Cmd_Flashlight_f(gentity_t *ent) {
	if(ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		return;
	}

	if(!ent->flashlight) {
		ent->flashlight = qtrue;
		ClientUserinfoChanged(ent->s.clientNum);
		return;
	}
	if(ent->flashlight) {
		ent->flashlight = qfalse;
		ClientUserinfoChanged(ent->s.clientNum);
		return;
	}
}

static void Cmd_Undo_f(gentity_t *ent) {
	int id;
	qboolean isRemoved;
	gentity_t *prop;

	if(cvarInt("g_gametype") != GT_SANDBOX) return;

	while(Undo_LastElement(ent, &id, &isRemoved)) {
		if(isRemoved) {
			Undo_RemoveElement(ent);
			continue;
		}

		prop = G_FindEntityForEntityNum(id);
		if(prop) {
			if(!strcmp(prop->sb_class, "sb.shooter"))
				trap_SendServerCommand(ent - g_entities, "undo \"Undone Shooter\"\n");
			else if(prop->objectType == OT_VEHICLE)
				trap_SendServerCommand(ent - g_entities, "undo \"Undone Vehicle\"\n");
			else if(!strcmp(prop->classname, "sandbox_npc"))
				trap_SendServerCommand(ent - g_entities, "undo \"Undone NPC\"\n");
			else if(!strcmp(prop->classname, "sandbox_prop"))
				trap_SendServerCommand(ent - g_entities, "undo \"Undone Prop\"\n");
			else if(prop->s.eType == ET_ITEM)
				trap_SendServerCommand(ent - g_entities, va("undo \"Undone %s\"\n", prop->item->pickup_name));
			G_FreeEntity(prop);
		}
		return;
	}
}

static void Cmd_Teleport_f(gentity_t *ent) {
	vec3_t origin;
	char buffer[MAX_TOKEN_CHARS];
	int i;

	if(!cvarInt("sv_cheats")) {
		trap_SendServerCommand(ent - g_entities, va("print \"Cheats are not enabled on this server.\n\""));
		return;
	}

	if(trap_Argc() != 4) {
		trap_SendServerCommand(ent - g_entities, va("print \"usage: setviewpos x y z\n\""));
		return;
	}

	for(i = 0; i < 3; i++) {
		trap_Argv(i + 1, buffer, sizeof(buffer));
		origin[i] = atof(buffer);
	}

	TeleportPlayer(ent, origin, ent->client->ps.viewangles, qtrue);
}

static void Cmd_ActivateTarget_f(gentity_t *ent) {
	char *p;
	char arg[MAX_TOKEN_CHARS];

	trap_Argv(0, arg, sizeof(arg));

	if(trap_Argc() < 2) return;

	p = ConcatArgs(1);

	ent->target = va("activate_%i_%s", ent->s.clientNum, p);

	G_PickAllTargets(ent);
}

// This is the table that ClientCommands runs the console entry against.
commands_t cmds[] = {
    // can be used even during intermission
    {"say", CMD_MESSAGE | CMD_INTERMISSION, Cmd_RunSay_f},
    {"say_team", CMD_MESSAGE | CMD_INTERMISSION, Cmd_RunSayTeam_f},
    {"tell", CMD_MESSAGE | CMD_INTERMISSION, Cmd_Tell_f},
    {"team", CMD_INTERMISSION, Cmd_Team_f},
    {"score", CMD_INTERMISSION, Cmd_Score_f},

    // cheats
    {"give", CMD_LIVING, Cmd_Give_f},
    {"god", CMD_CHEAT | CMD_LIVING, Cmd_God_f},
    {"tp", CMD_CHEAT, Cmd_Teleport_f},
    {"noclip", CMD_LIVING, Cmd_Noclip_f},

    // interaction
    {"exitvehicle", CMD_LIVING, Cmd_VehicleExit_f},
    {"kill", CMD_TEAM | CMD_LIVING, Cmd_Kill_f},
    {"flashlight", CMD_LIVING, Cmd_Flashlight_f},
    {"undo", CMD_LIVING, Cmd_Undo_f},
    {"activate", CMD_LIVING, Cmd_ActivateTarget_f},

    // internal
    {"wp", CMD_LIVING, Cmd_Weapon_f},
    {"sl", CMD_LIVING, Cmd_SpawnList_Item_f},
    {"tm", CMD_LIVING, Cmd_Modify_Prop_f},
    {"altfire_physgun", CMD_LIVING, Cmd_Altfire_Physgun_f},
    {"physgun_dist", CMD_LIVING, Cmd_PhysgunDist_f},

    // game commands
    {"follownext", CMD_NOTEAM, Cmd_RunFollowNext_f},
    {"followprev", CMD_NOTEAM, Cmd_RunFollowPrev_f},
    {"freespectator", CMD_NOTEAM, StopFollowing}};

static int numCmds = ARRAY_SIZE(cmds);

void ClientCommand(int clientNum) {
	gentity_t *ent;
	char cmd[MAX_TOKEN_CHARS];
	int i;

	ent = g_entities + clientNum;
	if(!ent->client) return;

	trap_Argv(0, cmd, sizeof(cmd));

	for(i = 0; i < numCmds; i++) {
		if(Q_stricmp(cmd, cmds[i].cmdName) == 0) break;
	}

	if(i == numCmds) {
		trap_SendServerCommand(clientNum, va("print \"Unknown command: %s\n\"", cmd));
		return;
	}

	if(!(cmds[i].cmdFlags & CMD_INTERMISSION) && level.intermissiontime) return;

	if(cmds[i].cmdFlags & CMD_CHEAT && !cvarInt("sv_cheats")) {
		trap_SendServerCommand(clientNum, "print \"Cheats are not enabled on this server\n\"");
		return;
	}

	if(cmds[i].cmdFlags & CMD_TEAM && ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		trap_SendServerCommand(clientNum, "print \"Join a team first\n\"");
		return;
	}

	if((cmds[i].cmdFlags & CMD_NOTEAM || (cmds[i].cmdFlags & CMD_CHEAT_TEAM && !cvarInt("sv_cheats"))) && ent->client->sess.sessionTeam != TEAM_NONE) {
		trap_SendServerCommand(clientNum, "print \"Cannot use this command when on a team\n\"");
		return;
	}

	if((ent->client->ps.pm_type == PM_DEAD) && (cmds[i].cmdFlags & CMD_LIVING)) {
		trap_SendServerCommand(clientNum, "print \"Must be alive to use this command\n\"");
		return;
	}

	cmds[i].cmdHandler(ent);
}
