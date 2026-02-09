// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

extern int SourceTechEntityList[MAX_GENTITIES];

#define BODY_QUEUE_SIZE 8
#define FRAMETIME 100 // msec
#define INTERMISSION_DELAY_TIME 1000

// gentity->flags
#define FL_GODMODE 1
#define FL_TEAMSLAVE 2
#define FL_NO_KNOCKBACK 4
#define FL_DROPPED_ITEM 8
#define FL_NO_BOTS 16
#define FL_NO_HUMANS 32

// movers are things like doors, plats, buttons, etc
typedef enum {
	MOVER_POS1,
	MOVER_POS2,
	MOVER_1TO2,
	MOVER_2TO1,

	ROTATOR_POS1,
	ROTATOR_POS2,
	ROTATOR_1TO2,
	ROTATOR_2TO1
} moverState_t;

#define MAX_NETNAME 36
#define MAX_UNDO_STACK 128

typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;

struct gentity_s {
	entityState_t s;  // communicated by server to clients
	entityShared_t r; // shared by both the server system and game

	struct gclient_s *client; // NULL if not a client

	qboolean inuse;

	char *classname; // set in QuakeEd
	int spawnflags;  // set in QuakeEd

	qboolean neverFree; // if true, FreeEntity will only unlink
	                    // bodyque uses this

	int flags; // FL_* variables

	char *model;
	char *model2;
	int freetime; // level.time when the object was freed

	int eventTime; // events will be cleared EVENT_VALID_MSEC after set
	qboolean freeAfterEvent;
	qboolean unlinkAfterEvent;

	qboolean physicsObject; // if true, it can be pushed by movers and fall off edges
	                        // all game items are physicsObjects,
	int clipmask;           // brushes with this content value will be collided against
	                        // when moving.  items and corpses do not collide against
	                        // players, for instance

	// movers
	moverState_t moverState;
	int soundPos1;
	int sound1to2;
	int sound2to1;
	int soundPos2;
	int soundLoop;
	gentity_t *parent;
	gentity_t *nextTrain;
	gentity_t *prevTrain;
	vec3_t pos1, pos2;

	char *message;

	int timestamp; // body queue sinking, etc

	float angle; // set in editor, -1 = up, -2 = down
	char *target;
	char *targetname;
	char *team;
	gentity_t *target_ent;

	float speed;
	vec3_t movedir;

	int nextthink;
	void (*think)(gentity_t *self);
	void (*reached)(gentity_t *self); // movers call this when hitting endpoint
	void (*blocked)(gentity_t *self, gentity_t *other);
	void (*touch)(gentity_t *self, gentity_t *other, trace_t *trace);
	void (*use)(gentity_t *self, gentity_t *other, gentity_t *activator);
	void (*pain)(gentity_t *self, gentity_t *attacker, int damage);
	void (*die)(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);

	int pain_debounce_time;
	int fly_sound_debounce_time; // wind tunnel
	int last_move_time;

	int health;

	qboolean takedamage;

	int damage;
	int splashDamage; // quad will increase this without increasing radius
	int splashRadius;
	int methodOfDeath;

	int count;

	gentity_t *chain;
	gentity_t *enemy;
	gentity_t *activator;
	gentity_t *teamchain;  // next entity in team
	gentity_t *teammaster; // master of the team

	int watertype;
	int waterlevel;

	int noise_index;

	// timing variables
	float wait;
	float random;

	item_t *item; // for bonus items

	// flashlight
	qboolean flashlight;

	// object private system
	gentity_t *owner;

	// sandbox
	qboolean sandboxObject;

	float sb_coltype;
	float sb_gravity;
	char *sb_class;
	char *sb_sound;
	int sb_material;
	int sb_phys;
	int sb_coll;
	int sb_isnpc;
	int sb_red;
	int sb_green;
	int sb_blue;
	int sb_radius;

	int npcType;

	// toolgun
	int tool_id;
	gentity_t *tool_entity;

	int skill;

	int type;
	int objectType;

	// opensandbox variables
	gentity_t *botspawn;
	char *clientname; // name of the bot to spawn for target_botspawn
	char *key;        // key for target_modify to change
	char *value;      // value for target_modify to change to
	int weapon;

	int swep_list[WEAPONS_NUM];
	int swep_ammo[WEAPONS_NUM];

	// Physgun and Gravitygun
	gentity_t *grabbedEntity; // physgun object for player
	qboolean isGrabbed;       // object is grabbed by player for prop
	float grabDist;           // physgun distance for player
	vec3_t grabOffset;        // physgun offset for player
	vec3_t grabOldOrigin;     // physgun old origin for prop
	int grabNewPhys;          // for freeze prop for prop

	// Props
	gentity_t *lastPlayer; // for damage and killfeed

	// Phys state (Trace)
	qboolean phys_inAir;
	qboolean phys_inSolid;

	// Phys state (Point)
	qboolean phys_inWater;

	// Phys settings
	int phys_weldedObjectsNum;
	gentity_t *physParentEnt;

	// Saved info (save)
	vec3_t phys_relativeOrigin;

	// Rotate vectors (save)
	vec3_t phys_rv_0;
	vec3_t phys_rv_1;
	vec3_t phys_rv_2;

	// Phys
	int phys_nextthink;
	void (*phys_think)(gentity_t *self);
	float phys_bounce;

	// Save weld (sync)
	int phys_welded; // welded
	int phys_parent; // master
};

typedef enum { CON_DISCONNECTED, CON_CONNECTING, CON_CONNECTED } clientConnected_t;

typedef enum { SPECTATOR_NOT, SPECTATOR_FREE, SPECTATOR_FOLLOW, SPECTATOR_SCOREBOARD } spectatorState_t;

typedef enum {
	TEAM_BEGIN, // Beginning a team game, spawn at base
	TEAM_ACTIVE // Now actively playing
} playerTeamStateState_t;

typedef struct {
	playerTeamStateState_t state;

	float lastreturnedflag;
	float lastfraggedcarrier;
} playerTeamState_t;

// client data that stays across multiple levels or tournament restarts
// this is achieved by writing all the data to cvar strings at game shutdown
// time and reading them back at connection time.  Anything added here
// MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
typedef struct {
	team_t sessionTeam;
	int spectatorNum; // for determining next-in-line to play
	spectatorState_t spectatorState;
	int spectatorClient; // for chasecam and follow mode
} clientSession_t;

typedef struct {
	int id;
	qboolean isRemoved;
} undo_stack_t;

// client data that stays across multiple respawns, but is cleared
// on each level change or team change at ClientBegin()
typedef struct {
	clientConnected_t connected;
	usercmd_t cmd; // we would lose angles if not persistant
	char netname[MAX_NETNAME];
	int maxHealth;
	playerTeamState_t teamState; // status in teamplay games
	undo_stack_t undoStack[MAX_UNDO_STACK];
} clientPersistant_t;

// this structure is cleared on each ClientSpawn(),
// except for 'client->pers' and 'client->sess'
struct gclient_s {
	// ps MUST be the first element, because the server expects it
	playerState_t ps; // communicated by server to clients

	// the rest of the structure is private to game
	clientPersistant_t pers;
	clientSession_t sess;

	qboolean readyToExit; // wishes to leave the intermission
	qboolean noclip;

	int lastCmdTime; // level.time of last usercmd_t, for EF_CONNECTION
	                 // we can't just use pers.lastCommand.time, because
	                 // of the g_sycronousclients case
	int buttons;
	int oldbuttons;
	int latched_buttons;

	vec3_t oldOrigin;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int damage_armor;          // damage absorbed by armor
	int damage_blood;          // damage taken out of health
	int damage_knockback;      // impact damage
	vec3_t damage_from;        // origin for vector calculation
	qboolean damage_fromWorld; // if true, don't use the damage_from vector

	int lasthurt_client; // last client that damaged this client
	int lasthurt_mod;    // type of damage the client did

	// timers
	int respawnTime; // can respawn when time > this, force after g_forcerespwan

	int airOutTime;

	qboolean fireHeld;     // used for hook
	gentity_t *hook;       // grapple hook if out
	gentity_t *lasersight; // flashlight

	// timeResidual is used to handle events that happen every second
	// like health / armor countdowns and regeneration
	int timeResidual;

	int timeEntityInfo;

	gentity_t *persistantPowerup;
	int portalID;
	int invulnerabilityTime;

	char *areabits;

	// the serverTime the button was pressed
	int attackTime;

	int vehicleNum;

	// the last frame number we got an update from this client
	int lastUpdateFrame;
	qboolean spawnprotected;
};

//
// this structure is cleared as each map is entered
//
#define MAX_SPAWN_VARS 64
#define MAX_SPAWN_VARS_CHARS 4096

typedef struct {
	struct gclient_s *clients; // [maxclients]

	struct gentity_s *gentities;
	int gentitySize;
	int num_entities; // current number, <= MAX_GENTITIES

	// store latched cvars here that we want to get at often
	int maxclients;

	int framenum;
	int time;         // in msec
	int previousTime; // so movers can back up when blocked

	int startTime; // level.time the map was started

	int teamScores[TEAM_NUM_TEAMS];
	int lastTeamLocationTime; // last time of client team location update

	qboolean newSession; // don't use any old session data, because
	                     // we changed gametype

	qboolean restarted; // waiting for a map_restart to fire

	int numConnectedClients;
	int numNonSpectatorClients;     // includes connecting clients
	int numPlayingClients;          // connected, non-spectators
	int sortedClients[MAX_CLIENTS]; // sorted by score
	int follow1, follow2;           // clientNums for auto-follow spectators

	int snd_fry; // sound index for standing in lava

	// spawn variables
	qboolean spawning; // the G_Spawn*() functions are valid
	int numSpawnVars;
	char *spawnVars[MAX_SPAWN_VARS][2]; // key / value pairs
	int numSpawnVarChars;
	char spawnVarChars[MAX_SPAWN_VARS_CHARS];

	// intermission state
	int intermissionQueued; // intermission was qualified, but
	                        // wait INTERMISSION_DELAY_TIME before
	                        // actually going there so the last
	                        // frag can be watched.  Disable future
	                        // kills during this delay
	int intermissiontime;   // time the intermission was started
	qboolean readyToExit;   // at least one client wants to exit
	int exitTime;
	vec3_t intermission_origin; // also used for spectator spawns
	vec3_t intermission_angle;

	qboolean locationLinked; // target_locations get linked
	gentity_t *locationHead; // head of the location list
	int bodyQueIndex;        // dead bodies
	gentity_t *bodyQue[BODY_QUEUE_SIZE];
	int portalSequence;

	int frameStartTime;
} level_locals_t;

typedef struct {
	char *name;
	void (*spawn)(gentity_t *ent);
} spawn_t;

// damage flags
#define DAMAGE_RADIUS 0x00000001             // damage was indirect
#define DAMAGE_NO_ARMOR 0x00000002           // armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK 0x00000004       // do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION 0x00000008      // armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_NO_TEAM_PROTECTION 0x00000010 // armor, shields, invulnerability, and godmode have no effect

// bot settings
#define MAX_FILEPATH 144
typedef struct bot_settings_s {
	char team[MAX_FILEPATH];
} bot_settings_t;

// ai_main.c
int BotAISetup(int restart);
int BotAIShutdown(int restart);
int BotAILoadMap(int restart);
int BotAISetupClient(int client, struct bot_settings_s *settings);
int BotAIShutdownClient(int client, qboolean restart);
int AI_Frame(int time);

// g_active.c
void ClientThink(int clientNum);
void G_RunClient(gentity_t *ent);
int G_CheckWeaponAmmo(int clientNum, int wp);
void PM_Add_SwepAmmo(int clientNum, int wp, int count);
void ClientEndFrame(gentity_t *ent);

// g_bot.c
qboolean G_BotConnect(int clientNum);
void G_AddBot(char *model, char *name, char *team, gentity_t *spawn);
void Svcmd_AddBot_f(void);
void SandboxBotSpawn(gentity_t *bot, char spawnid[]);

// g_client.c
void SP_info_player_deathmatch(gentity_t *ent);
void InitBodyQue(void);
void CopyToBodyQue(gentity_t *ent);
void SetClientViewAngle(gentity_t *ent, vec3_t angle);
void ClientRespawn(gentity_t *ent);
team_t PickTeam(int ignoreClientNum);
void ClientUserinfoChanged(int clientNum);
char *ClientConnect(int clientNum, qboolean firstTime, qboolean isBot);
void ClientBegin(int clientNum);
void ClientSpawn(gentity_t *ent);
void ClientDisconnect(int clientNum);
void DropClientSilently(int clientNum);
void SetUnlimitedWeapons(gentity_t *ent);
qboolean OnSameTeam(gentity_t *ent1, gentity_t *ent2);

// g_cmds.c
void DeathmatchScoreboardMessage(gentity_t *ent);
void G_SendGameCvars(gentity_t *ent);
void G_SendSwepWeapons(gentity_t *ent);
void G_SendSpawnSwepWeapons(gentity_t *ent);
void RespawnTimeMessage(gentity_t *ent, int time);
void Cmd_Score_f(gentity_t *ent);
char *ConcatArgs(int start);
void SetTeam(gentity_t *ent, char *s);
void StopFollowing(gentity_t *ent);
void Cmd_FollowCycle_f(gentity_t *ent, int dir);
void ClientCommand(int clientNum);

// g_combat.c
void AddScore(gentity_t *ent, int score);
void TossClientItems(gentity_t *self);
void TossClientCubes(gentity_t *self);
void TossClientPersistantPowerups(gentity_t *ent);
void body_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath);
void player_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath);
void G_Damage(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod);
qboolean CanDamage(gentity_t *targ, vec3_t origin);
void G_RadiusDamage(vec3_t origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod);

// g_items.c
void Set_Weapon(gentity_t *ent, int weapon, int status);
void Set_Ammo(gentity_t *ent, int weapon, int count);
void RespawnItem(gentity_t *ent);
void Touch_Item(gentity_t *ent, gentity_t *other, trace_t *trace);
gentity_t *LaunchItem(item_t *item, vec3_t origin, vec3_t velocity);
gentity_t *Drop_Item(gentity_t *ent, item_t *item);
void FinishSpawningItem(gentity_t *ent);
void G_SpawnItem(gentity_t *ent, item_t *item);
void G_RunItem(gentity_t *ent);

// g_main.c
void CalculateRanks(void);
void MoveClientToIntermission(gentity_t *ent);
void FindIntermissionPoint(void);
void SetLeader(int team, int client);
void G_RunThink(gentity_t *ent);

// g_misc.c
void TeleportPlayer(gentity_t *player, vec3_t origin, vec3_t angles, qboolean noKnockback);

// g_physics.c
void Phys_VehiclePlayer(gentity_t *self);
void Phys_CheckCarCollisions(gentity_t *ent);
void Phys_HoldDropStatic(gentity_t *player, vec3_t velocity);
void Phys_HoldDropDynamic(gentity_t *player, vec3_t velocity, qboolean isPhysgun);
void Phys_HoldSetup(gentity_t *player, qboolean isPhysgun);
void Phys_HoldFrame(gentity_t *player, vec3_t velocity, qboolean isPhysgun);
void Phys_Disable(gentity_t *ent, vec3_t origin);
void Phys_Enable(gentity_t *ent);
void Phys_Unweld(gentity_t *ent);
void Phys_Frame(gentity_t *ent);

// g_sandbox.c
void G_DieProp(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);
void SP_sandbox_npc(gentity_t *ent);
void SP_sandbox_prop(gentity_t *ent);
void G_BuildProp(char *arg02, char *arg03, vec3_t xyz, gentity_t *player, char *arg04, char *arg05, char *arg06, char *arg07, char *arg08, char *arg09, char *arg10, char *arg11, char *arg12, char *arg13, char *arg14, char *arg15, char *arg16, char *arg17, char *arg18, char *arg19, char *arg20, char *arg21, char *arg22);
void G_ModProp(gentity_t *targ, gentity_t *attacker, char *arg01, char *arg02, char *arg03, char *arg04, char *arg05);
void Undo_AddElement(gentity_t *ent, int id);
qboolean Undo_LastElement(gentity_t *ent, int *id, qboolean *isRemoved);
void Undo_RemoveElement(gentity_t *ent);

// g_session.c
void G_ReadSessionData(gclient_t *client);
void G_InitSessionData(gclient_t *client, char *userinfo);
void G_InitWorldSession(void);
void G_WriteSessionData(void);

// g_spawn.c
extern spawn_t gameInfoEntities[];
qboolean G_SpawnString(const char *key, const char *defaultString, char **out);
qboolean G_SpawnFloat(const char *key, const char *defaultString, float *out);
qboolean G_SpawnInt(const char *key, const char *defaultString, int *out);
qboolean G_SpawnVector(const char *key, const char *defaultString, float *out);
void G_SpawnEntitiesFromString(void);
void G_LoadMapfile_f(void);
void G_WriteMapfile_f(void);
void G_DeleteMapfile_f(void);
void G_ClearMap_f(void);

// g_svcmds.c
qboolean ConsoleCommand(void);

// g_utils.c
int G_ModelIndex(char *name);
int G_SoundIndex(char *name);
void G_TeamCommand(team_t team, char *cmd);
gentity_t *G_Find(gentity_t *from, int fieldofs, const char *match);
gentity_t *G_PickTarget(char *targetname);
void G_PickAllTargets(gentity_t *ent);
void G_UseTargets(gentity_t *ent, gentity_t *activator);
char *vtos(const vec3_t v);
void G_SetMovedir(vec3_t angles, vec3_t movedir);
void G_InitGentity(gentity_t *e);
gentity_t *G_Spawn(void);
void G_FreeEntity(gentity_t *ed);
gentity_t *G_TempEntity(vec3_t origin, int event);
void G_KillBox(gentity_t *ent);
void G_AddPredictableEvent(gentity_t *ent, int event, int eventParm);
void G_AddEvent(gentity_t *ent, int event, int eventParm);
void G_Sound(gentity_t *ent, int channel, int soundIndex);
void G_SetOrigin(gentity_t *ent, vec3_t origin);
gentity_t *FindEntityForPhysgun(gentity_t *ent, int range);
gentity_t *FindEntityForGravitygun(gentity_t *ent, int range);
void CrosshairPointPhys(gentity_t *ent, int range, vec3_t outPoint);
void CrosshairPointGravity(gentity_t *ent, int range, vec3_t outPoint);
gentity_t *G_FindEntityForEntityNum(int entityNum);
qboolean G_PlayerIsOwner(gentity_t *player, gentity_t *ent);
gentity_t *G_FindWeldEntity(gentity_t *ent);
gentity_t *FindRandomItem(void);
gentity_t *FindRandomSpawn(void);
gentity_t *FindRandomTeamSpawn(team_t team);

// g_weapon.c
void CalcMuzzlePoint(gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint);
void G_ExplodeMissile(gentity_t *ent);
qboolean Melee_Fire(gentity_t *ent, int weapon);
void Weapon_Toolgun_Info(gentity_t *ent);
void ProximityMine_Trigger(gentity_t *trigger, gentity_t *other, trace_t *trace);
void G_RunMissile(gentity_t *ent);
gentity_t *fire_missile(gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up, int weapon);
void FireWeapon(gentity_t *ent);

extern level_locals_t level;
extern gentity_t g_entities[MAX_GENTITIES];

#define FOFS(x) ((size_t)&(((gentity_t *)0)->x))

// CVARS
extern int mod_jumpheight;
extern int mod_gravity;

#define CMD_CHEAT 0x0001
#define CMD_CHEAT_TEAM 0x0002 // is a cheat when used on a team
#define CMD_MESSAGE 0x0004    // sends message to others (skip when muted)
#define CMD_TEAM 0x0008       // must be on a team
#define CMD_NOTEAM 0x0010     // must not be on a team
#define CMD_LIVING 0x0020
#define CMD_INTERMISSION 0x0040 // valid during intermission

typedef struct {
	char *cmdName;
	int cmdFlags;
	void (*cmdHandler)(gentity_t *ent);
} commands_t;

// SYSCALLS
void trap_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *gameClients, int sizeofGameClient);
void trap_DropClient(int clientNum, const char *reason);
void trap_SendServerCommand(int clientNum, const char *text);
void trap_SetConfigstring(int num, const char *string);
void trap_GetConfigstring(int num, char *buffer, int bufferSize);
void trap_GetUserinfo(int num, char *buffer, int bufferSize);
void trap_SetUserinfo(int num, const char *buffer);
void trap_GetServerinfo(char *buffer, int bufferSize);
void trap_SetBrushModel(gentity_t *ent, const char *name);
void trap_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
int trap_PointContents(const vec3_t point, int passEntityNum);
qboolean trap_InPVS(const vec3_t p1, const vec3_t p2);
void trap_AdjustAreaPortalState(gentity_t *ent, qboolean open);
void trap_LinkEntity(gentity_t *ent);
void trap_UnlinkEntity(gentity_t *ent);
int trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount);
qboolean trap_EntityContact(const vec3_t mins, const vec3_t maxs, const gentity_t *ent);
int trap_BotAllocateClient(void);
void trap_GetUsercmd(int clientNum, usercmd_t *cmd);
qboolean trap_GetEntityToken(char *buffer, int bufferSize);

int trap_BotLibSetup(void);
int trap_BotLibShutdown(void);
int trap_BotLibStartFrame(float time);
int trap_BotLibLoadMap(const char *mapname);
int trap_BotGetServerCommand(int clientNum, char *message, int size);
void trap_BotUserCommand(int client, usercmd_t *ucmd);
int trap_BotUpdateEntity(int ent, void *bue);
int trap_AAS_Initialized(void);
float trap_AAS_Time(void);
int trap_AAS_PointAreaNum(vec3_t point);
int trap_AAS_TraceAreas(vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas);
void trap_EA_Command(int client, char *command);
void trap_EA_Gesture(int client);
void trap_EA_Attack(int client);
void trap_EA_Use(int client);
void trap_EA_View(int client, vec3_t viewangles);
void trap_EA_GetInput(int client, float thinktime, void *input);
void trap_EA_ResetInput(int client);
void trap_BotMoveToGoal(int movestate, void *goal, int travelflags);
void trap_BotResetMoveState(int movestate);
int trap_BotAllocMoveState(void);
void trap_BotFreeMoveState(int handle);
void trap_BotInitMoveState(int handle, void *initmove);
