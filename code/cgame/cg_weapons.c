// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

static void CG_WeaponSelect(int id) {
    cg.weaponSelect = id;
    trap_Cmd(EXEC_NOW, va("wp %i", id));
}

static void CG_MachineGunEjectBrass(centity_t *cent) {
	localEntity_t *le;
	refEntity_t *re;
	vec3_t velocity, xvelocity;
	vec3_t offset, xoffset;
	float waterScale = 1.0f;
	vec3_t v[3];

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = 0;
	velocity[1] = -50 + 40 * crandom();
	velocity[2] = 100 + 50 * crandom();

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 5000 + (5000 / 4) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand() & 15);

	AnglesToAxis(cent->lerpAngles, v);

	offset[0] = 8;
	offset[1] = -4;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd(cent->lerpOrigin, xoffset, re->origin);

	VectorCopy(re->origin, le->pos.trBase);

	if(CG_PointContents(re->origin, -1) & CONTENTS_WATER) waterScale = 0.10f;

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale(xvelocity, waterScale, le->pos.trDelta);

	AxisCopy(axisDefault, re->axis);
	re->hModel = cgs.media.machinegunBrassModel;

	le->bounceFactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand() & 31;
	le->angles.trBase[1] = rand() & 31;
	le->angles.trBase[2] = rand() & 31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;
	le->leMarkType = LEMT_NONE;
}

static void CG_ShotgunEjectBrass(centity_t *cent) {
	localEntity_t *le;
	refEntity_t *re;
	vec3_t velocity, xvelocity;
	vec3_t offset, xoffset;
	vec3_t v[3];
	int i;

	for(i = 0; i < 2; i++) {
		float waterScale = 1.0f;

		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		velocity[0] = 60 + 60 * crandom();
		if(i == 0) {
			velocity[1] = 40 + 10 * crandom();
		} else {
			velocity[1] = -40 + 10 * crandom();
		}
		velocity[2] = 100 + 50 * crandom();

		le->leType = LE_FRAGMENT;
		le->startTime = cg.time;
		le->endTime = le->startTime + 15000 + 5000 * random();

		le->pos.trType = TR_GRAVITY;
		le->pos.trTime = cg.time;

		AnglesToAxis(cent->lerpAngles, v);

		offset[0] = 8;
		offset[1] = 0;
		offset[2] = 24;

		xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
		xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
		xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
		VectorAdd(cent->lerpOrigin, xoffset, re->origin);
		VectorCopy(re->origin, le->pos.trBase);
		if(CG_PointContents(re->origin, -1) & CONTENTS_WATER) waterScale = 0.10f;

		xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
		xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
		xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
		VectorScale(xvelocity, waterScale, le->pos.trDelta);

		AxisCopy(axisDefault, re->axis);
		re->hModel = cgs.media.shotgunBrassModel;
		le->bounceFactor = 0.3f;

		le->angles.trType = TR_LINEAR;
		le->angles.trTime = cg.time;
		le->angles.trBase[0] = rand() & 31;
		le->angles.trBase[1] = rand() & 31;
		le->angles.trBase[2] = rand() & 31;
		le->angles.trDelta[0] = 1;
		le->angles.trDelta[1] = 0.5;
		le->angles.trDelta[2] = 0;

		le->leFlags = LEF_TUMBLE;
		le->leMarkType = LEMT_NONE;
	}
}

void CG_RailTrail(clientInfo_t *ci, vec3_t start, vec3_t end, int weapon) {
	localEntity_t *le;
	refEntity_t *re;
	vec3_t s, e;
	VectorCopy(start, s);
	VectorCopy(end, e);

	if(weapon == WP_RAILGUN) {
		s[2] -= 4;
		e[2] -= 0;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;

	if(weapon == WP_RAILGUN) {
		le->endTime = cg.time + 400;
		re->shaderTime = cg.time / 1000.0f;
	} else {
		le->endTime = cg.time + 60;
		re->shaderTime = cg.time / 60.0f;
	}

	le->lifeRate = 1.0f / (le->endTime - le->startTime);

	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;

	VectorCopy(s, re->origin);
	VectorCopy(e, re->oldorigin);

	switch(weapon) {
	case WP_RAILGUN:
		re->shaderRGBA[0] = 0;
		re->shaderRGBA[1] = 255;
		re->shaderRGBA[2] = 0;
		re->shaderRGBA[3] = 255;

		le->color[0] = 0.0f;
		le->color[1] = 1.0f;
		le->color[2] = 0.0f;
		le->color[3] = 1.0f;

		re->origin[2] -= 8;
		re->oldorigin[2] -= 8;
		break;

	case WP_PHYSGUN:
		re->shaderRGBA[0] = 128;
		re->shaderRGBA[1] = 255;
		re->shaderRGBA[2] = 255;
		re->shaderRGBA[3] = 255;

		le->color[0] = 128 / 255.0f;
		le->color[1] = 255 / 255.0f;
		le->color[2] = 255 / 255.0f;
		le->color[3] = 1.0f;
		break;

	case WP_GRAVITYGUN:
		re->shaderRGBA[0] = 120;
		re->shaderRGBA[1] = 60;
		re->shaderRGBA[2] = 0;
		re->shaderRGBA[3] = 255;

		le->color[0] = 120.0f / 255.0f;
		le->color[1] = 60.0f / 255.0f;
		le->color[2] = 0.0f;
		le->color[3] = 1.0f;
		break;
	}

	AxisClear(re->axis);
}

static void CG_RocketTrail(centity_t *ent, const weaponInfo_t *wi) {
	int step;
	vec3_t origin, lastPos;
	int t;
	int startTime, contents;
	int lastContents;
	entityState_t *es;
	vec3_t up;
	localEntity_t *smoke;
	int therando;
	int theradio;

	up[0] = 5 - 10 * crandom();
	up[1] = 5 - 10 * crandom();
	up[2] = 8 - 5 * crandom();

	step = 18;

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ((startTime + step) / step);

	BG_EvaluateTrajectory(&es->pos, cg.time, origin);
	contents = CG_PointContents(origin, -1);

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if(es->pos.trType == TR_STATIONARY) {
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory(&es->pos, ent->trailTime, lastPos);
	lastContents = CG_PointContents(lastPos, -1);

	ent->trailTime = cg.time;

	if(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) {
		if(contents & lastContents & CONTENTS_WATER) CG_BubbleTrail(lastPos, origin, 8);
		return;
	}

	for(; t <= ent->trailTime; t += step) {
		BG_EvaluateTrajectory(&es->pos, t, lastPos);
		therando = crandom() * 4;

		theradio = wi->trailRadius * (rand() * 0.7); // what is this doing here
		if(therando == 3)
			smoke = CG_SmokePuff(lastPos, up, 27, 1, 1, 1, 0.9f, wi->wiTrailTime, t, 0, 0, cgs.media.lsmkShader1);
		else if(therando == 1)
			smoke = CG_SmokePuff(lastPos, up, 27, 1, 1, 1, 0.9f, wi->wiTrailTime, t, 0, 0, cgs.media.lsmkShader2);
		else if(therando == 2)
			smoke = CG_SmokePuff(lastPos, up, 27, 1, 1, 1, 0.9f, wi->wiTrailTime, t, 0, 0, cgs.media.lsmkShader3);
		else
			smoke = CG_SmokePuff(lastPos, up, 27, 1, 1, 1, 0.9f, wi->wiTrailTime, t, 0, 0, cgs.media.lsmkShader4);
		// use the optimized local entity add
		smoke->leType = LE_MOVE_SCALE_FADE;
		// smoke->trType = TR_GRAVITY;
	}
}

static void CG_NailTrail(centity_t *ent, const weaponInfo_t *wi) {
	int step;
	vec3_t origin, lastPos;
	int t;
	int startTime, contents;
	int lastContents;
	entityState_t *es;
	vec3_t up;
	localEntity_t *smoke;

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 50;

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ((startTime + step) / step);

	BG_EvaluateTrajectory(&es->pos, cg.time, origin);
	contents = CG_PointContents(origin, -1);

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if(es->pos.trType == TR_STATIONARY) {
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory(&es->pos, ent->trailTime, lastPos);
	lastContents = CG_PointContents(lastPos, -1);

	ent->trailTime = cg.time;

	if(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) {
		if(contents & lastContents & CONTENTS_WATER) CG_BubbleTrail(lastPos, origin, 8);
		return;
	}

	for(; t <= ent->trailTime; t += step) {
		BG_EvaluateTrajectory(&es->pos, t, lastPos);

		smoke = CG_SmokePuff(lastPos, up, wi->trailRadius, 1, 1, 1, 0.33f, wi->wiTrailTime, t, 0, 0, cgs.media.nailPuffShader);
		// use the optimized local entity add
		smoke->leType = LE_SCALE_FADE;
	}
}

void CG_RegisterWeapon(int weaponNum) {
	weaponInfo_t *weaponInfo;
	item_t *item, *ammo;
	char path[MAX_QPATH];
	vec3_t mins, maxs;
	int i;

	weaponInfo = &cg_weapons[weaponNum];

	if(weaponNum == 0) return;

	if(weaponInfo->registered) return;

	memset(weaponInfo, 0, sizeof(*weaponInfo));
	weaponInfo->registered = qtrue;

	for(item = gameInfoItems + 1; item->classname; item++) {
		if(item->giType == IT_WEAPON && item->giTag == weaponNum) {
			weaponInfo->item = item;
			break;
		}
	}

	iferr(!item->classname);
	CG_RegisterItemVisuals(item - gameInfoItems);

	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap_R_RegisterModel(item->world_model);

	// calc midpoint for rotation
	trap_R_ModelBounds(weaponInfo->weaponModel, mins, maxs);
	for(i = 0; i < 3; i++) {
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * (maxs[i] - mins[i]);
	}

	weaponInfo->weaponIcon = trap_R_RegisterShader(item->icon);
	weaponInfo->ammoIcon = trap_R_RegisterShader(item->icon);

	for(ammo = gameInfoItems + 1; ammo->classname; ammo++) {
		if(ammo->giType == IT_AMMO && ammo->giTag == weaponNum) {
			break;
		}
	}
	if(ammo->classname && ammo->world_model) weaponInfo->ammoModel = trap_R_RegisterModel(ammo->world_model);

	if(!weaponInfo->handsModel) weaponInfo->handsModel = trap_R_RegisterModel("models/weapons2/shotgun/shotgun_hand.md3");

	weaponInfo->loopFireSound = qfalse;

	switch(weaponNum) {
	case WP_GAUNTLET: break;

	case WP_LIGHTNING:
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		weaponInfo->readySound = trap_S_RegisterSound("sound/weapons/melee/fsthum.wav", qfalse);
		weaponInfo->firingSound = trap_S_RegisterSound("sound/weapons/lightning/lg_hum.wav", qfalse);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/lightning/lg_fire.wav", qfalse);
		cgs.media.lightningShader = trap_R_RegisterShader("lightningBoltNew");
		cgs.media.lightningExplosionModel = trap_R_RegisterModel("models/weaphits/crackle.md3");
		cgs.media.sfx_lghit1 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit.wav", qfalse);
		cgs.media.sfx_lghit2 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit2.wav", qfalse);
		cgs.media.sfx_lghit3 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit3.wav", qfalse);

		break;

	case WP_CHAINGUN:
		MAKERGB(weaponInfo->flashDlightColor, 1, 1, 0);
		weaponInfo->firingSound = trap_S_RegisterSound("sound/weapons/vulcan/wvulfire.wav", qfalse);
		weaponInfo->loopFireSound = qtrue;
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/vulcan/vulcanf1b.wav", qfalse);
		weaponInfo->flashSound[1] = trap_S_RegisterSound("sound/weapons/vulcan/vulcanf2b.wav", qfalse);
		weaponInfo->flashSound[2] = trap_S_RegisterSound("sound/weapons/vulcan/vulcanf3b.wav", qfalse);
		weaponInfo->flashSound[3] = trap_S_RegisterSound("sound/weapons/vulcan/vulcanf4b.wav", qfalse);
		weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader("bulletExplosion");
		break;

	case WP_MACHINEGUN:
		MAKERGB(weaponInfo->flashDlightColor, 1, 1, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/machinegun/machgf1b.wav", qfalse);
		weaponInfo->flashSound[1] = trap_S_RegisterSound("sound/weapons/machinegun/machgf2b.wav", qfalse);
		weaponInfo->flashSound[2] = trap_S_RegisterSound("sound/weapons/machinegun/machgf3b.wav", qfalse);
		weaponInfo->flashSound[3] = trap_S_RegisterSound("sound/weapons/machinegun/machgf4b.wav", qfalse);
		weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader("bulletExplosion");
		break;

	case WP_SHOTGUN:
		MAKERGB(weaponInfo->flashDlightColor, 1, 1, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/shotgun/sshotf1b.wav", qfalse);
		weaponInfo->ejectBrassFunc = CG_ShotgunEjectBrass;
		break;

	case WP_ROCKET_LAUNCHER:
		MAKERGB(weaponInfo->missileDlightColor, 1, 0.75f, 0);
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.75f, 0);
		weaponInfo->missileModel = trap_R_RegisterModel("models/ammo/rocket/rocket.md3");
		weaponInfo->missileSound = trap_S_RegisterSound("sound/weapons/rocket/rockfly.wav", qfalse);
		weaponInfo->missileTrailFunc = CG_RocketTrail;
		weaponInfo->missileDlight = 200;
		weaponInfo->wiTrailTime = 2000;
		weaponInfo->trailRadius = 64;
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/rocket/rocklf1a.wav", qfalse);
		cgs.media.rocketExplosionShader = trap_R_RegisterShader("rocketExplosion");
		break;

	case WP_PROX_LAUNCHER:
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.70f, 0);
		weaponInfo->missileModel = trap_R_RegisterModel("models/weaphits/proxmine.md3");
		weaponInfo->missileTrailFunc = CG_RocketTrail;
		weaponInfo->wiTrailTime = 700;
		weaponInfo->trailRadius = 32;
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/proxmine/wstbfire.wav", qfalse);
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader("grenadeExplosion");
		break;

	case WP_GRENADE_LAUNCHER:
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.70f, 0);
		weaponInfo->missileModel = trap_R_RegisterModel("models/ammo/grenade1.md3");
		weaponInfo->missileTrailFunc = CG_RocketTrail;
		weaponInfo->wiTrailTime = 700;
		weaponInfo->trailRadius = 32;
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/grenade/grenlf1a.wav", qfalse);
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader("grenadeExplosion");
		break;

	case WP_NAILGUN:
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.75f, 0);
		weaponInfo->missileTrailFunc = CG_NailTrail;
		weaponInfo->trailRadius = 16;
		weaponInfo->wiTrailTime = 250;
		weaponInfo->missileModel = trap_R_RegisterModel("models/weaphits/nail.md3");
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/nailgun/wnalfire.wav", qfalse);
		break;

	case WP_PLASMAGUN:
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.7f, 1);
		MAKERGB(weaponInfo->missileDlightColor, 0.40f, 1, 0.20f);
		weaponInfo->missileSound = trap_S_RegisterSound("sound/weapons/plasma/lasfly.wav", qfalse);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/plasma/hyprbf1a.wav", qfalse);
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader("plasmaExplosion");
		break;

	case WP_RAILGUN:
		weaponInfo->readySound = trap_S_RegisterSound("sound/weapons/railgun/rg_hum.wav", qfalse);
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.5f, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/railgun/railgf1a.wav", qfalse);
		cgs.media.railExplosionShader = trap_R_RegisterShader("railExplosion");
		cgs.media.railCoreShader = trap_R_RegisterShader("railCore");
		break;

	case WP_BFG:
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.7f, 1);
		MAKERGB(weaponInfo->missileDlightColor, 0.40f, 1, 0.20f);
		weaponInfo->readySound = trap_S_RegisterSound("sound/weapons/bfg/bfg_hum.wav", qfalse);
		weaponInfo->missileDlight = 200;
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/bfg/bfg_fire.wav", qfalse);
		cgs.media.bfgExplosionShader = trap_R_RegisterShader("bfgExplosion");
		weaponInfo->missileModel = trap_R_RegisterModel("models/weaphits/bfg.md3");
		weaponInfo->missileSound = trap_S_RegisterSound("sound/weapons/rocket/rockfly.wav", qfalse);
		break;

	case WP_TOOLGUN:
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/toolgun/fire1.wav", qfalse);
		weaponInfo->flashSound[1] = trap_S_RegisterSound("sound/weapons/toolgun/fire2.wav", qfalse);
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader("plasmaExplosion");
		cgs.media.lightningShader = trap_R_RegisterShader("lightningBoltNew");
		break;

	case WP_PHYSGUN:
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/misc/silence.wav", qfalse);
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader("plasmaExplosion");
		cgs.media.lightningShader = trap_R_RegisterShader("lightningBoltNew");
		break;

	case WP_GRAVITYGUN:
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/misc/silence.wav", qfalse);
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader("plasmaExplosion");
		cgs.media.lightningShader = trap_R_RegisterShader("lightningBoltNew");
		break;

	default:
		MAKERGB(weaponInfo->flashDlightColor, 1, 1, 1);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/rocket/rocklf1a.wav", qfalse);
		break;
	}
}

void CG_RegisterItemVisuals(int itemNum) {
	itemInfo_t *itemInfo;
	item_t *item;

	iferr(itemNum < 0 || itemNum >= gameInfoItemsNum);

	itemInfo = &cg_items[itemNum];
	if(itemInfo->registered) return;

	item = &gameInfoItems[itemNum];

	memset(itemInfo, 0, sizeof(&itemInfo));
	itemInfo->registered = qtrue;

	itemInfo->model = trap_R_RegisterModel(item->world_model);

	itemInfo->icon = trap_R_RegisterShader(item->icon);

	if(item->giType == IT_WEAPON) CG_RegisterWeapon(item->giTag);
}

static void CG_CalculateWeaponPosition(vec3_t origin, vec3_t angles) {
	float scale;
	int delta;
	float fracsin;

	VectorCopy(cg.refdef.vieworg, origin);
	VectorCopy(cg.refdefViewAngles, angles);

	if(!cvarInt("cg_enableBobbing")) return;

	// on odd legs, invert some angles
	if(cg.bobcycle & 1) {
		scale = -cg.xyspeed;
	} else {
		scale = cg.xyspeed;
	}

	// gun angles from bobbing
	angles[ROLL] += scale * cg.bobfracsin * 0.005;
	angles[YAW] += scale * cg.bobfracsin * 0.01;
	angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if(delta < LAND_DEFLECT_TIME) {
		origin[2] += cg.landChange * 0.25 * delta / LAND_DEFLECT_TIME;
	} else if(delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME) {
		origin[2] += cg.landChange * 0.25 * (LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
	}

	// idle drift
	scale = cg.xyspeed + 40;
	fracsin = sin(cg.time * 0.001);
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;
}

static void CG_LightningBolt(centity_t *cent, vec3_t origin) {
	trace_t trace;
	refEntity_t beam;
	vec3_t forward;
	vec3_t muzzlePoint, endPoint;
	clientInfo_t *ci;

	ci = &cgs.clientinfo[cent->currentState.clientNum];

	if(cent->currentState.weapon != WP_LIGHTNING && cent->currentState.weapon != WP_TOOLGUN && cent->currentState.weapon != WP_PHYSGUN && cent->currentState.weapon != WP_GRAVITYGUN) return;

	memset(&beam, 0, sizeof(beam));

	if(cent->currentState.number == cg.predictedPlayerState.clientNum) {
		AngleVectors(cg.predictedPlayerState.viewangles, forward, NULL, NULL);
		VectorCopy(cg.predictedPlayerState.origin, muzzlePoint);
	} else {
		AngleVectors(cent->lerpAngles, forward, NULL, NULL);
		VectorCopy(cent->lerpOrigin, muzzlePoint);
	}

	// FIXME: crouch
	muzzlePoint[2] += DEFAULT_VIEWHEIGHT;

	VectorMA(muzzlePoint, 14, forward, muzzlePoint);

	// project forward by the lightning range
	VectorMA(muzzlePoint, gameInfoWeapons[cent->currentState.weapon].range, forward, endPoint);

	// see if it hit a wall
	CG_Trace(&trace, muzzlePoint, vec3_origin, vec3_origin, endPoint, cent->currentState.number, MASK_SHOT);

	// this is the endpoint
	VectorCopy(trace.endpos, beam.oldorigin);

	// use the provided origin, even though it may be slightly
	// different than the muzzle origin
	VectorCopy(origin, beam.origin);

	if(cent->currentState.weapon == WP_PHYSGUN || cent->currentState.weapon == WP_GRAVITYGUN) {
		CG_RailTrail(ci, origin, trace.endpos, cent->currentState.weapon);
		return;
	}

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;
	trap_R_AddRefEntityToScene(&beam);

	// add the impact flare if it hit something
	if(trace.fraction < 1.0) {
		vec3_t angles;
		vec3_t dir;

		VectorSubtract(beam.oldorigin, beam.origin, dir);
		VectorNormalize(dir);

		memset(&beam, 0, sizeof(beam));
		beam.hModel = cgs.media.lightningExplosionModel;

		VectorMA(trace.endpos, -16, dir, beam.origin);

		// make a random orientation
		angles[0] = rand() % 360;
		angles[1] = rand() % 360;
		angles[2] = rand() % 360;
		AnglesToAxis(angles, beam.axis);
		trap_R_AddRefEntityToScene(&beam);
	}
}

static void CG_SpawnRailTrail(centity_t *cent, vec3_t origin) {
	clientInfo_t *ci;

	ci = &cgs.clientinfo[cent->currentState.clientNum];

	if(cent->currentState.weapon != WP_RAILGUN) return;
	if(!cent->pe.railgunFlash) return;
	cent->pe.railgunFlash = qtrue;
	CG_RailTrail(ci, origin, cent->pe.railgunImpact, WP_RAILGUN);
}

static void CG_PlayerFlashlight(centity_t *cent) {
	vec3_t forward, right, up;
	vec3_t start, end;
	trace_t trace;
	trace_t traceambient;
	float distance, lightIntensity, correctedIntensity;
	float minRadius = 32.0f;     // Minimum radius for focused light
	float maxRadius = 300.0f;    // Maximum radius for spread-out light
	float lightRadius = 600.0f;  // Maximum radius for spread-out light
	float maxBrightness = 0.32f; // Maximum brightness intensity
	float minBrightness = 0.01f; // Minimum brightness intensity
	float radius;

	AngleVectors(cent->lerpAngles, forward, right, up);
	VectorCopy(cent->lerpOrigin, start);
	VectorMA(start, lightRadius, forward, end);

	CG_Trace(&trace, start, NULL, NULL, end, cent->currentState.number, MASK_SHOT);
	CG_Trace(&traceambient, start, NULL, NULL, end, cent->currentState.number, MASK_SHOT);

	VectorMA(traceambient.endpos, -24, forward, traceambient.endpos);
	VectorMA(trace.endpos, -8, forward, trace.endpos);
	distance = VectorDistance(start, traceambient.endpos);

	// Calculate light intensity, clamped between minBrightness and maxBrightness
	lightIntensity = 1.0f - (distance / lightRadius);
	if(lightIntensity < 0.0f) lightIntensity = 0.0f;
	lightIntensity = lightIntensity * (maxBrightness - minBrightness) + minBrightness;
	if(lightIntensity > maxBrightness) {
		lightIntensity = maxBrightness;
	} else if(lightIntensity < minBrightness) {
		lightIntensity = minBrightness;
	}

	// Calculate radius based on distance, scaling between minRadius and maxRadius
	radius = minRadius + (distance / lightRadius) * (maxRadius - minRadius);
	if(radius > maxRadius) {
		radius = maxRadius;
	} else if(radius < minRadius) {
		radius = minRadius;
	}

	// Scale correctedIntensity with clamped lightIntensity
	correctedIntensity = (lightIntensity * 0.5f) + 0.03;

	// Add light with adjustable radius and intensity limits
	trap_R_AddLinearLightToScene(start, traceambient.endpos, radius, correctedIntensity, correctedIntensity, correctedIntensity);
	trap_R_AddLightToScene(trace.endpos, radius * 1.25, correctedIntensity, correctedIntensity, correctedIntensity);
}

void CG_AddPlayerWeapon(refEntity_t *parent, playerState_t *ps, centity_t *cent, int team, clientInfo_t *ci) {
	refEntity_t gun;
	refEntity_t flash;
	vec3_t angles;
	int weaponNum;
	weaponInfo_t *weapon;
	centity_t *nonPredictedCent;
	orientation_t lerped;

	weaponNum = cent->currentState.weapon;

	if(ci->flashlight) CG_PlayerFlashlight(&cg_entities[cent->currentState.clientNum]);

	CG_RegisterWeapon(weaponNum);
	weapon = &cg_weapons[weaponNum];

	// add the weapon
	memset(&gun, 0, sizeof(gun));
	VectorCopy(parent->lightingOrigin, gun.lightingOrigin);
	gun.renderfx = parent->renderfx;

	// set custom shading for railgun refire rate
	if(ps || cent->currentState.clientNum == cg.predictedPlayerState.clientNum) {
		if(weaponNum == WP_RAILGUN && cg.predictedPlayerState.weaponstate == WEAPON_FIRING) {
			float f;

			f = (float)cg.predictedPlayerState.weaponTime / 1500;
			gun.shaderRGBA[1] = 0;
			gun.shaderRGBA[0] = gun.shaderRGBA[2] = 255 * (1.0 - f);
		} else {
			gun.shaderRGBA[0] = 255;
			gun.shaderRGBA[1] = 100;
			gun.shaderRGBA[2] = 100;
			gun.shaderRGBA[3] = 255;
		}
	}

	if(weaponNum == WP_PHYSGUN) {
		gun.shaderRGBA[0] = 128;
		gun.shaderRGBA[1] = 255;
		gun.shaderRGBA[2] = 255;
		gun.shaderRGBA[3] = 255;
	}
	if(weaponNum == WP_GRAVITYGUN) {
		gun.shaderRGBA[0] = 120;
		gun.shaderRGBA[1] = 60;
		gun.shaderRGBA[2] = 0;
		gun.shaderRGBA[3] = 255;
	}

	gun.hModel = weapon->weaponModel;
	if(!gun.hModel) return;

	if(!ps) {
		// add weapon ready sound
		cent->pe.lightningFiring = qfalse;
		if((cent->currentState.eFlags & EF_FIRING) && weapon->firingSound) {
			// lightning gun and guantlet make a different sound when fire is held down
			trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound);
			cent->pe.lightningFiring = qtrue;
		} else if(weapon->readySound) {
			trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound);
		}
	}

	trap_R_LerpTag(&lerped, parent->hModel, parent->oldframe, parent->frame, 1.0 - parent->backlerp, "tag_weapon");
	VectorCopy(parent->origin, gun.origin);

	VectorMA(gun.origin, lerped.origin[0], parent->axis[0], gun.origin);

	// Make weapon appear left-handed for 2 and centered for 3
	if(ps && cvarInt("cg_drawGun") == 2)
		VectorMA(gun.origin, -lerped.origin[1], parent->axis[1], gun.origin);
	else if(!ps || cvarInt("cg_drawGun") != 3)
		VectorMA(gun.origin, lerped.origin[1], parent->axis[1], gun.origin);

	VectorMA(gun.origin, lerped.origin[2], parent->axis[2], gun.origin);

	MatrixMultiply(lerped.axis, ((refEntity_t *)parent)->axis, gun.axis);

	gun.backlerp = parent->backlerp;

	trap_R_AddRefEntityToScene(&gun);

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on teh single player podiums), so
	// go ahead and use the cent
	if((nonPredictedCent - cg_entities) != cent->currentState.clientNum) nonPredictedCent = cent;

	// add the flash
	if((weaponNum == WP_LIGHTNING || weaponNum == WP_GAUNTLET || weaponNum == WP_PHYSGUN || weaponNum == WP_GRAVITYGUN) && (nonPredictedCent->currentState.eFlags & EF_FIRING)) {
		// continuous flash
	} else {
		// impulse flash
		if(cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME && !cent->pe.railgunFlash) return;
	}

	memset(&flash, 0, sizeof(flash));
	VectorCopy(parent->lightingOrigin, flash.lightingOrigin);
	flash.renderfx = parent->renderfx;

	flash.hModel = weapon->flashModel;
	if(!flash.hModel) return;
	angles[YAW] = 0;
	angles[PITCH] = 0;
	angles[ROLL] = crandom() * 10;
	AnglesToAxis(angles, flash.axis);

	// colorize the railgun blast
	if(weaponNum == WP_RAILGUN) {
		clientInfo_t *ci;

		ci = &cgs.clientinfo[cent->currentState.clientNum];
		flash.shaderRGBA[0] = 0;
		flash.shaderRGBA[1] = 255;
		flash.shaderRGBA[2] = 0;
	}

	CG_PositionRotatedEntityOnTag(&flash, &gun, weapon->weaponModel, "tag_flash");
	trap_R_AddRefEntityToScene(&flash);

	if(ps || cg.renderingThirdPerson || cent->currentState.number != cg.predictedPlayerState.clientNum) {
		// add lightning bolt
		CG_LightningBolt(nonPredictedCent, flash.origin);

		// add rail trail
		CG_SpawnRailTrail(cent, flash.origin);

		if(weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2]) trap_R_AddLightToScene(flash.origin, 300 + (rand() & 31), weapon->flashDlightColor[0], weapon->flashDlightColor[1], weapon->flashDlightColor[2]);
	}
}

void CG_AddViewWeapon(playerState_t *ps) {
	refEntity_t hand;
	centity_t *cent;
	clientInfo_t *ci;
	float fovOffset;
	vec3_t angles;
	weaponInfo_t *weapon;

	if(ps->persistant[PERS_TEAM] == TEAM_SPECTATOR) return;

	if(ps->pm_type == PM_INTERMISSION) return;

	if(cg.renderingThirdPerson || cg.renderingEyesPerson) return;

	// allow the gun to be completely removed
	if(!cvarInt("cg_drawGun")) {
		vec3_t origin;

		if(cg.predictedPlayerState.eFlags & EF_FIRING) {
			// special hack for lightning gun...
			VectorCopy(cg.refdef.vieworg, origin);
			VectorMA(origin, -8, cg.refdef.viewaxis[2], origin);
			CG_LightningBolt(&cg_entities[ps->clientNum], origin);
		}
		return;
	}

	// drop gun lower at higher fov
	if(cvarInt("cg_fov") > 90) {
		fovOffset = -0.2 * (cvarInt("cg_fov") - 90);
	} else {
		fovOffset = 0;
	}

	cent = &cg.predictedPlayerEntity; // &cg_entities[cg.snap->ps.clientNum];
	CG_RegisterWeapon(ps->weapon);
	weapon = &cg_weapons[ps->weapon];

	memset(&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition(hand.origin, angles);

	VectorMA(hand.origin, 5, cg.refdef.viewaxis[0], hand.origin);
	VectorMA(hand.origin, -1, cg.refdef.viewaxis[1], hand.origin);
	VectorMA(hand.origin, (-1 + fovOffset), cg.refdef.viewaxis[2], hand.origin);

	AnglesToAxis(angles, hand.axis);

	// get clientinfo for animation map
	ci = &cgs.clientinfo[cent->currentState.clientNum];
	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;

	// add everything onto the hand
	CG_AddPlayerWeapon(&hand, ps, &cg.predictedPlayerEntity, ps->persistant[PERS_TEAM], ci);
}

void CG_NextWeapon_f(void) {
	qboolean weaponFound = qfalse;
	int original, i;

	if(!cg.snap || cg.snap->ps.pm_flags & PMF_FOLLOW || BG_InVehicle(cg.snap->ps.stats[STAT_VEHICLE])) return;

	if(cg.snap->ps.weapon == WP_PHYSGUN && cg.snap->ps.eFlags & EF_FIRING) {
		trap_Cmd(EXEC_INSERT, "physgun_dist 0\n");
		return;
	}

	original = cg.weaponSelect;

	for(i = 1; i < WEAPONS_NUM; i++) {
		CG_WeaponSelect(cg.weaponSelect+1);
		if(cg.weaponSelect >= WEAPONS_NUM) CG_WeaponSelect(1);

		if(cg.swep_listcl[cg.weaponSelect]) {
			weaponFound = qtrue;
			break;
		}
	}

	if(!weaponFound) CG_WeaponSelect(original);
}

void CG_PrevWeapon_f(void) {
	qboolean weaponFound = qfalse;
	int original, i;

	if(!cg.snap || cg.snap->ps.pm_flags & PMF_FOLLOW || BG_InVehicle(cg.snap->ps.stats[STAT_VEHICLE])) return;

	if(cg.snap->ps.weapon == WP_PHYSGUN && cg.snap->ps.eFlags & EF_FIRING) {
		trap_Cmd(EXEC_INSERT, "physgun_dist 1\n");
		return;
	}

	original = cg.weaponSelect;

	for(i = 1; i < WEAPONS_NUM; i++) {
		CG_WeaponSelect(cg.weaponSelect-1);
		if(cg.weaponSelect <= 0) CG_WeaponSelect(WEAPONS_NUM);

		if(cg.swep_listcl[cg.weaponSelect]) {
			weaponFound = qtrue;
			break;
		}
	}

	if(!weaponFound) CG_WeaponSelect(original);
}

void CG_Weapon_f(void) {
	int num;

	if(!cg.snap || cg.snap->ps.pm_flags & PMF_FOLLOW || BG_InVehicle(cg.snap->ps.stats[STAT_VEHICLE])) return;

	num = atoi(CG_Argv(1));

	if(!cg.swep_listcl[num]) return;

	CG_WeaponSelect(num);
}

void CG_FireWeapon(centity_t *cent) {
	entityState_t *ent;
	int c;
	weaponInfo_t *weap;

	ent = &cent->currentState;

	if(ent->weapon == WP_NONE) return;
	iferr(ent->weapon > WEAPONS_NUM);
	weap = &cg_weapons[ent->weapon];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;

	// lightning gun only does this this on initial press
	if(ent->weapon == WP_LIGHTNING) {
		if(cent->pe.lightningFiring) return;
	}

	if(ent->weapon == WP_TOOLGUN && cent->currentState.clientNum == cg.snap->ps.clientNum && cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR) {
		if(cvarInt("toolgun_tool") <= TL_CREATE) {
			trap_Cmd(EXEC_INSERT, va("%s\n", cvarString("spawn_cmd")));
		} else {
			trap_Cmd(EXEC_INSERT, va("%s %s\n", cvarString("toolgun_cmd"), cvarString("toolgun_mod5")));
		}
	}

	// play a sound
	for(c = 0; c < 4; c++) {
		if(!weap->flashSound[c]) break;
	}
	if(c > 0) {
		c = rand() % c;
		if(weap->flashSound[c]) trap_S_StartSound(NULL, ent->number, CHAN_WEAPON, weap->flashSound[c]);
	}

	// do brass ejection
	if(weap->ejectBrassFunc) weap->ejectBrassFunc(cent);

	CG_PredictWeaponEffects(cent);
}

void CG_MissileHitWall(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType) {
	qhandle_t mod;
	qhandle_t mark;
	qhandle_t shader;
	sfxHandle_t sfx;
	float radius;
	float light;
	vec3_t lightColor;
	localEntity_t *le;
	int r;
	qboolean alphaFade;
	qboolean isSprite;
	int duration;

	mark = 0;
	radius = 32;
	sfx = 0;
	mod = 0;
	shader = 0;
	light = 0;
	lightColor[0] = 1;
	lightColor[1] = 1;
	lightColor[2] = 0;

	// set defaults
	isSprite = qfalse;
	duration = 600;

	switch(weapon) {
	default:

	case WP_NAILGUN:
		if(soundType == IMPACTSOUND_FLESH) {
			sfx = cgs.media.sfx_nghitflesh;
		} else if(soundType == IMPACTSOUND_METAL) {
			sfx = cgs.media.sfx_nghitmetal;
		} else {
			sfx = cgs.media.sfx_nghit;
		}
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;

	case WP_LIGHTNING:
		// no explosion at LG impact, it is added with the beam
		r = rand() & 3;
		if(r < 2) {
			sfx = cgs.media.sfx_lghit2;
		} else if(r == 2) {
			sfx = cgs.media.sfx_lghit1;
		} else {
			sfx = cgs.media.sfx_lghit3;
		}
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;

	case WP_PROX_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_proxexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		break;

	case WP_GRENADE_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		break;
	case WP_ROCKET_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.rocketExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 1;
		lightColor[1] = 0.75;
		lightColor[2] = 0.0;
		break;
	case WP_RAILGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.railExplosionShader;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		radius = 24;
		break;
	case WP_PLASMAGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.plasmaExplosionShader;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		radius = 16;
		break;
	case WP_BFG:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.bfgExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 32;
		isSprite = qtrue;
		break;
	case WP_TOOLGUN:
		// no explosion at LG impact, it is added with the beam
		r = rand() & 3;
		if(r < 2) {
			sfx = cgs.media.sfx_lghit2;
		} else if(r == 2) {
			sfx = cgs.media.sfx_lghit1;
		} else {
			sfx = cgs.media.sfx_lghit3;
		}
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;
	case WP_PHYSGUN:
		mod = 0;
		shader = 0;
		sfx = 0;
		mark = 0;
		radius = 0;
		break;
	case WP_GRAVITYGUN:
		mod = 0;
		shader = 0;
		sfx = 0;
		mark = 0;
		radius = 0;
		break;
	case WP_SHOTGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;
		sfx = 0;
		radius = 4;
		break;

	case WP_CHAINGUN:
		mod = cgs.media.bulletFlashModel;
		if(soundType == IMPACTSOUND_FLESH) {
			sfx = cgs.media.sfx_chghitflesh;
		} else if(soundType == IMPACTSOUND_METAL) {
			sfx = cgs.media.sfx_chghitmetal;
		} else {
			sfx = cgs.media.sfx_chghit;
		}

		mark = cgs.media.bulletMarkShader;
		r = rand() & 3;
		if(r == 0) {
			sfx = cgs.media.sfx_ric1;
		} else if(r == 1) {
			sfx = cgs.media.sfx_ric2;
		} else {
			sfx = cgs.media.sfx_ric3;
		}

		radius = 8;
		break;

	case WP_MACHINEGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;
		r = rand() & 3;
		if(r == 0) {
			sfx = cgs.media.sfx_ric1;
		} else if(r == 1) {
			sfx = cgs.media.sfx_ric2;
		} else {
			sfx = cgs.media.sfx_ric3;
		}

		radius = 8;
		break;
	}

	if(sfx) trap_S_StartSound(origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx);

	if(mod) {
		le = CG_MakeExplosion(origin, dir, mod, shader, duration, isSprite);
		le->light = light;
		VectorCopy(lightColor, le->lightColor);
	}

	alphaFade = (mark == cgs.media.energyMarkShader); // plasma fades alpha, all others fade color
	if(weapon == WP_RAILGUN) {
		CG_ImpactMark(mark, origin, dir, random() * 360, 0, 1, 0, 1, alphaFade, radius, qfalse);
	} else {
		CG_ImpactMark(mark, origin, dir, random() * 360, 1, 1, 1, 1, alphaFade, radius, qfalse);
	}
}

void CG_MissileHitPlayer(int weapon, vec3_t origin, vec3_t dir, int entityNum) {
	CG_Bleed(origin, entityNum);

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch(weapon) {
	case WP_GRENADE_LAUNCHER:
	case WP_ROCKET_LAUNCHER:
	case WP_NAILGUN:
	case WP_CHAINGUN:
	case WP_PROX_LAUNCHER: CG_MissileHitWall(weapon, 0, origin, dir, IMPACTSOUND_FLESH); break;
	default: break;
	}
}

static void CG_ShotgunPellet(vec3_t start, vec3_t end, int skipNum) {
	trace_t tr;
	int sourceContentType, destContentType;

	CG_Trace(&tr, start, NULL, NULL, end, skipNum, MASK_SHOT);

	sourceContentType = trap_CM_PointContents(start, 0);
	destContentType = trap_CM_PointContents(tr.endpos, 0);

	// FIXME: should probably move this cruft into CG_BubbleTrail
	if(sourceContentType == destContentType) {
		if(sourceContentType & CONTENTS_WATER) CG_BubbleTrail(start, tr.endpos, 32);
	} else if(sourceContentType & CONTENTS_WATER) {
		trace_t trace;

		trap_CM_BoxTrace(&trace, end, start, NULL, NULL, 0, CONTENTS_WATER);
		CG_BubbleTrail(start, trace.endpos, 32);
	} else if(destContentType & CONTENTS_WATER) {
		trace_t trace;

		trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0, CONTENTS_WATER);
		CG_BubbleTrail(tr.endpos, trace.endpos, 32);
	}

	if(tr.surfaceFlags & SURF_NOIMPACT) return;

	if(cg_entities[tr.entityNum].currentState.eType == ET_PLAYER) {
		CG_MissileHitPlayer(WP_SHOTGUN, tr.endpos, tr.plane.normal, tr.entityNum);
	} else {
		if(tr.surfaceFlags & SURF_NOIMPACT) return;
		if(tr.surfaceFlags & SURF_METALSTEPS) {
			CG_MissileHitWall(WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_METAL);
		} else {
			CG_MissileHitWall(WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_DEFAULT);
		}
	}
}

void CG_ShotgunPattern(vec3_t origin, vec3_t origin2, int seed, int otherEntNum, int weapon) {
	int i;
	float r, u;
	vec3_t end;
	vec3_t forward, right, up;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2(origin2, forward);
	PerpendicularVector(right, forward);
	CrossProduct(forward, right, up);

	// generate the "random" spread pattern
	for(i = 0; i < gameInfoWeapons[weapon].count; i++) {
		r = Q_crandom(&seed) * gameInfoWeapons[weapon].spread * 16;
		u = Q_crandom(&seed) * gameInfoWeapons[weapon].spread * 16;
		VectorMA(origin, gameInfoWeapons[weapon].range * 16, forward, end);
		VectorMA(end, r, right, end);
		VectorMA(end, u, up, end);

		CG_ShotgunPellet(origin, end, otherEntNum);
	}
}

void CG_ShotgunFire(entityState_t *es, int weapon) {
	vec3_t v;
	int contents;
	vec3_t up;

	VectorSubtract(es->origin2, es->pos.trBase, v);
	VectorNormalize(v);
	VectorScale(v, 32, v);
	VectorAdd(es->pos.trBase, v, v);
	contents = trap_CM_PointContents(es->pos.trBase, 0);
	if(!(contents & CONTENTS_WATER)) {
		VectorSet(up, 0, 0, 8);
		CG_SmokePuff(v, up, 32, 1, 1, 1, 0.33f, 900, cg.time, 0, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader);
	}
	CG_ShotgunPattern(es->pos.trBase, es->origin2, es->eventParm, es->otherEntityNum, weapon);
}

static void CG_Tracer(vec3_t source, vec3_t dest) {
	vec3_t forward, right;
	polyVert_t verts[4];
	vec3_t line;
	float len, begin, end;
	vec3_t start, finish;
	vec3_t midpoint;

	// tracer
	VectorSubtract(dest, source, forward);
	len = VectorNormalize(forward);

	// start at least a little ways from the muzzle
	if(len < 100) return;
	begin = 50 + random() * (len - 60);
	end = begin + 100;
	if(end > len) end = len;
	VectorMA(source, begin, forward, start);
	VectorMA(source, end, forward, finish);

	line[0] = DotProduct(forward, cg.refdef.viewaxis[1]);
	line[1] = DotProduct(forward, cg.refdef.viewaxis[2]);

	VectorScale(cg.refdef.viewaxis[1], line[1], right);
	VectorMA(right, -line[0], cg.refdef.viewaxis[2], right);
	VectorNormalize(right);

	VectorMA(finish, 1, right, verts[0].xyz);
	verts[0].st[0] = 0;
	verts[0].st[1] = 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorMA(finish, -1, right, verts[1].xyz);
	verts[1].st[0] = 1;
	verts[1].st[1] = 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorMA(start, -1, right, verts[2].xyz);
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorMA(start, 1, right, verts[3].xyz);
	verts[3].st[0] = 0;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene(cgs.media.tracerShader, 4, verts);

	midpoint[0] = (start[0] + finish[0]) * 0.5;
	midpoint[1] = (start[1] + finish[1]) * 0.5;
	midpoint[2] = (start[2] + finish[2]) * 0.5;

	// add the tracer sound
	trap_S_StartSound(midpoint, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.tracerSound);
}

static qboolean CG_CalcMuzzlePoint(int entityNum, vec3_t muzzle) {
	vec3_t forward;
	centity_t *cent;

	if(entityNum == cg.snap->ps.clientNum) {
		VectorCopy(cg.snap->ps.origin, muzzle);
		muzzle[2] += cg.snap->ps.viewheight;
		AngleVectors(cg.snap->ps.viewangles, forward, NULL, NULL);
		VectorMA(muzzle, 14, forward, muzzle);
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if(!cent->currentValid) return qfalse;

	VectorCopy(cent->currentState.pos.trBase, muzzle);

	AngleVectors(cent->currentState.apos.trBase, forward, NULL, NULL);
	muzzle[2] += DEFAULT_VIEWHEIGHT;

	VectorMA(muzzle, 14, forward, muzzle);

	return qtrue;
}

void CG_Bullet(vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum) {
	trace_t trace;
	int sourceContentType, destContentType;
	vec3_t start;

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if(sourceEntityNum >= 0) {
		if(CG_CalcMuzzlePoint(sourceEntityNum, start)) {
			sourceContentType = trap_CM_PointContents(start, 0);
			destContentType = trap_CM_PointContents(end, 0);

			// do a complete bubble trail if necessary
			if((sourceContentType == destContentType) && (sourceContentType & CONTENTS_WATER)) CG_BubbleTrail(start, end, 32);
			// bubble trail from water into air
			else if((sourceContentType & CONTENTS_WATER)) {
				trap_CM_BoxTrace(&trace, end, start, NULL, NULL, 0, CONTENTS_WATER);
				CG_BubbleTrail(start, trace.endpos, 32);
			}
			// bubble trail from air into water
			else if((destContentType & CONTENTS_WATER)) {
				trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0, CONTENTS_WATER);
				CG_BubbleTrail(trace.endpos, end, 32);
			}

			// draw a tracer
			if(random() < 0.4) CG_Tracer(start, end);
		}
	}

	// impact splash and mark
	if(flesh) {
		CG_Bleed(end, fleshEntityNum);
	} else {
		CG_MissileHitWall(WP_MACHINEGUN, 0, end, normal, IMPACTSOUND_DEFAULT);
	}
}
