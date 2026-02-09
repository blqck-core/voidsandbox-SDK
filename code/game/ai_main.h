// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#define PRESENCE_NONE 1
#define PRESENCE_NORMAL 2
#define PRESENCE_CROUCH 4

#define ACTION_ATTACK 0x0000001
#define ACTION_USE 0x0000002
#define ACTION_RESPAWN 0x0000008
#define ACTION_JUMP 0x0000010
#define ACTION_MOVEUP 0x0000020
#define ACTION_CROUCH 0x0000080
#define ACTION_MOVEDOWN 0x0000100
#define ACTION_MOVEFORWARD 0x0000200
#define ACTION_MOVEBACK 0x0000800
#define ACTION_MOVELEFT 0x0001000
#define ACTION_MOVERIGHT 0x0002000
#define ACTION_DELAYEDJUMP 0x0008000
#define ACTION_TALK 0x0010000
#define ACTION_GESTURE 0x0020000
#define ACTION_WALK 0x0080000

typedef struct bot_input_s {
	float thinktime;   // time since last output (in seconds)
	vec3_t dir;        // movement direction
	float speed;       // speed in the range [0, 400]
	vec3_t viewangles; // the view angles
	int actionflags;   // one of the ACTION_? flags
	int weapon;        // weapon to use
} bot_input_t;

typedef struct bot_entitystate_s {
	int type;          // entity type
	int flags;         // entity flags
	vec3_t origin;     // origin of the entity
	vec3_t angles;     // angles of the model
	vec3_t old_origin; // for lerping
	vec3_t mins;       // bounding box minimums
	vec3_t maxs;       // bounding box maximums
	int groundent;     // ground entity
	int solid;         // solid type
	int modelindex;    // model used
	int modelindex2;   // weapons
	int frame;         // model frame number
	int event;         // impulse events -- muzzle flashes, footsteps, etc
	int eventParm;     // even parameter
	int powerups;      // bit flags
	int weapon;        // determines weapon and flash model, etc
} bot_entitystate_t;

typedef enum {
	SOLID_NOT,     // no interaction with other objects
	SOLID_TRIGGER, // only touch when inside, after moving
	SOLID_BBOX,    // touch on edge
	SOLID_BSP      // bsp clip, touch on edge
} solid_t;

// movement types
#define MOVE_WALK 1
#define MOVE_CROUCH 2
#define MOVE_JUMP 4

// move flags
#define MFL_BARRIERJUMP 1     // bot is performing a barrier jump
#define MFL_ONGROUND 2        // bot is in the ground
#define MFL_SWIMMING 4        // bot is swimming
#define MFL_AGAINSTLADDER 8   // bot is against a ladder
#define MFL_WATERJUMP 16      // bot is waterjumping
#define MFL_TELEPORTED 32     // bot is being teleported

typedef struct bot_initmove_s {
	vec3_t origin;     // origin of the bot
	vec3_t velocity;   // velocity of the bot
	vec3_t viewoffset; // view offset
	int entitynum;     // entity number of the bot
	int client;        // client number of the bot
	float thinktime;   // time the bot thinks
	int presencetype;  // presencetype of the bot
	vec3_t viewangles; // view angles of the bot
	int or_moveflags;  // values ored to the movement flags
} bot_initmove_t;

typedef struct bot_moveresult_s {
	int failure;             // true if movement failed all together
	int type;                // failure or blocked type
	int blocked;             // true if blocked by an entity
	int blockentity;         // entity blocking the bot
	int traveltype;          // last executed travel type
	int flags;               // result flags
	int weapon;              // weapon used for movement
	vec3_t movedir;          // movement direction
	vec3_t ideal_viewangles; // ideal viewangles for the movement
} bot_moveresult_t;

typedef struct bot_goal_s {
	vec3_t origin;     // origin of the goal
	int areanum;       // area number of the goal
	vec3_t mins, maxs; // mins and maxs of the goal
} bot_goal_t;

#define TFL_INVALID 0x00000001      // traveling temporary not possible
#define TFL_WALK 0x00000002         // walking
#define TFL_CROUCH 0x00000004       // crouching
#define TFL_BARRIERJUMP 0x00000008  // jumping onto a barrier
#define TFL_JUMP 0x00000010         // jumping
#define TFL_LADDER 0x00000020       // climbing a ladder
#define TFL_WALKOFFLEDGE 0x00000080 // walking of a ledge
#define TFL_SWIM 0x00000100         // swimming
#define TFL_WATERJUMP 0x00000200    // jumping out of the water
#define TFL_TELEPORT 0x00000400     // teleporting
#define TFL_ELEVATOR 0x00000800     // elevator
#define TFL_ROCKETJUMP 0x00001000   // rocket jumping
#define TFL_BFGJUMP 0x00002000      // bfg jumping
#define TFL_GRAPPLEHOOK 0x00004000  // grappling hook
#define TFL_DOUBLEJUMP 0x00008000   // double jump
#define TFL_RAMPJUMP 0x00010000     // ramp jump
#define TFL_STRAFEJUMP 0x00020000   // strafe jump
#define TFL_JUMPPAD 0x00040000      // jump pad
#define TFL_AIR 0x00080000          // travel through air
#define TFL_WATER 0x00100000        // travel through water
#define TFL_SLIME 0x00200000        // travel through slime
#define TFL_LAVA 0x00400000         // travel through lava
#define TFL_DONOTENTER 0x00800000   // travel through donotenter area
#define TFL_FUNCBOB 0x01000000      // func bobbing
#define TFL_FLIGHT 0x02000000       // flight
#define TFL_BRIDGE 0x04000000       // move over a bridge
#define TFL_NOTTEAM1 0x08000000     // not team 1
#define TFL_NOTTEAM2 0x10000000     // not team 2

// default travel flags
#define TFL_DEFAULT TFL_WALK | TFL_CROUCH | TFL_BARRIERJUMP | TFL_JUMP | TFL_LADDER | TFL_WALKOFFLEDGE | TFL_SWIM | TFL_WATERJUMP | TFL_TELEPORT | TFL_ELEVATOR | TFL_AIR | TFL_WATER | TFL_JUMPPAD | TFL_FUNCBOB

typedef struct bot_state_s {
	gentity_t *ent;
	qboolean inuse;
	int botthink_residual;
	bot_settings_t settings;
	int (*ainode)(struct bot_state_s *bs);
	float thinktime;
	vec3_t eye;
	int tfl;
	int setupcount;
	int ms;
	int enemy;
	vec3_t viewangles;
	vec3_t ideal_viewangles;
	bot_goal_t maingoal;
	int maingoal_time;
} bot_state_t;
