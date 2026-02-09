// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2026 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../qcommon/js_local.h"

#define AI_THINKTIME 300

bot_state_t *botstates[MAX_CLIENTS];
int numbots;
float floattime;

static int BotPointAreaNum(vec3_t origin) {
	int areanum, numareas, areas[10];
	vec3_t end;

	areanum = trap_AAS_PointAreaNum(origin);
	if(areanum) return areanum;
	VectorCopy(origin, end);
	end[2] += 10;
	numareas = trap_AAS_TraceAreas(origin, end, areas, NULL, 10);
	if(numareas > 0) return areas[0];
	return 0;
}

static bot_goal_t BotCreateGoal(vec3_t origin) {
	bot_goal_t goal;

	goal.areanum = BotPointAreaNum(origin);
	VectorCopy(origin, goal.origin);
	VectorSet(goal.mins, -25, -25, -25);
	VectorSet(goal.maxs, 25, 25, 25);
	return goal;
}

static int weaponOrder[] = {WP_NONE,

                            // Quake weapons here!
                            WP_GAUNTLET,
                            WP_GRENADE_LAUNCHER,
                            WP_PROX_LAUNCHER,
                            WP_MACHINEGUN,
                            WP_LIGHTNING,
                            WP_PLASMAGUN,
                            WP_ROCKET_LAUNCHER,
                            WP_SHOTGUN,
                            WP_CHAINGUN,
                            WP_NAILGUN,
                            WP_RAILGUN,
                            WP_BFG,

                            // Sandbox weapons here!
                            WP_PHYSGUN,
                            WP_GRAVITYGUN,
                            WP_TOOLGUN,

                            WEAPONS_NUM};

static int BotSelectWeapon(bot_state_t *bs) {
	int i, weaponID;
	for(i = WEAPONS_NUM - 1; i > 0; i--) {
		weaponID = weaponOrder[i];
		if(bs->ent->swep_list[weaponID] == WS_HAVE && (bs->ent->swep_ammo[weaponID] > 0 || bs->ent->swep_ammo[weaponID] == -1)) {
			if(weaponID != WP_NONE && weaponID != WP_PHYSGUN && weaponID != WP_GRAVITYGUN && weaponID != WP_TOOLGUN) {
				return weaponID;
			}
		}
	}
	return WP_NONE;
}

static void BotSetupForMovement(bot_state_t *bs) {
	bot_initmove_t initmove;

	memset(&initmove, 0, sizeof(bot_initmove_t));
	VectorCopy(bs->ent->client->ps.origin, initmove.origin);
	VectorCopy(bs->ent->client->ps.velocity, initmove.velocity);
	VectorClear(initmove.viewoffset);
	initmove.viewoffset[2] += bs->ent->client->ps.viewheight;
	initmove.entitynum = bs->ent->s.number;
	initmove.client = bs->ent->client->ps.clientNum;
	initmove.thinktime = bs->thinktime;
	// set the onground flag
	if(bs->ent->client->ps.groundEntityNum != ENTITYNUM_NONE) initmove.or_moveflags |= MFL_ONGROUND;
	// set the waterjump flag
	if((bs->ent->client->ps.pm_flags & PMF_TIME_WATERJUMP) && (bs->ent->client->ps.pm_time > 0)) {
		initmove.or_moveflags |= MFL_WATERJUMP;
	}
	// set presence type
	if(bs->ent->client->ps.pm_flags & PMF_DUCKED)
		initmove.presencetype = PRESENCE_CROUCH;
	else
		initmove.presencetype = PRESENCE_NORMAL;

	VectorCopy(bs->viewangles, initmove.viewangles);
	trap_BotInitMoveState(bs->ms, &initmove);
}

static qboolean BotIsDead(bot_state_t *bs) { return (bs->ent->client->ps.pm_type == PM_DEAD); }

static qboolean BotIsObserver(bot_state_t *bs) {
	char buf[MAX_INFO_STRING];
	if(bs->ent->client->ps.pm_type == PM_SPECTATOR) return qtrue;
	trap_GetConfigstring(CS_PLAYERS + bs->ent->client->ps.clientNum, buf, sizeof(buf));
	if(atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR) return qtrue;
	return qfalse;
}

static qboolean BotIntermission(bot_state_t *bs) {
	if(level.intermissiontime) return qtrue;
	return (bs->ent->client->ps.pm_type == PM_FREEZE || bs->ent->client->ps.pm_type == PM_INTERMISSION);
}

static int BotSameTeam(bot_state_t *bs, int entnum) {
	char info1[1024], info2[1024];

	if(bs->ent->client->ps.clientNum < 0 || bs->ent->client->ps.clientNum >= MAX_CLIENTS) return qfalse;

	if(entnum < 0 || entnum >= MAX_CLIENTS) return qfalse;

	trap_GetConfigstring(CS_PLAYERS + bs->ent->client->ps.clientNum, info1, sizeof(info1));
	trap_GetConfigstring(CS_PLAYERS + entnum, info2, sizeof(info2));

	if(cvarInt("g_gametype") >= GT_TEAM && atoi(Info_ValueForKey(info1, "nt")) <= NT_PLAYER) {
		if(atoi(Info_ValueForKey(info1, "t")) == atoi(Info_ValueForKey(info2, "t"))) return qtrue;
	} else {
		if(!BG_FactionShouldAttack(gameInfoNPCTypes[atoi(Info_ValueForKey(info1, "nt"))].faction, gameInfoNPCTypes[atoi(Info_ValueForKey(info2, "nt"))].faction)) return qtrue;
	}
	return qfalse;
}

static qboolean BotEntityVisible(int viewer, vec3_t eye, int entityNum) {
	trace_t trace;
	gentity_t *ent = &g_entities[entityNum];

	trap_Trace(&trace, eye, NULL, NULL, ent->r.currentOrigin, viewer, CONTENTS_SOLID | CONTENTS_PLAYERCLIP);

	if(trace.fraction >= 1 || trace.entityNum == entityNum)
		return qtrue;
	else
		return qfalse;
}

static qboolean BotFindEnemy(bot_state_t *bs) {
	int i;
	qboolean enemyFound, vis;
	float squaredist, cursquaredist;
	vec3_t dir;
	gentity_t *ent;

	cursquaredist = Square(16384.00f);
	enemyFound = qfalse;
	bs->enemy = -1;

	for(i = 0; i < MAX_CLIENTS; i++) {
		ent = &g_entities[i];
		if(!ent->client) continue;
		if(!ent->health) continue;
		if(ent->client->sess.sessionTeam == TEAM_SPECTATOR) continue;
		if(ent->client->ps.clientNum == bs->ent->client->ps.clientNum) continue;

		// calculate the distance
		VectorSubtract(ent->r.currentOrigin, bs->ent->r.currentOrigin, dir);
		squaredist = VectorLengthSquared(dir);
		if(squaredist > Square(16384.00f)) continue;
		if(BotSameTeam(bs, i)) continue;

		if(bs->ent->npcType != NT_NEXTBOT)
			vis = BotEntityVisible(bs->ent->s.number, bs->eye, i);
		else
			vis = qtrue;

		if(!vis) continue;

		if(bs->enemy >= 0 && squaredist > cursquaredist) continue;
		bs->enemy = i;
		cursquaredist = squaredist;
		enemyFound = qtrue;
	}

	return enemyFound;
}

static void BotAimAtEnemy(bot_state_t *bs) {
	vec3_t dir;
	gentity_t *ent;

	ent = &g_entities[bs->enemy];
	VectorSubtract(ent->r.currentOrigin, bs->eye, dir);
	vectoangles(dir, bs->ideal_viewangles);
	VectorCopy(bs->ideal_viewangles, bs->viewangles);
	trap_EA_View(bs->ent->client->ps.clientNum, bs->viewangles);
}

static void BotAimAtMovementDirection(bot_state_t *bs) {
	bot_input_t bi;
	vec3_t temp_dir;

	trap_EA_GetInput(bs->ent->client->ps.clientNum, (float)bs->thinktime / 1000, &bi);

	if(VectorLength(bi.dir) < 0.1f) return;
	VectorCopy(bi.dir, temp_dir);
	temp_dir[2] = 0;

	vectoangles(temp_dir, bs->ideal_viewangles);

	bs->ideal_viewangles[PITCH] = 0;
	// bs->ideal_viewangles[YAW] = 0;
	bs->ideal_viewangles[ROLL] = 0;
}

static bot_goal_t AI_GoalFromRandomItem(bot_state_t *bs) {
	gentity_t *ent;
	bot_goal_t goal;

	ent = FindRandomItem();
	if(ent) {
		goal = BotCreateGoal(ent->r.currentOrigin);
		return goal;
	} else {
		ent = FindRandomSpawn();
		if(ent) {
			goal = BotCreateGoal(ent->r.currentOrigin);
			return goal;
		}
	}
	if(!ent) goal = BotCreateGoal(bs->ent->r.currentOrigin);
	return goal;
}

static int AI_Battle(bot_state_t *bs) {
	gentity_t *ent = &g_entities[bs->enemy];

	bs->maingoal = BotCreateGoal(ent->r.currentOrigin);
	bs->maingoal_time = level.time + 30000;
	BotSetupForMovement(bs);
	trap_BotMoveToGoal(bs->ms, &bs->maingoal, bs->tfl);

	BotAimAtEnemy(bs);
	trap_EA_Attack(bs->ent->client->ps.clientNum);
	return qtrue;
}

static int AINode_Default(bot_state_t *bs) {

	if(BotIsObserver(bs) || BotIntermission(bs)) return qfalse;

	if(BotIsDead(bs)) {
		trap_BotResetMoveState(bs->ms);
		trap_EA_Attack(bs->ent->client->ps.clientNum);
		return qfalse;
	}

	bs->tfl = TFL_DEFAULT;

	if(BotFindEnemy(bs)) {
		AI_Battle(bs);
		return qfalse;
	}

	if(level.time > bs->maingoal_time) {
		bs->maingoal = AI_GoalFromRandomItem(bs);
		bs->maingoal_time = level.time + 30000;
	}

	if(bs->maingoal.areanum) {
		BotSetupForMovement(bs);
		trap_BotMoveToGoal(bs->ms, &bs->maingoal, bs->tfl);
		BotAimAtMovementDirection(bs);
	}

	return qtrue;
}

static void BotDeathmatchAI(bot_state_t *bs) {
	char buf[144];

	if(bs->setupcount > 0) {
		bs->setupcount--;
		if(bs->setupcount > 0) return;

		Com_sprintf(buf, sizeof(buf), "team %s", bs->settings.team);
		trap_EA_Command(bs->ent->client->ps.clientNum, buf);
		bs->setupcount = 0;
	}

	if(!bs->ainode) bs->ainode = AINode_Default;
	bs->ainode(bs); // execute AI node
}

static float BotChangeViewAngle(float angle, float ideal_angle, float speed) {
	float move;

	angle = AngleMod(angle);
	ideal_angle = AngleMod(ideal_angle);
	if(angle == ideal_angle) return angle;
	move = ideal_angle - angle;
	if(ideal_angle > angle) {
		if(move > 180.0) move -= 360.0;
	} else {
		if(move < -180.0) move += 360.0;
	}
	if(move > 0) {
		if(move > speed) move = speed;
	} else {
		if(move < -speed) move = -speed;
	}
	return AngleMod(angle + move);
}

static void BotChangeViewAngles(bot_state_t *bs, float thinktime) {
	float diff, factor, maxchange, anglespeed;
	int i;

	factor = 0.80f;
	maxchange = 200;

	maxchange *= thinktime;
	for(i = 0; i < 2; i++) {
		diff = abs(AngleDifference(bs->viewangles[i], bs->ideal_viewangles[i]));
		anglespeed = diff * factor;
		if(anglespeed > maxchange) anglespeed = maxchange;
		bs->viewangles[i] = BotChangeViewAngle(bs->viewangles[i], bs->ideal_viewangles[i], anglespeed);
	}

	trap_EA_View(bs->ent->client->ps.clientNum, bs->viewangles);
}

static void BotInputToUserCommand(bot_input_t *bi, usercmd_t *ucmd, int delta_angles[3], int time) {
	vec3_t angles, forward, right;
	short temp;
	int j;

	memset(ucmd, 0, sizeof(usercmd_t));

	ucmd->serverTime = time;

	if(bi->actionflags & ACTION_DELAYEDJUMP) {
		bi->actionflags |= ACTION_JUMP;
		bi->actionflags &= ~ACTION_DELAYEDJUMP;
	}

	if(bi->actionflags & ACTION_ATTACK) ucmd->buttons |= BUTTON_ATTACK;

	ucmd->angles[PITCH] = ANGLE2SHORT(bi->viewangles[PITCH]);
	ucmd->angles[YAW] = ANGLE2SHORT(bi->viewangles[YAW]);

	for(j = 0; j < 2; j++) {
		temp = ucmd->angles[j] - delta_angles[j];
		ucmd->angles[j] = temp;
	}

	if(bi->dir[2])
		angles[PITCH] = bi->viewangles[PITCH];
	else
		angles[PITCH] = 0;

	angles[YAW] = bi->viewangles[YAW];
	angles[ROLL] = 0;
	AngleVectors(angles, forward, right, NULL);

	bi->speed = bi->speed * 127 / 400;

	if(bi->actionflags & ACTION_MOVEFORWARD) ucmd->buttons |= BMOVE_W;
	if(bi->actionflags & ACTION_MOVEBACK) ucmd->buttons |= BMOVE_S;
	if(bi->actionflags & ACTION_MOVELEFT) ucmd->buttons |= BMOVE_A;
	if(bi->actionflags & ACTION_MOVERIGHT) ucmd->buttons |= BMOVE_D;
	if(bi->actionflags & ACTION_JUMP) ucmd->buttons |= BMOVE_J;
	if(bi->actionflags & ACTION_CROUCH) ucmd->buttons |= BMOVE_C;
}

static void BotUpdateInput(bot_state_t *bs, int time, int elapsed_time) {
	bot_input_t bi;
	int j;

	for(j = 0; j < 2; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] + SHORT2ANGLE(bs->ent->client->ps.delta_angles[j]));
	}

	BotChangeViewAngles(bs, (float)elapsed_time / 1000);
	trap_EA_GetInput(bs->ent->client->ps.clientNum, (float)time / 1000, &bi);

	if(bi.actionflags & ACTION_RESPAWN) {
		if(bs->ent->client->pers.cmd.buttons & BUTTON_ATTACK) bi.actionflags &= ~(ACTION_RESPAWN | ACTION_ATTACK);
	}

	BotInputToUserCommand(&bi, &bs->ent->client->pers.cmd, bs->ent->client->ps.delta_angles, time);

	for(j = 0; j < 2; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] - SHORT2ANGLE(bs->ent->client->ps.delta_angles[j]));
	}
}

static int BotAI(int client, float thinktime) {
	bot_state_t *bs;
	char buf[1024];
	int i;

	trap_EA_ResetInput(client);

	while(trap_BotGetServerCommand(client, buf, sizeof(buf))) {
		// NOTHING, WE JUST ANSWER TO EVERY COMMAND
	}

	bs = botstates[client];
	if(!bs || !bs->inuse) {
		G_Printf("BotAI: client %d is not setup\n", client);
		return qfalse;
	}

	for(i = 0; i < 2; i++) bs->viewangles[i] = AngleMod(bs->viewangles[i] + SHORT2ANGLE(bs->ent->client->ps.delta_angles[i]));

	bs->thinktime = thinktime;
	VectorCopy(bs->ent->client->ps.origin, bs->ent->r.currentOrigin);
	VectorCopy(bs->ent->client->ps.origin, bs->eye);
	bs->eye[2] += bs->ent->client->ps.viewheight;

	BotDeathmatchAI(bs);
    BotSelectWeapon(bs);

	for(i = 0; i < 2; i++) bs->viewangles[i] = AngleMod(bs->viewangles[i] - SHORT2ANGLE(bs->ent->client->ps.delta_angles[i]));

	return qtrue;
}

static void BotScheduleBotThink(void) {
	int i, botnum;

	botnum = 0;

	for(i = 0; i < MAX_CLIENTS; i++) {
		if(!botstates[i] || !botstates[i]->inuse) continue;
		botstates[i]->botthink_residual = AI_THINKTIME * botnum / numbots;
		botnum++;
	}
}

int BotAISetupClient(int client, struct bot_settings_s *settings) {
	bot_state_t *bs;

	if(!botstates[client]) botstates[client] = G_Alloc(sizeof(bot_state_t));
	bs = botstates[client];

	if(bs && bs->inuse) {
		G_Printf("BotAISetupClient: client %d already setup\n", client);
		return qfalse;
	}

	if(!trap_AAS_Initialized()) {
		G_Printf("AAS not initialized\n");
		return qfalse;
	}

	bs->ent = &g_entities[client];
	bs->inuse = qtrue;
	bs->setupcount = 4;
	bs->ms = trap_BotAllocMoveState();
	numbots++;

	BotScheduleBotThink();

	return qtrue;
}

int BotAIShutdownClient(int client, qboolean restart) {
	bot_state_t *bs;

	bs = botstates[client];
	if(!bs || !bs->inuse) return qfalse;

	trap_BotFreeMoveState(bs->ms);

	memset(bs, 0, sizeof(bot_state_t));
	bs->inuse = qfalse;
	numbots--;

	return qtrue;
}

void BotResetState(bot_state_t *bs) {
	int inuse, movestate;
	bot_settings_t settings;

	// save some things that should not be reset here
	memcpy(&settings, &bs->settings, sizeof(bot_settings_t));
	inuse = bs->inuse;
	movestate = bs->ms;

	// reset the whole state
	memset(bs, 0, sizeof(bot_state_t));
	// copy back some state stuff that should not be reset
	bs->ms = movestate;
	memcpy(&bs->settings, &settings, sizeof(bot_settings_t));
	bs->inuse = inuse;

	// reset several states
	if(bs->ms) trap_BotResetMoveState(bs->ms);
}

int BotAILoadMap(int restart) {
	int i;

	if(!restart) trap_BotLibLoadMap(cvarString("sv_mapname"));

	for(i = 0; i < MAX_CLIENTS; i++) {
		if(botstates[i] && botstates[i]->inuse) {
			BotResetState(botstates[i]);
			botstates[i]->setupcount = 4;
		}
	}

	return qtrue;
}

int AI_Frame(int time) {
	bot_entitystate_t state;
	gentity_t *ent;
	int i, elapsed_time, thinktime;
	static int local_time, botlib_residual;

	elapsed_time = time - local_time;
	local_time = time;
	botlib_residual += elapsed_time;

	if(elapsed_time > AI_THINKTIME)
		thinktime = elapsed_time;
	else
		thinktime = AI_THINKTIME;

	if(botlib_residual >= thinktime) {
		botlib_residual -= thinktime;
		trap_BotLibStartFrame((float)time / 1000);

		for(i = 0; i < MAX_GENTITIES; i++) {
			ent = &g_entities[i];

			if(!ent->inuse || !ent->r.linked || ent->client || ent->r.svFlags & SVF_NOCLIENT || ent->s.eType != ET_MOVER || ent->r.contents == CONTENTS_TRIGGER) {
				trap_BotUpdateEntity(i, NULL);
				continue;
			}

			memset(&state, 0, sizeof(bot_entitystate_t));

			VectorCopy(ent->r.currentOrigin, state.origin);
			VectorCopy(ent->r.currentAngles, state.angles);
			VectorCopy(ent->s.origin2, state.old_origin);
			VectorCopy(ent->r.mins, state.mins);
			VectorCopy(ent->r.maxs, state.maxs);
			state.type = ent->s.eType;
			state.flags = ent->s.eFlags;
			if(ent->r.bmodel)
				state.solid = SOLID_BSP;
			else
				state.solid = SOLID_BBOX;
			state.groundent = ent->s.groundEntityNum;
			state.modelindex = ent->s.modelindex;
			state.modelindex2 = ent->s.modelindex2;
			state.frame = ent->s.frame;
			state.event = ent->s.event;
			state.eventParm = ent->s.eventParm;
			state.powerups = ent->s.powerups;
			state.weapon = ent->s.weapon;

			trap_BotUpdateEntity(i, &state);
		}
	}

	floattime = trap_AAS_Time();

	for(i = 0; i < MAX_CLIENTS; i++) {
		if(!botstates[i] || !botstates[i]->inuse) continue;
		botstates[i]->botthink_residual += elapsed_time;

		if(botstates[i]->botthink_residual >= thinktime) {
			botstates[i]->botthink_residual -= thinktime;
			if(botstates[i]->ent->client->pers.connected == CON_CONNECTED) BotAI(i, (float)thinktime / 1000);
		}
	}

	for(i = 0; i < MAX_CLIENTS; i++) {
		if(!botstates[i] || !botstates[i]->inuse) continue;
		if(botstates[i]->ent->client->pers.connected != CON_CONNECTED) continue;

		BotUpdateInput(botstates[i], time, elapsed_time);
		trap_BotUserCommand(botstates[i]->ent->client->ps.clientNum, &botstates[i]->ent->client->pers.cmd);
	}

	return qtrue;
}

int BotAISetup(int restart) {
	int errnum;

	if(restart) return qtrue;

	memset(botstates, 0, sizeof(botstates));
	errnum = trap_BotLibSetup();
	if(errnum != 0) return qfalse;
	return qtrue;
}

int BotAIShutdown(int restart) {
	int i;

	if(restart) {
		for(i = 0; i < MAX_CLIENTS; i++) {
			if(botstates[i] && botstates[i]->inuse) {
				BotAIShutdownClient(botstates[i]->ent->client->ps.clientNum, restart);
			}
		}
	} else {
		trap_BotLibShutdown();
	}
	return qtrue;
}
