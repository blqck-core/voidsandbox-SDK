// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

static void CG_Obituary(entityState_t *ent) {
	int mod, target, attacker;
	const char *targetInfo, *attackerInfo;
	char targetName[36], attackerName[36];

	attacker = ent->otherEntityNum;
	target = ent->otherEntityNum2;
	mod = ent->eventParm;

	iferr(target < 0 || target >= MAX_CLIENTS);

	if(attacker < 0 || attacker >= MAX_CLIENTS) {
		attackerInfo = NULL;
	} else {
		attackerInfo = CG_ConfigString(CS_PLAYERS + attacker);
	}

	targetInfo = CG_ConfigString(CS_PLAYERS + target);
	if(!targetInfo) return;
	StringCopy(targetName, Info_ValueForKey(targetInfo, "n"), sizeof(targetName) - 2);
	strcat(targetName, S_COLOR_WHITE);

	if(attackerInfo) {
		StringCopy(attackerName, Info_ValueForKey(attackerInfo, "n"), sizeof(attackerName) - 2);
		strcat(attackerName, S_COLOR_WHITE);
	}

	if(attackerInfo) {
		CG_AddNotify(va("%s -> %s", attackerName, targetName), NOTIFY_KILL, mod);
	} else {
		CG_AddNotify(va("%s", targetName), NOTIFY_KILL, mod);
	}
}

static void CG_ItemPickup(int itemNum) {
	CG_AddNotify(gameInfoItems[itemNum].pickup_name, NOTIFY_ITEM, itemNum);

	// see if it should be the grabbed weapon
	if(gameInfoItems[itemNum].giType == IT_AMMO)
		if(cg.swep_listcl[gameInfoItems[itemNum].giTag] == WS_NOAMMO) cg.swep_listcl[gameInfoItems[itemNum].giTag] = WS_HAVE;
	if(gameInfoItems[itemNum].giType == IT_WEAPON) cg.swep_listcl[gameInfoItems[itemNum].giTag] = WS_HAVE;
}

/*
==============
CG_EntityEvent

An entity has an event value
also called by CG_CheckPlayerstateEvents
==============
*/
void CG_EntityEvent(centity_t *cent, vec3_t position) {
	entityState_t *es;
	int event;
	vec3_t dir;
	const char *s;
	int clientNum;
	clientInfo_t *ci;
	byte r, g, b;
	int rnum;

	es = &cent->currentState;
	event = es->event & ~EV_EVENT_BITS;

	if(!event) return;

	clientNum = es->clientNum;
	if(clientNum < 0 || clientNum >= MAX_CLIENTS) clientNum = 0;
	ci = &cgs.clientinfo[clientNum];

	switch(event) {
	case EV_FOOTSTEP: trap_S_StartSound(NULL, es->number, CHAN_BODY, cgs.media.footsteps[FOOTSTEP_NORMAL][rand() & 3]); break;
	case EV_FOOTSTEP_METAL: trap_S_StartSound(NULL, es->number, CHAN_BODY, cgs.media.footsteps[FOOTSTEP_METAL][rand() & 3]); break;
	case EV_FOOTSPLASH: trap_S_StartSound(NULL, es->number, CHAN_BODY, cgs.media.footsteps[FOOTSTEP_SPLASH][rand() & 3]); break;
	case EV_SWIM: trap_S_StartSound(NULL, es->number, CHAN_BODY, cgs.media.footsteps[FOOTSTEP_SPLASH][rand() & 3]); break;
	case EV_FALL_SHORT: trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.landSound); break;
	case EV_FALL_MEDIUM: trap_S_StartSound(NULL, es->number, CHAN_VOICE, trap_S_RegisterSound("sounds/player/fallsmall", qfalse)); break;
	case EV_FALL_FAR: trap_S_StartSound(NULL, es->number, CHAN_AUTO, trap_S_RegisterSound("sounds/player/fallbig", qfalse)); break;
	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16: // smooth out step up transitions
	{
		float oldStep;
		int delta;
		int step;

		if(clientNum != cg.predictedPlayerState.clientNum) {
			break;
		}
		// if we are interpolating, we don't need to smooth steps
		if(cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW)) {
			break;
		}
		// check for stepping up before a previous step is completed
		delta = cg.time - cg.stepTime;
		if(delta < STEP_TIME) {
			oldStep = cg.stepChange * (STEP_TIME - delta) / STEP_TIME;
		} else {
			oldStep = 0;
		}

		// add this amount
		step = 4 * (event - EV_STEP_4 + 1);
		cg.stepChange = oldStep + step;
		if(cg.stepChange > MAX_STEP_CHANGE) {
			cg.stepChange = MAX_STEP_CHANGE;
		}
		cg.stepTime = cg.time;
		break;
	}
	case EV_HORN: trap_S_StartSound(NULL, es->number, CHAN_AUTO, trap_S_RegisterSound("sound/vehicle/horn.ogg", qfalse)); break;
	case EV_OT1_IMPACT:
		rnum = rand() % 3 + 1;
		if(rnum == 1) trap_S_StartSound(NULL, es->number, CHAN_AUTO, trap_S_RegisterSound("sound/objects/basic/impact1", qfalse));
		if(rnum == 2) trap_S_StartSound(NULL, es->number, CHAN_AUTO, trap_S_RegisterSound("sound/objects/basic/impact2", qfalse));
		if(rnum == 3) trap_S_StartSound(NULL, es->number, CHAN_AUTO, trap_S_RegisterSound("sound/objects/basic/impact3", qfalse));
		break;
	case EV_GRAVITYSOUND: trap_S_StartSound(NULL, es->number, CHAN_AUTO, trap_S_RegisterSound("sound/gravitygun", qfalse)); break;
	case EV_WATER_TOUCH: trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.watrInSound); break;
	case EV_WATER_LEAVE: trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.watrOutSound); break;
	case EV_WATER_UNDER: trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.watrUnSound); break;
	case EV_ITEM_PICKUP: {
		sfxHandle_t pickupSound = 0;
		item_t *item;
		int index;

		index = es->eventParm; // player predicted

		if(index < 1 || index >= gameInfoItemsNum) {
			break;
		}
		item = &gameInfoItems[index];

		// powerups and team items will have a separate global sound, this one
		// will be played at prediction time
		switch(item->giType) {
		case IT_WEAPON: pickupSound = trap_S_RegisterSound("sound/misc/we_pkup.wav", qfalse); break;
		case IT_AMMO: pickupSound = trap_S_RegisterSound("sound/misc/am_pkup.wav", qfalse); break;
		case IT_ARMOR: pickupSound = trap_S_RegisterSound("sound/misc/ar_pkup.wav", qfalse); break;
		case IT_HEALTH: pickupSound = trap_S_RegisterSound("sound/misc/he_pkup.wav", qfalse); break;
		}

		if(pickupSound) {
			trap_S_StartSound(NULL, es->number, CHAN_AUTO, pickupSound);
		}

		// show icon and name on status bar
		if(es->number == cg.snap->ps.clientNum) {
			CG_ItemPickup(index);
		}
	} break;
	case EV_NOAMMO: trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.noAmmoSound); break;
	case EV_CHANGE_WEAPON: trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.selectSound); break;
	case EV_FIRE_WEAPON: CG_FireWeapon(cent); break;
	case EV_PLAYER_TELEPORT_IN:
		trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.teleInSound);
		CG_SpawnEffect(position);
		break;
	case EV_PLAYER_TELEPORT_OUT:
		trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.teleOutSound);
		CG_SpawnEffect(position);
		break;
	case EV_ITEM_POP: trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.respawnSound); break;
	case EV_ITEM_RESPAWN:
		cent->miscTime = cg.time; // scale up from this
		trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.respawnSound);
		break;
	case EV_GRENADE_BOUNCE:
		if(rand() & 1) {
			trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.hgrenb1aSound);
		} else {
			trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.hgrenb2aSound);
		}
		break;
	case EV_PROXIMITY_MINE_STICK:
		if(es->eventParm & SURF_FLESH) {
			trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.wstbimplSound);
		} else if(es->eventParm & SURF_METALSTEPS) {
			trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.wstbimpmSound);
		} else {
			trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.wstbimpdSound);
		}
		break;
	case EV_PROXIMITY_MINE_TRIGGER: trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.wstbactvSound); break;
	case EV_LIGHTNINGBOLT: CG_LightningBoltBeam(es->origin2, es->pos.trBase); break;
	case EV_MISSILE_HIT:
		ByteToDir(es->eventParm, dir);
		CG_MissileHitPlayer(es->weapon, position, dir, es->otherEntityNum);
		break;
	case EV_MISSILE_MISS:
		ByteToDir(es->eventParm, dir);
		CG_MissileHitWall(es->weapon, 0, position, dir, IMPACTSOUND_DEFAULT);
		break;
	case EV_MISSILE_MISS_METAL:
		ByteToDir(es->eventParm, dir);
		CG_MissileHitWall(es->weapon, 0, position, dir, IMPACTSOUND_METAL);
		break;
	case EV_RAILTRAIL:
		cent->currentState.weapon = WP_RAILGUN;
		if(es->clientNum == cg.predictedPlayerState.clientNum) {
			// do nothing, because it was already predicted
		} else {
			CG_RailTrail(ci, es->origin2, es->pos.trBase, WP_RAILGUN);
			if(es->eventParm != 255) {
				ByteToDir(es->eventParm, dir);
				CG_MissileHitWall(es->weapon, es->clientNum, position, dir, IMPACTSOUND_DEFAULT);
			}
		}
		break;
	case EV_BULLET_HIT_WALL:
		if(es->clientNum == cg.predictedPlayerState.clientNum) {
			// do nothing, because it was already predicted
		} else {
			ByteToDir(es->eventParm, dir);
			CG_Bullet(es->pos.trBase, es->otherEntityNum, dir, qfalse, ENTITYNUM_WORLD);
		}
		break;
	case EV_BULLET_HIT_FLESH:
		if(es->clientNum == cg.predictedPlayerState.clientNum) {
			// do nothing, because it was already predicted
		} else {
			CG_Bullet(es->pos.trBase, es->otherEntityNum, dir, qtrue, es->eventParm);
		}
		break;
	case EV_SHOTGUN:
		if(es->otherEntityNum == cg.predictedPlayerState.clientNum) {
			// do nothing, because it was already predicted
		} else {
			CG_ShotgunFire(es, es->weapon);
		}
		break;
	case EV_GENERAL_SOUND:
		if(cgs.gameSounds[es->eventParm]) {
			trap_S_StartSound(NULL, es->number, CHAN_VOICE, cgs.gameSounds[es->eventParm]);
		} else {
			s = CG_ConfigString(CS_SOUNDS + es->eventParm);
			trap_S_StartSound(NULL, es->number, CHAN_VOICE, trap_S_RegisterSound(s, qfalse));
		}
		break;
	case EV_GLOBAL_SOUND: // play from the player's head so it never diminishes
		if(cgs.gameSounds[es->eventParm]) {
			trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.gameSounds[es->eventParm]);
		} else {
			s = CG_ConfigString(CS_SOUNDS + es->eventParm);
			trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO, trap_S_RegisterSound(s, qfalse));
		}
		break;
	case EV_PAIN: trap_S_StartSound(NULL, cent->currentState.number, CHAN_VOICE, trap_S_RegisterSound(va("sounds/player/hit%i", (rand() % 3)+1), qfalse)); break;
	case EV_OBITUARY: CG_Obituary(es); break;
	case EV_STOPLOOPINGSOUND:
		trap_S_StopLoopingSound(es->number);
		es->loopSound = 0;
		break;
	case EV_EXPLOSION:
		// show explosion
		dir[0] = 0;
		dir[1] = 0;
		dir[2] = 0;
		CG_MakeExplosion(cent->lerpOrigin, dir, cgs.media.dishFlashModel, cgs.media.rocketExplosionShader, 1000, qtrue);
		break;
	case EV_SMOKEPUFF:
		// es->constantLight is used to specify color of the smoke puff
		r = es->constantLight & 255;
		g = (es->constantLight >> 8) & 255;
		b = (es->constantLight >> 16) & 255;

		// es->generic1 is used to specify movement speed of the smokepuff
		VectorSet(dir, es->angles[0] * es->generic1, es->angles[1] * es->generic1, es->angles[2] * es->generic1);
		CG_SmokePuff(cent->lerpOrigin, dir, es->otherEntityNum, r / 255, g / 255, b / 255, 0.33f, es->eventParm * 1000, cg.time, 0, 0, cgs.media.smokePuffShader);
		break;
	case EV_WATERPUFF:
		// es->generic1 is used to specify movement speed of the smokepuff
		CG_SmokePuff(cent->lerpOrigin, dir, es->otherEntityNum, 1.0, 1.0, 1.0, 1.00f, es->eventParm * 2000, cg.time, 0, 0, cgs.media.lsplShader);
		CG_SmokePuff(cent->lerpOrigin, dir, es->otherEntityNum * 0.80, 1.0, 1.0, 1.0, 1.00f, es->eventParm * 1500, cg.time, 0, 0, cgs.media.lsplShader);
		CG_SmokePuff(cent->lerpOrigin, dir, es->otherEntityNum * 0.60, 1.0, 1.0, 1.0, 1.00f, es->eventParm * 1000, cg.time, 0, 0, cgs.media.lsplShader);
		CG_SmokePuff(cent->lerpOrigin, dir, es->otherEntityNum * 0.40, 1.0, 1.0, 1.0, 1.00f, es->eventParm * 750, cg.time, 0, 0, cgs.media.lsplShader);
		CG_SmokePuff(cent->lerpOrigin, dir, es->otherEntityNum * 0.20, 1.0, 1.0, 1.0, 1.00f, es->eventParm * 500, cg.time, 0, 0, cgs.media.lsplShader);
		CG_SmokePuff(cent->lerpOrigin, dir, es->otherEntityNum * 0.10, 1.0, 1.0, 1.0, 1.00f, es->eventParm * 250, cg.time, 0, 0, cgs.media.lsplShader);
		trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.watrInSound);
		break;
	case EV_PARTICLES_GRAVITY: CG_ParticlesFromEntityState(cent->lerpOrigin, PT_GRAVITY, es); break;
	case EV_PARTICLES_LINEAR: CG_ParticlesFromEntityState(cent->lerpOrigin, PT_LINEAR_BOTH, es); break;
	case EV_PARTICLES_LINEAR_UP: CG_ParticlesFromEntityState(cent->lerpOrigin, PT_LINEAR_UP, es); break;
	case EV_PARTICLES_LINEAR_DOWN: CG_ParticlesFromEntityState(cent->lerpOrigin, PT_LINEAR_DOWN, es); break;
	default: err("Unknown event"); break;
	}
}

void CG_CheckEvents(centity_t *cent) {
	// check for event-only entities
	if(cent->currentState.eType > ET_EVENTS) {
		if(cent->previousEvent) return; // already fired
		// if this is a player event set the entity number of the client entity number
		if(cent->currentState.eFlags & EF_PLAYER_EVENT) cent->currentState.number = cent->currentState.otherEntityNum;

		cent->previousEvent = 1;

		cent->currentState.event = cent->currentState.eType - ET_EVENTS;
	} else {
		// check for events riding with another entity
		if(cent->currentState.event == cent->previousEvent) return;
		cent->previousEvent = cent->currentState.event;
		if((cent->currentState.event & ~EV_EVENT_BITS) == 0) return;
	}

	// calculate the position at exactly the frame time
	BG_EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin);
	CG_SetEntitySoundPosition(cent);
	CG_EntityEvent(cent, cent->lerpOrigin);
}
