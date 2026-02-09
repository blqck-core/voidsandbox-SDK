// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

static void CG_DamageFeedback(int yawByte, int pitchByte, int damage) {
	float left, front, up;
	float kick;
	int health;
	float scale;
	vec3_t dir;
	vec3_t angles;
	float dist;
	float yaw, pitch;

	// the lower on health you are, the greater the view kick will be
	health = cg.snap->ps.stats[STAT_HEALTH];
	if(health < 40) {
		scale = 1;
	} else {
		scale = 40.0 / health;
	}
	kick = damage * scale;

	if(kick < 5) kick = 5;
	if(kick > 10) kick = 10;

	// if yaw and pitch are both 255, make the damage always centered (falling, etc)
	if(yawByte == 255 && pitchByte == 255) {
		cg.damageX = 0;
		cg.damageY = 0;
	} else {
		// positional
		pitch = pitchByte / 255.0 * 360;
		yaw = yawByte / 255.0 * 360;

		angles[PITCH] = pitch;
		angles[YAW] = yaw;
		angles[ROLL] = 0;

		AngleVectors(angles, dir, NULL, NULL);
		VectorSubtract(vec3_origin, dir, dir);

		front = DotProduct(dir, cg.refdef.viewaxis[0]);
		left = DotProduct(dir, cg.refdef.viewaxis[1]);
		up = DotProduct(dir, cg.refdef.viewaxis[2]);

		dir[0] = front;
		dir[1] = left;
		dir[2] = 0;
		dist = VectorLength(dir);
		if(dist < 0.1) dist = 0.1f;

		if(front <= 0.1) front = 0.1f;
		cg.damageX = -left / front;
		cg.damageY = up / dist;
	}

	// clamp the position
	if(cg.damageX > 1.0) cg.damageX = 1.0;
	if(cg.damageX < -1.0) cg.damageX = -1.0;

	if(cg.damageY > 1.0) cg.damageY = 1.0;
	if(cg.damageY < -1.0) cg.damageY = -1.0;

	// don't let the screen flashes vary as much
	if(kick > 10) kick = 10;
	cg.damageValue = kick;
	cg.damageTime = cg.snap->serverTime;
}

/*
================
CG_Respawn

A respawn happened this snapshot
================
*/
void CG_Respawn(void) {
	int i;

	// no error decay on player movement
	cg.thisFrameTeleport = qtrue;

	for(i = 1; i < WEAPONS_NUM; i++) cg.swep_listcl[i] = cg.swep_spawncl[i]; // OpenSandbox weapon predict

	// select WP_NONE for updating from server
	cg.weaponSelect = 0;
	cg.zoomed = qfalse;
}

static void CG_CheckPlayerstateEvents(playerState_t *ps, playerState_t *ops) {
	int i;
	int event;
	centity_t *cent;

	if(ps->externalEvent && ps->externalEvent != ops->externalEvent) {
		cent = &cg_entities[ps->clientNum];
		cent->currentState.event = ps->externalEvent;
		cent->currentState.eventParm = ps->externalEventParm;
		CG_EntityEvent(cent, cent->lerpOrigin);
	}

	cent = &cg.predictedPlayerEntity; // cg_entities[ ps->clientNum ];
	// go through the predictable events buffer
	for(i = ps->eventSequence - MAX_PS_EVENTS; i < ps->eventSequence; i++) {
		// if we have a new predictable event
		if(i >= ops->eventSequence || (i > ops->eventSequence - MAX_PS_EVENTS && ps->events[i & (MAX_PS_EVENTS - 1)] != ops->events[i & (MAX_PS_EVENTS - 1)])) {
			event = ps->events[i & (MAX_PS_EVENTS - 1)];
			cent->currentState.event = event;
			cent->currentState.eventParm = ps->eventParms[i & (MAX_PS_EVENTS - 1)];
			CG_EntityEvent(cent, cent->lerpOrigin);
			cg.predictableEvents[i & (MAX_PREDICTED_EVENTS - 1)] = event;
			cg.eventSequence++;
		}
	}
}

static void CG_CheckLocalSounds(playerState_t *ps, playerState_t *ops, qboolean isVehicle) {
	int health, armor;

	// don't play the sounds if the player just changed teams
	if(ps->persistant[PERS_TEAM] != ops->persistant[PERS_TEAM]) return;

	// hit changes
	if(ps->persistant[PERS_HITS] > ops->persistant[PERS_HITS]) {
		armor = ps->persistant[PERS_ATTACKEE_ARMOR] & 0xff;
		health = ps->persistant[PERS_ATTACKEE_ARMOR] >> 8;

		if(!isVehicle && cgs.gametype != GT_SANDBOX) {
			if(health > 100) {
				trap_S_StartLocalSound(cgs.media.hitSoundHighArmor, CHAN_LOCAL_SOUND);
			} else if(health > 50) {
				trap_S_StartLocalSound(cgs.media.hitSound, CHAN_LOCAL_SOUND);
			} else {
				trap_S_StartLocalSound(cgs.media.hitSoundLowArmor, CHAN_LOCAL_SOUND);
			}
		}
	} else if(ps->persistant[PERS_HITS] < ops->persistant[PERS_HITS]) {
		trap_S_StartLocalSound(cgs.media.hitTeamSound, CHAN_LOCAL_SOUND);
	}

	// don't start voice in intermission
	if(cg.intermissionStarted) return;
}

void CG_TransitionPlayerState(playerState_t *ps, playerState_t *ops) {
	// check for changing follow mode
	if(ps->clientNum != ops->clientNum) {
		cg.thisFrameTeleport = qtrue; // make sure we don't get any unwanted transition effects
		*ops = *ps;
	}

	// damage events (player is getting wounded)
	if(ps->damageEvent != ops->damageEvent && ps->damageCount) CG_DamageFeedback(ps->damageYaw, ps->damagePitch, ps->damageCount);

	// respawning
	if(ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT]) CG_Respawn();

	if(cg.mapRestart) {
		CG_Respawn();
		cg.mapRestart = qfalse;
	}

	if(!cg.snap->ps.stats[STAT_VEHICLE]) {
		if(cg.snap->ps.pm_type != PM_INTERMISSION && ps->persistant[PERS_TEAM] != TEAM_SPECTATOR) CG_CheckLocalSounds(ps, ops, qfalse);
	} else {
		if(cg.snap->ps.pm_type != PM_INTERMISSION && ps->persistant[PERS_TEAM] != TEAM_SPECTATOR) CG_CheckLocalSounds(ps, ops, qtrue);
	}

	// run events
	CG_CheckPlayerstateEvents(ps, ops);

	// smooth the ducking viewheight change
	if(ps->viewheight != ops->viewheight) {
		cg.duckChange = ps->viewheight - ops->viewheight;
		cg.duckTime = cg.time;
	}
}
