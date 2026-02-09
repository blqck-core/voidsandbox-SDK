// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#define FADE_TIME 500
#define DAMAGE_TIME 500
#define LAND_DEFLECT_TIME 150
#define LAND_RETURN_TIME 300
#define STEP_TIME 200
#define DUCK_TIME 100
#define PAIN_TWITCH_TIME 200
#define ITEM_SCALEUP_TIME 2000
#define ZOOM_TIME 200
#define MUZZLE_FLASH_TIME 75
#define SINK_TIME 3000 // time for fragments to sink into ground before going away

#define MAX_NOTIFICATIONS 15
#define NOTIFICATION_FADE_TIME 500

#define MAX_STEP_CHANGE 32

#define MAX_VERTS_ON_POLY 128 * 1
#define MAX_MARK_POLYS 1024 * 8

#define ICON_SIZE 16

#define NUM_CROSSHAIRS 99

#define DEFAULT_MODEL "beret"

#define NOTIFY_INFO 1
#define NOTIFY_UNDO 2
#define NOTIFY_ITEM 3
#define NOTIFY_KILL 4

#define MAX_3D_STRING_QUEUE 8192
#define MAX_LOCAL_ENTITIES 4096

typedef struct {
	float x, y, z;
	const char *str;
	int style;
	vec4_t color;
	float fontSize;
	float min, max;
	qboolean useScale, useFade, useTrace;
} queued3DString_t;

typedef enum {
	FOOTSTEP_NORMAL,
	FOOTSTEP_BOOT,
	FOOTSTEP_FLESH,
	FOOTSTEP_MECH,
	FOOTSTEP_ENERGY,
	FOOTSTEP_METAL,
	FOOTSTEP_SPLASH,

	FOOTSTEP_TOTAL
} footstep_t;

typedef enum { IMPACTSOUND_DEFAULT, IMPACTSOUND_METAL, IMPACTSOUND_FLESH } impactSound_t;

// player entities need to track more information
// than any other type of entity.

// note that not every player entity is a client entity,
// because corpses after respawn are outside the normal
// client numbering range

// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct {
	int oldFrame;
	int oldFrameTime; // time when ->oldFrame was exactly on
	int frame;
	int frameTime; // time when ->frame will be exactly on
	float backlerp;
	float yawAngle;
	qboolean yawing;
	float pitchAngle;
	qboolean pitching;
} lerpFrame_t;

// Notifications
typedef struct {
	char text[128];
	int type;
	int number;
	int startTime;
	qboolean active;
} notification_t;

typedef struct {
	lerpFrame_t legs, torso, flag;
	int painTime;
	int painDirection; // flip from 0 to 1
	int lightningFiring;
	vec3_t railgunImpact;
	qboolean railgunFlash;
	float barrelAngle;
	int barrelTime;
	qboolean barrelSpinning;
	vec3_t eyepos; // where our eyes at
	lerpFrame_t head;
} playerEntity_t;

// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s {
	entityState_t currentState; // from cg.frame
	entityState_t nextState;    // from cg.nextFrame, if available
	qboolean interpolate;       // true if next is valid to interpolate to
	qboolean currentValid;      // true if cg.frame holds this entity
	int muzzleFlashTime;        // move to playerEntity?
	int previousEvent;
	int trailTime; // so missile trails can handle dropped initial packets
	int dustTrailTime;
	int miscTime;
	int snapShotTime; // last time this entity was found in a snapshot
	playerEntity_t pe;

	// exact interpolated position of entity on this frame
	vec3_t lerpOrigin;
	vec3_t lerpAngles;
	vec3_t eyesOrigin;
	vec3_t eyesAngles;
} centity_t;

// local entities are created as a result of events or predicted actions,
// and live independantly from all server transmitted entities

typedef struct markPoly_s {
	struct markPoly_s *prevMark, *nextMark;
	int time;
	qhandle_t markShader;
	qboolean alphaFade; // fade alpha instead of rgb
	float color[4];
	poly_t poly;
	polyVert_t verts[MAX_VERTS_ON_POLY];
} markPoly_t;

typedef enum { LE_EXPLOSION, LE_SPRITE_EXPLOSION, LE_FRAGMENT, LE_FRAGMENT2, LE_MOVE_SCALE_FADE, LE_FALL_SCALE_FADE, LE_FADE_RGB, LE_SCALE_FADE, LE_SHOWREFENTITY } leType_t;

typedef enum {
	LEF_PUFF_DONT_SCALE = 0x0001, // do not scale size over time
	LEF_TUMBLE = 0x0002,          // tumble over time, used for ejecting shells
} leFlag_t;

typedef enum { LEMT_NONE, LEMT_BLOOD } leMarkType_t; // fragment local entities can leave marks on walls

typedef enum { LEBS_NONE, LEBS_BLOOD } leBounceSoundType_t; // fragment local entities can make sounds on impacts

typedef enum {
	LETT_NONE,   // does not emit a puff trail
	LETT_BLOOD,  // emits a blood trail
} leTrailType_t; // defines bounce behavior and trail on fragment local entities

typedef enum { PT_GRAVITY, PT_LINEAR_UP, PT_LINEAR_DOWN, PT_LINEAR_BOTH } particleType_t;

typedef struct localEntity_s {
	struct localEntity_s *prev, *next;
	leType_t leType;
	int leFlags;
	int startTime;
	int endTime;
	int fadeInTime;
	float lifeRate; // 1.0 / (endTime - startTime)
	trajectory_t pos;
	trajectory_t angles;
	float bounceFactor; // 0.0 = no bounce, 1.0 = perfect
	float color[4];
	float radius;
	float light;
	vec3_t lightColor;
	leMarkType_t leMarkType;               // mark to leave on fragment impact
	leBounceSoundType_t leBounceSoundType; // sound to play on fragment impact
	leTrailType_t leTrailType;             // trail to show behind fragment
	refEntity_t refEntity;
} localEntity_t;

typedef struct {
	int client;
	int score;
} score_t;

// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
#define MAX_CUSTOM_SOUNDS 32

typedef struct {
	qboolean infoValid;
	char name[MAX_QPATH];
	team_t team;
	int isNPC; // 0 = not NPC, 1 = NPC
	int vehicleNum;
	int flashlight;
	char playerSkinName[MAX_QPATH];
	qhandle_t playerSkin;
} clientInfo_t;

// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weaponInfo_s {
	qboolean registered;
	item_t *item;

	qhandle_t handsModel; // the hands don't actually draw, they just position the weapon
	qhandle_t weaponModel;
	qhandle_t barrelModel;
	qhandle_t flashModel;

	vec3_t weaponMidpoint; // so it will rotate centered instead of by tag

	float flashDlight;
	vec3_t flashDlightColor;
	sfxHandle_t flashSound[4]; // fast firing weapons randomly choose

	qhandle_t weaponIcon;
	qhandle_t ammoIcon;

	qhandle_t ammoModel;

	qhandle_t missileModel;
	sfxHandle_t missileSound;
	void (*missileTrailFunc)(centity_t *, const struct weaponInfo_s *wi);
	float missileDlight;
	vec3_t missileDlightColor;
	int missileRenderfx;

	void (*ejectBrassFunc)(centity_t *);

	float trailRadius;
	float wiTrailTime;

	sfxHandle_t readySound;
	sfxHandle_t firingSound;
	qboolean loopFireSound;
} weaponInfo_t;

// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct {
	qboolean registered;
	qhandle_t model;
	qhandle_t icon;
} itemInfo_t;

#define MAX_SKULLTRAIL 32

typedef struct {
	vec3_t positions[MAX_SKULLTRAIL];
	int numpositions;
} skulltrail_t;

#define MAX_SOUNDBUFFER 20
#define MAX_PREDICTED_EVENTS 16

typedef struct {
	int clientFrame; // incremented each frame

	int clientNum;

	qboolean demoPlayback;
	qboolean loading;             // don't defer players at initial startup
	qboolean intermissionStarted; // don't play voice rewards, because game will end shortly

	// there are only one or two snapshot_t that are relevent at a time
	int latestSnapshotNum;  // the number of snapshots the client system has received
	int latestSnapshotTime; // the time from latestSnapshotNum, so we don't need to read the snapshot yet

	snapshot_t *snap;     // cg.snap->serverTime <= cg.time
	snapshot_t *nextSnap; // cg.nextSnap->serverTime > cg.time, or NULL
	snapshot_t activeSnapshots[2];

	float frameInterpolation; // (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime)

	qboolean thisFrameTeleport;
	qboolean nextFrameTeleport;

	int frametime; // cg.time - cg.oldTime

	int time;    // this is the time value that the client
	             // is rendering at.
	int oldTime; // time at last frame, used for missile trails and prediction checking

	int physicsTime; // either cg.snap->time or cg.nextSnap->time

	qboolean mapRestart; // set on a map restart to set back the weapon

	qboolean renderingThirdPerson; // during deaths, chasecams, etc
	int renderingEyesPerson;       // during deaths, chasecams, etc

	// prediction state
	playerState_t predictedPlayerState;
	centity_t predictedPlayerEntity;
	qboolean validPPS; // clear until the first call to CG_PredictPlayerState
	int predictedErrorTime;
	vec3_t predictedError;

	int eventSequence;
	int predictableEvents[MAX_PREDICTED_EVENTS];

	float stepChange; // for stair up smoothing
	int stepTime;

	float duckChange; // for duck viewheight smoothing
	int duckTime;

	float landChange; // for landing hard
	int landTime;

	// input state sent to server
	int weaponSelect;

	int swep_listcl[WEAPONS_NUM];
	int swep_spawncl[WEAPONS_NUM]; // stores spawn weapons

	float savedSens; // physgun

	// auto rotating items
	vec3_t autoAngles;
	vec3_t autoAxis[3];
	vec3_t autoAnglesFast;
	vec3_t autoAxisFast[3];

	// view rendering
	refdef_t refdef;
	vec3_t refdefViewAngles; // will be converted to refdef.viewaxis

	// zoom key
	qboolean zoomed;
	int zoomTime;
	float zoomSensitivity;

	// information screen text during loading
	char infoScreenText[MAX_STRING_CHARS];
	float infoScreenValue;

	// scoreboard
	int scoresRequestTime;
	int numScores;
	int teamScores[2];
	score_t scores[MAX_CLIENTS];
	qboolean showScores;
	qboolean scoreBoardShowing;

	// skull trails
	skulltrail_t skulltrails[MAX_CLIENTS];

	// centerprinting
	int centerPrintTime;
	int centerPrintCharWidth;
	int centerPrintY;
	char centerPrint[256];
	int centerPrintLines;

	// notifications
	notification_t notifications[MAX_NOTIFICATIONS];

	char entityInfo[512];

	// sound buffer mainly for announcer sounds
	int soundBufferIn;
	int soundBufferOut;
	int soundTime;
	qhandle_t soundBuffer[MAX_SOUNDBUFFER];

	// blend blobs
	float damageTime;
	float damageX, damageY, damageValue;

	// temp working variables for player view
	float bobfracsin;
	int bobcycle;
	float xyspeed;

	// time that the client will respawn. If 0 = the player is alive.
	int respawnTime;
} cg_t;

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t, and powerupInfo_t
typedef struct {
	qhandle_t redCubeModel;
	qhandle_t blueCubeModel;
	qhandle_t redCubeIcon;
	qhandle_t blueCubeIcon;
	qhandle_t redFlagModel;
	qhandle_t blueFlagModel;
	qhandle_t neutralFlagModel;
	qhandle_t redFlagShader[3];
	qhandle_t blueFlagShader[3];

	qhandle_t redFlagBaseModel;
	qhandle_t blueFlagBaseModel;
	qhandle_t neutralFlagBaseModel;

	qhandle_t overloadBaseModel;
	qhandle_t overloadTargetModel;
	qhandle_t overloadLightsModel;
	qhandle_t overloadEnergyModel;

	qhandle_t harvesterModel;
	qhandle_t harvesterRedSkin;
	qhandle_t harvesterBlueSkin;
	qhandle_t harvesterNeutralModel;

	// gib explosions
	qhandle_t gibAbdomen;
	qhandle_t gibArm;
	qhandle_t gibChest;
	qhandle_t gibFist;
	qhandle_t gibFoot;
	qhandle_t gibForearm;
	qhandle_t gibIntestine;
	qhandle_t gibLeg;
	qhandle_t gibSkull;
	qhandle_t gibBrain;

	qhandle_t sparkShader;

	qhandle_t machinegunBrassModel;
	qhandle_t shotgunBrassModel;

	qhandle_t railCoreShader;

	qhandle_t lightningShader;

	qhandle_t selectShader;
	qhandle_t viewBloodShader;
	qhandle_t tracerShader;
	qhandle_t crosshairShader[NUM_CROSSHAIRS];
	qhandle_t noammoShader;

	qhandle_t smokePuffShader;
	qhandle_t shotgunSmokePuffShader;
	qhandle_t plasmaBallShader;
	qhandle_t flameBallShader;
	qhandle_t antimatterBallShader;
	qhandle_t waterBubbleShader;
	qhandle_t bloodTrailShader;

	qhandle_t lsmkShader1;
	qhandle_t lsmkShader2;
	qhandle_t lsmkShader3;
	qhandle_t lsmkShader4;
	qhandle_t lsplShader;
	qhandle_t grappleShader;

	qhandle_t nailPuffShader;
	qhandle_t blueProxMine;

	qhandle_t shadowMarkShader;

	// wall mark shaders
	qhandle_t bloodMarkShader;
	qhandle_t bulletMarkShader;
	qhandle_t burnMarkShader;
	qhandle_t holeMarkShader;
	qhandle_t energyMarkShader;

	// powerup shaders
	qhandle_t quadShader;
	qhandle_t redQuadShader;
	qhandle_t quadWeaponShader;
	qhandle_t invisShader;
	qhandle_t regenShader;
	qhandle_t battleSuitShader;
	qhandle_t battleWeaponShader;
	qhandle_t hastePuffShader;

	qhandle_t ptexShader[2];

	// weapon effect models
	qhandle_t bulletFlashModel;
	qhandle_t ringFlashModel;
	qhandle_t dishFlashModel;
	qhandle_t lightningExplosionModel;

	// weapon effect shaders
	qhandle_t railExplosionShader;
	qhandle_t plasmaExplosionShader;
	qhandle_t bulletExplosionShader;
	qhandle_t rocketExplosionShader;
	qhandle_t grenadeExplosionShader;
	qhandle_t bfgExplosionShader;
	qhandle_t bloodExplosionShader;

	// special effects models
	qhandle_t teleportEffectModel;
	qhandle_t dustPuffShader;

	// postprocess
	qhandle_t postProcess;

	// Icons OpenSandbox
	qhandle_t errIcon;
	qhandle_t notifyIcon;
	qhandle_t undoIcon;

	// sounds
	sfxHandle_t quadSound;
	sfxHandle_t tracerSound;
	sfxHandle_t selectSound;
	sfxHandle_t useNothingSound;
	sfxHandle_t footsteps[FOOTSTEP_TOTAL][4];
	sfxHandle_t carengine[11];
	sfxHandle_t sfx_lghit1;
	sfxHandle_t sfx_lghit2;
	sfxHandle_t sfx_lghit3;
	sfxHandle_t sfx_ric1;
	sfxHandle_t sfx_ric2;
	sfxHandle_t sfx_ric3;
	sfxHandle_t sfx_rockexp;
	sfxHandle_t sfx_plasmaexp;

	sfxHandle_t sfx_proxexp;
	sfxHandle_t sfx_nghit;
	sfxHandle_t sfx_nghitflesh;
	sfxHandle_t sfx_nghitmetal;
	sfxHandle_t sfx_chghit;
	sfxHandle_t sfx_chghitflesh;
	sfxHandle_t sfx_chghitmetal;

	sfxHandle_t gibSound;
	sfxHandle_t gibBounce1Sound;
	sfxHandle_t gibBounce2Sound;
	sfxHandle_t gibBounce3Sound;
	sfxHandle_t teleInSound;
	sfxHandle_t teleOutSound;
	sfxHandle_t noAmmoSound;
	sfxHandle_t respawnSound;
	sfxHandle_t talkSound;
	sfxHandle_t landSound;
	sfxHandle_t fallSound;
	sfxHandle_t jumpPadSound;

	sfxHandle_t notifySound;
	sfxHandle_t undoSound;

	sfxHandle_t hitSound;
	sfxHandle_t hitSoundHighArmor;
	sfxHandle_t hitSoundLowArmor;
	sfxHandle_t hitTeamSound;

	sfxHandle_t watrInSound;
	sfxHandle_t watrOutSound;
	sfxHandle_t watrUnSound;

	sfxHandle_t medkitSound;

	// teamplay sounds
	sfxHandle_t redScoredSound;
	sfxHandle_t blueScoredSound;
	sfxHandle_t redLeadsSound;
	sfxHandle_t blueLeadsSound;
	sfxHandle_t teamsTiedSound;

	sfxHandle_t captureYourTeamSound;
	sfxHandle_t captureOpponentSound;
	sfxHandle_t returnYourTeamSound;
	sfxHandle_t returnOpponentSound;

	sfxHandle_t redFlagReturnedSound;
	sfxHandle_t blueFlagReturnedSound;
	sfxHandle_t enemyTookYourFlagSound;
	sfxHandle_t enemyTookTheFlagSound;
	sfxHandle_t yourTeamTookEnemyFlagSound;
	sfxHandle_t yourTeamTookTheFlagSound;
	sfxHandle_t yourBaseIsUnderAttackSound;

	sfxHandle_t hgrenb1aSound;
	sfxHandle_t hgrenb2aSound;
	sfxHandle_t wstbimplSound;
	sfxHandle_t wstbimpmSound;
	sfxHandle_t wstbimpdSound;
	sfxHandle_t wstbactvSound;
} cgMedia_t;

#define CONSOLE_MAXHEIGHT 16
#define CONSOLE_WIDTH 80
typedef struct {
	char msgs[CONSOLE_MAXHEIGHT][CONSOLE_WIDTH * 3 + 1];
	int msgTimes[CONSOLE_MAXHEIGHT];
	int insertIdx;
	int displayIdx;
} console_t;

// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct {
	gameState_t gameState; // gamestate from server

	int serverCommandSequence; // reliable command stream counter
	int processedSnapshotNum;  // the number of snapshots cgame has requested

	qboolean localServer; // detected on startup by checking sv_running

	// parsed from serverinfo
	gametype_t gametype;
	int teamflags;
	int fraglimit;
	int capturelimit;
	int timelimit;
	int maxclients;
	char mapname[MAX_QPATH];

	int levelStartTime;

	int scores1, scores2;  // from configstrings
	int redflag, blueflag; // flag status from configstrings
	int flagStatus;

	//
	// locally derived information from gamestate
	//
	qhandle_t gameModels[MAX_MODELS];
	sfxHandle_t gameSounds[MAX_SOUNDS];

	int numInlineModels;
	qhandle_t inlineDrawModel[MAX_MODELS];
	vec3_t inlineModelMidpoints[MAX_MODELS];

	clientInfo_t clientinfo[MAX_CLIENTS];

	console_t console;
	console_t chat;
	console_t teamChat;

	// media
	cgMedia_t media;
} cgs_t;

extern cgs_t cgs;
extern cg_t cg;
extern centity_t cg_entities[MAX_GENTITIES];
extern weaponInfo_t cg_weapons[WEAPONS_NUM];
extern itemInfo_t cg_items[MAX_ITEMS];
extern markPoly_t cg_markPolys[MAX_MARK_POLYS];

extern int mod_jumpheight;
extern int mod_gravity;

// cg_consolecmds.c
qboolean CG_ConsoleCommand(void);
void CG_InitConsoleCommands(void);

// cg_draw.c
void CG_Add3DString(float x, float y, float z, const char *str, int style, const vec4_t color, float fontSize, float min, float max, qboolean useTrace);
void CG_DrawHead(float x, float y, float w, float h, int clientNum);
void CG_AddToGenericConsole(const char *str, console_t *console);
void CG_AddNotify(const char *text, int type, int number);
void CG_DrawActive(void);

// cg_drawtools.c
void CG_DrawProgressBar(float x, float y, float width, float height, float progress, float segmentWidth, const float *barColor, const float *bgColor);
void CG_DrawPic(float x, float y, float width, float height, qhandle_t hShader);
void CG_Draw3DString(float x, float y, float z, const char *str, int style, vec4_t color, float fontSize, float min, float max, qboolean useTrace);
float *CG_FadeColor(int startMsec, int totalMsec);

// cg_effects.c
void CG_BubbleTrail(vec3_t start, vec3_t end, float spacing);
localEntity_t *CG_SmokePuff(const vec3_t p, const vec3_t vel, float radius, float r, float g, float b, float a, float duration, int startTime, int fadeInTime, int leFlags, qhandle_t hShader);
void CG_SpawnEffect(vec3_t org);
void CG_LightningBoltBeam(vec3_t start, vec3_t end);
localEntity_t *CG_MakeExplosion(vec3_t origin, vec3_t dir, qhandle_t hModel, qhandle_t shader, int msec, qboolean isSprite);
void CG_Bleed(vec3_t origin, int entityNum);
void CG_GibPlayer(vec3_t playerOrigin);
void CG_ParticlesFromEntityState(vec3_t origin, int type, entityState_t *es);

// cg_ents.c
void CG_PositionRotatedEntityOnTag(refEntity_t *entity, const refEntity_t *parent, qhandle_t parentModel, char *tagName);
void CG_SetEntitySoundPosition(centity_t *cent);
void CG_AddPacketEntities(void);

// cg_event.c
void CG_PainEvent(centity_t *cent, int health);
void CG_EntityEvent(centity_t *cent, vec3_t position);
void CG_CheckEvents(centity_t *cent);

// cg_info.c
void CG_LoadingString(const char *s, float value);
void CG_LoadingClient(int clientNum);
void CG_DrawInformation(void);

// cg_localeents.c
void CG_InitLocalEntities(void);
localEntity_t *CG_AllocLocalEntity(void);
void CG_AddLocalEntities(void);

// cg_main.c
void QDECL CG_PrintfChat(qboolean team, const char *msg, ...);
void QDECL CG_Printf(const char *msg, ...);
void QDECL CG_Error(const char *msg, ...);
void QDECL Com_Error(int level, const char *error, ...);
void QDECL Com_Printf(const char *msg, ...);
char *CG_Argv(int arg);
const char *CG_ConfigString(int index);

// cg_marks.c
void CG_InitMarkPolys(void);
void CG_ImpactMark(qhandle_t markShader, const vec3_t origin, const vec3_t dir, float orientation, float red, float green, float blue, float alpha, qboolean alphaFade, float radius, qboolean temporary);
void CG_AddMarks(void);

// cg_players.c
void CG_UpdateClientInfo(int clientNum);
void CG_Player(centity_t *cent);
void CG_ResetPlayerEntity(centity_t *cent);

// cg_playerstate.c
void CG_Respawn(void);
void CG_TransitionPlayerState(playerState_t *ps, playerState_t *ops);

// cg_predict.c
void CG_BuildSolidList(void);
void CG_Trace(trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask);
int CG_PointContents(const vec3_t point, int passEntityNum);
void CG_PredictPlayerState(void);

// cg_scoreboard.c
qboolean CG_DrawScoreboard(void);

// cg_servercmds.c
void CG_ParseServerinfo(void);
void CG_SetConfigValues(void);
void CG_ExecuteNewServerCommands(int latestSequence);

// cg_snapshot.c
void CG_TransitionEntity(centity_t *cent);
void CG_ProcessSnapshots(void);

// cg_unlagged.c
void CG_PredictWeaponEffects(centity_t *cent);

// cg_view.c
void CG_ZoomDown_f(void);
void CG_ZoomUp_f(void);
void CG_AddBufferedSound(sfxHandle_t sfx);
void CG_DrawActiveFrame(int serverTime, qboolean demoPlayback);

// cg_weapons.c
void CG_RailTrail(clientInfo_t *ci, vec3_t start, vec3_t end, int weapon);
void CG_RegisterWeapon(int weaponNum);
void CG_RegisterItemVisuals(int itemNum);
void CG_AddPlayerWeapon(refEntity_t *parent, playerState_t *ps, centity_t *cent, int team, clientInfo_t *ci);
void CG_AddViewWeapon(playerState_t *ps);
void CG_NextWeapon_f(void);
void CG_PrevWeapon_f(void);
void CG_Weapon_f(void);
void CG_FireWeapon(centity_t *cent);
void CG_MissileHitWall(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType);
void CG_MissileHitPlayer(int weapon, vec3_t origin, vec3_t dir, int entityNum);
void CG_ShotgunPattern(vec3_t origin, vec3_t origin2, int seed, int otherEntNum, int weapon);
void CG_ShotgunFire(entityState_t *es, int weapon);
void CG_Bullet(vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum);

// syscalls
void trap_AddCommand(const char *cmdName);
void trap_SendClientCommand(const char *s);
void trap_CM_LoadMap(const char *mapname);
int trap_CM_NumInlineModels(void);
clipHandle_t trap_CM_InlineModel(int index);
clipHandle_t trap_CM_TempBoxModel(const vec3_t mins, const vec3_t maxs);
int trap_CM_PointContents(const vec3_t p, clipHandle_t model);
int trap_CM_TransformedPointContents(const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles);
void trap_CM_BoxTrace(trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask);
void trap_CM_TransformedBoxTrace(trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask, const vec3_t origin, const vec3_t angles);
int trap_CM_MarkFragments(int numPoints, const vec3_t *points, const vec3_t projection, int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer);
void trap_S_StartSound(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx);
void trap_S_ClearLoopingSounds(qboolean killall);
void trap_S_AddLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx);
void trap_S_UpdateEntityPosition(int entityNum, const vec3_t origin);
void trap_S_Respatialize(int entityNum, const vec3_t origin, vec3_t axis[3]);
void trap_S_StartBackgroundTrack(const char *intro, const char *loop);
void trap_R_LoadWorldMap(const char *mapname);
void trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs);
void trap_GetGameState(gameState_t *gamestate);
void trap_GetCurrentSnapshotNumber(int *snapshotNumber, int *serverTime);
qboolean trap_GetSnapshot(int snapshotNumber, snapshot_t *snapshot);
qboolean trap_GetServerCommand(int serverCommandNumber);
int trap_GetCurrentCmdNumber(void);
qboolean trap_GetUserCmd(int cmdNumber, usercmd_t *ucmd);
void trap_SetUserCmdValue(float sensitivityScale);
int trap_Key_GetKey(const char *binding);
void trap_S_AddRealLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx);
void trap_S_StopLoopingSound(int entnum);
void trap_ImportOBJ(const char *name);
