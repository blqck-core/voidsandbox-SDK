// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

static void CG_PositionEntityOnTag(refEntity_t *entity, const refEntity_t *parent, qhandle_t parentModel, char *tagName) {
	int i;
	orientation_t lerped;

	// lerp the tag
	trap_R_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame, 1.0 - parent->backlerp, tagName);

	VectorCopy(parent->origin, entity->origin);
	for(i = 0; i < 3; i++) {
		VectorMA(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply(lerped.axis, ((refEntity_t *)parent)->axis, entity->axis);
	entity->backlerp = parent->backlerp;
}

void CG_PositionRotatedEntityOnTag(refEntity_t *entity, const refEntity_t *parent, qhandle_t parentModel, char *tagName) {
	int i;
	orientation_t lerped;
	vec3_t tempAxis[3];

	// lerp the tag
	trap_R_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame, 1.0 - parent->backlerp, tagName);

	VectorCopy(parent->origin, entity->origin);
	for(i = 0; i < 3; i++) {
		VectorMA(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply(entity->axis, lerped.axis, tempAxis);
	MatrixMultiply(tempAxis, ((refEntity_t *)parent)->axis, entity->axis);
}

void CG_SetEntitySoundPosition(centity_t *cent) {
	if(cent->currentState.solid == SOLID_BMODEL) {
		vec3_t origin;
		float *v;

		v = cgs.inlineModelMidpoints[cent->currentState.modelindex];
		VectorAdd(cent->lerpOrigin, v, origin);
		trap_S_UpdateEntityPosition(cent->currentState.number, origin);
	} else {
		trap_S_UpdateEntityPosition(cent->currentState.number, cent->lerpOrigin);
	}
}

static void CG_EntityEffects(centity_t *cent) {
	// update sound origins
	CG_SetEntitySoundPosition(cent);

	// add loop sound
	if(cent->currentState.loopSound) {
		if(cent->currentState.eType != ET_SPEAKER) {
			trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.gameSounds[cent->currentState.loopSound]);
		} else {
			trap_S_AddRealLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.gameSounds[cent->currentState.loopSound]);
		}
	}

	// constant light glow
	if(cent->currentState.constantLight) {
		int cl;
		float i, r, g, b;

		cl = cent->currentState.constantLight;

		r = (float)(cl & 0xFF) / 255.0;
		g = (float)((cl >> 8) & 0xFF) / 255.0;
		b = (float)((cl >> 16) & 0xFF) / 255.0;
		i = (float)((cl >> 24) & 0xFF) * 4.0;
		trap_R_AddLightToScene(cent->lerpOrigin, i * 8, r, g, b);
	}
}

static void CG_General(centity_t *cent) {
	refEntity_t ent;
	centity_t *weldroot;
	refEntity_t wheelfr;
	refEntity_t wheelfl;
	refEntity_t wheelrr;
	refEntity_t wheelrl;
	char str[MAX_QPATH];
	entityState_t *s1;
	int cl;
	int r, g, b;

	s1 = &cent->currentState;

	// if set to invisible, skip
	if(!s1->modelindex && !s1->modelindex2) return;

	memset(&ent, 0, sizeof(ent));
	memset(&wheelfr, 0, sizeof(wheelfr));
	memset(&wheelfl, 0, sizeof(wheelfl));
	memset(&wheelrr, 0, sizeof(wheelrr));
	memset(&wheelrl, 0, sizeof(wheelrl));

	// set frame
	ent.frame = s1->frame;
	ent.oldframe = ent.frame;
	ent.backlerp = 0;

	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);
	VectorCopy(s1->angles, cent->lerpAngles);

	if(!s1->modelindex2) {
		ent.hModel = cgs.gameModels[s1->modelindex];
	} else {
		ent.hModel = cgs.gameModels[s1->modelindex2];
	}
	ent.reType = RT_MODEL;
	if(!s1->modelindex2) {
		ent.customSkin = trap_R_RegisterSkin(va("mtr/%s/%i.skin", CG_ConfigString(CS_MODELS + s1->modelindex), s1->generic2));
	} else {
		ent.customSkin = trap_R_RegisterSkin(va("mtr/%s/%i.skin", CG_ConfigString(CS_MODELS + s1->modelindex2), s1->generic2));
	}
	if(s1->generic2 > 0) {
		if(!s1->modelindex2) {
			ent.customShader = trap_R_RegisterShader(va("mtr/%s/%i", CG_ConfigString(CS_MODELS + s1->modelindex), s1->generic2));
		} else {
			ent.customShader = trap_R_RegisterShader(va("mtr/%s/%i", CG_ConfigString(CS_MODELS + s1->modelindex2), s1->generic2));
		}
	}
	if(s1->generic2 == 255) {
		if(cg.snap->ps.weapon == WP_PHYSGUN || cg.snap->ps.weapon == WP_GRAVITYGUN || cg.snap->ps.weapon == WP_TOOLGUN) {
			ent.customShader = cgs.media.ptexShader[1];
		} else {
			ent.customShader = cgs.media.ptexShader[0];
		}
	}

	cl = cent->currentState.constantLight;
	r = (cl & 0xFF);
	g = ((cl >> 8) & 0xFF);
	b = ((cl >> 16) & 0xFF);
	ent.shaderRGBA[0] = r;
	ent.shaderRGBA[1] = g;
	ent.shaderRGBA[2] = b;
	ent.shaderRGBA[3] = 255;

	if(!s1->modelindex2) {
		Com_sprintf(str, sizeof(str), CG_ConfigString(CS_MODELS + s1->modelindex));
	} else {
		Com_sprintf(str, sizeof(str), CG_ConfigString(CS_MODELS + s1->modelindex2));
	}

	// Weld sync
	if(s1->otherEntityNum) {
		vec3_t forward, right, up;
		vec3_t rotatedOffset, finalPos;
		vec3_t start_origin;

		weldroot = &cg_entities[s1->otherEntityNum];
		if(weldroot->currentState.torsoAnim == OT_VEHICLE && weldroot->currentState.generic1 && weldroot->currentState.generic1 - 1 == cg.predictedPlayerState.clientNum) {
			VectorCopy(cg.predictedPlayerState.origin, start_origin);
		} else {
			VectorCopy(weldroot->lerpOrigin, start_origin);
		}

		//
		// Origin
		//

		AngleVectors(weldroot->currentState.apos.trBase, forward, right, up);
		rotatedOffset[0] = forward[0] * s1->origin2[0] + right[0] * s1->origin2[1] + up[0] * s1->origin2[2];
		rotatedOffset[1] = forward[1] * s1->origin2[0] + right[1] * s1->origin2[1] + up[1] * s1->origin2[2];
		rotatedOffset[2] = forward[2] * s1->origin2[0] + right[2] * s1->origin2[1] + up[2] * s1->origin2[2];

		VectorAdd(start_origin, rotatedOffset, finalPos);

		VectorCopy(finalPos, ent.origin);
		VectorCopy(finalPos, ent.oldorigin);
	}

	// Client-side rotating
	if(s1->apos.trDelta[0] || s1->apos.trDelta[1] || s1->apos.trDelta[2]) {
		// Type
		float rotationMultiplier = (s1->pos.trType != TR_GRAVITY_WATER) ? PHYS_ROTATING : PHYS_ROTATING * 0.5;

		// Rotate
		cent->lerpAngles[0] -= s1->apos.trDelta[0] * rotationMultiplier;
		cent->lerpAngles[1] -= s1->apos.trDelta[1] * rotationMultiplier;
		cent->lerpAngles[0] -= s1->apos.trDelta[2] * rotationMultiplier * 0.20;
		cent->lerpAngles[1] -= s1->apos.trDelta[2] * rotationMultiplier * 0.20;
	}

	// convert angles to axis
	AnglesToAxis(cent->lerpAngles, ent.axis);

	if(s1->scales[0] != 0.0) VectorScale(ent.axis[0], s1->scales[0], ent.axis[0]);
	if(s1->scales[1] != 0.0) VectorScale(ent.axis[1], s1->scales[1], ent.axis[1]);
	if(s1->scales[2] != 0.0) VectorScale(ent.axis[2], s1->scales[2], ent.axis[2]);

	if(s1->torsoAnim == OT_VEHICLE && s1->generic1 && s1->generic1 - 1 == cg.predictedPlayerState.clientNum) {
		if(VectorLength(cg.predictedPlayerState.velocity) > 5) {
			VectorCopy(cg.predictedPlayerState.origin, ent.origin);
			VectorCopy(cg.predictedPlayerState.origin, ent.oldorigin);
			VelocityToAxis(cg.predictedPlayerState.velocity, ent.axis, 1.00f);
		}
	}

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	if(s1->torsoAnim == OT_VEHICLE) {
		trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.carengine[s1->legsAnim]);

		wheelfr.frame = s1->frame;
		wheelfr.oldframe = wheelfr.frame;
		wheelfr.backlerp = 0;

		VectorCopy(cent->lerpOrigin, wheelfr.origin);
		VectorCopy(cent->lerpOrigin, wheelfr.oldorigin);

		wheelfr.hModel = trap_R_RegisterModel("models/v_wheel");
		wheelfr.customSkin = ent.customSkin;
		wheelfr.customShader = ent.customShader;
		wheelfr.reType = RT_MODEL;

		AnglesToAxis(cent->lerpAngles, wheelfr.axis);

		if(s1->scales[0] != 0.0) VectorScale(wheelfr.axis[0], s1->scales[0], wheelfr.axis[0]);
		if(s1->scales[1] != 0.0) VectorScale(wheelfr.axis[1], s1->scales[1], wheelfr.axis[1]);
		if(s1->scales[2] != 0.0) VectorScale(wheelfr.axis[2], s1->scales[2], wheelfr.axis[2]);

		CG_PositionEntityOnTag(&wheelfr, &ent, ent.hModel, "tag_wheelfr");

		wheelfl.frame = s1->frame;
		wheelfl.oldframe = wheelfl.frame;
		wheelfl.backlerp = 0;

		VectorCopy(cent->lerpOrigin, wheelfl.origin);
		VectorCopy(cent->lerpOrigin, wheelfl.oldorigin);

		wheelfl.hModel = trap_R_RegisterModel("models/v_wheel");
		wheelfl.customSkin = ent.customSkin;
		wheelfl.customShader = ent.customShader;
		wheelfl.reType = RT_MODEL;

		AnglesToAxis(cent->lerpAngles, wheelfl.axis);

		if(s1->scales[0] != 0.0) VectorScale(wheelfl.axis[0], s1->scales[0], wheelfl.axis[0]);
		if(s1->scales[1] != 0.0) VectorScale(wheelfl.axis[1], s1->scales[1], wheelfl.axis[1]);
		if(s1->scales[2] != 0.0) VectorScale(wheelfl.axis[2], s1->scales[2], wheelfl.axis[2]);

		CG_PositionEntityOnTag(&wheelfl, &ent, ent.hModel, "tag_wheelfl");

		wheelrr.frame = s1->frame;
		wheelrr.oldframe = wheelrr.frame;
		wheelrr.backlerp = 0;

		VectorCopy(cent->lerpOrigin, wheelrr.origin);
		VectorCopy(cent->lerpOrigin, wheelrr.oldorigin);

		wheelrr.hModel = trap_R_RegisterModel("models/v_wheel");
		wheelrr.customSkin = ent.customSkin;
		wheelrr.customShader = ent.customShader;
		wheelrr.reType = RT_MODEL;

		AnglesToAxis(cent->lerpAngles, wheelrr.axis);

		if(s1->scales[0] != 0.0) VectorScale(wheelrr.axis[0], s1->scales[0], wheelrr.axis[0]);
		if(s1->scales[1] != 0.0) VectorScale(wheelrr.axis[1], s1->scales[1], wheelrr.axis[1]);
		if(s1->scales[2] != 0.0) VectorScale(wheelrr.axis[2], s1->scales[2], wheelrr.axis[2]);

		CG_PositionEntityOnTag(&wheelrr, &ent, ent.hModel, "tag_wheelrr");

		wheelrl.frame = s1->frame;
		wheelrl.oldframe = wheelrl.frame;
		wheelrl.backlerp = 0;

		VectorCopy(cent->lerpOrigin, wheelrl.origin);
		VectorCopy(cent->lerpOrigin, wheelrl.oldorigin);

		wheelrl.hModel = trap_R_RegisterModel("models/v_wheel");
		wheelrl.customSkin = ent.customSkin;
		wheelrl.customShader = ent.customShader;
		wheelrl.reType = RT_MODEL;

		AnglesToAxis(cent->lerpAngles, wheelrl.axis);

		if(s1->scales[0] != 0.0) VectorScale(wheelrl.axis[0], s1->scales[0], wheelrl.axis[0]);
		if(s1->scales[1] != 0.0) VectorScale(wheelrl.axis[1], s1->scales[1], wheelrl.axis[1]);
		if(s1->scales[2] != 0.0) VectorScale(wheelrl.axis[2], s1->scales[2], wheelrl.axis[2]);

		CG_PositionEntityOnTag(&wheelrl, &ent, ent.hModel, "tag_wheelrl");

		trap_R_AddRefEntityToScene(&wheelfr);
		trap_R_AddRefEntityToScene(&wheelfl);
		trap_R_AddRefEntityToScene(&wheelrr);
		trap_R_AddRefEntityToScene(&wheelrl);
	}
}

static void CG_Speaker(centity_t *cent) {
	if(!cent->currentState.clientNum) return;
	if(cg.time < cent->miscTime) return;

	trap_S_StartSound(NULL, cent->currentState.number, CHAN_ITEM, cgs.gameSounds[cent->currentState.eventParm]);
	cent->miscTime = cg.time + cent->currentState.frame * 100 + cent->currentState.clientNum * 100 * crandom();
}

static void CG_Item(centity_t *cent) {
	refEntity_t ent;
	entityState_t *es;
	item_t *item;
	int msec;
	float frac;
	weaponInfo_t *wi;

	es = &cent->currentState;
	if(es->modelindex >= gameInfoItemsNum) {
		CG_Error("Bad item index %i on entity", es->modelindex);
		return;
	}

	CG_RegisterItemVisuals(es->modelindex);

	// if set to invisible, skip
	if(!es->modelindex || (es->eFlags & EF_NODRAW)) return;

	item = &gameInfoItems[es->modelindex];

	memset(&ent, 0, sizeof(ent));

	// autorotate at one of two speeds
	if(item->giType == IT_HEALTH) {
		VectorCopy(cg.autoAnglesFast, cent->lerpAngles);
		AxisCopy(cg.autoAxisFast, ent.axis);
	} else {
		VectorCopy(cg.autoAngles, cent->lerpAngles);
		AxisCopy(cg.autoAxis, ent.axis);
	}

	wi = NULL;
	// the weapons have their origin where they attatch to player
	// models, so we need to offset them or they will rotate
	// eccentricly
	if(item->giType == IT_WEAPON) {
		wi = &cg_weapons[item->giTag];
		cent->lerpOrigin[0] -= wi->weaponMidpoint[0] * ent.axis[0][0] + wi->weaponMidpoint[1] * ent.axis[1][0] + wi->weaponMidpoint[2] * ent.axis[2][0];
		cent->lerpOrigin[1] -= wi->weaponMidpoint[0] * ent.axis[0][1] + wi->weaponMidpoint[1] * ent.axis[1][1] + wi->weaponMidpoint[2] * ent.axis[2][1];
		cent->lerpOrigin[2] -= wi->weaponMidpoint[0] * ent.axis[0][2] + wi->weaponMidpoint[1] * ent.axis[1][2] + wi->weaponMidpoint[2] * ent.axis[2][2];

		cent->lerpOrigin[2] += 8; // an extra height boost
	}

	ent.hModel = cg_items[es->modelindex].model;

	ent.reType = RT_MODEL;

	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	ent.nonNormalizedAxes = qfalse;

	// if just respawned, slowly scale up
	msec = cg.time - cent->miscTime;
	if(msec >= 0 && msec < ITEM_SCALEUP_TIME) {
		frac = (float)msec / ITEM_SCALEUP_TIME;
		VectorScale(ent.axis[0], frac, ent.axis[0]);
		VectorScale(ent.axis[1], frac, ent.axis[1]);
		VectorScale(ent.axis[2], frac, ent.axis[2]);
		ent.nonNormalizedAxes = qtrue;
	} else {
		frac = 1.0;
		if(es->scales[0] != 0.0) {
			VectorScale(ent.axis[0], es->scales[0], ent.axis[0]);
		}
		if(es->scales[1] != 0.0) {
			VectorScale(ent.axis[1], es->scales[1], ent.axis[1]);
		}
		if(es->scales[2] != 0.0) {
			VectorScale(ent.axis[2], es->scales[2], ent.axis[2]);
		}
	}

	// increase the size of the weapons when they are presented as items
	if(item->giType == IT_WEAPON) {
		VectorScale(ent.axis[0], 1.5, ent.axis[0]);
		VectorScale(ent.axis[1], 1.5, ent.axis[1]);
		VectorScale(ent.axis[2], 1.5, ent.axis[2]);
		ent.nonNormalizedAxes = qtrue;
	}

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	if(item->giType == IT_WEAPON && wi->barrelModel) {
		refEntity_t barrel;

		memset(&barrel, 0, sizeof(barrel));

		barrel.hModel = wi->barrelModel;

		VectorCopy(ent.lightingOrigin, barrel.lightingOrigin);
		barrel.renderfx = ent.renderfx;

		CG_PositionRotatedEntityOnTag(&barrel, &ent, wi->weaponModel, "tag_barrel");

		AxisCopy(ent.axis, barrel.axis);
		barrel.nonNormalizedAxes = ent.nonNormalizedAxes;

		trap_R_AddRefEntityToScene(&barrel);
	}
}

static void CG_Missile(centity_t *cent) {
	refEntity_t ent;
	entityState_t *s1;
	const weaponInfo_t *weapon;

	s1 = &cent->currentState;
	if(s1->weapon > WEAPONS_NUM) s1->weapon = 0;
	weapon = &cg_weapons[s1->weapon];

	// calculate the axis
	VectorCopy(s1->angles, cent->lerpAngles);

	// add trails
	if(weapon->missileTrailFunc) weapon->missileTrailFunc(cent, weapon);

	// add dynamic light
	if(weapon->missileDlight) trap_R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight, weapon->missileDlightColor[0], weapon->missileDlightColor[1], weapon->missileDlightColor[2]);

	// add missile sound
	if(weapon->missileSound) {
		vec3_t velocity;
		BG_EvaluateTrajectoryDelta(&cent->currentState.pos, cg.time, velocity);
		trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, velocity, weapon->missileSound);
	}

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	if(cent->currentState.weapon == WP_PLASMAGUN) {
		ent.reType = RT_SPRITE;
		ent.radius = 16;
		ent.rotation = 0;
		ent.customShader = cgs.media.plasmaBallShader;
		trap_R_AddRefEntityToScene(&ent);
		return;
	}

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx;

	if(cent->currentState.weapon == WP_PROX_LAUNCHER) {
		if(s1->generic1 == TEAM_BLUE) ent.hModel = cgs.media.blueProxMine;
	}

	// convert direction of travel into axis
	if(VectorNormalize2(s1->pos.trDelta, ent.axis[0]) == 0) ent.axis[0][2] = 1;

	// spin as it moves
	if(s1->pos.trType != TR_STATIONARY) {
		RotateAroundDirection(ent.axis, cg.time / 4);
	} else {
		if(s1->weapon == WP_PROX_LAUNCHER) {
			AnglesToAxis(cent->lerpAngles, ent.axis);
		} else {
			RotateAroundDirection(ent.axis, s1->time);
		}
	}

	trap_R_AddRefEntityToScene(&ent);
}

static void CG_Mover(centity_t *cent) {
	refEntity_t ent;
	entityState_t *s1;

	s1 = &cent->currentState;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);
	AnglesToAxis(cent->lerpAngles, ent.axis);

	// flicker between two skins (FIXME?)
	ent.skinNum = (cg.time >> 6) & 1;

	// get the model, either as a bmodel or a modelindex
	if(s1->solid == SOLID_BMODEL) {
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	} else {
		ent.hModel = cgs.gameModels[s1->modelindex];
	}

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	// add the secondary model
	if(s1->modelindex2) {
		ent.skinNum = 0;
		ent.hModel = cgs.gameModels[s1->modelindex2];
		trap_R_AddRefEntityToScene(&ent);
	}
}

static void CG_Portal(centity_t *cent) {
	refEntity_t ent;
	entityState_t *s1;

	s1 = &cent->currentState;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(s1->origin2, ent.oldorigin);
	ByteToDir(s1->eventParm, ent.axis[0]);
	PerpendicularVector(ent.axis[1], ent.axis[0]);

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	VectorSubtract(vec3_origin, ent.axis[1], ent.axis[1]);

	CrossProduct(ent.axis[0], ent.axis[1], ent.axis[2]);
	ent.reType = RT_PORTALSURFACE;
	ent.oldframe = s1->powerups;
	ent.frame = s1->frame;                     // rotation speed
	ent.skinNum = s1->clientNum / 256.0 * 360; // roll offset

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}

void CG_AdjustPositionForMover(const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out) {
	centity_t *cent;
	vec3_t oldOrigin, origin, deltaOrigin;
	vec3_t oldAngles, angles, deltaAngles;

	if(moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL) {
		VectorCopy(in, out);
		return;
	}

	cent = &cg_entities[moverNum];
	if(cent->currentState.eType != ET_MOVER) {
		VectorCopy(in, out);
		return;
	}

	BG_EvaluateTrajectory(&cent->currentState.pos, fromTime, oldOrigin);
	BG_EvaluateTrajectory(&cent->currentState.apos, fromTime, oldAngles);

	BG_EvaluateTrajectory(&cent->currentState.pos, toTime, origin);
	BG_EvaluateTrajectory(&cent->currentState.apos, toTime, angles);

	VectorSubtract(origin, oldOrigin, deltaOrigin);
	VectorSubtract(angles, oldAngles, deltaAngles);

	VectorAdd(in, deltaOrigin, out);
}

static void CG_InterpolateEntityPosition(centity_t *cent, qboolean STPhys) {
	vec3_t current, next;
	float f;

	// it would be an internal error to find an entity that interpolates without
	// a snapshot ahead of the current one
	if(cg.nextSnap == NULL) {
		CG_Error("CG_InterpoateEntityPosition: cg.nextSnap == NULL");
	}

	f = cg.frameInterpolation;

	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	if(STPhys) {
		ST_EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, current, cent->currentState.angles2[A2_MASS]);
		ST_EvaluateTrajectory(&cent->nextState.pos, cg.nextSnap->serverTime, next, cent->currentState.angles2[A2_MASS]);
	} else {
		BG_EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, current);
		BG_EvaluateTrajectory(&cent->nextState.pos, cg.nextSnap->serverTime, next);
	}

	cent->lerpOrigin[0] = current[0] + f * (next[0] - current[0]);
	cent->lerpOrigin[1] = current[1] + f * (next[1] - current[1]);
	cent->lerpOrigin[2] = current[2] + f * (next[2] - current[2]);

	if(STPhys) {
		ST_EvaluateTrajectory(&cent->currentState.apos, cg.snap->serverTime, current, cent->currentState.angles2[A2_MASS]);
		ST_EvaluateTrajectory(&cent->nextState.apos, cg.nextSnap->serverTime, next, cent->currentState.angles2[A2_MASS]);
	} else {
		BG_EvaluateTrajectory(&cent->currentState.apos, cg.snap->serverTime, current);
		BG_EvaluateTrajectory(&cent->nextState.apos, cg.nextSnap->serverTime, next);
	}

	cent->lerpAngles[0] = LerpAngle(current[0], next[0], f);
	cent->lerpAngles[1] = LerpAngle(current[1], next[1], f);
	cent->lerpAngles[2] = LerpAngle(current[2], next[2], f);
}

static void CG_CalcEntityLerpPositions(centity_t *cent, qboolean STPhys) {
	clientInfo_t *ci;
	int timeshift = 0;

	ci = &cgs.clientinfo[cent->currentState.clientNum];

	if(cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE || cent->interpolate && cent->currentState.pos.trType == TR_LINEAR_STOP && cent->currentState.number < MAX_CLIENTS) {
		if(STPhys) {
			CG_InterpolateEntityPosition(cent, qfalse);
		} else {
			CG_InterpolateEntityPosition(cent, qfalse);
		}
		return;
	}

	if(cent->currentState.number < MAX_CLIENTS && cent->currentState.clientNum != cg.predictedPlayerState.clientNum) {
		cent->currentState.pos.trType = TR_LINEAR_STOP;
		cent->currentState.pos.trTime = cg.snap->serverTime;
		cent->currentState.pos.trDuration = 1000 / cvarInt("sv_fps");
	}

	// just use the current frame and evaluate as best we can
	if(STPhys) {
		ST_EvaluateTrajectory(&cent->currentState.pos, cg.time + timeshift, cent->lerpOrigin, cent->currentState.angles2[A2_MASS]);
		ST_EvaluateTrajectory(&cent->currentState.apos, cg.time + timeshift, cent->lerpAngles, cent->currentState.angles2[A2_MASS]);
	} else {
		BG_EvaluateTrajectory(&cent->currentState.pos, cg.time + timeshift, cent->lerpOrigin);
		BG_EvaluateTrajectory(&cent->currentState.apos, cg.time + timeshift, cent->lerpAngles);
	}

	// if there's a time shift
	if(timeshift != 0) {
		trace_t tr;
		vec3_t lastOrigin;

		if(STPhys) {
			ST_EvaluateTrajectory(&cent->currentState.pos, cg.time, lastOrigin, cent->currentState.angles2[A2_MASS]);
		} else {
			BG_EvaluateTrajectory(&cent->currentState.pos, cg.time, lastOrigin);
		}

		CG_Trace(&tr, lastOrigin, vec3_origin, vec3_origin, cent->lerpOrigin, cent->currentState.number, MASK_SHOT);

		// don't let the projectile go through the floor
		if(tr.fraction < 1.0f) {
			cent->lerpOrigin[0] = lastOrigin[0] + tr.fraction * (cent->lerpOrigin[0] - lastOrigin[0]);
			cent->lerpOrigin[1] = lastOrigin[1] + tr.fraction * (cent->lerpOrigin[1] - lastOrigin[1]);
			cent->lerpOrigin[2] = lastOrigin[2] + tr.fraction * (cent->lerpOrigin[2] - lastOrigin[2]);
		}
	}

	// adjust for riding a mover if it wasn't rolled into the predicted
	// player state
	if(cent != &cg.predictedPlayerEntity) {
		CG_AdjustPositionForMover(cent->lerpOrigin, cent->currentState.groundEntityNum, cg.snap->serverTime, cg.time, cent->lerpOrigin);
	}
}

static void CG_AddCEntity(centity_t *cent) {
	// event-only entities will have been dealt with already
	if(cent->currentState.eType >= ET_EVENTS) return;

	// calculate the current origin
	if(cent->currentState.torsoAnim) {
		CG_CalcEntityLerpPositions(cent, qtrue);
	} else {
		CG_CalcEntityLerpPositions(cent, qfalse);
	}

	// add automatic effects
	CG_EntityEffects(cent);

	switch(cent->currentState.eType) {
	default: CG_Error("Bad entity type: %i\n", cent->currentState.eType); break;
	case ET_INVISIBLE: break;
	case ET_GENERAL: CG_General(cent); break;
	case ET_PLAYER: CG_Player(cent); break;
	case ET_ITEM: CG_Item(cent); break;
	case ET_MISSILE: CG_Missile(cent); break;
	case ET_MOVER: CG_Mover(cent); break;
	case ET_PORTAL: CG_Portal(cent); break;
	case ET_SPEAKER: CG_Speaker(cent); break;
	}
}

void CG_AddPacketEntities(void) {
	int num;
	centity_t *cent;
	playerState_t *ps;

	// set cg.frameInterpolation
	if(cg.nextSnap) {
		int delta;

		delta = (cg.nextSnap->serverTime - cg.snap->serverTime);
		if(delta == 0) {
			cg.frameInterpolation = 0;
		} else {
			cg.frameInterpolation = (float)(cg.time - cg.snap->serverTime) / delta;
		}
	} else {
		cg.frameInterpolation = 0; // actually, it should never be used, because no entities should be marked as interpolating
	}

	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = (cg.time & 4095) * 360 / 4096.0;
	cg.autoAngles[2] = 0;

	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = (cg.time & 4095) * 360 / 4096.0f;
	cg.autoAnglesFast[2] = 0;

	AnglesToAxis(cg.autoAngles, cg.autoAxis);
	AnglesToAxis(cg.autoAnglesFast, cg.autoAxisFast);

	// generate and add the entity from the playerstate
	ps = &cg.predictedPlayerState;
	BG_PlayerStateToEntityState(ps, &cg.predictedPlayerEntity.currentState, qfalse);
	CG_AddCEntity(&cg.predictedPlayerEntity);

	// lerp the non-predicted value for lightning gun origins
	CG_CalcEntityLerpPositions(&cg_entities[cg.snap->ps.clientNum], qfalse);

	// add each entity sent over by the server
	if(cg.nextSnap) {
		for(num = 0; num < cg.nextSnap->numEntities; num++) {
			cent = &cg_entities[cg.nextSnap->entities[num].number];
			if(cent->nextState.eType == ET_MISSILE || cent->nextState.eType == ET_GENERAL) {
				CG_TransitionEntity(cent);
				cent->interpolate = qtrue;
				CG_AddCEntity(cent);
			}
		}
	}

	for(num = 0; num < cg.snap->numEntities; num++) {
		cent = &cg_entities[cg.snap->entities[num].number];
		if(!cg.nextSnap || (cent->nextState.eType != ET_MISSILE && cent->nextState.eType != ET_GENERAL)) CG_AddCEntity(cent);
	}
}
