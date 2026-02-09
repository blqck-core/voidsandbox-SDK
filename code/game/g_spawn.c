// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../qcommon/js_local.h"

qboolean G_SpawnString(const char *key, const char *defaultString, char **out) {
	int i;

	if(!level.spawning) *out = (char *)defaultString;

	for(i = 0; i < level.numSpawnVars; i++) {
		if(!Q_stricmp(key, level.spawnVars[i][0])) {
			*out = level.spawnVars[i][1];
			return qtrue;
		}
	}

	*out = (char *)defaultString;
	return qfalse;
}

qboolean G_SpawnFloat(const char *key, const char *defaultString, float *out) {
	char *s;
	qboolean present;

	present = G_SpawnString(key, defaultString, &s);
	*out = atof(s);
	return present;
}

qboolean G_SpawnInt(const char *key, const char *defaultString, int *out) {
	char *s;
	qboolean present;

	present = G_SpawnString(key, defaultString, &s);
	*out = atoi(s);
	return present;
}

qboolean G_SpawnVector(const char *key, const char *defaultString, float *out) {
	char *s;
	qboolean present;

	present = G_SpawnString(key, defaultString, &s);
	sscanf(s, "%f %f %f", &out[0], &out[1], &out[2]);
	return present;
}

typedef enum { F_INT, F_FLOAT, F_STRING, F_VECTOR, F_ANGLEHACK, F_IGNORE } fieldtype_t;

typedef struct {
	char *name;
	int ofs;
	fieldtype_t type;
	int flags;
} field_t;

// clang-format off
static field_t gameInfoFields[] = {
	{"classname", 			FOFS(classname), 				F_STRING},
	{"model", 				FOFS(model), 					F_STRING},
	{"model2", 				FOFS(model2), 					F_STRING},
	{"origin", 				FOFS(s.origin), 				F_VECTOR},
	{"angles", 				FOFS(s.angles), 				F_VECTOR},
	{"modelscale_vec", 		FOFS(s.scales), 				F_VECTOR},
	{"spawnflags", 			FOFS(spawnflags), 				F_INT},
	{"speed", 				FOFS(speed), 					F_FLOAT},
	{"target", 				FOFS(target), 					F_STRING},
	{"targetname", 			FOFS(targetname), 				F_STRING},
	{"message", 			FOFS(message), 					F_STRING},
	{"team", 				FOFS(team), 					F_STRING},
	{"wait", 				FOFS(wait), 					F_FLOAT},
	{"random", 				FOFS(random), 					F_FLOAT},
	{"count", 				FOFS(count), 					F_INT},
	{"health", 				FOFS(health), 					F_INT},
	{"light", 				0, 								F_IGNORE},
	{"dmg", 				FOFS(damage), 					F_INT},
	{"angle", 				FOFS(s.angles), 				F_ANGLEHACK},
	{"clientname", 			FOFS(clientname), 				F_STRING},
	{"sb_class", 			FOFS(sb_class), 				F_STRING},
	{"sb_sound", 			FOFS(sb_sound), 				F_STRING},
	{"sb_coltype", 			FOFS(sb_coltype), 				F_FLOAT},
	{"sb_material", 		FOFS(sb_material), 				F_INT},
	{"sb_gravity", 			FOFS(sb_gravity), 				F_FLOAT},
	{"sb_phys", 			FOFS(sb_phys), 					F_INT},
	{"sb_coll", 			FOFS(sb_coll), 					F_INT},
	{"sb_red", 				FOFS(sb_red), 					F_INT},
	{"sb_green", 			FOFS(sb_green), 				F_INT},
	{"sb_blue", 			FOFS(sb_blue), 					F_INT},
	{"sb_radius", 			FOFS(sb_radius), 				F_INT},
	{"sb_isnpc", 			FOFS(sb_isnpc), 				F_INT},
	{"phys_relativeOrigin", FOFS(phys_relativeOrigin), 		F_VECTOR},
	{"phys_rv_0", 			FOFS(phys_rv_0), 				F_VECTOR},
	{"phys_rv_1", 			FOFS(phys_rv_1), 				F_VECTOR},
	{"phys_rv_2", 			FOFS(phys_rv_2), 				F_VECTOR},
	{"phys_bounce", 		FOFS(phys_bounce),				F_FLOAT},
	{"phys_welded", 		FOFS(phys_welded), 				F_INT},
	{"phys_parent", 		FOFS(phys_parent), 				F_INT},
	{"objectType", 			FOFS(objectType), 				F_INT},
	{"type", 				FOFS(type), 					F_INT},
	{NULL}
};
// clang-format on

// clang-format off
spawn_t	gameInfoEntities[] = {
	{"info_player_deathmatch", 		SP_info_player_deathmatch},
	{"sandbox_prop", 				SP_sandbox_prop},
	{"sandbox_npc", 				SP_sandbox_npc},
	{NULL, 0}
};
// clang-format on

/*
===============
G_CallSpawn

Finds the spawn function for the entity and calls it,
returning qfalse if not found
===============
*/
static qboolean G_CallSpawn(gentity_t *ent) {
	spawn_t *s;
	item_t *item;
	char itemname[128];

	if(strcmp(ent->classname, "none") == 0) return qfalse;

	Com_sprintf(itemname, sizeof(itemname), "%s", ent->classname);

	if(itemname[0] == 0) {
		G_Printf("G_CallSpawn: NULL classname\n");
		return qfalse;
	}

	// check item spawn functions
	for(item = gameInfoItems + 1; item->classname; item++) {
		if(!strcmp(item->classname, itemname)) {
			G_SpawnItem(ent, item);
			return qtrue;
		}
	}

	// check normal spawn functions
	for(s = gameInfoEntities; s->name; s++) {
		if(!strcmp(s->name, itemname)) {
			// found it
			s->spawn(ent);
			return qtrue;
		}
	}

	G_Printf("%s doesn't have a spawn function\n", itemname);
	return qfalse;
}

/*
=============
G_NewString

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
static char *G_NewString(const char *string) {
	char *newb, *new_p;
	int i, l;

	l = strlen(string) + 1;
	newb = G_Alloc(l);
	new_p = newb;

	// turn \n into a real linefeed
	for(i = 0; i < l; i++) {
		if(string[i] == '\\' && i < l - 1) {
			i++;
			if(string[i] == 'n') {
				*new_p++ = '\n';
			} else {
				*new_p++ = '\\';
			}
		} else {
			*new_p++ = string[i];
		}
	}

	return newb;
}

/*
===============
G_ParseField

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
static void G_ParseField(const char *key, const char *value, gentity_t *ent) {
	field_t *f;
	byte *b;
	float v;
	vec3_t vec;

	for(f = gameInfoFields; f->name; f++) {
		if(!Q_stricmp(f->name, key)) {
			// found it
			b = (byte *)ent;

			switch(f->type) {
			case F_STRING: *(char **)(b + f->ofs) = G_NewString(value); break;
			case F_VECTOR:
				sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
				((float *)(b + f->ofs))[0] = vec[0];
				((float *)(b + f->ofs))[1] = vec[1];
				((float *)(b + f->ofs))[2] = vec[2];
				break;
			case F_INT: *(int *)(b + f->ofs) = atoi(value); break;
			case F_FLOAT: *(float *)(b + f->ofs) = atof(value); break;
			case F_ANGLEHACK:
				v = atof(value);
				((float *)(b + f->ofs))[0] = 0;
				((float *)(b + f->ofs))[1] = v;
				((float *)(b + f->ofs))[2] = 0;
				break;
			default:
			case F_IGNORE: break;
			}
			return;
		}
	}
}

/*
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnVars[], then call the class specfic spawn function
===================
*/
static void G_SpawnGEntityFromSpawnVars(void) {
	int i;
	gentity_t *ent;
	char *s, *value, *gametypeName;
	static char *gametypeNames[] = {"sandbox", "ffa", "team"};

	// get the next free entity
	ent = G_Spawn();

	for(i = 0; i < level.numSpawnVars; i++) G_ParseField(level.spawnVars[i][0], level.spawnVars[i][1], ent);

	if(cvarInt("g_gametype") >= GT_TEAM) {
		G_SpawnInt("notteam", "0", &i);
		if(i) {
			G_FreeEntity(ent);
			return;
		}
	} else {
		G_SpawnInt("notfree", "0", &i);
		if(i) {
			G_FreeEntity(ent);
			return;
		}
	}

	G_SpawnInt("notta", "0", &i);
	if(i) {
		G_FreeEntity(ent);
		return;
	}

	if(G_SpawnString("!gametype", NULL, &value)) {
		if(cvarInt("g_gametype") >= GT_SANDBOX && cvarInt("g_gametype") < GT_MAX_GAME_TYPE) {
			gametypeName = gametypeNames[cvarInt("g_gametype")];

			s = strstr(value, gametypeName);
			if(s) {
				G_FreeEntity(ent);
				return;
			}
		}
	}

	if(G_SpawnString("gametype", NULL, &value)) {
		if(cvarInt("g_gametype") >= GT_SANDBOX && cvarInt("g_gametype") < GT_MAX_GAME_TYPE) {
			gametypeName = gametypeNames[cvarInt("g_gametype")];

			s = strstr(value, gametypeName);
			if(!s) {
				G_FreeEntity(ent);
				return;
			}
		}
	}

	// move editor origin to pos
	VectorCopy(ent->s.origin, ent->s.pos.trBase);
	VectorCopy(ent->s.origin, ent->r.currentOrigin);

	// if we didn't get a classname, don't bother spawning anything
	if(!G_CallSpawn(ent)) G_FreeEntity(ent);
}

static char *G_AddSpawnVarToken(const char *string) {
	int l;
	char *dest;

	l = strlen(string);
	if(level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS) {
		G_Error("G_AddSpawnVarToken: MAX_SPAWN_VARS");
	}

	dest = level.spawnVarChars + level.numSpawnVarChars;
	memcpy(dest, string, l + 1);

	level.numSpawnVarChars += l + 1;

	return dest;
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnVars[]

This does not actually spawn an entity.
====================
*/
static qboolean G_ParseSpawnVars(void) {
	char keyname[MAX_TOKEN_CHARS];
	char com_token[MAX_TOKEN_CHARS];

	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	// parse the opening brace
	if(!trap_GetEntityToken(com_token, sizeof(com_token))) {
		// end of spawn string
		return qfalse;
	}
	if(com_token[0] != '{') {
		G_Error("G_ParseSpawnVars: found %s when expecting {", com_token);
	}

	// go through all the key / value pairs
	while(1) {
		// parse key
		if(!trap_GetEntityToken(keyname, sizeof(keyname))) {
			G_Error("G_ParseSpawnVars: EOF without closing brace");
		}

		if(keyname[0] == '}') {
			break;
		}

		// parse value
		if(!trap_GetEntityToken(com_token, sizeof(com_token))) {
			G_Error("G_ParseSpawnVars: EOF without closing brace");
		}

		if(com_token[0] == '}') {
			G_Error("G_ParseSpawnVars: closing brace without data");
		}
		if(level.numSpawnVars == MAX_SPAWN_VARS) {
			G_Error("G_ParseSpawnVars: MAX_SPAWN_VARS");
		}
		level.spawnVars[level.numSpawnVars][0] = G_AddSpawnVarToken(keyname);
		level.spawnVars[level.numSpawnVars][1] = G_AddSpawnVarToken(com_token);
		level.numSpawnVars++;
	}

	return qtrue;
}

static void SP_worldspawn(void) {
	char *s;

	G_SpawnString("classname", "", &s);
	if(Q_stricmp(s, "worldspawn")) {
		G_Error("SP_worldspawn: The first entity isn't 'worldspawn'");
	}

	// make some data visible to connecting client
	trap_SetConfigstring(CS_LEVEL_START_TIME, va("%i", level.startTime));

	G_SpawnString("music", "", &s);
	trap_SetConfigstring(CS_MUSIC, s);

	G_SpawnString("gravity", "800", &s);
	cvarSet("g_gravity", s);

	G_SpawnString("enableDust", "0", &s);
	cvarSet("g_enableDust", s);

	G_SpawnString("enableSnow", "0", &s);
	cvarSet("g_enableSnow", s);

	G_SpawnString("enableBreath", "0", &s);
	cvarSet("g_enableBreath", s);

	g_entities[ENTITYNUM_WORLD].s.number = ENTITYNUM_WORLD;
	g_entities[ENTITYNUM_WORLD].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_WORLD].classname = "worldspawn";

	g_entities[ENTITYNUM_NONE].s.number = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_NONE].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_NONE].classname = "nothing";
}

/*
==============
G_SpawnEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void G_SpawnEntitiesFromString(void) {
	// allow calls to G_Spawn*()
	level.spawning = qtrue;
	level.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if(!G_ParseSpawnVars()) {
		G_Error("SpawnEntities: no entities");
	}
	SP_worldspawn();

	// parse ents
	while(G_ParseSpawnVars()) {
		G_SpawnGEntityFromSpawnVars();
	}

	level.spawning = qfalse; // any future calls to G_Spawn*() will be errors
}

#define MAX_TOKENNUM 1000000
#define MAX_MAPFILE_LENGTH (4 * 1024 * 1024) // 1MB
static char mapbuffer[MAX_MAPFILE_LENGTH];

typedef enum { TOT_LPAREN, TOT_RPAREN, TOT_WORD, TOT_NUMBER, TOT_NIL, TOT_MAX } tokenType_t;

#define TOKENVALUE_SIZE 128

typedef struct {
	char value[TOKENVALUE_SIZE];
	int type;
} token_t;

token_t tokens2[MAX_TOKENNUM];

static void G_RelinkEntities(void) {
	int i, j;
	int entNum, newEntNum;

	for(i = 0; i < MAX_GENTITIES; i++) {
		if(g_entities[i].phys_parent) {
			entNum = g_entities[i].phys_parent;
			newEntNum = g_entities[i].s.number;
			for(j = 0; j < MAX_GENTITIES; j++) {
				if(g_entities[j].phys_welded == entNum) {
					g_entities[j].physParentEnt = G_FindEntityForEntityNum(newEntNum);
					g_entities[i].phys_weldedObjectsNum += 1;
				}
			}
		}
	}
}

static qboolean SkippedChar(char in) { return (in == '\n' || in == '\r' || in == ';' || in == '\t' || in == ' '); }

static qboolean G_ClassnameAllowed(char *input) {
	int i;
	char *classes_allowed[] = {"sandbox_prop", "sandbox_npc", 0};

	for(i = 0; classes_allowed[i] != 0; i++) { // Allowed classlist
		if(!strcmp(input, classes_allowed[i])) {
			return qtrue;
		}
	}

	return BG_CheckClassname(input); // Items
}

static void G_ClearEntities(void) {
	int i;
	for(i = 0; i < MAX_CLIENTS; i++) { // NPCs
		if(g_entities[i].npcType > NT_PLAYER) {
			DropClientSilently(g_entities[i].client->ps.clientNum);
		}
	}
	for(i = 0; i < MAX_GENTITIES; i++) { // Items and Other
		if(!G_ClassnameAllowed(g_entities[i].classname)) continue;
		g_entities[i].nextthink = 0;
		G_FreeEntity(&g_entities[i]);
	}
}

static int G_SetTokens2(char *in, char *out, int start) {
	int i = 0;
	while(in[start + i] != ' ') {
		if(in[start + i] == '\0') {
			out[i] = in[start + 1];
			return MAX_MAPFILE_LENGTH;
		}
		out[i] = in[start + i];
		i++;
	}
	out[i] = '\0';
	return start + i + 1;
}

static int G_SetTokenType2(char *value) {
	int count = 0;
	qboolean lpar = qfalse, rpar = qfalse, number = qfalse, character = qfalse;

	while(value[count] != '\0') {
		if(value[count] == '{')
			lpar = qtrue;
		else if(value[count] == '}')
			rpar = qtrue;
		else if(value[count] >= '0' && value[count] <= '9')
			number = qtrue;
		else if((value[count] >= 'a' && value[count] <= 'z') || (value[count] >= 'A' && value[count] <= 'Z'))
			character = qtrue;
		count++;
	}

	if(lpar && !(rpar || number || character))
		return TOT_LPAREN;
	else if(rpar && !(lpar || number || character))
		return TOT_RPAREN;
	else if(number && !(lpar || rpar || character))
		return TOT_NUMBER;
	else if(character && !(lpar || rpar))
		return TOT_WORD;
	else
		return TOT_NIL;
}

static int G_FindNextToken2(char *find, token_t *in, int start) {
	int i;
	int cmp;

	for(i = start; i < MAX_TOKENNUM; i++) {
		cmp = strcmp(in[i].value, find);
		if(cmp == 0) return i;
	}
	return -1;
}

static qboolean G_AbeforeB2(char *A, char *B, token_t *in, int start) {
	int a = G_FindNextToken2(A, in, start);
	int b = G_FindNextToken2(B, in, start);

	if(b == -1 && a != -1) return qtrue;
	if(a == -1 && b != -1) return qfalse;
	if(a < b)
		return qtrue;
	else
		return qfalse;
}

static char *G_ClearString(char *input) {
	if(input[0] == '"') {
		input[0] = '\0';
		input++;
	}
	if(input[strlen(input) - 1] == '"') {
		input[strlen(input) - 1] = '\0';
	}
	return input;
}

static void G_LoadMapfileEntity(token_t *in, int min, int max) {
	int i;
	char *buf;
	vec3_t vec;

	field_t *field;
	byte *b;

	gentity_t *ent;
	ent = G_Spawn();

	b = (byte *)ent;

	for(i = min; i <= max; i++) {
		for(field = gameInfoFields; field->name; field++) {
			if(!strcmp(va("\"%s\"", field->name), in[i].value)) {
				if(!strcmp(field->name, "team") || !strcmp(field->name, "angle")) break;
				switch(field->type) {
				case F_STRING: *(char **)(b + field->ofs) = G_NewString(G_ClearString(in[i + 1].value)); break;
				case F_VECTOR:
					buf = in[i + 1].value;
					strcat(buf, " ");
					strcat(buf, in[i + 2].value);
					strcat(buf, " ");
					strcat(buf, in[i + 3].value);
					sscanf(G_ClearString(buf), "%f %f %f", &vec[0], &vec[1], &vec[2]);
					((float *)(b + field->ofs))[0] = vec[0];
					((float *)(b + field->ofs))[1] = vec[1];
					((float *)(b + field->ofs))[2] = vec[2];
					break;
				case F_INT: *(int *)(b + field->ofs) = atoi(G_ClearString(in[i + 1].value)); break;
				case F_FLOAT: *(float *)(b + field->ofs) = atof(G_ClearString(in[i + 1].value)); break;
				default:
				case F_IGNORE: break;
				}
				break;
			}
		}
	}

	if(!G_CallSpawn(ent)) {
		G_FreeEntity(ent);
	}
}

static void G_LoadMapfile(char *filename) {
	qboolean lastSpace = qtrue;
	qboolean pgbreak = qfalse;
	int i = 0;
	int charCount = 0;
	int tokenNum = 0;
	int maxTokennum;
	int lpar, rpar;
	int len;
	fileHandle_t f;

	len = FS_Open(filename, &f, FS_READ);

	if(!f) {
		G_Printf("%s", va(S_COLOR_YELLOW "mapfile not found: %s\n", filename));
		return;
	}

	if(len >= 2500000 * 6) {
		trap_Error(va(S_COLOR_RED "map file too large: %s is %i, max allowed is %i", filename, len, 2500000 * 6));
		FS_Close(f);
		return;
	}

	FS_Read(mapbuffer, len, f);
	if(len <= 10) return;
	mapbuffer[len] = 0;
	FS_Close(f);

	G_ClearEntities();

	COM_Compress(mapbuffer);

	for(i = 0; i < MAX_MAPFILE_LENGTH; i++) {
		// Filter comments( start at # and end at break )
		if(mapbuffer[i] == '#') {
			while(i < MAX_MAPFILE_LENGTH && !pgbreak) {
				if(mapbuffer[i] == '\n' || mapbuffer[i] == '\r') pgbreak = qtrue;
				i++;
			}
			pgbreak = qfalse;
			lastSpace = qtrue;
		}

		if(SkippedChar(mapbuffer[i])) {
			if(!lastSpace) {
				mapbuffer[charCount] = ' ';
				charCount++;
				lastSpace = qtrue;
			}
			continue;
		}

		lastSpace = qfalse;
		mapbuffer[charCount] = mapbuffer[i];
		charCount++;
	}

	i = 0;
	while(i < MAX_MAPFILE_LENGTH && tokenNum < MAX_TOKENNUM) {
		i = G_SetTokens2(mapbuffer, tokens2[tokenNum].value, i);
		tokens2[tokenNum].type = G_SetTokenType2(tokens2[tokenNum].value);
		tokenNum++;
	}
	maxTokennum = tokenNum;

	G_Printf("Mapfile parser found %i tokens\n", maxTokennum);

	for(tokenNum = 0; tokenNum < maxTokennum; tokenNum++) {
		if(strcmp(tokens2[tokenNum].value, "{") == 0) {
			lpar = tokenNum;
			if(G_AbeforeB2((char *)"{", (char *)"}", tokens2, tokenNum + 2)) {
				G_Printf("error: \"}\" expected at %s\n", tokens2[tokenNum].value);
				break;
			}
			rpar = G_FindNextToken2((char *)"}", tokens2, tokenNum + 2);
			if(rpar != -1) {
				G_LoadMapfileEntity(tokens2, lpar + 1, rpar - 1);
				tokenNum = rpar;
			}
		}
	}
}

void G_LoadMapfile_f(void) {
	char filename[MAX_QPATH];

	if(trap_Argc() < 2) {
		G_Printf("Usage: loadmap <filename>\n");
		return;
	}

	trap_Argv(1, filename, sizeof(filename));

	G_LoadMapfile(filename);
	cvarSet("mapfile", filename);
	cvarSet("lastmap", cvarString("sv_mapname"));

	G_RelinkEntities();

	trap_SendServerCommand(-1, "print \"^2Map loaded!\n\"");
}

void G_WriteMapfile_f(void) {
	int i;
	fileHandle_t f;
	char *string;
	char filename[MAX_QPATH];
	field_t *field;
	byte *b;

	if(trap_Argc() < 2) {
		G_Printf("Usage: savemap <filename>\n");
		return;
	}

	trap_Argv(1, filename, sizeof(filename));

	FS_Open(va("%s", filename), &f, FS_WRITE);

	string = va("//OpenSandbox Map File\n");
	FS_Write(string, strlen(string), f);

	for(i = 0; i < MAX_GENTITIES; i++) {
		if(!g_entities[i].inuse) continue;

		if(!G_ClassnameAllowed(g_entities[i].classname)) continue;
		if(g_entities[i].flags & FL_DROPPED_ITEM) continue;

		b = (byte *)&g_entities[i];

		string = va("{\n");
		FS_Write(string, strlen(string), f);

		for(field = gameInfoFields; field->name; field++) {
			switch(field->type) {
			case F_STRING:
				if(*(char **)(b + field->ofs)) {
					string = va("   \"%s\"   \"%s\"\n", field->name, *(char **)(b + field->ofs));
					FS_Write(string, strlen(string), f);
				}
				break;
			case F_VECTOR:
				if((((float *)(b + field->ofs))[0] || ((float *)(b + field->ofs))[1] || ((float *)(b + field->ofs))[2])) {
					string = va("   \"%s\"   \"%f %f %f\"\n", field->name, ((float *)(b + field->ofs))[0], ((float *)(b + field->ofs))[1], ((float *)(b + field->ofs))[2]);
					FS_Write(string, strlen(string), f);
				}
				break;
			case F_INT:
				if(*(int *)(b + field->ofs)) {
					string = va("   \"%s\"   \"%i\"\n", field->name, *(int *)(b + field->ofs));
					FS_Write(string, strlen(string), f);
				}
				break;
			case F_FLOAT:
				if(*(float *)(b + field->ofs)) {
					string = va("   \"%s\"   \"%f\"\n", field->name, *(float *)(b + field->ofs));
					FS_Write(string, strlen(string), f);
				}
				break;
			default:
			case F_IGNORE: break;
			}
		}
		string = va("}\n\n");
		FS_Write(string, strlen(string), f);
	}
	FS_Close(f);
	trap_SendServerCommand(-1, "print \"^2Map saved!\n\"");
}

void G_DeleteMapfile_f(void) {
	fileHandle_t f;
	char filename[MAX_QPATH];
	char *string;

	if(trap_Argc() < 2) {
		G_Printf("Usage: deletemap <filename>\n");
		return;
	}

	trap_Argv(1, filename, sizeof(filename));

	FS_Open(va("%s", filename), &f, FS_WRITE);

	string = va("deleted");
	FS_Write(string, strlen(string), f);

	FS_Close(f);
	trap_SendServerCommand(-1, "print \"^2Map deleted!\n\"");
}

void G_ClearMap_f(void) {
	int i;
	for(i = 0; i < MAX_CLIENTS; i++) { // NPCs
		if(g_entities[i].npcType > NT_PLAYER) {
			DropClientSilently(g_entities[i].client->ps.clientNum);
		}
	}
	for(i = 0; i < MAX_GENTITIES; i++) { // Sandbox objects
		if(g_entities[i].sandboxObject) {
			g_entities[i].nextthink = 0;
			G_FreeEntity(&g_entities[i]);
		}
	}
	trap_SendServerCommand(-1, "print \"^2Map cleaned!\n\"");
}
