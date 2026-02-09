// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

static int G_FindConfigstringIndex(char *name, int start, int max, qboolean create) {
	int i;
	char s[MAX_STRING_CHARS];

	if(!name || !name[0]) {
		return 0;
	}

	for(i = 1; i < max; i++) {
		trap_GetConfigstring(start + i, s, sizeof(s));
		if(!s[0]) {
			break;
		}
		if(!strcmp(s, name)) {
			return i;
		}
	}

	if(!create) {
		return 0;
	}

	iferr(i == max);

	trap_SetConfigstring(start + i, name);

	return i;
}

int G_ModelIndex(char *name) { return G_FindConfigstringIndex(name, CS_MODELS, MAX_MODELS, qtrue); }

int G_SoundIndex(char *name) { return G_FindConfigstringIndex(name, CS_SOUNDS, MAX_SOUNDS, qtrue); }

/*
================
G_TeamCommand

Broadcasts a command to only a specific team
================
*/
void G_TeamCommand(team_t team, char *cmd) {
	int i;

	for(i = 0; i < level.maxclients; i++) {
		if(level.clients[i].pers.connected == CON_CONNECTED) {
			if(level.clients[i].sess.sessionTeam == team) {
				trap_SendServerCommand(i, va("%s", cmd));
			}
		}
	}
}

/*
=============
G_Find

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the entity after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.
=============
*/
gentity_t *G_Find(gentity_t *from, int fieldofs, const char *match) {
	char *s;

	if(!from)
		from = g_entities;
	else
		from++;

	for(; from < &g_entities[level.num_entities]; from++) {
		if(!from->inuse) continue;
		s = *(char **)((byte *)from + fieldofs);
		if(!s) continue;
		if(!Q_stricmp(s, match)) return from;
	}

	return NULL;
}

/*
=============
G_PickTarget
Selects a random entity from among the targets
=============
*/
#define MAXCHOICES 32

gentity_t *G_PickTarget(char *targetname) {
	gentity_t *ent = NULL;
	int num_choices = 0;
	gentity_t *choice[MAXCHOICES];

	if(!targetname) {
		print("G_PickTarget called with NULL targetname\n");
		return NULL;
	}

	while(1) {
		ent = G_Find(ent, FOFS(targetname), targetname);
		if(!ent) break;
		choice[num_choices++] = ent;
		if(num_choices == MAXCHOICES) break;
	}

	if(!num_choices) {
		print("G_PickTarget: target %s not found\n", targetname);
		return NULL;
	}

	return choice[rand() % num_choices];
}

/*
=============
G_PickAllTargets
Selects all entities
=============
*/
void G_PickAllTargets(gentity_t *ent) {
	gentity_t *t = NULL;

	while((t = G_Find(t, FOFS(targetname), ent->target)) != NULL) {
		if(t->use) {
			t->use(t, ent, ent);
		}
	}
}

/*
==============================
G_UseTargets
"activator" should be set to the entity that initiated the firing.
Search for (string)targetname in all entities that
match (string)self.target and call their .use function
==============================
*/
void G_UseTargets(gentity_t *ent, gentity_t *activator) {
	gentity_t *t;

	if(!ent || !ent->target) return;

	t = NULL;
	while((t = G_Find(t, FOFS(targetname), ent->target)) != NULL) {
		if(t == ent) {
			print("WARNING: Entity used itself.\n");
		} else {
			if(t->use) {
				t->use(t, ent, activator);
			}
		}
		if(!ent->inuse) {
			print("entity was removed while using targets\n");
			return;
		}
	}
}

/*
=============
VectorToString
This is just a convenience function
for printing vectors
=============
*/
char *vtos(const vec3_t v) {
	static int index;
	static char str[8][32];
	char *s;

	// use an array so that multiple vtos won't collide
	s = str[index];
	index = (index + 1) & 7;

	Com_sprintf(s, 32, "(%i %i %i)", (int)v[0], (int)v[1], (int)v[2]);

	return s;
}

/*
===============
G_SetMovedir
The editor only specifies a single value for angles (yaw),
but we have special constants to generate an up or down direction.
Angles will be cleared, because it is being used to represent a direction
instead of an orientation.
===============
*/
void G_SetMovedir(vec3_t angles, vec3_t movedir) {
	static vec3_t VEC_UP = {0, -1, 0};
	static vec3_t MOVEDIR_UP = {0, 0, 1};
	static vec3_t VEC_DOWN = {0, -2, 0};
	static vec3_t MOVEDIR_DOWN = {0, 0, -1};

	if(VectorCompare(angles, VEC_UP)) {
		VectorCopy(MOVEDIR_UP, movedir);
	} else if(VectorCompare(angles, VEC_DOWN)) {
		VectorCopy(MOVEDIR_DOWN, movedir);
	} else {
		AngleVectors(angles, movedir, NULL, NULL);
	}
	VectorClear(angles);
}

void G_InitGentity(gentity_t *e) {
	e->inuse = qtrue;
	e->classname = "noclass";
	e->s.number = e - g_entities;
	e->r.ownerNum = ENTITYNUM_NONE;
}

/*
=================
G_Spawn
Either finds a free entity, or allocates a new one.
=================
*/
gentity_t *G_Spawn(void) {
	int i, force;
	gentity_t *e;

	e = NULL; // shut up warning
	i = 0;    // shut up warning
	for(force = 0; force < 2; force++) {
		// if we go through all entities and can't find one to free,
		// override the normal minimum times before use
		e = &g_entities[MAX_CLIENTS];
		for(i = MAX_CLIENTS; i < level.num_entities; i++, e++) {
			if(e->inuse) {
				continue;
			}

			// the first couple seconds of server time can involve a lot of
			// freeing and allocating, so relax the replacement policy
			if(!force && e->freetime > level.startTime + 2000 && level.time - e->freetime < 1000) {
				continue;
			}

			// reuse this slot
			G_InitGentity(e);
			return e;
		}
		if(i != cvarInt("g_maxEntities") - 1) {
			break;
		}
	}
	if(i >= cvarInt("g_maxEntities") - 1) {
		print("G_Spawn: no free entities. Check g_maxEntities cvar\n");
		G_FreeEntity(e);
		return e;
	}

	// open up a new slot
	level.num_entities++;

	// let the server system know that there are more entities
	trap_LocateGameData(level.gentities, level.num_entities, sizeof(gentity_t), &level.clients[0].ps, sizeof(level.clients[0]));

	G_InitGentity(e);
	return e;
}

/*
=================
G_FreeEntity
Marks the entity as free
=================
*/
void G_FreeEntity(gentity_t *ed) {
	trap_UnlinkEntity(ed); // unlink from world

	if(ed->neverFree) return;

	if(ed->objectType == OT_VEHICLE && ed->parent && ed->parent->client->vehicleNum == ed->s.number) { // Reset vehicle
		ed->parent->client->vehicleNum = 0;
		ClientUserinfoChanged(ed->parent->s.clientNum);
		VectorSet(ed->parent->r.mins, -15, -15, -24);
		VectorSet(ed->parent->r.maxs, 15, 15, 32);
		ed->parent->client->ps.gravity = cvarFloat("g_gravity");
	}

	Phys_Unweld(ed);

	if(!strcmp(ed->classname, "sandbox_npc")) {
		if(ed->parent && ed->parent->client && ed->parent->client->pers.connected == CON_CONNECTED) DropClientSilently(ed->parent->s.clientNum);
	}

	memset(ed, 0, sizeof(*ed));
	ed->classname = "freed";
	ed->freetime = level.time;
	ed->inuse = qfalse;
}

/*
=================
G_TempEntity
Spawns an event entity that will be auto-removed
=================
*/
gentity_t *G_TempEntity(vec3_t origin, int event) {
	gentity_t *e;
	vec3_t snapped;

	e = G_Spawn();
	e->s.eType = ET_EVENTS + event;

	e->classname = "tempEntity";
	e->eventTime = level.time;
	e->freeAfterEvent = qtrue;

	VectorCopy(origin, snapped);
	SnapVector(snapped); // save network bandwidth
	G_SetOrigin(e, snapped);

	// find cluster for PVS
	trap_LinkEntity(e);

	return e;
}

void G_KillBox(gentity_t *ent) {
	int i, num;
	gentity_t *hit;
	vec3_t mins, maxs;

	VectorAdd(ent->client->ps.origin, ent->r.mins, mins);
	VectorAdd(ent->client->ps.origin, ent->r.maxs, maxs);
	num = trap_EntitiesInBox(mins, maxs, SourceTechEntityList, MAX_GENTITIES);

	for(i = 0; i < num; i++) {
		hit = &g_entities[SourceTechEntityList[i]];
		if(!hit->client) {
			continue;
		}

		// nail it
		G_Damage(hit, ent, ent, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
	}
}

/*
===============
G_AddPredictableEvent
Use for non-pmove events that would also be predicted on the
client side: jumppads and item pickups
Adds an event+parm and twiddles the event counter
===============
*/
void G_AddPredictableEvent(gentity_t *ent, int event, int eventParm) {
	if(!ent->client) {
		return;
	}
	BG_AddPredictableEventToPlayerstate(event, eventParm, &ent->client->ps);
}

/*
===============
G_AddEvent
Adds an event+parm and twiddles the event counter
===============
*/
void G_AddEvent(gentity_t *ent, int event, int eventParm) {
	int bits;

	if(!event) {
		print("G_AddEvent: zero event added for entity %i\n", ent->s.number);
		return;
	}

	// clients need to add the event in playerState_t instead of entityState_t
	if(ent->client) {
		bits = ent->client->ps.externalEvent & EV_EVENT_BITS;
		bits = (bits + EV_EVENT_BIT1) & EV_EVENT_BITS;
		ent->client->ps.externalEvent = event | bits;
		ent->client->ps.externalEventParm = eventParm;
		ent->client->ps.externalEventTime = level.time;
	} else {
		bits = ent->s.event & EV_EVENT_BITS;
		bits = (bits + EV_EVENT_BIT1) & EV_EVENT_BITS;
		ent->s.event = event | bits;
		ent->s.eventParm = eventParm;
	}
	ent->eventTime = level.time;
}

void G_Sound(gentity_t *ent, int channel, int soundIndex) {
	gentity_t *te;

	te = G_TempEntity(ent->r.currentOrigin, EV_GENERAL_SOUND);
	te->s.eventParm = soundIndex;
}

/*
================
G_SetOrigin

Sets the pos trajectory for a fixed position
================
*/
void G_SetOrigin(gentity_t *ent, vec3_t origin) {
	VectorCopy(origin, ent->s.pos.trBase);
	ent->s.pos.trType = TR_STATIONARY;
	ent->s.pos.trTime = 0;
	ent->s.pos.trDuration = 0;
	VectorClear(ent->s.pos.trDelta);

	VectorCopy(origin, ent->r.currentOrigin);
}

gentity_t *FindEntityForPhysgun(gentity_t *ent, int range) {
	vec3_t end, start, forward, up, right;
	trace_t tr;
	gentity_t *traceEnt;

	// Set Aiming Directions
	AngleVectors(ent->client->ps.viewangles, forward, right, up);
	CalcMuzzlePoint(ent, forward, right, up, start);
	VectorMA(start, range, forward, end);

	// Trace Position
	trap_Trace(&tr, start, NULL, NULL, end, ent->s.number, MASK_SELECT);

	ent->grabDist = Distance(start, tr.endpos);

	traceEnt = &g_entities[tr.entityNum]; // entity for return

	if(traceEnt->grabbedEntity == ent) return NULL;

	if(!traceEnt->sandboxObject && traceEnt->npcType <= NT_PLAYER && traceEnt->s.eType != ET_ITEM) return NULL;

	if(traceEnt->physParentEnt) { // WELD-TOOL
		VectorSubtract(traceEnt->physParentEnt->r.currentOrigin, tr.endpos, ent->grabOffset);
		return traceEnt->physParentEnt;
	} else {
		VectorSubtract(traceEnt->r.currentOrigin, tr.endpos, ent->grabOffset);
	}

	if(traceEnt && tr.entityNum != ENTITYNUM_NONE && tr.entityNum != ENTITYNUM_WORLD) {
		return traceEnt;
	} else {
		return NULL;
	}
}

gentity_t *FindEntityForGravitygun(gentity_t *ent, int range) {
	vec3_t end, start, forward, up, right;
	trace_t tr;
	gentity_t *traceEnt;

	// Set Aiming Directions
	AngleVectors(ent->client->ps.viewangles, forward, right, up);
	CalcMuzzlePoint(ent, forward, right, up, start);
	VectorMA(start, range, forward, end);

	// Trace Position
	trap_Trace(&tr, start, NULL, NULL, end, ent->s.number, MASK_SELECT);

	ent->grabDist = 128;

	traceEnt = &g_entities[tr.entityNum]; // entity for return

	if(!traceEnt->sandboxObject && traceEnt->npcType <= NT_PLAYER && traceEnt->s.eType != ET_ITEM) return NULL;

	if(traceEnt->phys_weldedObjectsNum || traceEnt->physParentEnt) return NULL;

	VectorSubtract(traceEnt->r.currentOrigin, tr.endpos, ent->grabOffset);

	if(traceEnt && tr.entityNum != ENTITYNUM_NONE && tr.entityNum != ENTITYNUM_WORLD) {
		return traceEnt;
	} else {
		return NULL;
	}
}

void CrosshairPointPhys(gentity_t *ent, int range, vec3_t outPoint) {
	vec3_t end, start, forward, up, right;
	trace_t tr;
	vec3_t adjustedMins, adjustedMaxs;
	vec3_t adjustedOffset;

	VectorNegate(ent->grabOffset, adjustedOffset);

	VectorSubtract(ent->grabbedEntity->r.mins, adjustedOffset, adjustedMins);
	VectorSubtract(ent->grabbedEntity->r.maxs, adjustedOffset, adjustedMaxs);

	AngleVectors(ent->client->ps.viewangles, forward, right, up);
	CalcMuzzlePoint(ent, forward, right, up, start);
	VectorMA(start, range, forward, end);

	trap_Trace(&tr, start, adjustedMins, adjustedMaxs, end, ent->s.number, MASK_OBJECTS);

	VectorCopy(tr.endpos, outPoint);
}

void CrosshairPointGravity(gentity_t *ent, int range, vec3_t outPoint) {
	vec3_t end, start, forward, up, right;
	trace_t tr;

	AngleVectors(ent->client->ps.viewangles, forward, right, up);
	CalcMuzzlePoint(ent, forward, right, up, start);
	VectorMA(start, range, forward, end);

	trap_Trace(&tr, start, ent->grabbedEntity->r.mins, ent->grabbedEntity->r.maxs, end, ent->s.number, MASK_OBJECTS);

	VectorCopy(tr.endpos, outPoint);
}

gentity_t *G_FindEntityForEntityNum(int entityNum) {
	int i;
	gentity_t *ent;

	for(i = 0, ent = g_entities; i < level.num_entities; i++, ent++) {
		if(ent && ent->s.number == entityNum && ent->inuse) return ent;
	}

	return NULL;
}

qboolean G_PlayerIsOwner(gentity_t *player, gentity_t *ent) {
	if(ent->owner) {
		if(ent->owner != player) {
			trap_SendServerCommand(player->s.clientNum, va("lp \"Owned by %s\n\"", ent->owner->client->pers.netname));
			return qfalse; // ent owned by another player
		} else {
			return qtrue; // ent owned by this player
		}
	} else {
		return qtrue; // ent not owned
	}
}

gentity_t *G_FindWeldEntity(gentity_t *ent) {
	if(ent->physParentEnt) {
		return ent->physParentEnt;
	} else {
		return ent;
	}
}

gentity_t *FindRandomItem(void) {
	gentity_t *finded[MAX_GENTITIES];
	int i, num_finded = 0;

	for(i = 0; i < level.num_entities; i++) {
		gentity_t *ent = &g_entities[i];

		if(!ent->inuse) continue;
		if(ent->s.eType == ET_ITEM) {
			finded[num_finded++] = ent;
			if(num_finded >= MAX_GENTITIES) break;
		}
	}

	if(num_finded > 0) return finded[rand() % num_finded];

	return NULL;
}

gentity_t *FindRandomSpawn(void) {
	gentity_t *finded[MAX_GENTITIES];
	int i, num_finded = 0;
	const char *classname = "info_player_deathmatch";

	for(i = 0; i < level.num_entities; i++) {
		gentity_t *ent = &g_entities[i];

		if(!ent->inuse) continue;
		if(!strcmp(ent->classname, classname)) {
			finded[num_finded++] = ent;
			if(num_finded >= MAX_GENTITIES) break;
		}
	}

	if(num_finded > 0) return finded[rand() % num_finded];

	return NULL;
}

gentity_t *FindRandomTeamSpawn(team_t team) {
	gentity_t *finded[MAX_GENTITIES];
	int i, num_finded = 0;
	const char *classname = (team == TEAM_RED) ? "info_player_redspawn" : "info_player_bluespawn";

	for(i = 0; i < level.num_entities; i++) {
		gentity_t *ent = &g_entities[i];

		if(!ent->inuse) continue;
		if(!strcmp(ent->classname, classname)) {
			finded[num_finded++] = ent;
			if(num_finded >= MAX_GENTITIES) break;
		}
	}

	if(num_finded > 0) return finded[rand() % num_finded];

	return NULL;
}
