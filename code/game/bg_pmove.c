// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../qcommon/q_shared.h"
#include "bg_local.h"
#include "bg_public.h"

pmove_t *pm;
pml_t pml;

// movement parameters
static float pm_stopspeed = 100.0f;
static float pm_veh00001stopspeed = 100.0f;
static float pm_duckScale = 0.25f;
static float pm_swimScale = 0.50f;

static float pm_accelerate = 10.0f;
static float pm_airaccelerate = 1.0f;
static float pm_veh00001accelerate = 10.0f * 0.090;
static float pm_wateraccelerate = 4.0f;

static float pm_friction = 6.0f;
static float pm_veh00001friction = 10.0f * 0.090;
static float pm_waterfriction = 1.0f;

static int c_pmove = 0;

void PM_AddEvent(int newEvent) { BG_AddPredictableEventToPlayerstate(newEvent, 0, pm->ps); }

void PM_AddTouchEnt(int entityNum) {
	int i;

	if(entityNum == ENTITYNUM_WORLD) {
		return;
	}
	if(pm->numtouch == MAXTOUCH) {
		return;
	}

	// see if it is already added
	for(i = 0; i < pm->numtouch; i++) {
		if(pm->touchents[i] == entityNum) {
			return;
		}
	}

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce) {
	float backoff;
	float change;
	int i;

	backoff = DotProduct(in, normal);

	if(backoff < 0) {
		backoff *= overbounce;
	} else {
		backoff /= overbounce;
	}

	for(i = 0; i < 3; i++) {
		change = normal[i] * backoff;
		out[i] = in[i] - change;
	}
}

/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction(void) {
	vec3_t vec;
	float *vel;
	float speed, newspeed, control;
	float drop;

	vel = pm->ps->velocity;

	VectorCopy(vel, vec);
	if(pml.walking) {
		vec[2] = 0; // ignore slope movement
	}

	speed = VectorLength(vec);
	if(speed < 1) {
		vel[0] = 0;
		vel[1] = 0; // allow sinking underwater
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	// apply ground friction
	if(pm->waterlevel <= 1) {
		if(!pm->ps->stats[STAT_VEHICLE]) { // VEHICLE-SYSTEM: disable player phys for all
			if(pml.walking && !(pml.groundTrace.surfaceFlags & SURF_SLICK)) {
				// if getting knocked back, no friction
				if(!(pm->ps->pm_flags & PMF_TIME_KNOCKBACK)) {
					control = speed < pm_stopspeed ? pm_stopspeed : speed;
					drop += control * pm_friction * pml.frametime;
				}
			}
		}
		if(BG_InVehicle(pm->ps->stats[STAT_VEHICLE])) { // VEHICLE-SYSTEM: turn vehicle phys
			if(pml.walking && !(pml.groundTrace.surfaceFlags & SURF_SLICK)) {
				// if getting knocked back, no friction
				if(!(pm->ps->pm_flags & PMF_TIME_KNOCKBACK)) {
					control = speed < pm_veh00001stopspeed ? pm_veh00001stopspeed : speed;
					if(ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_U) > 0) { // VEHICLE-SYSTEM: space break
						drop += pm_veh00001stopspeed * pm_veh00001friction * 16 * pml.frametime;
					} else {
						drop += control * pm_veh00001friction * pml.frametime;
					}
				}
			}
		}
	}

	// apply water friction even if just wading
	if(pm->waterlevel) {
		drop += speed * pm_waterfriction * pm->waterlevel * pml.frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if(newspeed < 0) {
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}

/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate(vec3_t wishdir, float wishspeed, float accel) {
	if(!pm->ps->stats[STAT_VEHICLE]) { // VEHICLE-SYSTEM: disable player accelerate for all
		int i;
		float addspeed, accelspeed, currentspeed;

		currentspeed = DotProduct(pm->ps->velocity, wishdir);
		addspeed = wishspeed - currentspeed;
		if(addspeed <= 0) {
			return;
		}
		accelspeed = accel * pml.frametime * wishspeed;
		if(accelspeed > addspeed) {
			accelspeed = addspeed;
		}

		for(i = 0; i < 3; i++) {
			pm->ps->velocity[i] += accelspeed * wishdir[i];
		}
	} else if(BG_InVehicle(pm->ps->stats[STAT_VEHICLE])) { // VEHICLE-SYSTEM: accelerate for 1
		// vehicle
		int i;
		float addspeed, accelspeed, currentspeed;

		currentspeed = DotProduct(pm->ps->velocity, wishdir);
		addspeed = wishspeed - currentspeed;
		if(addspeed <= 0) {
			return;
		}
		accelspeed = accel * pml.frametime * wishspeed;
		if(accelspeed > addspeed) {
			accelspeed = addspeed;
		}

		for(i = 0; i < 3; i++) {
			pm->ps->velocity[i] += (accelspeed * wishdir[i]);
		}
	}
}

static float PM_CmdScale(usercmd_t *cmd) {
    float forward = ST_GetPlayerMove(cmd->buttons, PLAYER_MOVE_F);
    float right = ST_GetPlayerMove(cmd->buttons, PLAYER_MOVE_R);
    float total = sqrt(forward * forward + right * right);
    
    if(total == 0) return 0;
    return 320 * 127 / (127.0 * total);
}

static qboolean PM_CheckJump(void) {
	if(BG_InVehicle(pm->ps->stats[STAT_VEHICLE])) { // VEHICLE-SYSTEM: disable jump for 1
		return qfalse;                              // don't allow jump for vehicle
	}

	if(pm->ps->pm_flags & PMF_RESPAWNED) {
		return qfalse; // don't allow jump until all buttons are up
	}

	if(ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_U) <= 0) return qfalse;

	if(pm->ps->pm_flags & PMF_JUMP_HELD) {
		pm->cmd.buttons &= ~BMOVE_J;
		return qfalse;
	}

	pml.groundPlane = qfalse; // jumping away
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pm->ps->velocity[2] = mod_jumpheight;
	PM_AddEvent(EV_JUMP);

	return qtrue;
}

static qboolean PM_CheckWaterJump(void) {
	vec3_t spot;
	int cont;
	vec3_t flatforward;

	if(pm->ps->pm_time) {
		return qfalse;
	}

	// check for water jump
	if(pm->waterlevel != 2) {
		return qfalse;
	}

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize(flatforward);

	VectorMA(pm->ps->origin, 30, flatforward, spot);
	spot[2] += 4;
	cont = pm->pointcontents(spot, pm->ps->clientNum);
	if(!(cont & CONTENTS_SOLID)) {
		return qfalse;
	}

	spot[2] += 16;
	cont = pm->pointcontents(spot, pm->ps->clientNum);
	if(cont & (CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_BODY)) {
		return qfalse;
	}

	// jump out of water
	VectorScale(pml.forward, 200, pm->ps->velocity);
	pm->ps->velocity[2] = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return qtrue;
}

/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
static void PM_WaterJumpMove(void) {
	PM_StepSlideMove(qtrue);

	pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	if(pm->ps->velocity[2] < 0) {
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time = 0;
	}
}

static void PM_WaterMove(void) {
	int i;
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;
	float scale;
	float vel;

	if(PM_CheckWaterJump()) {
		PM_WaterJumpMove();
		return;
	}

	PM_Friction();

	scale = PM_CmdScale(&pm->cmd);
	//
	// user intentions
	//
	if(!scale || BG_InVehicle(pm->ps->stats[STAT_VEHICLE])) { // VEHICLE-SYSTEM: disable water move for 1
		wishvel[0] = 0;
		wishvel[1] = 0;
		if(!pm->ps->stats[STAT_VEHICLE]) { // VEHICLE-SYSTEM: water slow move for all
			wishvel[2] = -30;              // sink towards bottom
		} else {
			wishvel[2] = -2; // sink towards bottom
		}
	} else {
		for(i = 0; i < 3; i++) wishvel[i] = scale * pml.forward[i] * ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_F) + scale * pml.right[i] * ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_R);

		wishvel[2] += scale * ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_U);
	}

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if(wishspeed > pm->ps->speed * pm_swimScale) {
		wishspeed = pm->ps->speed * pm_swimScale;
	}

	PM_Accelerate(wishdir, wishspeed, pm_wateraccelerate);

	// make sure we can go up slopes easily under water
	if(pml.groundPlane && DotProduct(pm->ps->velocity, pml.groundTrace.plane.normal) < 0) {
		vel = VectorLength(pm->ps->velocity);
		// slide along the ground plane
		PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP);

		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, vel, pm->ps->velocity);
	}

	PM_SlideMove(qfalse);
}

static void PM_NoclipMove(void) {
	float speed, drop, friction, control, newspeed;
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;
	float scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = VectorLength(pm->ps->velocity);
	if(speed < 1) {
		VectorCopy(vec3_origin, pm->ps->velocity);
	} else {
		drop = 0;

		friction = pm_friction * 1.5; // extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control * friction * pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if(newspeed < 0) newspeed = 0;
		newspeed /= speed;

		VectorScale(pm->ps->velocity, newspeed, pm->ps->velocity);
	}

	// accelerate
	scale = PM_CmdScale(&pm->cmd);

	fmove = ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_F);
	smove = ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_R);

	for(i = 0; i < 3; i++) wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	wishvel[2] += ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_U);

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	PM_Accelerate(wishdir, wishspeed, pm_accelerate);

	// move
	VectorMA(pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);
}

static void PM_AirMove(void) {
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;
	float scale;
	usercmd_t cmd;

	PM_Friction();

	if(!pm->ps->stats[STAT_VEHICLE]) { // VEHICLE-SYSTEM: disable air move for all
		smove = ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_R);
		fmove = ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_F);
	} else {
		smove = 0;
		fmove = 0;
	}

	cmd = pm->cmd;
	scale = PM_CmdScale(&cmd);

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize(pml.forward);
	VectorNormalize(pml.right);

	for(i = 0; i < 2; i++) {
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	}
	wishvel[2] = 0;

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	// not on ground, so little effect on velocity
	PM_Accelerate(wishdir, wishspeed, pm_airaccelerate);

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if(pml.groundPlane) {
		PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP);
	}

	PM_StepSlideMove(qtrue);
}

static void PM_WalkMove(void) {
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;
	float scale;
	usercmd_t cmd;
	float accelerate;
	float vel;

	if(pm->waterlevel > 2 && DotProduct(pml.forward, pml.groundTrace.plane.normal) > 0) {
		// begin swimming
		PM_WaterMove();
		return;
	}

	if(PM_CheckJump()) {
		// jumped away
		if(pm->waterlevel > 1) {
			PM_WaterMove();
		} else {
			PM_AirMove();
		}
		return;
	}

	PM_Friction();

	fmove = ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_F);
	if(!pm->ps->stats[STAT_VEHICLE]) { // VEHICLE-SYSTEM: disable strafe for all
		smove = ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_R);
	} else {
		smove = 0;
	}

	cmd = pm->cmd;
	scale = PM_CmdScale(&cmd);

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity(pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP);
	PM_ClipVelocity(pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP);

	VectorNormalize(pml.forward);
	VectorNormalize(pml.right);

	for(i = 0; i < 3; i++) {
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	}

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	// clamp the speed lower if ducking
	if(pm->ps->pm_flags & PMF_DUCKED) {
		if(wishspeed > pm->ps->speed * pm_duckScale) {
			wishspeed = pm->ps->speed * pm_duckScale;

			// if player's speed is lowered by target_playerspeed, we can get excessively low movement speeds, so set a mimimum movement speed
			if(wishspeed < 80) // 80 is g_speed's default value (320) * pm_duckScale (0.25)
				wishspeed = 80;

			if(wishspeed > pm->ps->speed) // we don't want the crouch movement speed to be higher than the player's normal movement speed
				wishspeed = pm->ps->speed;
		}
	}

	// clamp the speed lower if wading or walking on the bottom
	if(pm->waterlevel) {
		float waterScale;

		waterScale = pm->waterlevel / 3.0;
		waterScale = 1.0 - (1.0 - pm_swimScale) * waterScale;
		if(wishspeed > pm->ps->speed * waterScale) {
			wishspeed = pm->ps->speed * waterScale;
		}
	}

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if((pml.groundTrace.surfaceFlags & SURF_SLICK) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK) {
		accelerate = pm_airaccelerate;
	} else if(BG_InVehicle(pm->ps->stats[STAT_VEHICLE])) { // VEHICLE-SYSTEM: accelerate for 1
		accelerate = pm_veh00001accelerate;
	} else {
		accelerate = pm_accelerate;
	}

	PM_Accelerate(wishdir, wishspeed, accelerate);

	if((pml.groundTrace.surfaceFlags & SURF_SLICK) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK || BG_InVehicle(pm->ps->stats[STAT_VEHICLE])) { // VEHICLE-SYSTEM: slick move for 1
		pm->ps->velocity[2] -= (pm->ps->gravity * pml.frametime);
	}

	vel = VectorLength(pm->ps->velocity);

	// slide along the ground plane
	PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP);

	// don't decrease velocity when going up or down a slope
	VectorNormalize(pm->ps->velocity);
	VectorScale(pm->ps->velocity, vel, pm->ps->velocity);

	// don't do anything if standing still
	if(!pm->ps->velocity[0] && !pm->ps->velocity[1]) {
		return;
	}

	PM_StepSlideMove(qfalse);
}

static void PM_DeadMove(void) {
	float forward;

	if(!pml.walking) return;

	forward = VectorLength(pm->ps->velocity);
	forward -= 20;
	if(forward <= 0) {
		VectorClear(pm->ps->velocity);
	} else {
		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, forward, pm->ps->velocity);
	}
}

/*
================
PM_FootstepForSurface

Returns an event number apropriate for the groundsurface
================
*/
static int PM_FootstepForSurface(void) {
	if(pml.groundTrace.surfaceFlags & SURF_NOSTEPS) {
		return 0;
	}
	if(pml.groundTrace.surfaceFlags & SURF_METALSTEPS) {
		return EV_FOOTSTEP_METAL;
	}
	if(pml.groundTrace.surfaceFlags & SURF_FLESH) {
		return EV_FOOTSTEP_FLESH;
	}
	return EV_FOOTSTEP;
}

/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand(void) {
	float delta;
	float dist;
	float vel, acc;
	float t;
	float a, b, c, den;

	pm->ps->legsTimer = TIMER_LAND;

	// calculate the exact velocity on landing
	dist = pm->ps->origin[2] - pml.previous_origin[2];
	vel = pml.previous_velocity[2];
	acc = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den = b * b - 4 * a * c;
	if(den < 0) {
		return;
	}
	t = (-b - sqrt(den)) / (2 * a);

	delta = vel + t * acc;
	delta = delta * delta * 0.0001;

	// ducking while falling doubles damage
	if(pm->ps->pm_flags & PMF_DUCKED) {
		delta *= 2;
	}

	// never take falling damage if completely underwater
	if(pm->waterlevel == 3) {
		return;
	}

	// reduce falling damage if there is standing water
	if(pm->waterlevel == 2) {
		delta *= 0.25;
	}
	if(pm->waterlevel == 1) {
		delta *= 0.5;
	}

	if(delta < 1) {
		return;
	}

	// create a local entity event to play the sound

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	if(!pm->ps->stats[STAT_VEHICLE]) {
		if(!(pml.groundTrace.surfaceFlags & SURF_NODAMAGE)) {
			if(delta > 60) {
				PM_AddEvent(EV_FALL_FAR);
			} else if(delta > 40) {
				// this is a pain grunt, so don't play it if dead
				if(pm->ps->stats[STAT_HEALTH] > 0) {
					PM_AddEvent(EV_FALL_MEDIUM);
				}
			} else if(delta > 7) {
				PM_AddEvent(EV_FALL_SHORT);
			} else {
				PM_AddEvent(PM_FootstepForSurface());
			}
		}
	}

	// start footstep cycle over
	pm->ps->bobCycle = 0;
}

static int PM_CorrectAllSolid(trace_t *trace) {
	int i, j, k;
	vec3_t point;

	// jitter around
	for(i = -1; i <= 1; i++) {
		for(j = -1; j <= 1; j++) {
			for(k = -1; k <= 1; k++) {
				VectorCopy(pm->ps->origin, point);
				point[0] += (float)i;
				point[1] += (float)j;
				point[2] += (float)k;
				pm->trace(trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
				if(!trace->allsolid) {
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25;

					pm->trace(trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;

	return qfalse;
}

static void PM_GroundTrace(void) {
	vec3_t point;
	trace_t trace;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] - 0.25;

	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if(trace.allsolid) {
		if(!PM_CorrectAllSolid(&trace)) return;
	}

	// check if getting thrown off the ground
	if(pm->ps->velocity[2] > 0 && DotProduct(pm->ps->velocity, trace.plane.normal) > 10) {
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// slopes that are too steep will not be considered onground
	if(trace.plane.normal[2] < MIN_WALK_NORMAL) {
		// FIXME: if they can't slide down the slope, let them
		// walk (sharp crevices)
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qtrue;
		pml.walking = qfalse;
		return;
	}

	pml.groundPlane = qtrue;
	pml.walking = qtrue;

	// hitting solid ground will end a waterjump
	if(pm->ps->pm_flags & PMF_TIME_WATERJUMP) {
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time = 0;
	}

	if(pm->ps->groundEntityNum == ENTITYNUM_NONE) {
		PM_CrashLand();

		// don't do landing time if we were just going down a slope
		if(pml.previous_velocity[2] < -200) {
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time = 250;
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;

	PM_AddTouchEnt(trace.entityNum);
}

static void PM_SetWaterLevel(void) {
	vec3_t point;
	int cont;
	int sample1;
	int sample2;

	pm->waterlevel = 0;
	pm->watertype = 0;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + MINS_Z + 1;
	cont = pm->pointcontents(point, pm->ps->clientNum);

	if(cont & MASK_WATER) {
		sample2 = pm->ps->viewheight - MINS_Z;
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pm->ps->origin[2] + MINS_Z + sample1;
		cont = pm->pointcontents(point, pm->ps->clientNum);
		if(cont & MASK_WATER) {
			pm->waterlevel = 2;
			point[2] = pm->ps->origin[2] + MINS_Z + sample2;
			cont = pm->pointcontents(point, pm->ps->clientNum);
			if(cont & MASK_WATER) {
				pm->waterlevel = 3;
			}
		}
	}
}

/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static void PM_CheckDuck(void) {
	trace_t trace;

	pm->mins[0] = -15;
	pm->mins[1] = -15;
	pm->mins[2] = MINS_Z;

	pm->maxs[0] = 15;
	pm->maxs[1] = 15;

	if(pm->ps->pm_type == PM_DEAD) {
		pm->maxs[2] = -8;
		pm->ps->viewheight = DEAD_VIEWHEIGHT;
		return;
	}

	if(ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_U) < 0) {               // duck
		if(!pm->ps->stats[STAT_VEHICLE]) { // VEHICLE-SYSTEM: disable duck for all
			pm->ps->pm_flags |= PMF_DUCKED;
		} else {
		}
	} else { // stand up if possible
		if(pm->ps->pm_flags & PMF_DUCKED) {
			// try to stand up
			pm->maxs[2] = 32;
			pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask);
			if(!trace.allsolid) pm->ps->pm_flags &= ~PMF_DUCKED;
		}
	}

	if(pm->ps->pm_flags & PMF_DUCKED) {
		pm->maxs[2] = 16;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
	} else {
		pm->maxs[2] = 32;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
	}

	if(pm->ps->stats[STAT_VEHICLE]) { // VEHICLE-SYSTEM: collision for all
		pm->mins[0] = -25;
		pm->mins[1] = -25;
		pm->mins[2] = -15;
		pm->maxs[0] = 25;
		pm->maxs[1] = 25;
		pm->maxs[2] = 15;
	}
}

static void PM_Footsteps(void) {
	float bobmove;
	int old;
	qboolean footstep;

	pm->xyspeed = sqrt(pm->ps->velocity[0] * pm->ps->velocity[0] + pm->ps->velocity[1] * pm->ps->velocity[1]);

	if(pm->ps->groundEntityNum == ENTITYNUM_NONE) return;

	// if not trying to move
	if(!ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_F) && !ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_R)) {
		if(pm->xyspeed < 5) pm->ps->bobCycle = 0; // start at beginning of cycle again
		return;
	}

	footstep = qfalse;

	if(pm->ps->pm_flags & PMF_DUCKED) {
		bobmove = 0.5; // ducked characters bob much faster
	} else {
		bobmove = 0.4f; // faster speeds bob faster
	    footstep = qtrue;
	}

	// check for footstep / splash sounds
	old = pm->ps->bobCycle;
	pm->ps->bobCycle = (int)(old + bobmove * pml.msec) & 255;

	// if we just crossed a cycle boundary, play an apropriate footstep event
	if(((old + 64) ^ (pm->ps->bobCycle + 64)) & 128) {
		if(pm->waterlevel == 0) {
			PM_AddEvent(PM_FootstepForSurface());
		} else if(pm->waterlevel == 1) {
			// splashing
			PM_AddEvent(EV_FOOTSPLASH);
		} else if(pm->waterlevel == 2) {
			// wading / swimming at surface
			PM_AddEvent(EV_SWIM);
		} else if(pm->waterlevel == 3) {
			// no sound when completely underwater
		}
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents(void) {
	if(!pml.previous_waterlevel && pm->waterlevel) {
		PM_AddEvent(EV_WATER_TOUCH);
	}

	if(pml.previous_waterlevel && !pm->waterlevel) {
		PM_AddEvent(EV_WATER_LEAVE);
	}

	if(pml.previous_waterlevel != 3 && pm->waterlevel == 3) {
		PM_AddEvent(EV_WATER_UNDER);
	}

	if(pml.previous_waterlevel == 3 && pm->waterlevel != 3) {
		PM_AddEvent(EV_WATER_CLEAR);
	}
}

/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter
==============
*/
static void PM_Weapon(void) {
	int addTime;

	// don't allow attack until all buttons are up
	if(pm->ps->pm_flags & PMF_RESPAWNED) return;

	// ignore if spectator
	if(pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR || pm->ps->pm_type == PM_SPECTATOR) return;

	// check for dead player
	if(pm->ps->stats[STAT_HEALTH] <= 0) {
		pm->ps->weapon = WP_NONE;
		return;
	}

	// make weapon function
	if(pm->ps->weaponTime > 0) pm->ps->weaponTime -= pml.msec;
	if(pm->ps->weaponTime > 0) return;

	// check for fire
	if(!(pm->cmd.buttons & BUTTON_ATTACK)) {
		pm->ps->weaponTime = 0;
		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	if(BG_InVehicle(pm->ps->stats[STAT_VEHICLE])) return;

	// start the animation even if out of ammo
	if(pm->ps->weapon == WP_GAUNTLET) {
		// the guantlet only "fires" when it actually hits something
		if(!pm->gauntletHit) {
			pm->ps->weaponTime = 0;
			pm->ps->weaponstate = WEAPON_READY;
			return;
		}
	}
	
	pm->ps->weaponstate = WEAPON_FIRING;

#ifdef GAME
	if(!G_CheckWeaponAmmo(pm->ps->clientNum, pm->ps->weapon)) {
		PM_AddEvent(EV_NOAMMO);
		pm->ps->weaponTime += 500;
		return;
	}
#else
	if(!pm->ps->stats[STAT_AMMO]) {
		PM_AddEvent(EV_NOAMMO);
		pm->ps->weaponTime += 500;
		return;
	}
#endif

#ifdef GAME
	if(!(pm->ps->stats[STAT_AMMO] == -1 || pm->ps->stats[STAT_AMMO] >= 9999)) {
		if(G_CheckWeaponAmmo(pm->ps->clientNum, pm->ps->weapon) > 0) {
			PM_Add_SwepAmmo(pm->ps->clientNum, pm->ps->weapon, -1);
		}
	}
#endif
	PM_AddEvent(EV_FIRE_WEAPON);
	addTime = gameInfoWeapons[pm->ps->weapon].delay;
	pm->ps->weaponTime += addTime;
}

static void PM_DropTimers(void) {
	// drop misc timing counter
	if(pm->ps->pm_time) {
		if(pml.msec >= pm->ps->pm_time) {
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		} else {
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
	if(pm->ps->legsTimer > 0) {
		pm->ps->legsTimer -= pml.msec;
		if(pm->ps->legsTimer < 0) {
			pm->ps->legsTimer = 0;
		}
	}

	if(pm->ps->torsoTimer > 0) {
		pm->ps->torsoTimer -= pml.msec;
		if(pm->ps->torsoTimer < 0) {
			pm->ps->torsoTimer = 0;
		}
	}
}

/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated isntead of a full move
================
*/
void PM_UpdateViewAngles(playerState_t *ps, const usercmd_t *cmd) {
	short temp;
	int i;

	if(pm->cmd.buttons & BUTTON_USE && pm->cmd.buttons & BUTTON_ATTACK && pm->ps->weapon == WP_PHYSGUN) {
		return;
	}

	if(ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPINTERMISSION) {
		return; // no view changes at all
	}

	if(ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0) {
		return; // no view changes at all
	}

	// circularly clamp the angles with deltas
	for(i = 0; i < 3; i++) {
		temp = cmd->angles[i] + ps->delta_angles[i];
		if(i == PITCH) {
			// don't let the player look up or down more than 90 degrees
			if(temp > 16000) {
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp = 16000;
			} else if(temp < -16000) {
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp = -16000;
			}
		}
		ps->viewangles[i] = SHORT2ANGLE(temp);
	}
}

static void PmoveSingle(pmove_t *pmove) {
	pm = pmove;

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
	c_pmove++;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

	if(pm->ps->stats[STAT_HEALTH] <= 0) {
		pm->tracemask &= ~CONTENTS_BODY; // corpses can fly through bodies
	}

	// set the firing flag for continuous beam weapons
	if(!(pm->ps->pm_flags & PMF_RESPAWNED) && pm->ps->pm_type != PM_INTERMISSION && (pm->cmd.buttons & BUTTON_ATTACK) && pm->ps->stats[STAT_AMMO]) {
		pm->ps->eFlags |= EF_FIRING;
	} else {
		pm->ps->eFlags &= ~EF_FIRING;
	}

	// clear the respawned flag if attack and use are cleared
	if(pm->ps->stats[STAT_HEALTH] > 0 && !(pm->cmd.buttons & BUTTON_ATTACK)) {
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// clear all pmove local vars
	memset(&pml, 0, sizeof(pml));

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if(pml.msec < 1) {
		pml.msec = 1;
	} else if(pml.msec > 200) {
		pml.msec = 200;
	}
	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	VectorCopy(pm->ps->origin, pml.previous_origin);

	// save old velocity for crashlanding
	VectorCopy(pm->ps->velocity, pml.previous_velocity);

	pml.frametime = pml.msec * 0.001;

	// update the viewangles
	PM_UpdateViewAngles(pm->ps, &pm->cmd);

	AngleVectors(pm->ps->viewangles, pml.forward, pml.right, pml.up);

	if(ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_U) <= 0) pm->ps->pm_flags &= ~PMF_JUMP_HELD;

	// decide if backpedaling animations should be used
	if(ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_F) < 0) {
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	} else if(ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_F) > 0 || (ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_F) == 0 && ST_GetPlayerMove(pm->cmd.buttons, PLAYER_MOVE_R))) {
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if(pm->ps->pm_type >= PM_DEAD) {
		pm->cmd.buttons &= ~(BMOVE_W | BMOVE_A | BMOVE_S | BMOVE_D | BMOVE_J | BMOVE_C);
	}

	if(pm->ps->pm_type == PM_SPECTATOR) {
		PM_CheckDuck();
		PM_NoclipMove();
		PM_DropTimers();
		return;
	}

	if(pm->ps->pm_type == PM_NOCLIP) {
		PM_NoclipMove();
		PM_DropTimers();
		PM_Weapon();
		PM_CheckDuck();
		PM_GroundTrace();
		return;
	}

	if(pm->ps->pm_type == PM_FREEZE) {
		PM_CheckDuck(); // to make the player stand up, otherwise he'll be in a crouched position
		return;         // no movement at all
	}

	if(pm->ps->pm_type == PM_INTERMISSION || pm->ps->pm_type == PM_SPINTERMISSION) {
		return; // no movement at all
	}

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	// set mins, maxs, and viewheight
	PM_CheckDuck();

	// set groundentity
	PM_GroundTrace();

	if(pm->ps->pm_type == PM_DEAD) {
		PM_DeadMove();
	}

	PM_DropTimers();

	if(pm->ps->pm_flags & PMF_TIME_WATERJUMP) {
		PM_WaterJumpMove();
	} else if(pm->waterlevel > 1) {
		// swimming
		PM_WaterMove();
	} else if(pml.walking) {
		// walking on ground
		PM_WalkMove();
	} else {
		// airborne
		PM_AirMove();
	}

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();
	PM_SetWaterLevel();

	// weapons
	PM_Weapon();

	// footstep events / legs animations
	if(!pm->ps->stats[STAT_VEHICLE]) { // VEHICLE-SYSTEM: footsteps lock for all
		PM_Footsteps();
	}

	// entering / leaving water splashes
	PM_WaterEvents();
}

/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove(pmove_t *pmove) {
	int finalTime;

	finalTime = pmove->cmd.serverTime;

	if(finalTime < pmove->ps->commandTime) {
		return; // should not happen
	}

	if(finalTime > pmove->ps->commandTime + 1000) {
		pmove->ps->commandTime = finalTime - 1000;
	}

	pmove->ps->pmove_framecount = (pmove->ps->pmove_framecount + 1) & ((1 << PS_PMOVEFRAMECOUNTBITS) - 1);

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while(pmove->ps->commandTime != finalTime) {
		int msec;

		msec = finalTime - pmove->ps->commandTime;

		if(msec < 1) {
			msec = 1; // ниже — нахер, баги, говно и обосрание
		} else if(msec > 33) {
			msec = 33; // выше — значит просадка ниже 30 FPS, режем на 33, чтобы игрок не творил хуйню
		}
		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle(pmove);

		if(pmove->ps->pm_flags & PMF_JUMP_HELD) pmove->cmd.buttons |= BMOVE_J;
	}
}
