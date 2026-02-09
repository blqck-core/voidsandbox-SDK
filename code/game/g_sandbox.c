// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

static void SB_Shooter_Use(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	vec3_t dir;
	vec3_t up, right;

	if(!ent->r.linked) return;

	G_SetMovedir(ent->s.angles, ent->movedir);
	VectorCopy(ent->movedir, dir);

	fire_missile(ent, ent->s.origin, dir, right, up, ent->s.weapon);

	G_AddEvent(ent, EV_FIRE_WEAPON, 0);
}

static void SB_Shooter(gentity_t *ent) {
	ent->use = SB_Shooter_Use;
	ent->s.weapon = ent->spawnflags;

	if(!ent->spawnflags) {
		G_FreeEntity(ent);
		return;
	}

	trap_LinkEntity(ent);
}

// clang-format off
static spawn_t gameInfoSandboxEntities[] = {
	{"sb.shooter", 				SB_Shooter},

	{NULL, 0}
};
// clang-format on

void G_DieProp(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod) {
	G_FreeEntity(self);
}

static void G_TouchProp(gentity_t *self, gentity_t *other, trace_t *trace) {
	if(self->objectType == OT_VEHICLE) {
		if(!other->health || !other->client || other->npcType <= NT_PLAYER || other->client->vehicleNum) {
			return;
		}
		if(self->parent && self->parent->client->vehicleNum) {
			return;
		}
		other->client->vehicleNum = self->s.number;
		self->parent = other;
		ClientUserinfoChanged(other->s.clientNum);
		VectorCopy(self->s.origin, other->s.origin);
		VectorCopy(self->s.pos.trBase, other->s.pos.trBase);
		other->s.apos.trBase[1] = self->s.apos.trBase[1];
		VectorCopy(self->r.currentOrigin, other->r.currentOrigin);
		VectorSet(other->r.mins, -25, -25, -15);
		VectorSet(other->r.maxs, 25, 25, 15);
		self->think = Phys_VehiclePlayer;
		self->nextthink = level.time + 1;
	}
}

static void setModel(gentity_t *ent, char *modelName) {
	int len;
	if(!ent || !modelName) {
		print("Invalid entity or model name\n");
		return;
	}

	len = strlen(modelName);
	if(len == 0) {
		print("Model name is empty\n");
		return;
	}

	ent->s.modelindex = G_ModelIndex(modelName);
	CopyAlloc(ent->model, modelName);

	if(len >= 4 && !Q_stricmp(ent->model + len - 4, ".md3")) ent->model[len - 4] = '\0';

	if(FS_FileExists(va("%s.bsp", ent->model))) {
		trap_SetBrushModel(ent, va("%s.bsp", ent->model));
		ent->s.modelindex2 = G_ModelIndex(modelName);
	}
}

void SP_sandbox_npc(gentity_t *ent) { G_AddBot(ent->clientname, ent->message, "Blue", ent); }

void SP_sandbox_prop(gentity_t *ent) {
	spawn_t *s;
	qboolean spawn_entity = qfalse;

	// Create entity
	CopyAlloc(ent->classname, ent->sb_class);

	// Origin
	VectorCopy(ent->s.origin, ent->s.pos.trBase);    // Client
	VectorCopy(ent->s.origin, ent->r.currentOrigin); // Physics

	// Type
	ent->sandboxObject = qtrue;

	// Classic entity spawn
	for(s = gameInfoEntities; s->name; s++) {
		if(!strcmp(s->name, ent->classname)) {
			s->spawn(ent);

			spawn_entity = qtrue;
		}
	}

	// Sandbox entity spawn
	for(s = gameInfoSandboxEntities; s->name; s++) {
		if(!strcmp(s->name, ent->classname)) {
			s->spawn(ent);

			spawn_entity = qtrue;
		}
	}

	// Light
	ent->s.constantLight = ent->sb_red | (ent->sb_green << 8) | (ent->sb_blue << 16) | (ent->sb_radius << 24);

	// Sound
	ent->s.loopSound = G_SoundIndex(ent->sb_sound);

	// Setting collision
	if(ent->objectType != OT_VEHICLE || spawn_entity) {
		VectorSet(ent->r.mins, -ent->sb_coltype * ent->s.scales[0], -ent->sb_coltype * ent->s.scales[1], -ent->sb_coltype * ent->s.scales[2]);
		VectorSet(ent->r.maxs, ent->sb_coltype * ent->s.scales[0], ent->sb_coltype * ent->s.scales[1], ent->sb_coltype * ent->s.scales[2]);
	} else {
		VectorSet(ent->r.mins, -25, -25, -15);
		VectorSet(ent->r.maxs, 25, 25, 15);
	}

	// Phys frame
	ent->s.pos.trTime = level.time;

	// Damage
	ent->takedamage = qtrue;

	// Physics
	if(ent->sb_phys == PHYS_STATIC) {
		ent->s.pos.trType = TR_STATIONARY;
	}
	if(ent->sb_phys == PHYS_DYNAMIC) {
		ent->s.pos.trType = TR_GRAVITY;
		ent->s.pos.trTime = level.time;
	}

	// Collision
	ent->r.contents = ent->sb_coll;

	// Material
	ent->s.generic2 = ent->sb_material;

	// Mass
	ent->s.angles2[A2_MASS] = ent->sb_gravity;

	// Type
	ent->s.torsoAnim = ent->objectType;

	// Prop class for saving
	ent->classname = "sandbox_prop";

	if(!spawn_entity) {
		// Type
		ent->s.eType = ET_GENERAL;

		// Die function
		ent->die = G_DieProp;

		// Touch function
		ent->touch = G_TouchProp;
	}

	// Angles
	VectorCopy(ent->s.angles, ent->s.apos.trBase);

	// Load model
	setModel(ent, ent->model);

	// Link
	trap_LinkEntity(ent);
}

void G_BuildProp(char *arg02, char *arg03, vec3_t xyz, gentity_t *player, char *arg04, char *arg05, char *arg06, char *arg07, char *arg08, char *arg09, char *arg10, char *arg11, char *arg12, char *arg13, char *arg14, char *arg15, char *arg16, char *arg17, char *arg18, char *arg19, char *arg20, char *arg21, char *arg22) {
	gentity_t *ent;
	vec3_t position;
	spawn_t *s;
	item_t *item;
	qboolean spawn_entity = qfalse;
	int i;
	qboolean extended_spawn = qtrue;
	qboolean allow_spawn = qfalse;

	position[0] = ((int)((xyz[0] + (xyz[0] < 0 ? -atoi(arg06) : atoi(arg06))) / (atoi(arg06) * 2)) * (atoi(arg06) * 2));
	position[1] = ((int)((xyz[1] + (xyz[1] < 0 ? -atoi(arg06) : atoi(arg06))) / (atoi(arg06) * 2)) * (atoi(arg06) * 2));
	if(atoi(arg09) <= 0) {
		position[2] = ((int)((xyz[2] + (xyz[2] < 0 ? -atoi(arg06) : atoi(arg06))) / (atoi(arg06) * 2)) * (atoi(arg06) * 2));
	} else {
		position[2] = xyz[2] + atof(arg05);
	}

	// Create entity
	ent = G_Spawn();
	CopyAlloc(ent->classname, arg03);
	CopyAlloc(ent->sb_class, arg03);
	for(i = 0; i < gameInfoSandboxSpawnsNum; i++) { // Check allowed sandbox list
		if(!strcmp(ent->classname, gameInfoSandboxSpawns[i])) {
			allow_spawn = qtrue;
		}
	}

	if(!allow_spawn) {
		trap_SendServerCommand(player->s.clientNum, va("lp \"Spawning of %s is not allowed\n\"", ent->classname));
		G_FreeEntity(ent);
		return;
	}

	Undo_AddElement(player, ent->s.number);

	// Origin
	VectorCopy(position, ent->s.origin);
	VectorCopy(position, ent->s.pos.trBase);
	VectorCopy(position, ent->r.currentOrigin);

	// Basic
	ent->sandboxObject = qtrue;
	ent->spawnflags = atoi(arg07);

	if(extended_spawn) {
		// Damage
		ent->takedamage = qtrue;

		// Owner
		if(atoi(arg04) == 1) {
			ent->owner = player;
		}

		// Material
		ent->s.generic2 = atoi(arg08);
		ent->sb_material = atoi(arg08);

		// Physics
		if(atoi(arg09) == 0) {
			ent->s.pos.trType = TR_STATIONARY;
			ent->s.pos.trTime = level.time;
			ent->phys_bounce = atof(arg21);
			ent->sb_phys = PHYS_STATIC;
		}
		if(atoi(arg09) == 1) {
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
			ent->phys_bounce = atof(arg21);
			ent->sb_phys = PHYS_DYNAMIC;
		}

		// Collision
		ent->r.contents = atoi(arg10);
		ent->sb_coll = atoi(arg10);

		// Sound
		ent->s.loopSound = G_SoundIndex(arg11);
		CopyAlloc(ent->sb_sound, arg11);

		// HP
		ent->health = atoi(arg12);

		// Light
		ent->s.constantLight = atoi(arg13) | (atoi(arg14) << 8) | (atoi(arg15) << 16) | (atoi(arg16) << 24);
		ent->sb_red = atoi(arg13);
		ent->sb_green = atoi(arg14);
		ent->sb_blue = atoi(arg15);
		ent->sb_radius = atoi(arg16);

		// Scale
		ent->s.scales[0] = atof(arg17);
		ent->s.scales[1] = atof(arg18);
		ent->s.scales[2] = atof(arg19);

		// Type
		ent->objectType = atoi(arg20);
		ent->s.torsoAnim = atoi(arg20);

		// Mass
		ent->sb_gravity = atof(arg22);
		ent->s.angles2[A2_MASS] = atof(arg22);
	}

	// Item spawn
	for(item = gameInfoItems + 1; item->classname; item++) {
		if(!strcmp(item->classname, ent->classname)) {
			position[2] += 48;
			VectorCopy(position, ent->s.origin);
			VectorCopy(position, ent->s.pos.trBase);
			VectorCopy(position, ent->r.currentOrigin);
			G_SpawnItem(ent, item);
			return;
		}
	}

	// Classic entity spawn
	for(s = gameInfoEntities; s->name; s++) {
		if(!strcmp(s->name, ent->classname)) {
			s->spawn(ent);

			spawn_entity = qtrue;

			if(!extended_spawn) { // Standard spawn
				return;
			}
		}
	}

	// Sandbox entity spawn
	for(s = gameInfoSandboxEntities; s->name; s++) {
		if(!strcmp(s->name, ent->classname)) {
			s->spawn(ent);

			spawn_entity = qtrue;

			if(!extended_spawn) { // Standard spawn
				return;
			}
		}
	}

	// Prop class for saving
	ent->classname = "sandbox_prop";

	if(!spawn_entity) {
		// Type
		ent->s.eType = ET_GENERAL;

		// Die function
		ent->die = G_DieProp;

		// Touch function
		ent->touch = G_TouchProp;
	}

	// Setting collsion
	if(atoi(arg21) <= 0 || spawn_entity) {
		ent->sb_coltype = atof(arg05);
		VectorSet(ent->r.mins, -ent->sb_coltype * ent->s.scales[0], -ent->sb_coltype * ent->s.scales[1], -ent->sb_coltype * ent->s.scales[2]);
		VectorSet(ent->r.maxs, ent->sb_coltype * ent->s.scales[0], ent->sb_coltype * ent->s.scales[1], ent->sb_coltype * ent->s.scales[2]);
	} else {
		ent->sb_coltype = atof(arg05);
		VectorSet(ent->r.mins, -25, -25, -15);
		VectorSet(ent->r.maxs, 25, 25, 15);
	}

	// Load model
	setModel(ent, arg02);

	// Link
	trap_LinkEntity(ent);
}

void G_ModProp(gentity_t *targ, gentity_t *attacker, char *arg01, char *arg02, char *arg03, char *arg04, char *arg05) { // tool_id
	gentity_t *entity;

	entity = targ;
	if(cvarInt("g_gametype") != GT_SANDBOX) return;

	if(!G_PlayerIsOwner(attacker, entity)) return;

	if(attacker->tool_id == TL_CREATE) {
		// client-side command for spawn prop
	}

	if(attacker->tool_id == TL_MATERIAL) {
		entity->s.generic2 = atoi(arg01);
		entity->sb_material = atoi(arg01);
	}

	if(attacker->tool_id == TL_DELETE) {
		if(!entity->client) {
			G_FreeEntity(entity);
		} else {
			DropClientSilently(entity->client->ps.clientNum);
		}
	}

	if(attacker->tool_id == TL_PHYSICS) {
		if(atoi(arg05) == 0) {
			entity->s.pos.trType = TR_STATIONARY;
			entity->sb_phys = PHYS_STATIC;
			Phys_Disable(entity, entity->s.pos.trBase);
		}
		if(atoi(arg05) == 1) {
			entity->s.pos.trType = TR_GRAVITY;
			entity->s.pos.trTime = level.time;
			entity->sb_phys = PHYS_DYNAMIC;
			Phys_Enable(entity);
		}
	}

	if(attacker->tool_id == TL_PRIVATE) {
		if(atoi(arg05) == 0) {
			entity->owner = 0;
		}
		if(atoi(arg05) == 1) {
			entity->owner = attacker;
		}
		if(atoi(arg05) == 2) {
			if(entity->owner) {
				trap_SendServerCommand(attacker->s.clientNum, va("lp \"Owned by %s\n\"", entity->owner->client->pers.netname));
			}
			if(!entity->owner) {
				trap_SendServerCommand(attacker->s.clientNum, "lp \"Not owned\n\"");
			}
		}
	}

	if(attacker->tool_id == TL_COLLISION) {
		if(atoi(arg05) == 0) {
			entity->r.contents = CONTENTS_SOLID;
			entity->sb_coll = CONTENTS_SOLID;
		}
		if(atoi(arg05) == 1) {
			entity->r.contents = CONTENTS_TRIGGER;
			entity->sb_coll = CONTENTS_TRIGGER;
		}
	}

	if(attacker->tool_id == TL_HEALTH) {
		entity->health = atoi(arg01);
	}

	if(attacker->tool_id == TL_COLOR) {
		entity->s.constantLight = atoi(arg01) | (atoi(arg02) << 8) | (atoi(arg03) << 16) | (atoi(arg04) << 24);
		entity->sb_red = atoi(arg01);
		entity->sb_green = atoi(arg02);
		entity->sb_blue = atoi(arg03);
		entity->sb_radius = atoi(arg04);
	}

	if(attacker->tool_id == TL_ANGLE) {
		entity = G_FindWeldEntity(entity); // find weld root or return ent
		if(atoi(arg05) == 0) {
			entity->s.apos.trBase[0] += atof(arg01);
		}
		if(atoi(arg05) == 1) {
			entity->s.apos.trBase[1] += atof(arg01);
		}
		if(atoi(arg05) == 2) {
			entity->s.apos.trBase[2] += atof(arg01);
		}
		if(atoi(arg05) == 3) {
			entity->s.apos.trBase[0] = 0.0;
			entity->s.apos.trBase[1] = 0.0;
			entity->s.apos.trBase[2] = 0.0;
		}
		VectorCopy(entity->s.apos.trBase, targ->s.angles);
		VectorCopy(entity->s.apos.trBase, targ->r.currentAngles);
	}

	if(attacker->tool_id == TL_SCALE) {
		if(atoi(arg05) == 0) {
			entity->s.scales[0] = atof(arg01);
		}
		if(atoi(arg05) == 1) {
			entity->s.scales[1] = atof(arg01);
		}
		if(atoi(arg05) == 2) {
			entity->s.scales[2] = atof(arg01);
		}
		if(atoi(arg05) == 3) {
			entity->s.scales[0] = 1.0;
			entity->s.scales[1] = 1.0;
			entity->s.scales[2] = 1.0;
		}
		VectorSet(entity->r.mins, -entity->sb_coltype * entity->s.scales[0], -entity->sb_coltype * entity->s.scales[1], -entity->sb_coltype * entity->s.scales[2]);
		VectorSet(entity->r.maxs, entity->sb_coltype * entity->s.scales[0], entity->sb_coltype * entity->s.scales[1], entity->sb_coltype * entity->s.scales[2]);
	}

	if(attacker->tool_id == TL_REPLACEITEM) {
		item_t *item;
		int i;
		if(entity->s.eType != ET_ITEM) {
			trap_SendServerCommand(attacker->s.clientNum, "lp \"This must be the item\n\"");
			return;
		}
		for(item = gameInfoItems + 1, i = 1; item->classname; item++, i++) {
			if(!strcmp(item->classname, arg01)) {
				entity->item = &gameInfoItems[i];
				entity->item->classname = gameInfoItems[i].classname;
				entity->classname = gameInfoItems[i].classname;
				entity->s.modelindex = i;
			}
		}
	}

	if(attacker->tool_id == TL_COUNT) {
		entity->count = atoi(arg01);
	}

	if(attacker->tool_id == TL_WELD) {
		vec3_t parentForward, parentRight, parentUp;
		vec3_t weldForward, weldRight, weldUp;
		vec3_t relForward, relRight, relUp;

		if(entity->client) {
			trap_SendServerCommand(attacker->s.clientNum, "lp \"This is not prop\n\"");
			return;
		}

		if(atoi(arg05) == 0) { // Weld
			if(!attacker->tool_entity) {
				if(!entity->phys_weldedObjectsNum) {
					attacker->tool_entity = entity;
					trap_SendServerCommand(attacker->s.clientNum, "lp \"Сlick on the second object\n\"");
				} else {
					trap_SendServerCommand(attacker->s.clientNum, "lp \"This is root object\n\"");
					return;
				}
			} else {
				entity = G_FindWeldEntity(entity); // find weld root or return ent
				attacker->tool_entity->physParentEnt = entity;

				// Save origin
				VectorSubtract(attacker->tool_entity->r.currentOrigin, entity->r.currentOrigin, attacker->tool_entity->phys_relativeOrigin);
				attacker->tool_entity->phys_relativeOrigin[1] = -attacker->tool_entity->phys_relativeOrigin[1];

				// Vector from parent
				AngleVectors(entity->s.apos.trBase, parentForward, parentRight, parentUp);
				AngleVectors(attacker->tool_entity->s.apos.trBase, weldForward, weldRight, weldUp);

				// Diff vectors
				relForward[0] = DotProduct(weldForward, parentForward);
				relForward[1] = DotProduct(weldForward, parentRight);
				relForward[2] = DotProduct(weldForward, parentUp);

				relRight[0] = DotProduct(weldRight, parentForward);
				relRight[1] = DotProduct(weldRight, parentRight);
				relRight[2] = DotProduct(weldRight, parentUp);

				relUp[0] = DotProduct(weldUp, parentForward);
				relUp[1] = DotProduct(weldUp, parentRight);
				relUp[2] = DotProduct(weldUp, parentUp);

				OrthogonalizeMatrix(relForward, relRight, relUp);

				// Save result
				VectorCopy(relForward, attacker->tool_entity->phys_rv_0);
				VectorCopy(relRight, attacker->tool_entity->phys_rv_1);
				VectorCopy(relUp, attacker->tool_entity->phys_rv_2);
				entity->phys_weldedObjectsNum += 1;
				attacker->tool_entity = NULL;
			}
		}
		if(atoi(arg05) == 1) { // Unweld
			attacker->tool_entity = NULL;

			if(!entity->phys_weldedObjectsNum && !entity->physParentEnt) {
				trap_SendServerCommand(attacker->s.clientNum, "lp \"This prop not welded\n\"");
				return;
			}

			Phys_Unweld(entity);
		}
	}

	if(attacker->tool_id == TL_BIND) {
		if(atoi(arg05) == 0) {
			CopyAlloc(entity->targetname, va("activate_%i_%s", attacker->s.clientNum, arg01));
		}
		if(atoi(arg05) == 1) {
			entity->targetname = NULL;
		}
	}
}

static void Undo_ShiftStack(gentity_t *ent) {
	int i;

	for(i = MAX_UNDO_STACK - 1; i > 0; i--) ent->client->pers.undoStack[i] = ent->client->pers.undoStack[i - 1];
}

void Undo_AddElement(gentity_t *ent, int id) {
	Undo_ShiftStack(ent);

	ent->client->pers.undoStack[0].id = id;
	ent->client->pers.undoStack[0].isRemoved = qfalse;
}

qboolean Undo_LastElement(gentity_t *ent, int *id, qboolean *isRemoved) {
	int i;
	gentity_t *pent;

	for(i = 0; i < MAX_UNDO_STACK - 1; i++) {
		pent = G_FindEntityForEntityNum(ent->client->pers.undoStack[i].id);
		if(pent) {
			ent->client->pers.undoStack[i].isRemoved = qfalse;
		} else {
			ent->client->pers.undoStack[i].isRemoved = qtrue;
		}
	}

	if(ent->client->pers.undoStack[0].id == 0 && ent->client->pers.undoStack[0].isRemoved == qfalse) return qfalse;

	*id = ent->client->pers.undoStack[0].id;
	*isRemoved = ent->client->pers.undoStack[0].isRemoved;

	return qtrue;
}

void Undo_RemoveElement(gentity_t *ent) {
	int i;

	for(i = 0; i < MAX_UNDO_STACK - 1; i++) ent->client->pers.undoStack[i] = ent->client->pers.undoStack[i + 1];

	ent->client->pers.undoStack[MAX_UNDO_STACK - 1].id = 0;
	ent->client->pers.undoStack[MAX_UNDO_STACK - 1].isRemoved = qfalse;
}
