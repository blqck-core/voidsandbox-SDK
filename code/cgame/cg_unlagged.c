// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

void CG_PredictWeaponEffects(centity_t *cent) {
	vec3_t muzzlePoint, forward, right, up;
	entityState_t *ent = &cent->currentState;

	// if the client isn't us, forget it
	if(cent->currentState.number != cg.predictedPlayerState.clientNum) return;

	// get the muzzle point
	VectorCopy(cg.predictedPlayerState.origin, muzzlePoint);
	muzzlePoint[2] += cg.predictedPlayerState.viewheight;

	// get forward, right, and up
	AngleVectors(cg.predictedPlayerState.viewangles, forward, right, up);
	VectorMA(muzzlePoint, 14, forward, muzzlePoint);

	if(gameInfoWeapons[ent->weapon].wType == WT_RAILGUN) { // was it a rail attack?
		// do we have it on for the rail gun?
		trace_t trace;
		vec3_t endPoint;

		// trace forward
		VectorMA(muzzlePoint, gameInfoWeapons[ent->weapon].range, forward, endPoint);

		// find the rail's end point
		CG_Trace(&trace, muzzlePoint, vec3_origin, vec3_origin, endPoint, cg.predictedPlayerState.clientNum, CONTENTS_SOLID);

		// do the magic-number adjustment
		VectorMA(muzzlePoint, 4, right, muzzlePoint);
		VectorMA(muzzlePoint, -1, up, muzzlePoint);

		if(!cg.renderingThirdPerson) {
			if(cvarInt("cg_drawGun") == 2)
				VectorMA(muzzlePoint, 8, cg.refdef.viewaxis[1], muzzlePoint);
			else if(cvarInt("cg_drawGun") == 3)
				VectorMA(muzzlePoint, 4, cg.refdef.viewaxis[1], muzzlePoint);
		}

		// draw a rail trail
		CG_RailTrail(&cgs.clientinfo[cent->currentState.number], muzzlePoint, trace.endpos, ent->weapon);

		// explosion at end if not SURF_NOIMPACT
		if(!(trace.surfaceFlags & SURF_NOIMPACT)) {
			// predict an explosion
			CG_MissileHitWall(ent->weapon, cg.predictedPlayerState.clientNum, trace.endpos, trace.plane.normal, IMPACTSOUND_DEFAULT);
		}
	} else if(gameInfoWeapons[ent->weapon].wType == WT_SHOTGUN) { // was it a shotgun attack?
		int contents;
		vec3_t endPoint, v;
		vec3_t up;

		// do everything like the server does
		SnapVector(muzzlePoint);

		VectorScale(forward, gameInfoWeapons[ent->weapon].range, endPoint);
		SnapVector(endPoint);

		VectorSubtract(endPoint, muzzlePoint, v);
		VectorNormalize(v);
		VectorScale(v, 32, v);
		VectorAdd(muzzlePoint, v, v);

		contents = trap_CM_PointContents(muzzlePoint, 0);
		if(!(contents & CONTENTS_WATER)) {
			VectorSet(up, 0, 0, 8);
			CG_SmokePuff(v, up, 32, 1, 1, 1, 0.33f, 900, cg.time, 0, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader);
		}

		// do the shotgun pellets
		CG_ShotgunPattern(muzzlePoint, endPoint, cg.oldTime % 256, cg.predictedPlayerState.clientNum, ent->weapon);
	} else if(gameInfoWeapons[ent->weapon].wType == WT_BULLET) { // was it a machinegun attack?
		int seed = cg.oldTime % 256;
		float r, u;
		trace_t tr;
		qboolean flesh;
		int fleshEntityNum = 0;
		vec3_t endPoint;

		// do everything exactly like the server does
		r = Q_random(&seed) * M_PI * 2.0f;
		u = sin(r) * Q_crandom(&seed) * gameInfoWeapons[ent->weapon].spread * 16;
		r = cos(r) * Q_crandom(&seed) * gameInfoWeapons[ent->weapon].spread * 16;

		VectorMA(muzzlePoint, gameInfoWeapons[ent->weapon].range, forward, endPoint);
		VectorMA(endPoint, r, right, endPoint);
		VectorMA(endPoint, u, up, endPoint);

		CG_Trace(&tr, muzzlePoint, NULL, NULL, endPoint, cg.predictedPlayerState.clientNum, MASK_SHOT);

		if(tr.surfaceFlags & SURF_NOIMPACT) return;

		// snap the endpos to integers, but nudged towards the line
		SnapVectorTowards(tr.endpos, muzzlePoint);

		if(tr.entityNum < MAX_CLIENTS) {
			flesh = qtrue;
			fleshEntityNum = tr.entityNum;
		} else {
			flesh = qfalse;
		}

		CG_Bullet(tr.endpos, cg.predictedPlayerState.clientNum, tr.plane.normal, flesh, fleshEntityNum);
	}
}
