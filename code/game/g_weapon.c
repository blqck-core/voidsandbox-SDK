// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

static float s_quadFactor;
static vec3_t forward, right, up;
static vec3_t muzzle;

/*
================
General funcs
================
*/
void CalcMuzzlePoint(gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint) {
	VectorCopy(ent->s.pos.trBase, muzzlePoint);
	muzzlePoint[2] += ent->client->ps.viewheight;
	VectorMA(muzzlePoint, 14, forward, muzzlePoint);
	SnapVector(muzzlePoint);
}

static void CalcMuzzlePointOrigin(gentity_t *ent, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint) {
	VectorCopy(ent->s.pos.trBase, muzzlePoint);
	muzzlePoint[2] += ent->client->ps.viewheight;
	VectorMA(muzzlePoint, 14, forward, muzzlePoint);
	SnapVector(muzzlePoint);
}

static void G_BounceMissile(gentity_t *ent, trace_t *trace) {
	vec3_t velocity;
	float dot;
	int hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + (level.time - level.previousTime) * trace->fraction;
	BG_EvaluateTrajectoryDelta(&ent->s.pos, hitTime, velocity);
	dot = DotProduct(velocity, trace->plane.normal);
	VectorMA(velocity, -2 * dot, trace->plane.normal, ent->s.pos.trDelta);

	VectorScale(ent->s.pos.trDelta, ent->phys_bounce, ent->s.pos.trDelta);
	// check for stop
	if(trace->plane.normal[2] > 0.2 && VectorLength(ent->s.pos.trDelta) < 40) {
		G_SetOrigin(ent, trace->endpos);
		return;
	}

	VectorAdd(ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
	ent->s.pos.trTime = level.time;
}

void G_ExplodeMissile(gentity_t *ent) {
	vec3_t dir;
	vec3_t origin;

	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);
	SnapVector(origin);
	G_SetOrigin(ent, origin);

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eType = ET_GENERAL;
	G_AddEvent(ent, EV_MISSILE_MISS, DirToByte(dir));

	ent->freeAfterEvent = qtrue;

	// splash damage
	if(ent->splashDamage) {
		G_RadiusDamage(ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent, ent->methodOfDeath);
	}

	trap_LinkEntity(ent);
}

static void Guided_Missile_Think(gentity_t *missile) {
	vec3_t forward, right, up;
	vec3_t muzzle;

	gentity_t *player = missile->parent;

	if(!player) {
		G_Printf("Guided_Missile_Think : missile has no owner!\n");
		return;
	}

	AngleVectors(player->client->ps.viewangles, forward, right, up);
	CalcMuzzlePoint(player, forward, right, up, muzzle);
	VectorCopy(missile->r.currentOrigin, missile->s.pos.trBase);

	missile->s.pos.trType = TR_LINEAR;
	missile->s.pos.trTime = level.time - 50;

	VectorSubtract(muzzle, missile->r.currentOrigin, muzzle);
	VectorScale(forward, gameInfoWeapons[missile->s.weapon].speed, forward);
	VectorAdd(forward, muzzle, muzzle);
	VectorNormalize(muzzle);
	VectorCopy(forward, missile->s.pos.trDelta);
	vectoangles(muzzle, missile->s.angles);
	SnapVector(missile->s.pos.trDelta);

	missile->nextthink = level.time + 25;

	if(level.time > missile->wait) {
		G_ExplodeMissile(missile);
	}
}

/*
===============
Melee type
===============
*/
qboolean Melee_Fire(gentity_t *ent, int weapon) {
	trace_t tr;
	vec3_t end;
	gentity_t *tent;
	gentity_t *traceEnt;

	// set aiming directions
	AngleVectors(ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePoint(ent, forward, right, up, muzzle);
	VectorMA(muzzle, gameInfoWeapons[weapon].range, forward, end);
	trap_Trace(&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);

	if(tr.surfaceFlags & SURF_NOIMPACT) {
		return qfalse;
	}

	traceEnt = &g_entities[tr.entityNum];

	// send blood impact
	if(traceEnt->takedamage && traceEnt->client) {
		tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
		tent->s.otherEntityNum = traceEnt->s.number;
		tent->s.eventParm = DirToByte(tr.plane.normal);
		tent->s.weapon = ent->s.weapon;
	}

	if(!traceEnt->takedamage && !ent->client->vehicleNum) return qfalse;

	s_quadFactor = 1;
	s_quadFactor *= gameInfoNPCTypes[ent->npcType].damage;

	G_Damage(traceEnt, ent, ent, forward, tr.endpos, gameInfoWeapons[weapon].damage * s_quadFactor, 0, WP_GAUNTLET);

	return qtrue;
}

/*
===============
Bullet type
===============
*/
static void Bullet_Fire(gentity_t *ent, int weapon) {
	trace_t tr;
	vec3_t end;
	float r, u;
	gentity_t *tent, *traceEnt;
	int passent;
	int seed = ent->client->attackTime % 256;

	r = Q_random(&seed) * M_PI * 2.0f;
	u = sin(r) * Q_crandom(&seed) * gameInfoWeapons[weapon].spread * 16;
	r = cos(r) * Q_crandom(&seed) * gameInfoWeapons[weapon].spread * 16;

	VectorMA(muzzle, gameInfoWeapons[weapon].range, forward, end);
	VectorMA(end, r, right, end);
	VectorMA(end, u, up, end);

	passent = ent->s.number;
	trap_Trace(&tr, muzzle, NULL, NULL, end, passent, MASK_SHOT);

	if(tr.surfaceFlags & SURF_NOIMPACT) return;

	traceEnt = &g_entities[tr.entityNum];

	// snap the endpos to integers, but nudged towards the line
	SnapVectorTowards(tr.endpos, muzzle);

	// send bullet impact
	if(traceEnt->takedamage && traceEnt->client) {
		tent = G_TempEntity(tr.endpos, EV_BULLET_HIT_FLESH);
		tent->s.eventParm = traceEnt->s.number;
		tent->s.clientNum = ent->s.clientNum;
	} else {
		tent = G_TempEntity(tr.endpos, EV_BULLET_HIT_WALL);
		tent->s.eventParm = DirToByte(tr.plane.normal);
		tent->s.clientNum = ent->s.clientNum;
	}
	tent->s.otherEntityNum = ent->s.number;

	if(traceEnt->takedamage && !(traceEnt->client && traceEnt->client->invulnerabilityTime > level.time)) G_Damage(traceEnt, ent, ent, forward, tr.endpos, gameInfoWeapons[weapon].damage * s_quadFactor, 0, gameInfoWeapons[weapon].mod);
}

/*
===============
Shotgun type
===============
*/
static void ShotgunPellet(vec3_t start, vec3_t end, gentity_t *ent, int weapon) {
	trace_t tr;
	int passent;
	gentity_t *traceEnt;
	vec3_t tr_start, tr_end;

	passent = ent->s.number;
	VectorCopy(start, tr_start);
	VectorCopy(end, tr_end);
	trap_Trace(&tr, tr_start, NULL, NULL, tr_end, passent, MASK_SHOT);
	traceEnt = &g_entities[tr.entityNum];

	if(tr.surfaceFlags & SURF_NOIMPACT) return;

	if(traceEnt->takedamage && !(traceEnt->client && traceEnt->client->invulnerabilityTime > level.time)) G_Damage(traceEnt, ent, ent, forward, tr.endpos, gameInfoWeapons[weapon].damage * s_quadFactor, 0, gameInfoWeapons[weapon].mod);
}

static void ShotgunPattern(vec3_t origin, vec3_t origin2, int seed, gentity_t *ent, int weapon) {
	int i;
	float r, u;
	vec3_t end, forward, right, up;

	VectorNormalize2(origin2, forward);
	PerpendicularVector(right, forward);
	CrossProduct(forward, right, up);

	for(i = 0; i < gameInfoWeapons[weapon].count; i++) {
		r = Q_crandom(&seed) * gameInfoWeapons[weapon].spread * 16;
		u = Q_crandom(&seed) * gameInfoWeapons[weapon].spread * 16;
		VectorMA(origin, gameInfoWeapons[weapon].range * 16, forward, end);
		VectorMA(end, r, right, end);
		VectorMA(end, u, up, end);

		ShotgunPellet(origin, end, ent, weapon);
	}
}

static void Shotgun_Fire(gentity_t *ent, int weapon) {
	gentity_t *tent;

	tent = G_TempEntity(muzzle, EV_SHOTGUN);
	tent->s.weapon = weapon;
	VectorScale(forward, gameInfoWeapons[weapon].range, tent->s.origin2);
	SnapVector(tent->s.origin2);
	tent->s.eventParm = ent->client->attackTime % 256;
	tent->s.otherEntityNum = ent->s.number;

	ShotgunPattern(tent->s.pos.trBase, tent->s.origin2, tent->s.eventParm, ent, weapon);
}

/*
===============
Railgun type
===============
*/
#define MAX_RAIL_HITS 4
static void Railgun_Fire(gentity_t *ent, int weapon) {
	vec3_t end;
	trace_t trace;
	gentity_t *tent, *traceEnt, *unlinkedEntities[MAX_RAIL_HITS];
	int i, hits, unlinked, passent;

	VectorMA(muzzle, gameInfoWeapons[weapon].range, forward, end);

	unlinked = 0;
	hits = 0;
	passent = ent->s.number;
	do {
		trap_Trace(&trace, muzzle, NULL, NULL, end, passent, MASK_SHOT);
		if(trace.entityNum >= ENTITYNUM_MAX_NORMAL) break;

		traceEnt = &g_entities[trace.entityNum];
		if(traceEnt->takedamage && traceEnt->client) {
			G_Damage(traceEnt, ent, ent, forward, trace.endpos, gameInfoWeapons[weapon].damage * s_quadFactor, 0, gameInfoWeapons[weapon].mod);
		}
		if(trace.contents & CONTENTS_SOLID) break;

		trap_UnlinkEntity(traceEnt);
		unlinkedEntities[unlinked] = traceEnt;
		unlinked++;
	} while(unlinked < MAX_RAIL_HITS);

	for(i = 0; i < unlinked; i++) trap_LinkEntity(unlinkedEntities[i]);

	SnapVectorTowards(trace.endpos, muzzle);
	tent = G_TempEntity(trace.endpos, EV_RAILTRAIL);
	tent->s.clientNum = ent->s.clientNum;

	VectorCopy(muzzle, tent->s.origin2);

	VectorMA(tent->s.origin2, 4, right, tent->s.origin2);
	VectorMA(tent->s.origin2, -1, up, tent->s.origin2);

	if(trace.surfaceFlags & SURF_NOIMPACT) {
		tent->s.eventParm = 255;
	} else {
		tent->s.eventParm = DirToByte(trace.plane.normal);
	}
	tent->s.clientNum = ent->s.clientNum;
}

/*
===============
Lightning type
===============
*/
static void Lightning_Fire(gentity_t *ent, int weapon) {
	trace_t tr;
	vec3_t end;
	gentity_t *traceEnt, *tent;
	int i, passent;

	passent = ent->s.number;
	for(i = 0; i < 10; i++) {
		VectorMA(muzzle, gameInfoWeapons[weapon].range, forward, end);
		trap_Trace(&tr, muzzle, NULL, NULL, end, passent, MASK_SHOT);

		if(i) {
			tent = G_TempEntity(muzzle, EV_LIGHTNINGBOLT);
			VectorCopy(tr.endpos, end);
			SnapVector(end);
			VectorCopy(end, tent->s.origin2);
		}
		if(tr.entityNum == ENTITYNUM_NONE) return;

		traceEnt = &g_entities[tr.entityNum];

		if(traceEnt->takedamage && traceEnt->client) {
			G_Damage(traceEnt, ent, ent, forward, tr.endpos, gameInfoWeapons[weapon].damage * s_quadFactor, 0, gameInfoWeapons[weapon].mod);
		}

		if(traceEnt->takedamage && traceEnt->client) {
			tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
			tent->s.otherEntityNum = traceEnt->s.number;
			tent->s.eventParm = DirToByte(tr.plane.normal);
			tent->s.weapon = ent->s.weapon;
		} else if(!(tr.surfaceFlags & SURF_NOIMPACT)) {
			tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
			tent->s.eventParm = DirToByte(tr.plane.normal);
		}

		break;
	}
}

/*
===============
Toolgun type
===============
*/
void Weapon_Toolgun_Info(gentity_t *ent) {
	trace_t tr;
	vec3_t end;
	gentity_t *traceEnt;
	int i;

	char info_entity[MAX_ENTITYINFO][64];

	// set aiming directions
	AngleVectors(ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePoint(ent, forward, right, up, muzzle);
	VectorMA(muzzle, gameInfoWeapons[ent->s.weapon].range, forward, end);
	trap_Trace(&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SELECT);

	traceEnt = &g_entities[tr.entityNum];

	if(!traceEnt->sandboxObject && traceEnt->npcType <= NT_PLAYER && traceEnt->s.eType != ET_ITEM) {
		trap_SendServerCommand(ent->s.clientNum, va("t_info \"%s\"", ""));
		return;
	}

	if(gameInfoWeapons[ent->s.weapon].wType == WT_TOOLGUN) {
		// Classname
		if(!strcmp(traceEnt->sb_class, "none") || !strcmp(traceEnt->sb_class, "")) {
			strcpy(info_entity[0], traceEnt->classname);
		} else {
			strcpy(info_entity[0], traceEnt->sb_class);
		}

		// Model
		if(traceEnt->s.eType == ET_PLAYER) {
			strcpy(info_entity[1], va("%i", traceEnt->s.clientNum));
		} else if(traceEnt->s.eType == ET_ITEM) {
			strcpy(info_entity[1], "<NULL>");
		} else {
			strcpy(info_entity[1], traceEnt->model);
		}

		// Material
		if(traceEnt->sb_material) {
			strcpy(info_entity[2], va("%i", traceEnt->sb_material));
		} else {
			strcpy(info_entity[2], "0");
		}

		// Count
		if(traceEnt->s.eType == ET_ITEM) {
			strcpy(info_entity[3], va("%i", traceEnt->count));
		} else {
			strcpy(info_entity[3], "<NULL>");
		}

		for(i = 0; i < MAX_ENTITYINFO; i++) {
			if(!strcmp(info_entity[i], "")) {
				strcpy(info_entity[i], "<NULL>");
			}
		}

		// Send entityInfo
		trap_SendServerCommand(ent->s.clientNum, va("t_info \"%s %s %s %s\"", info_entity[0], info_entity[1], info_entity[2], info_entity[3]));
	}

	return;
}

/*
===============
Prox type
===============
*/
static void ProximityMine_Explode(gentity_t *mine) {
	G_ExplodeMissile(mine);
	// if the prox mine has a trigger free it
	if(mine->activator) {
		G_FreeEntity(mine->activator);
		mine->activator = NULL;
	}
}

static void ProximityMine_Die(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod) {
	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + 1;
}

void ProximityMine_Trigger(gentity_t *trigger, gentity_t *other, trace_t *trace) {
	vec3_t v;
	gentity_t *mine;

	if(!other->client) return;

	// trigger is a cube, do a distance test now to act as if it's a sphere
	VectorSubtract(trigger->s.pos.trBase, other->s.pos.trBase, v);
	if(VectorLength(v) > trigger->parent->splashRadius) {
		return;
	}

	if(cvarInt("g_gametype") >= GT_TEAM) {
		if(trigger->parent->s.generic1 == other->client->sess.sessionTeam) {
			return;
		}
	}

	// ok, now check for ability to damage so we don't get triggered thru walls, closed doors, etc...
	if(!CanDamage(other, trigger->s.pos.trBase)) {
		return;
	}

	// trigger the mine!
	mine = trigger->parent;
	mine->s.loopSound = 0;
	G_AddEvent(mine, EV_PROXIMITY_MINE_TRIGGER, 0);
	mine->nextthink = level.time + 500;

	G_FreeEntity(trigger);
}

static void ProximityMine_Activate(gentity_t *ent) {
	gentity_t *trigger;
	float r;

	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + 120 * 1000;

	ent->takedamage = qtrue;
	ent->health = 1;
	ent->die = ProximityMine_Die;

	ent->s.loopSound = G_SoundIndex("sound/weapons/proxmine/wstbtick.wav");

	// build the proximity trigger
	trigger = G_Spawn();

	trigger->classname = "proxmine_trigger";

	r = ent->splashRadius;
	VectorSet(trigger->r.mins, -r, -r, -r);
	VectorSet(trigger->r.maxs, r, r, r);

	G_SetOrigin(trigger, ent->s.pos.trBase);

	trigger->parent = ent;
	trigger->r.contents = CONTENTS_TRIGGER;
	trigger->touch = ProximityMine_Trigger;

	trap_LinkEntity(trigger);

	// set pointer to trigger so the entity can be freed when the mine explodes
	ent->activator = trigger;
}

static void ProximityMine_ExplodeOnPlayer(gentity_t *mine) {
	gentity_t *player;

	player = mine->enemy;
	player->client->ps.eFlags &= ~EF_TICKING;

	G_SetOrigin(mine, player->s.pos.trBase);
	// make sure the explosion gets to the client
	mine->r.svFlags &= ~SVF_NOCLIENT;
	mine->methodOfDeath = WP_PROX_LAUNCHER;
	G_ExplodeMissile(mine);
}

static void ProximityMine_Player(gentity_t *mine, gentity_t *player) {
	if(mine->s.eFlags & EF_NODRAW) {
		return;
	}

	G_AddEvent(mine, EV_PROXIMITY_MINE_STICK, 0);

	if(player->s.eFlags & EF_TICKING) {
		player->activator->splashDamage += mine->splashDamage;
		player->activator->splashRadius *= 1.50;
		mine->think = G_FreeEntity;
		mine->nextthink = level.time;
		return;
	}

	player->client->ps.eFlags |= EF_TICKING;
	player->activator = mine;

	mine->s.eFlags |= EF_NODRAW;
	mine->r.svFlags |= SVF_NOCLIENT;
	mine->s.pos.trType = TR_LINEAR;
	VectorClear(mine->s.pos.trDelta);

	mine->enemy = player;
	mine->think = ProximityMine_ExplodeOnPlayer;
	if(player->client->invulnerabilityTime > level.time) {
		mine->nextthink = level.time + 2 * 1000;
	} else {
		mine->nextthink = level.time + 10 * 1000;
	}
}

/*
===============
Missile type
===============
*/
static void G_MissileImpact(gentity_t *ent, trace_t *trace) {
	gentity_t *other;
	qboolean hitClient = qfalse;
	other = &g_entities[trace->entityNum];

	// check for bounce
	if(!other->takedamage && (ent->s.eFlags & (EF_BOUNCE))) {
		G_BounceMissile(ent, trace);
		if(ent->s.weapon == WP_GRENADE_LAUNCHER) {
			G_AddEvent(ent, EV_GRENADE_BOUNCE, 0);
		}
		return;
	}
	
	// impact damage
	if(other->takedamage) {
		// FIXME: wrong damage direction?
		if(ent->damage) {
			vec3_t velocity;

			BG_EvaluateTrajectoryDelta(&ent->s.pos, level.time, velocity);
			if(VectorLength(velocity) == 0) {
				velocity[2] = 1; // stepped on a grenade
			}
			G_Damage(other, ent, &g_entities[ent->r.ownerNum], velocity, ent->s.origin, ent->damage, 0, ent->methodOfDeath);
		}
	}

	if(ent->s.weapon == WP_PROX_LAUNCHER) {
		if(ent->s.pos.trType != TR_GRAVITY) {
			return;
		}

		// if it's a player, stick it on to them (flag them and remove this entity)
		if(other->s.eType == ET_PLAYER && other->health > 0) {
			ProximityMine_Player(ent, other);
			return;
		}

		SnapVectorTowards(trace->endpos, ent->s.pos.trBase);
		G_SetOrigin(ent, trace->endpos);
		ent->s.pos.trType = TR_STATIONARY;
		VectorClear(ent->s.pos.trDelta);

		G_AddEvent(ent, EV_PROXIMITY_MINE_STICK, trace->surfaceFlags);

		ent->think = ProximityMine_Activate;
		ent->nextthink = level.time + 2000;

		vectoangles(trace->plane.normal, ent->s.angles);
		ent->s.angles[0] += 90;

		// link the prox mine to the other entity
		ent->enemy = other;
		ent->die = ProximityMine_Die;
		VectorCopy(trace->plane.normal, ent->movedir);
		VectorSet(ent->r.mins, -4, -4, -4);
		VectorSet(ent->r.maxs, 4, 4, 4);
		trap_LinkEntity(ent);

		return;
	}

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if(other->takedamage && other->client) {
		G_AddEvent(ent, EV_MISSILE_HIT, DirToByte(trace->plane.normal));
		ent->s.otherEntityNum = other->s.number;
	} else if(trace->surfaceFlags & SURF_METALSTEPS) {
		G_AddEvent(ent, EV_MISSILE_MISS_METAL, DirToByte(trace->plane.normal));
	} else {
		G_AddEvent(ent, EV_MISSILE_MISS, DirToByte(trace->plane.normal));
	}

	ent->freeAfterEvent = qtrue;

	// change over to a normal entity right at the point of impact
	ent->s.eType = ET_GENERAL;

	SnapVectorTowards(trace->endpos, ent->s.pos.trBase); // save net bandwidth

	G_SetOrigin(ent, trace->endpos);

	// splash damage (doesn't apply to person directly hit)
	if(ent->splashDamage) {
		G_RadiusDamage(trace->endpos, ent->parent, ent->splashDamage, ent->splashRadius, other, ent->methodOfDeath);
	}

	trap_LinkEntity(ent);
}

void G_RunMissile(gentity_t *ent) {
	vec3_t origin;
	trace_t tr;
	int passent;

	// get current position
	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);

	// if this missile bounced off an invulnerability sphere
	if(ent->target_ent) {
		passent = ent->target_ent->s.number;
	}
	// prox mines that left the owner bbox will attach to anything, even the owner
	else if(ent->s.weapon == WP_PROX_LAUNCHER && ent->count) {
		passent = ENTITYNUM_NONE;
	} else {
		// ignore interactions with the missile owner
		passent = ent->r.ownerNum;
	}
	// trace a line from the previous position to the current position
	trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask);

	if(tr.startsolid || tr.allsolid) {
		// make sure the tr.entityNum is set to the entity we're stuck in
		trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask);
		tr.fraction = 0;
	} else {
		VectorCopy(tr.endpos, ent->r.currentOrigin);
	}

	trap_LinkEntity(ent);

	if(tr.fraction != 1) {
		G_MissileImpact(ent, &tr);
		if(ent->s.eType != ET_MISSILE) {
			return; // exploded
		}
	}
	// if the prox mine wasn't yet outside the player body
	if(ent->s.weapon == WP_PROX_LAUNCHER && !ent->count) {
		// check if the prox mine is outside the owner bbox
		trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ENTITYNUM_NONE, ent->clipmask);
		if(!tr.startsolid || tr.entityNum != ent->r.ownerNum) {
			ent->count = 1;
		}
	}
	// check think function after bouncing
	G_RunThink(ent);
}

gentity_t *fire_missile(gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up, int weapon) {
	gentity_t *bolt;
	vec3_t dir, end;
	trace_t tr;
	float r, u, scale;
	int random_mt = (rand() % (gameInfoWProps[gameInfoWeapons[weapon].mType].mtMax - gameInfoWProps[gameInfoWeapons[weapon].mType].mtMin + 1)) + gameInfoWProps[gameInfoWeapons[weapon].mType].mtMin;

	VectorNormalize(dir);

	// create missile
	bolt = G_Spawn();

	// classname
	bolt->classname = gameInfoWeapons[weapon].classname;

	if(gameInfoWeapons[weapon].mType > MT_PROPS) {
		bolt->s.eType = ET_GENERAL;
		bolt->spawnflags = 0;
		bolt->sandboxObject = qtrue;

		bolt->objectType = gameInfoWProps[gameInfoWeapons[weapon].mType].oType;
		bolt->s.torsoAnim = gameInfoWProps[gameInfoWeapons[weapon].mType].oType;
		bolt->s.generic2 = random_mt;
		bolt->sb_material = random_mt;
		bolt->s.pos.trType = TR_GRAVITY;
		bolt->s.pos.trTime = level.time;
		bolt->phys_bounce = gameInfoWeapons[weapon].bounceModifier;
		bolt->sb_phys = PHYS_DYNAMIC;
		bolt->s.modelindex = G_ModelIndex(gameInfoWProps[gameInfoWeapons[weapon].mType].modelname);
		bolt->model = gameInfoWProps[gameInfoWeapons[weapon].mType].modelname;

		if(gameInfoWProps[gameInfoWeapons[weapon].mType].solid) {
			bolt->r.contents = CONTENTS_SOLID;
			bolt->sb_coll = CONTENTS_SOLID;
		} else {
			bolt->r.contents = CONTENTS_TRIGGER;
			bolt->sb_coll = CONTENTS_TRIGGER;
		}

		bolt->health = gameInfoWProps[gameInfoWeapons[weapon].mType].health;
		bolt->s.scales[0] = gameInfoWProps[gameInfoWeapons[weapon].mType].scale;
		bolt->s.scales[1] = gameInfoWProps[gameInfoWeapons[weapon].mType].scale;
		bolt->s.scales[2] = gameInfoWProps[gameInfoWeapons[weapon].mType].scale;
		bolt->sb_gravity = gameInfoWProps[gameInfoWeapons[weapon].mType].gravity;
		bolt->s.angles2[A2_MASS] = gameInfoWProps[gameInfoWeapons[weapon].mType].gravity;
		bolt->sb_coltype = gameInfoWProps[gameInfoWeapons[weapon].mType].colSize;
		bolt->lastPlayer = self;
		bolt->takedamage = qtrue;
		bolt->die = G_DieProp;
		VectorMA(start, 64, forward, start);
		VectorSet(bolt->r.mins, -gameInfoWProps[gameInfoWeapons[weapon].mType].colSize * gameInfoWProps[gameInfoWeapons[weapon].mType].scale, -gameInfoWProps[gameInfoWeapons[weapon].mType].colSize * gameInfoWProps[gameInfoWeapons[weapon].mType].scale, -gameInfoWProps[gameInfoWeapons[weapon].mType].colSize * gameInfoWProps[gameInfoWeapons[weapon].mType].scale);
		VectorSet(bolt->r.maxs, gameInfoWProps[gameInfoWeapons[weapon].mType].colSize * gameInfoWProps[gameInfoWeapons[weapon].mType].scale, gameInfoWProps[gameInfoWeapons[weapon].mType].colSize * gameInfoWProps[gameInfoWeapons[weapon].mType].scale, gameInfoWProps[gameInfoWeapons[weapon].mType].colSize * gameInfoWProps[gameInfoWeapons[weapon].mType].scale);

		trap_Trace(&tr, start, bolt->r.mins, bolt->r.maxs, start, bolt->s.number, MASK_OBJECTS);

		if(tr.startsolid) {
			G_FreeEntity(bolt);
			return NULL;
		}

		VectorCopy(start, bolt->s.pos.trBase);
	} else {
		bolt->s.eType = ET_MISSILE;
		bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
		bolt->s.weapon = gameInfoWeapons[weapon].mEffect;
		bolt->r.ownerNum = self->s.number;
		bolt->parent = self;
		bolt->clipmask = MASK_SHOT;
		bolt->target_ent = NULL;

		// think
		if(gameInfoWeapons[weapon].guided) {
			bolt->nextthink = level.time + 50;
			bolt->think = Guided_Missile_Think;
			bolt->wait = level.time + gameInfoWeapons[weapon].timeout;
		} else {
			bolt->nextthink = level.time + gameInfoWeapons[weapon].timeout;
			bolt->think = G_ExplodeMissile;
		}

		// damage
		bolt->damage = gameInfoWeapons[weapon].damage;
		bolt->methodOfDeath = gameInfoWeapons[weapon].mod;
		bolt->splashDamage = gameInfoWeapons[weapon].splashDamage;
		bolt->splashRadius = gameInfoWeapons[weapon].splashRadius;

		// physics
		if(gameInfoWeapons[weapon].bounce) {
			bolt->s.eFlags = EF_BOUNCE;
			bolt->phys_bounce = gameInfoWeapons[weapon].bounceModifier;
		}
		if(gameInfoWeapons[weapon].gravity) {
			bolt->s.pos.trType = TR_GRAVITY;
		} else {
			bolt->s.pos.trType = TR_LINEAR;
		}
		bolt->s.pos.trTime = level.time - 50;
		VectorCopy(start, bolt->s.pos.trBase);
	}

	// speed
	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * gameInfoWeapons[weapon].spread * 16;
	r = cos(r) * crandom() * gameInfoWeapons[weapon].spread * 16;
	VectorMA(start, 8192 * 16, forward, end);
	VectorMA(end, r, right, end);
	VectorMA(end, u, up, end);
	VectorSubtract(end, start, dir);
	VectorNormalize(dir);

	scale = gameInfoWeapons[weapon].speed + (random() * gameInfoWeapons[weapon].speedRandom);
	VectorScale(dir, scale, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);
	VectorCopy(start, bolt->r.currentOrigin);

	if(gameInfoWeapons[weapon].mType == MT_MINE) {
		bolt->s.generic1 = self->client->sess.sessionTeam;
		bolt->count = 0;
	}

	return bolt;
}

static void Missile_Fire(gentity_t *ent, int weapon) {
	gentity_t *m;
	int count;

	if(weapon == WP_GRENADE_LAUNCHER || weapon == WP_PROX_LAUNCHER) { // extra vertical velocity
		forward[2] += 0.2f;
		VectorNormalize(forward);
	}
	for(count = 0; count < gameInfoWeapons[weapon].count; count++) {
		m = fire_missile(ent, muzzle, forward, right, up, weapon);
		m->damage *= s_quadFactor;
		m->splashDamage *= s_quadFactor;
	}
}

/*
===============
Fire Weapon
===============
*/
void FireWeapon(gentity_t *ent) {
	// Make people drop out of follow mode (this should be moved, so people can change betwean players.)
	if(ent->client->sess.spectatorState == SPECTATOR_FOLLOW) {
		StopFollowing(ent);
		return;
	}

	s_quadFactor = 1;
	s_quadFactor *= gameInfoNPCTypes[ent->npcType].damage;

	if(ent->client->spawnprotected) ent->client->spawnprotected = qfalse;

	// set aiming directions
	AngleVectors(ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePointOrigin(ent, ent->client->oldOrigin, forward, right, up, muzzle);

	switch(gameInfoWeapons[ent->s.weapon].wType) {
	case WT_BULLET: Bullet_Fire(ent, ent->s.weapon); break;
	case WT_SHOTGUN: Shotgun_Fire(ent, ent->s.weapon); break;
	case WT_LIGHTNING: Lightning_Fire(ent, ent->s.weapon); break;
	case WT_RAILGUN: Railgun_Fire(ent, ent->s.weapon); break;
	case WT_EMPTY: break;
	case WT_TOOLGUN: break;
	case WT_MISSILE: Missile_Fire(ent, ent->s.weapon); break;
	default: break;
	}
}
