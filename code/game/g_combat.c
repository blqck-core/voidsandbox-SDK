// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../qcommon/js_local.h"

/*
============
AddScore

Adds score to both the client and his team
============
*/
void AddScore(gentity_t *ent, int score) {
	if(!ent->client) return;

	ent->client->ps.persistant[PERS_SCORE] += score;
	if(cvarInt("g_gametype") == GT_TEAM) level.teamScores[ent->client->ps.persistant[PERS_TEAM]] += score;
	CalculateRanks();
}

/*
=================
TossClientItems

Toss the weapon and powerups for the killed player
=================
*/
void TossClientItems(gentity_t *self) {
	item_t *item;
	int i;
	gentity_t *drop;

	if(!gameInfoNPCTypes[self->npcType].dropItems) return;

	// drop all weapons
	for(i = 1; i < WEAPONS_NUM; i++) {
		if(self->swep_list[i] >= WS_HAVE) {
			item = BG_FindItemForWeapon(i);
			if(!item || i == WP_GAUNTLET || i == WP_PHYSGUN || i == WP_GRAVITYGUN || i == WP_TOOLGUN) continue;
			drop = Drop_Item(self, item);
			drop->count = (self->swep_ammo[i]);
			if(drop->count < 1) {
				drop->count = 1;
			}
		}
	}
}

/*
==================
GibEntity
==================
*/
static void GibEntity(gentity_t *self, int killer) {
	G_AddEvent(self, EV_GIB_PLAYER, killer);
	self->takedamage = qfalse;
	self->s.eType = ET_INVISIBLE;
	self->r.contents = 0;
}

/*
==================
body_die
==================
*/
void body_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath) {
	if(self->health > GIB_HEALTH) {
		return;
	}
	if(!cvarInt("g_blood")) {
		self->health = GIB_HEALTH + 1;
		return;
	}

	GibEntity(self, 0);
}

/*
==================
player_die
==================
*/
void player_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath) {
	gentity_t *ent;
	int killer;
	int i;

	if(self->client->ps.pm_type == PM_DEAD || level.intermissiontime) return;

	self->client->noclip = 0;

	if((self->client->ps.eFlags & EF_TICKING) && self->activator) {
		self->client->ps.eFlags &= ~EF_TICKING;
		self->activator->think = G_FreeEntity;
		self->activator->nextthink = level.time;
	}

	self->client->ps.pm_type = PM_DEAD;

	if(attacker) killer = attacker->s.number;

	// broadcast the death event to everyone
	ent = G_TempEntity(self->r.currentOrigin, EV_OBITUARY);
	ent->s.eventParm = meansOfDeath;
	ent->s.otherEntityNum = killer;
	ent->s.otherEntityNum2 = self->s.number;
	ent->r.svFlags = SVF_BROADCAST; // send to everyone

	self->enemy = attacker;

	if(attacker && attacker->client) {
		if(attacker == self || OnSameTeam(self, attacker)) {
			AddScore(attacker, -1);
		} else {
			AddScore(attacker, 1);
		}
	} else {
		AddScore(self, -1);
	}

	TossClientItems(self);

	Cmd_Score_f(self); // show scores
	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for(i = 0; i < level.maxclients; i++) {
		gclient_t *client;

		client = &level.clients[i];
		if(client->pers.connected != CON_CONNECTED) {
			continue;
		}
		if(client->sess.sessionTeam != TEAM_SPECTATOR) {
			continue;
		}
		if(client->sess.spectatorClient == self->s.number) {
			Cmd_Score_f(g_entities + i);
		}
	}

	self->takedamage = qtrue; // can still be gibbed
	self->s.weapon = WP_NONE;
	self->s.powerups = 0;
	self->r.contents = CONTENTS_CORPSE;
	self->s.loopSound = 0;
	self->r.maxs[2] = -8;

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	self->client->respawnTime = level.time + 1700;

	RespawnTimeMessage(self, self->client->respawnTime);

	// remove powerups
	memset(self->client->ps.powerups, 0, sizeof(self->client->ps.powerups));

	G_AddEvent(self, EV_PAIN, killer);
	trap_LinkEntity(self);
}

/*
================
CheckArmor
================
*/
static int CheckArmor(gentity_t *ent, int damage, int dflags) {
	gclient_t *client;
	int save;
	int count;

	if(!damage) return 0;

	client = ent->client;

	if(!client || dflags & DAMAGE_NO_ARMOR) return 0;

	// armor
	count = client->ps.stats[STAT_ARMOR];
	save = damage * 0.80;
	if(save >= count) save = count;

	if(!save) return 0;

	client->ps.stats[STAT_ARMOR] -= save;

	return save;
}

static qboolean G_EnterInCar(gentity_t *player, gentity_t *vehicle) {
	// Validate conditions
	if(!player->client || vehicle->objectType != OT_VEHICLE || player->client->vehicleNum) return qtrue;

	// Assign vehicle to player
	player->client->vehicleNum = vehicle->s.number;
	vehicle->parent = player;

	// Update player properties
	ClientUserinfoChanged(player->s.clientNum);

	// Position synchronization
	VectorCopy(vehicle->s.origin, player->s.origin);
	VectorCopy(vehicle->s.pos.trBase, player->s.pos.trBase);
	player->s.apos.trBase[1] = vehicle->s.apos.trBase[1]; // Only copy yaw
	VectorCopy(vehicle->r.currentOrigin, player->r.currentOrigin);

	// Adjust player collision bounds
	VectorSet(player->r.mins, -25, -25, -15);
	VectorSet(player->r.maxs, 25, 25, 15);

	// Activate vehicle physics
	vehicle->think = Phys_VehiclePlayer;
	vehicle->nextthink = level.time + 1;

	return qtrue;
}

void G_Damage(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod) {
	gclient_t *client;
	int take, save, asave, knockback;

	if(attacker->npcType && !gameInfoNPCTypes[attacker->npcType].friendlyFire && targ->npcType == attacker->npcType) return;

	if(!targ->takedamage && !targ->sandboxObject && mod == WP_TOOLGUN) return;

	if(mod == WP_GAUNTLET) {
		if(targ->objectType == OT_VEHICLE) {
			if(G_EnterInCar(attacker, targ)) {
				return;
			}
		}
	}

	if(targ->client && targ->client->vehicleNum) targ = G_FindEntityForEntityNum(targ->client->vehicleNum); // damage car

	// the intermission has allready been qualified for, so don't
	// allow any extra scoring
	if(level.intermissionQueued) return;

	if(!inflictor) inflictor = &g_entities[ENTITYNUM_WORLD];
	if(!attacker) attacker = &g_entities[ENTITYNUM_WORLD];

	client = targ->client;

	if(!dir) {
		dflags |= DAMAGE_NO_KNOCKBACK;
	} else {
		VectorNormalize(dir);
	}

	knockback = damage;

	if(targ->flags & FL_NO_KNOCKBACK) knockback = 0;
	if(dflags & DAMAGE_NO_KNOCKBACK) knockback = 0;

	// figure momentum add, even if the damage won't be taken
	if(knockback && targ->client || knockback && targ->sandboxObject) {
		vec3_t kvel;
		float mass;

		mass = 200;

		if(targ->sandboxObject) {
			VectorScale(dir, cvarFloat("g_knockback") * 3 * (float)knockback / mass, kvel);
		} else {
			VectorScale(dir, cvarFloat("g_knockback") * (float)knockback / mass, kvel);
		}
		if(targ->client) {
			VectorAdd(targ->client->ps.velocity, kvel, targ->client->ps.velocity);
		}
		if(targ->sandboxObject) { // WELD-TOOL
			if(!targ->physParentEnt) {
				Phys_Enable(targ);
				targ->lastPlayer = attacker;
				VectorAdd(targ->s.pos.trDelta, kvel, targ->s.pos.trDelta);
			} else {
				Phys_Enable(targ->physParentEnt);
				targ->physParentEnt->lastPlayer = attacker;
				if(targ->physParentEnt->phys_weldedObjectsNum > 0) { // mass
					VectorScale(kvel, 1.0f / targ->physParentEnt->phys_weldedObjectsNum, kvel);
				}
				VectorAdd(targ->physParentEnt->s.pos.trDelta, kvel, targ->physParentEnt->s.pos.trDelta);
			}
		}

		// set the timer so that the other client can't cancel
		// out the movement immediately
		if(!targ->sandboxObject) {
			if(!targ->client->ps.pm_time) {
				int t;

				t = knockback * 2;
				if(t < 50) {
					t = 50;
				}
				if(t > 200) {
					t = 200;
				}
				targ->client->ps.pm_time = t;
				targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			}
		}
	}

	if(targ->sandboxObject && targ->health == -1) return;

	// check for completely getting out of the damage
	if(!(dflags & DAMAGE_NO_PROTECTION)) {
		if(targ != attacker && !(dflags & DAMAGE_NO_TEAM_PROTECTION) && OnSameTeam(targ, attacker)) {
			if(!cvarInt("g_friendlyFire")) return;
		}
		if(mod == WP_PROX_LAUNCHER) {
			if(inflictor && inflictor->parent && OnSameTeam(targ, inflictor->parent)) return;
			if(targ == attacker) return;
		}

		if(targ->flags & FL_GODMODE) return; // check for godmode
	}

	// add to the attacker's hit counter (if the target isn't a general entity like a prox mine)
	if(attacker->client && targ != attacker && targ->health > 0 && targ->s.eType != ET_MISSILE && targ->s.eType != ET_GENERAL) {
		if(OnSameTeam(targ, attacker)) {
			attacker->client->ps.persistant[PERS_HITS]--;
		} else {
			attacker->client->ps.persistant[PERS_HITS]++;
		}
		attacker->client->ps.persistant[PERS_ATTACKEE_ARMOR] = (targ->health << 8) | (client->ps.stats[STAT_ARMOR]);
	}

	// always give half damage if hurting self
	// calculated after knockback, so rocket jumping works
	if(targ == attacker) damage *= 0.20;

	if(damage < 1) damage = 1;
	take = damage;
	save = 0;

	// save some from armor
	asave = CheckArmor(targ, take, dflags);
	take -= asave;

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if(client) {
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
		if(dir) {
			VectorCopy(dir, client->damage_from);
			client->damage_fromWorld = qfalse;
		} else {
			VectorCopy(targ->r.currentOrigin, client->damage_from);
			client->damage_fromWorld = qtrue;
		}
	}

	if(targ->client) {
		// set the last client who damaged the target
		targ->client->lasthurt_client = attacker->s.number;
		targ->client->lasthurt_mod = mod;
	}

	// do the damage
	if(take) {
		targ->health = targ->health - take;
		if(targ->client) {
			targ->client->ps.stats[STAT_HEALTH] = targ->health;
		}

		if(targ->health <= 0) {
			if(client) targ->flags |= FL_NO_KNOCKBACK;

			if(targ->health < -999) targ->health = -999;

			targ->enemy = attacker;
			targ->die(targ, inflictor, attacker, take, mod);
			return;
		} else if(targ->pain) {
			targ->pain(targ, attacker, take);
		}
	}
}

/*
============
CanDamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage(gentity_t *targ, vec3_t origin) {
	vec3_t dest;
	trace_t tr;
	vec3_t midpoint;

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	VectorAdd(targ->r.absmin, targ->r.absmax, midpoint);
	VectorScale(midpoint, 0.5, midpoint);

	VectorCopy(midpoint, dest);
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if(tr.fraction == 1.0 || tr.entityNum == targ->s.number) return qtrue;

	// this should probably check in the plane of projection,
	// rather than in world coordinate, and also include Z
	VectorCopy(midpoint, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if(tr.fraction == 1.0) return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if(tr.fraction == 1.0) return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if(tr.fraction == 1.0) return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if(tr.fraction == 1.0) return qtrue;

	return qfalse;
}

/*
============
G_RadiusDamage
============
*/
void G_RadiusDamage(vec3_t origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod) {
	float points, dist;
	gentity_t *ent;
	int entityList[MAX_GENTITIES];
	int numListedEntities;
	vec3_t mins, maxs;
	vec3_t v;
	vec3_t dir;
	int i, e;

	if(radius < 1) radius = 1;

	for(i = 0; i < 3; i++) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	for(e = 0; e < numListedEntities; e++) {
		ent = &g_entities[entityList[e]];

		if(ent == ignore) continue;
		if(!ent->takedamage) continue;

		// find the distance from the edge of the bounding box
		for(i = 0; i < 3; i++) {
			if(origin[i] < ent->r.absmin[i]) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if(origin[i] > ent->r.absmax[i]) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}

		dist = VectorLength(v);
		if(dist >= radius) continue;

		points = damage * (1.0 - dist / radius);

		if(CanDamage(ent, origin)) {
			VectorSubtract(ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;
			G_Damage(ent, NULL, attacker, dir, origin, (int)points, DAMAGE_RADIUS, mod);
		}
	}

	return;
}
