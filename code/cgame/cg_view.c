// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

static void CG_CalcVrect(void) {
	cg.refdef.width = glconfig.vidWidth;
	cg.refdef.width &= ~1;

	cg.refdef.height = glconfig.vidHeight;
	cg.refdef.height &= ~1;

	cg.refdef.x = (glconfig.vidWidth - cg.refdef.width) / 2;
	cg.refdef.y = (glconfig.vidHeight - cg.refdef.height) / 2;
}

static void CG_OffsetThirdPersonView(void) {
	vec3_t forward, right, up;
	vec3_t view;
	vec3_t focusAngles;
	trace_t trace;
	static vec3_t mins = {-4, -4, -4};
	static vec3_t maxs = {4, 4, 4};
	vec3_t focusPoint;
	float focusDist;
	float forwardScale, sideScale;

	cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight;

	VectorCopy(cg.refdefViewAngles, focusAngles);

	if(focusAngles[PITCH] > 45) focusAngles[PITCH] = 45; // don't go too far overhead
	AngleVectors(focusAngles, forward, right, NULL);

	VectorMA(cg.refdef.vieworg, 512, forward, focusPoint);

	VectorCopy(cg.refdef.vieworg, view);

	view[2] += 8;
	if(!BG_InVehicle(cg.snap->ps.stats[STAT_VEHICLE])) {
		VectorMA(view, cvarFloat("cg_thirdPersonOffset"), right, view);
	} else {
		VectorMA(view, 0, right, view);
	}

	cg.refdefViewAngles[PITCH] *= 0.5;

	AngleVectors(cg.refdefViewAngles, forward, right, up);

	forwardScale = cos(0 / 180 * M_PI);
	sideScale = sin(0 / 180 * M_PI);
	if(!BG_InVehicle(cg.snap->ps.stats[STAT_VEHICLE])) {
		VectorMA(view, -cvarFloat("cg_thirdPersonRange") * forwardScale, forward, view);
		VectorMA(view, -cvarFloat("cg_thirdPersonRange") * sideScale, right, view);
	} else {
		VectorMA(view, -180 * forwardScale, forward, view);
		VectorMA(view, -180 * sideScale, right, view);
	}

	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything
	CG_Trace(&trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID);
	if(trace.fraction != 1.0) {
		VectorCopy(trace.endpos, view);
		view[2] += (1.0 - trace.fraction) * 32;
		// try another trace to this position, because a tunnel may have the ceiling
		// close enogh that this is poking out
		CG_Trace(&trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID);
		VectorCopy(trace.endpos, view);
	}

	VectorCopy(view, cg.refdef.vieworg);

	// select pitch to look at focus point from vieword
	VectorSubtract(focusPoint, cg.refdef.vieworg, focusPoint);
	focusDist = sqrt(focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1]);
	if(focusDist < 1) focusDist = 1; // should never happen
	cg.refdefViewAngles[PITCH] = -180 / M_PI * atan2(focusPoint[2], focusDist);
	cg.refdefViewAngles[YAW] -= 0;
}

static void CG_StepOffset(void) {
	int timeDelta;

	// smooth out stair climbing
	timeDelta = cg.time - cg.stepTime;
	if(timeDelta < STEP_TIME) cg.refdef.vieworg[2] -= cg.stepChange * (STEP_TIME - timeDelta) / STEP_TIME;
}

static void CG_OffsetFirstPersonView(void) {
	float *origin;
	float *angles;
	float bob;
	float delta;
	float speed;
	float f;
	vec3_t predictedVelocity;
	int timeDelta;

	if(cg.snap->ps.pm_type == PM_INTERMISSION) return;

	origin = cg.refdef.vieworg;
	angles = cg.refdefViewAngles;

	// add angles based on velocity
	VectorCopy(cg.predictedPlayerState.velocity, predictedVelocity);

	if(!cvarInt("cg_enableBobbing")) {
		angles[PITCH] += DotProduct(predictedVelocity, cg.refdef.viewaxis[0]);
		angles[ROLL] -= DotProduct(predictedVelocity, cg.refdef.viewaxis[1]);
	} else {
		delta = DotProduct(predictedVelocity, cg.refdef.viewaxis[0]);
		angles[PITCH] += delta * 0.002;
		delta = DotProduct(predictedVelocity, cg.refdef.viewaxis[1]);
		angles[ROLL] -= delta * 0.005;

		// make sure the bob is visible even at low speeds
		speed = cg.xyspeed > 200 ? cg.xyspeed : 200;
		delta = cg.bobfracsin * 0.002 * speed;
		if(cg.predictedPlayerState.pm_flags & PMF_DUCKED) delta *= 3; // crouching
		angles[PITCH] += delta;
		delta = cg.bobfracsin * 0.002 * speed;
		if(cg.predictedPlayerState.pm_flags & PMF_DUCKED) delta *= 3; // crouching accentuates roll
		if(cg.bobcycle & 1) delta = -delta;
		angles[ROLL] += delta;
	}

	// add view height
	origin[2] += cg.predictedPlayerState.viewheight;

	// smooth out duck height changes
	timeDelta = cg.time - cg.duckTime;
	if(timeDelta < DUCK_TIME) cg.refdef.vieworg[2] -= cg.duckChange * (DUCK_TIME - timeDelta) / DUCK_TIME;

	// add bob height
	if(!cvarInt("cg_enableBobbing")) {
		bob = 0.0f;
	} else {
		bob = cg.bobfracsin * cg.xyspeed * 0.002;
		if(bob > 6) {
			bob = 6;
		}
	}

	origin[2] += bob;

	// add fall height
	delta = cg.time - cg.landTime;
	if(delta < LAND_DEFLECT_TIME) {
		f = delta / LAND_DEFLECT_TIME;
		cg.refdef.vieworg[2] += cg.landChange * f;
	} else if(delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME) {
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - (delta / LAND_RETURN_TIME);
		cg.refdef.vieworg[2] += cg.landChange * f;
	}

	// add step offset
	CG_StepOffset();
}

void CG_ZoomDown_f(void) {
	if(cg.zoomed) return;
	if(cg.snap->ps.weapon != WP_PHYSGUN) {
		cg.zoomed = qtrue;
		cg.zoomTime = cg.time;
	} else {
		trap_Cmd(EXEC_INSERT, "altfire_physgun\n");
	}
}

void CG_ZoomUp_f(void) {
	if(!cg.zoomed) return;
	cg.zoomed = qfalse;
	cg.zoomTime = cg.time;
}

#define WAVE_AMPLITUDE 1
#define WAVE_FREQUENCY 0.4

static void CG_CalcFov(void) {
	float x;
	float phase;
	float v;
	int contents;
	float fov_x, fov_y;
	float zoomFov;
	float f;

	if(cg.predictedPlayerState.pm_type == PM_INTERMISSION) {
		// if in intermission, use a fixed value
		fov_x = 90;
	} else {
		fov_x = cvarFloat("cg_fov");
		if(fov_x < 1) {
			fov_x = 1;
		} else if(fov_x > 140) {
			fov_x = 140;
		}

		// account for zooms
		zoomFov = cvarFloat("cg_zoomFov");
		if(zoomFov < 1) {
			zoomFov = 1;
		} else if(zoomFov > 140) {
			zoomFov = 140;
		}

		if(cg.zoomed) {
			f = (cg.time - cg.zoomTime) / (float)ZOOM_TIME;
			if(f > 1.0) {
				fov_x = zoomFov;
			} else {
				fov_x = fov_x + f * (zoomFov - fov_x);
			}
		} else {
			f = (cg.time - cg.zoomTime) / (float)ZOOM_TIME;
			if(f > 1.0) {
				fov_x = fov_x;
			} else {
				fov_x = zoomFov + f * (fov_x - zoomFov);
			}
		}
	}

	x = cg.refdef.width / tan(fov_x / 360 * M_PI);
	fov_y = atan2(cg.refdef.height, x);
	fov_y = fov_y * 360 / M_PI;

	// warp if underwater
	contents = CG_PointContents(cg.refdef.vieworg, -1);
	if(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) {
		phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		v = WAVE_AMPLITUDE * sin(phase);
		fov_x += v;
		fov_y -= v;
	}

	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	if(!cg.zoomed) {
		cg.zoomSensitivity = 1;
	} else {
		cg.zoomSensitivity = cg.refdef.fov_y / 75.0;
	}

	return;
}

static void CG_DamageBlendBlob(void) {
	int t;
	int maxTime;
	refEntity_t ent;

	if(!cg.damageValue) return;

	maxTime = DAMAGE_TIME;
	t = cg.time - cg.damageTime;
	if(t <= 0 || t >= maxTime) return;

	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_FIRST_PERSON;

	VectorMA(cg.refdef.vieworg, 8, cg.refdef.viewaxis[0], ent.origin);
	VectorMA(ent.origin, cg.damageX * -8, cg.refdef.viewaxis[1], ent.origin);
	VectorMA(ent.origin, cg.damageY * 8, cg.refdef.viewaxis[2], ent.origin);

	ent.radius = cg.damageValue * 3;
	ent.customShader = cgs.media.viewBloodShader;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 200 * (1.0 - ((float)t / maxTime));
	trap_R_AddRefEntityToScene(&ent);
}

static void CG_CalcViewValues(void) {
	playerState_t *ps;

	memset(&cg.refdef, 0, sizeof(cg.refdef));

	// calculate size of 3D view
	CG_CalcVrect();

	ps = &cg.predictedPlayerState;

	// intermission view
	if(ps->pm_type == PM_INTERMISSION) {
		VectorCopy(ps->origin, cg.refdef.vieworg);
		VectorCopy(ps->viewangles, cg.refdefViewAngles);
		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
		CG_CalcFov();
		return;
	}

	cg.bobcycle = (ps->bobCycle & 128) >> 7;
	cg.bobfracsin = fabs(sin((ps->bobCycle & 127) / 127.0 * M_PI));

	cg.xyspeed = sqrt(ps->velocity[0] * ps->velocity[0] + ps->velocity[1] * ps->velocity[1]);

	VectorCopy(ps->origin, cg.refdef.vieworg);
	VectorCopy(ps->viewangles, cg.refdefViewAngles);

	if(cg.renderingThirdPerson) {
		// back away from character
		CG_OffsetThirdPersonView();
	} else {
		// offset for local bobbing and kicks
		CG_OffsetFirstPersonView();
	}

	// position eye reletive to origin
	AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);

	// field of view
	CG_CalcFov();
	return;
}

void CG_AddBufferedSound(sfxHandle_t sfx) {
	if(!sfx) return;
	cg.soundBuffer[cg.soundBufferIn] = sfx;
	cg.soundBufferIn = (cg.soundBufferIn + 1) % MAX_SOUNDBUFFER;
	if(cg.soundBufferIn == cg.soundBufferOut) cg.soundBufferOut++;
}

static void CG_PlayBufferedSounds(void) {
	if(cg.soundTime < cg.time) {
		if(cg.soundBufferOut != cg.soundBufferIn && cg.soundBuffer[cg.soundBufferOut]) {
			trap_S_StartLocalSound(cg.soundBuffer[cg.soundBufferOut], CHAN_ANNOUNCER);
			cg.soundBuffer[cg.soundBufferOut] = 0;
			cg.soundBufferOut = (cg.soundBufferOut + 1) % MAX_SOUNDBUFFER;
			cg.soundTime = cg.time + 750;
		}
	}
}

void CG_DrawActiveFrame(int serverTime, qboolean demoPlayback) {
	cg.time = serverTime;
	cg.demoPlayback = demoPlayback;

	ST_UpdateCGUI();

	if(cg.infoScreenText[0] != 0) {
		CG_DrawInformation();
		return;
	}

	trap_S_ClearLoopingSounds(qfalse);
	trap_R_ClearScene();
	CG_ProcessSnapshots();

	if(!cg.snap || (cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE)) {
		CG_DrawInformation();
		return;
	}

	trap_SetUserCmdValue(cg.zoomSensitivity);
	cg.clientFrame++;
	CG_PredictPlayerState();

	cg.renderingThirdPerson = cvarFloat("cg_thirdPerson") && cg.snap->ps.pm_type != PM_SPECTATOR || (cg.snap->ps.stats[STAT_HEALTH] <= 0) || cg.snap->ps.stats[STAT_VEHICLE];
	cg.renderingEyesPerson = !cvarFloat("cg_thirdPerson") && cvarInt("cg_cameraEyes") && cg.snap->ps.pm_type != PM_SPECTATOR || cg.snap->ps.stats[STAT_VEHICLE];

	CG_CalcViewValues();

	if(!cg.renderingThirdPerson) CG_DamageBlendBlob();

	// build the render lists
	CG_AddPacketEntities();
	CG_AddMarks();
	CG_AddLocalEntities();

	CG_AddViewWeapon(&cg.predictedPlayerState);
	CG_PlayBufferedSounds();
	cg.refdef.time = cg.time;
	memcpy(cg.refdef.areamask, cg.snap->areamask, sizeof(cg.refdef.areamask));
	trap_S_Respatialize(cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis);
	CG_DrawActive();

	// add frames
	cg.frametime = cg.time - cg.oldTime;
	if(cg.frametime < 0) cg.frametime = 0;
	cg.oldTime = cg.time;
}
