// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#ifndef __Q_SHARED_H
#define __Q_SHARED_H

#define Q_EXPORT

#define __attribute__(x)

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef int size_t;

typedef char *va_list;
#define _INTSIZEOF(n) ((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))
#define va_start(ap, v) (ap = (va_list) & v + _INTSIZEOF(v))
#define va_arg(ap, t) (*(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)))
#define va_end(ap) (ap = (va_list)0)

#define INT_MAX 0x7fffffff       /* maximum (signed) int value */
#define INT_MIN (-INT_MAX - 1)   /* minimum (signed) int value */
#define LONG_MAX 0x7fffffffL     /* maximum (signed) long value */
#define LONG_MIN (-LONG_MAX - 1) /* minimum (signed) long value */

#define isalnum(c) (isalpha(c) || isdigit(c))
#define isalpha(c) (isupper(c) || islower(c))
#define isascii(c) ((c) > 0 && (c) <= 0x7f)
#define iscntrl(c) (((c) >= 0) && (((c) <= 0x1f) || ((c) == 0x7f)))
#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isgraph(c) ((c) != ' ' && isprint(c))
#define islower(c) ((c) >= 'a' && (c) <= 'z')
#define isprint(c) ((c) >= ' ' && (c) <= '~')
#define ispunct(c) (((c) > ' ' && (c) <= '~') && !isalnum(c))
#define isspace(c) ((c) == ' ' || (c) == '\f' || (c) == '\n' || (c) == '\r' || (c) == '\t' || (c) == '\v')
#define isupper(c) ((c) >= 'A' && (c) <= 'Z')
#define isxdigit(c) (isxupper(c) || isxlower(c))
#define isxlower(c) (isdigit(c) || (c >= 'a' && c <= 'f'))
#define isxupper(c) (isdigit(c) || (c >= 'A' && c <= 'F'))

// Misc functions
typedef int cmp_t(const void *, const void *);

// bg_lib.c
void qsort(void *a, size_t n, size_t es, cmp_t *cmp);
size_t strlen(const char *string);
int strlenru(const char *string);
int ifstrlenru(const char *string);
char *strcat(char *strDestination, const char *strSource);
char *strcpy(char *strDestination, const char *strSource);
int strcmp(const char *string1, const char *string2);
char *strchr(const char *string, int c);
char *strrchr(const char *string, int c);
char *strstr(const char *string, const char *strCharSet);
int tolower(int c);
int toupper(int c);
void *memmove(void *dest, const void *src, size_t count);
double tan(double x);
void srand(unsigned seed);
int rand(void);
double atof(const char *string);
int atoi(const char *string);
int abs(int n);
double fabs(double x);
int Q_vsnprintf(char *str, size_t length, const char *fmt, va_list args);
int Q_snprintf(char *str, size_t length, const char *fmt, ...);
int sscanf(const char *buffer, const char *fmt, ...);

// sharedsyscalls
double sin(double x);
double cos(double x);
double acos(double x);
double atan2(double y, double x);
double sqrt(double x);

#define QDECL
#define ID_INLINE

// endianness
short ShortSwap(short l);
int LongSwap(int l);
float FloatSwap(const float *f);

#define LittleShort
#define LittleLong
#define LittleFloat
#define BigShort
#define BigLong
#define BigFloat

/**********************************************************************
  VM Considerations

  The VM can not use the standard system headers because we aren't really
  using the compiler they were meant for.  We use bg_lib.h which contains
  prototypes for the functions we define for our own use in bg_lib.c.

  When writing mods, please add needed headers HERE, do not start including
  stuff like <stdio.h> in the various .c files that make up each of the VMs
  since you will be including system headers files can will have issues.

  Remember, if you use a C library function that is not defined in bg_lib.c,
  you will have to add your own version for support in the VM.
 **********************************************************************/

typedef int intptr_t;
typedef unsigned char byte;

typedef enum { qfalse, qtrue } qboolean;

typedef union {
	float f;
	int i;
	unsigned int ui;
} floatint_t;

typedef int qhandle_t;
typedef int sfxHandle_t;
typedef int fileHandle_t;
typedef int clipHandle_t;

#define PAD(x, y) (((x) + (y) - 1) & ~((y) - 1))

#define ALIGN(x)

#define MAX_QINT 0x7fffffff
#define MIN_QINT (-MAX_QINT - 1)

#define ARRAY_LEN(x) (sizeof(x) / sizeof(*(x)))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define STRARRAY_LEN(x) (ARRAY_LEN(x) - 1)

// angle indexes
#define PITCH 0 // up / down
#define YAW 1   // left / right
#define ROLL 2  // fall over

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define MAX_STRING_CHARS 1024  // max length of a string passed to Cmd_TokenizeString
#define MAX_STRING_TOKENS 1024 // max tokens resulting from Cmd_TokenizeString
#define MAX_TOKEN_CHARS 1024   // max length of an individual token

#define MAX_INFO_STRING 1024
#define MAX_INFO_KEY 1024
#define MAX_INFO_VALUE 1024

#define BIG_INFO_STRING 8192 // used for system info key only
#define BIG_INFO_KEY 8192
#define BIG_INFO_VALUE 8192

#define MAX_QPATH 256 // max length of a quake game pathname

#define MAX_SAY_TEXT 150

// paramters for command buffer stuffing
typedef enum {
	EXEC_NOW,    // don't return until completed, a VM should NEVER use this,
	             // because some commands might cause the VM to be unloaded...
	EXEC_INSERT, // insert at current position, but don't run yet
	EXEC_APPEND  // add to end of the command buffer (normal case)
} cbufExec_t;

//
// these aren't needed by any of the VMs.  put in another header?
//
#define MAX_MAP_AREA_BYTES 32 // bit vector of area visibility

// print levels from renderer (FIXME: set up for game / cgame?)
typedef enum {
	PRINT_ALL,
	PRINT_DEVELOPER, // only print when "developer 1"
	PRINT_WARNING,
	PRINT_ERROR
} printParm_t;

// parameters to the main Error routine
typedef enum {
	ERR_FATAL,            // exit the entire game with a popup window
	ERR_DROP,             // print to console and disconnect from game
	ERR_SERVERDISCONNECT, // don't kill server
	ERR_DISCONNECT
} errorParm_t;

#define PULSE_DIVISOR 75

#define UI_LEFT 0x00000000 // format mask
#define UI_CENTER 0x00000001
#define UI_RIGHT 0x00000002
#define UI_FORMATMASK 0x00000007
#define UI_DROPSHADOW 0x00000010 // other
#define UI_PULSE 0x00000020

#define Com_Memset memset
#define Com_Memcpy memcpy

/*
==============================================================
MATHLIB
==============================================================
*/

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];

#ifndef M_PI
#define M_PI 3.14159265358979323846f // matches value in gcc v2 math.h
#endif

// 180 / pi
#ifndef M_180_PI
#define M_180_PI 57.295779513082320876798154814105f
#endif

#define NUMVERTEXNORMALS 162
extern vec3_t bytedirs[NUMVERTEXNORMALS];

// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define BIGCHAR_WIDTH 12
#define BIGCHAR_HEIGHT 12

#define Q_COLOR_ESCAPE '^'
#define Q_IsColorString(p) ((p) && *(p) == Q_COLOR_ESCAPE && *((p) + 1) && *((p) + 1) >= '0' && *((p) + 1) <= '9') // ^[0-9]
#define COLOR_BLACK '0'
#define COLOR_RED '1'
#define COLOR_GREEN '2'
#define COLOR_YELLOW '3'
#define COLOR_BLUE '4'
#define COLOR_CYAN '5'
#define COLOR_MAGENTA '6'
#define COLOR_WHITE '7'
#define COLOR_MENU '8'
#define ColorIndex(c) ((c) - '0')

#define S_COLOR_BLACK "^0"
#define S_COLOR_RED "^1"
#define S_COLOR_GREEN "^2"
#define S_COLOR_YELLOW "^3"
#define S_COLOR_BLUE "^4"
#define S_COLOR_CYAN "^5"
#define S_COLOR_MAGENTA "^6"
#define S_COLOR_WHITE "^7"
#define S_COLOR_MENU "^8"

extern vec4_t g_color_table[9];

// clang-format off
#define	MAKERGB( v, r, g, b ) v[0]=r;v[1]=g;v[2]=b
#define	MAKERGBA( v, r, g, b, a ) v[0]=r;v[1]=g;v[2]=b;v[3]=a
// clang-format on

#define DEG2RAD(a) (((a) * M_PI) / 180.0F)
#define RAD2DEG(a) (((a) * 180.0f) / M_PI)

struct cplane_s;

extern vec3_t vec3_origin;
extern vec3_t axisDefault[3];

float Q_fabs(float f);
float Q_rsqrt(float f); // reciprocal square root

// this isn't a real cheap function to call!
int DirToByte(vec3_t dir);
void ByteToDir(int b, vec3_t dir);

#define DotProduct(x, y) ((x)[0] * (y)[0] + (x)[1] * (y)[1] + (x)[2] * (y)[2])
#define VectorSubtract(a, b, c) ((c)[0] = (a)[0] - (b)[0], (c)[1] = (a)[1] - (b)[1], (c)[2] = (a)[2] - (b)[2])
#define Vector4Subtract(a, b, c) ((c)[0] = (a)[0] - (b)[0], (c)[1] = (a)[1] - (b)[1], (c)[2] = (a)[2] - (b)[2], (c)[3] = (a)[3] - (b)[3])
#define VectorAdd(a, b, c) ((c)[0] = (a)[0] + (b)[0], (c)[1] = (a)[1] + (b)[1], (c)[2] = (a)[2] + (b)[2])
#define Vector4Add(a, b, c) ((c)[0] = (a)[0] + (b)[0], (c)[1] = (a)[1] + (b)[1], (c)[2] = (a)[2] + (b)[2], (c)[3] = (a)[3] + (b)[3])
#define VectorCopy(a, b) ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2])
#define VectorScale(v, s, o) ((o)[0] = (v)[0] * (s), (o)[1] = (v)[1] * (s), (o)[2] = (v)[2] * (s))
#define VectorMA(v, s, b, o) ((o)[0] = (v)[0] + (b)[0] * (s), (o)[1] = (v)[1] + (b)[1] * (s), (o)[2] = (v)[2] + (b)[2] * (s))

#ifdef VM
#ifdef VectorCopy
#undef VectorCopy
// this is a little hack to get more efficient copies in our interpreter
typedef struct {
	float v[3];
} vec3struct_t;
#define VectorCopy(a, b) (*(vec3struct_t *)b = *(vec3struct_t *)a)
#endif
#endif

#define VectorClear(a) ((a)[0] = (a)[1] = (a)[2] = 0)
#define VectorNegate(a, b) ((b)[0] = -(a)[0], (b)[1] = -(a)[1], (b)[2] = -(a)[2])
#define VectorSet(v, x, y, z) ((v)[0] = (x), (v)[1] = (y), (v)[2] = (z))
#define Vector4Copy(a, b) ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2], (b)[3] = (a)[3])

#define Byte4Copy(a, b) ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2], (b)[3] = (a)[3])

// clang-format off
#ifdef GAME
#define CopyAlloc(dest, src) do {\
    if(src) { \
        int len = strlen(src) + 1; \
        dest = G_Alloc(len); \
        if (dest != NULL) { \
            strcpy(dest, src); \
        } \
    } else { \
        dest = NULL; \
    } \
} while(0)
#endif
// clang-format on

// clang-format off
#define	SnapVector(v) {v[0]=((int)(v[0]));v[1]=((int)(v[1]));v[2]=((int)(v[2]));}
// clang-format on

// just in case you do't want to use the macros
vec_t _DotProduct(const vec3_t v1, const vec3_t v2);
void _VectorSubtract(const vec3_t veca, const vec3_t vecb, vec3_t out);
void _VectorAdd(const vec3_t veca, const vec3_t vecb, vec3_t out);
void _VectorCopy(const vec3_t in, vec3_t out);
void _VectorScale(const vec3_t in, float scale, vec3_t out);
void _VectorMA(const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc);

float NormalizeColor(const vec3_t in, vec3_t out);

float RadiusFromBounds(const vec3_t mins, const vec3_t maxs);
void AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs);

int VectorCompare(const vec3_t v1, const vec3_t v2);
vec_t VectorLength(const vec3_t v);
vec_t VectorLengthSquared(const vec3_t v);
vec_t Distance(const vec3_t p1, const vec3_t p2);
vec_t DistanceSquared(const vec3_t p1, const vec3_t p2);
void VectorNormalizeFast(vec3_t v);
void VectorInverse(vec3_t v);
void CrossProduct(const vec3_t v1, const vec3_t v2, vec3_t cross);

vec_t VectorNormalize(vec3_t v); // returns vector length
vec_t VectorNormalize2(const vec3_t v, vec3_t out);
void Vector4Scale(const vec4_t in, vec_t scale, vec4_t out);

int Q_rand(int *seed);
float Q_random(int *seed);
float Q_crandom(int *seed);

#define random() ((rand() & 0x7fff) / ((float)0x7fff))
#define crandom() (2.0 * (random() - 0.5))

void vectoangles(const vec3_t value1, vec3_t angles);
void AnglesToAxis(const vec3_t angles, vec3_t axis[3]);
void AxisToAngles(vec3_t axis[3], vec3_t angles);
void OrthogonalizeMatrix(vec3_t forward, vec3_t right, vec3_t up);
void VelocityToAxis(const vec3_t velocity, vec3_t axis[3], float lerpFactor);
float Lerp(float a, float b, float f);

void AxisClear(vec3_t axis[3]);
void AxisCopy(vec3_t in[3], vec3_t out[3]);

float AngleMod(float a);
float LerpAngle(float from, float to, float frac);
float AngleSubtract(float a1, float a2);
void AnglesSubtract(vec3_t v1, vec3_t v2, vec3_t v3);
float AngleAdd(float a1, float a2);
void AngleMA(vec3_t aa, float scale, vec3_t ab, vec3_t ac);
void LerpAngles(vec3_t from, vec3_t to, vec3_t dest, float frac);

float AngleNormalize360(float angle);
float AngleNormalize180(float angle);
float AngleDelta(float angle1, float angle2);

qboolean PlaneFromPoints(vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c);
void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal);
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);
void RotateAroundDirection(vec3_t axis[3], float yaw);
// perpendicular vector could be replaced by this

void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]);
void AngleVectors(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
void PerpendicularVector(vec3_t dst, const vec3_t src);
void SnapVectorTowards(vec3_t v, vec3_t to);

float Com_Clamp(float min, float max, float value);

char *COM_SkipPath(char *pathname);
const char *COM_GetExtension(const char *name);
void COM_StripExtension(const char *in, char *out, int destsize);
void COM_DefaultExtension(char *path, int maxSize, const char *extension);

char *COM_Parse(char **data_p);
char *COM_ParseExt(char **data_p, qboolean allowLineBreak);
int COM_Compress(char *data_p);

#define MAX_TOKENLENGTH 1024

typedef struct pc_token_s {
	int type;
	int subtype;
	int intvalue;
	float floatvalue;
	char string[MAX_TOKENLENGTH];
} pc_token_t;

void QDECL Com_sprintf(char *dest, int size, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

// mode parm for FS_FOpenFile
typedef enum { FS_READ, FS_WRITE, FS_APPEND, FS_APPEND_SYNC } fsMode_t;

typedef enum { FS_SEEK_CUR, FS_SEEK_END, FS_SEEK_SET } fsOrigin_t;

//=============================================

int Q_isprint(int c);
int Q_islower(int c);
int Q_isupper(int c);
int Q_isalpha(int c);

// portable case insensitive compare
int Q_stricmp(const char *s1, const char *s2);
#define Q_strequal(s1, s2) (Q_stricmp(s1, s2) == 0)
int Q_strncmp(const char *s1, const char *s2, int n);
int Q_stricmpn(const char *s1, const char *s2, int n);
char *Q_strlwr(char *s1);
char *Q_strupr(char *s1);
const char *Q_stristr(const char *s, const char *find);

// buffer size safe library replacements
void Q_StringCopy(char *dest, const char *src, int destsize);
void Q_strcat(char *dest, int size, const char *src);
char *Q_CleanStr(char *string);

char *QDECL va(char *format, ...) __attribute__((format(printf, 1, 2)));

float AngleDifference(float ang1, float ang2);
float VectorToYaw(const vec3_t vec);

//
// key / value info strings
//
char *Info_ValueForKey(const char *s, const char *key);
void Info_RemoveKey(char *s, const char *key);
void Info_RemoveKey_big(char *s, const char *key);
void Info_SetValueForKey(char *s, const char *key, const char *value);
void Info_SetValueForKey_Big(char *s, const char *key, const char *value);
qboolean Info_Validate(const char *s);
void Info_NextPair(const char **s, char *key, char *value);

// this is only here so the functions in q_shared.c and bg_*.c can link
void QDECL Com_Error(int level, const char *error, ...) __attribute__((format(printf, 2, 3))) __attribute__((noreturn));
void QDECL Com_Printf(const char *msg, ...) __attribute__((format(printf, 1, 2)));

/*
==========================================================

CVARS (console variables)

Many variables can be used for cheating purposes, so when
cheats is zero, force all unspecified variables to their
default values.
==========================================================
*/

#define CVAR_ARCHIVE 1    // set to cause it to be saved
#define CVAR_USERINFO 2   // sent to server on connect or change
#define CVAR_SYSTEMINFO 4 // these cvars will be duplicated on all clients
#define CVAR_SERVERINFO 8 // sent in response to front end requests
#define CVAR_LATCH 16     // will only change after restart
#define CVAR_CHEAT 32     // can not be changed if cheats are disabled

/*
==============================================================

COLLISION DETECTION

==============================================================
*/
#define CONTENTS_SOLID 1 // an eye is never valid in a solid
#define CONTENTS_LAVA 8
#define CONTENTS_SLIME 16
#define CONTENTS_WATER 32
#define CONTENTS_FOG 64
#define CONTENTS_NOTTEAM1 128
#define CONTENTS_NOTTEAM2 256
#define CONTENTS_NOBOTCLIP 512
#define CONTENTS_AREAPORTAL 32768
#define CONTENTS_PLAYERCLIP 65536
#define CONTENTS_MONSTERCLIP 131072
#define CONTENTS_TELEPORTER 262144
#define CONTENTS_JUMPPAD 524288
#define CONTENTS_CLUSTERPORTAL 1048576
#define CONTENTS_DONOTENTER 2097152
#define CONTENTS_BOTCLIP 4194304
#define CONTENTS_MOVER 8388608
#define CONTENTS_ORIGIN 16777216
#define CONTENTS_BODY 33554432
#define CONTENTS_CORPSE 67108864
#define CONTENTS_DETAIL 134217728
#define CONTENTS_STRUCTURAL 268435456
#define CONTENTS_TRANSLUCENT 536870912
#define CONTENTS_TRIGGER 1073741824
#define CONTENTS_NODROP 2147483648

#define SURF_NODAMAGE 0x1 // never give falling damage
#define SURF_SLICK 0x2    // effects game physics
#define SURF_SKY 0x4      // lighting from environment map
#define SURF_LADDER 0x8
#define SURF_NOIMPACT 0x10       // don't make missile explosions
#define SURF_NOMARKS 0x20        // don't leave missile marks
#define SURF_FLESH 0x40          // make flesh sounds and effects
#define SURF_NODRAW 0x80         // don't generate a drawsurface at all
#define SURF_HINT 0x100          // make a primary bsp splitter
#define SURF_SKIP 0x200          // completely ignore, allowing non-closed brushes
#define SURF_NOLIGHTMAP 0x400    // surface doesn't need a lightmap
#define SURF_POINTLIGHT 0x800    // generate lighting info at vertexes
#define SURF_METALSTEPS 0x1000   // clanking footsteps
#define SURF_NOSTEPS 0x2000      // no footstep sounds
#define SURF_NONSOLID 0x4000     // don't collide against curves with this set
#define SURF_LIGHTFILTER 0x8000  // act as a light filter during q3map -light
#define SURF_ALPHASHADOW 0x10000 // do per-pixel light shadow casting in q3map
#define SURF_NODLIGHT 0x20000    // don't dlight even if solid (solid lava, skies)
#define SURF_DUST 0x40000        // leave a dust trail when walking on this surface

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
typedef struct cplane_s {
	vec3_t normal;
	float dist;
	byte type;     // for fast side tests: 0,1,2 = axial, 3 = nonaxial
	byte signbits; // signx + (signy<<1) + (signz<<2), used as lookup during collision
	byte pad[2];
} cplane_t;

// a trace is returned when a box is swept through the world
typedef struct {
	qboolean allsolid;   // if true, plane is not valid
	qboolean startsolid; // if true, the initial point was in a solid area
	float fraction;      // time completed, 1.0 = didn't hit anything
	vec3_t endpos;       // final position
	cplane_t plane;      // surface normal at impact, transformed to world space
	int surfaceFlags;    // surface hit
	int contents;        // contents on other side of surface hit
	int entityNum;       // entity the contacted sirface is a part of
} trace_t;

// markfragments are returned by CM_MarkFragments()
typedef struct {
	int firstPoint;
	int numPoints;
} markFragment_t;

typedef struct {
	vec3_t origin;
	vec3_t axis[3];
} orientation_t;

// in order from highest priority to lowest
// if none of the catchers are active, bound key strings will be executed
#define KEYCATCH_CONSOLE 0x0001
#define KEYCATCH_UI 0x0002
#define KEYCATCH_MESSAGE 0x0004
#define KEYCATCH_CGAME 0x0008

// sound channels
// channel 0 never willingly overrides
// other channels will allways override a playing sound on that channel
typedef enum {
	CHAN_AUTO,
	CHAN_LOCAL, // menu sounds, etc
	CHAN_WEAPON,
	CHAN_VOICE,
	CHAN_ITEM,
	CHAN_BODY,
	CHAN_LOCAL_SOUND, // chat messages, etc
	CHAN_ANNOUNCER    // announcer voices, etc
} soundChannel_t;

/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/

#define ANGLE2SHORT(x) ((int)((x) * 65536 / 360) & 65535)
#define SHORT2ANGLE(x) ((x) * (360.0 / 65536))

#define SNAPFLAG_RATE_DELAYED 1
#define SNAPFLAG_NOT_ACTIVE 2  // snapshot used during connection and for zombies
#define SNAPFLAG_SERVERCOUNT 4 // toggled every map_restart so transitions can be detected

//
// per-level limits
//
#define MAX_CUSTOMSTRINGS 128
#define MAX_MODELS 1024
#define MAX_SOUNDS 256
#define MAX_CLIENTS 128
#define MAX_LOCATIONS 64
#define GENTITYNUM_BITS 12
#define MAX_GENTITIES (1 << GENTITYNUM_BITS)

// entitynums are communicated with GENTITY_BITS, so any reserved
// values that are going to be communcated over the net need to
// also be in this range
#define ENTITYNUM_NONE (MAX_GENTITIES - 1)
#define ENTITYNUM_WORLD (MAX_GENTITIES - 2)
#define ENTITYNUM_MAX_NORMAL (MAX_GENTITIES - 3)

#define MAX_CONFIGSTRINGS 1600
#define MAX_GAMESTATE_CHARS 65535
#define MAX_CVARS 131072

// these are the only configstrings that the system reserves, all the
// other ones are strictly for servergame to clientgame communication
#define CS_SERVERINFO 0 // an info string with all the serverinfo cvars
#define CS_SYSTEMINFO 1

typedef struct {
	int stringOffsets[MAX_CONFIGSTRINGS];
	char stringData[MAX_GAMESTATE_CHARS];
	int dataCount;
} gameState_t;

// bit field limits
#define MAX_STATS 16
#define MAX_PERSISTANT 16
#define MAX_POWERUPS 16

#define MAX_PS_EVENTS 2

#define PS_PMOVEFRAMECOUNTBITS 6

// playerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c

// playerState_t is a full superset of entityState_t as it is used by players,
// so if a playerState_t is transmitted, the entityState_t can be fully derived
// from it.
typedef struct playerState_s {
	int commandTime; // cmd->serverTime of last executed command
	int pm_type;
	int bobCycle; // for view bobbing and footstep generation
	int pm_flags; // ducked, jump_held, etc
	int pm_time;

	vec3_t origin;
	vec3_t velocity;
	int weaponTime;
	int gravity;
	int speed;
	int delta_angles[3]; // add to command angles to get view direction
	                     // changed by spawns, rotating objects, and teleporters

	int groundEntityNum; // ENTITYNUM_NONE = in air

	int legsTimer; // don't change low priority animations until this runs out
	int legsAnim;  // mask off ANIM_TOGGLEBIT

	int torsoTimer; // don't change low priority animations until this runs out
	int torsoAnim;  // mask off ANIM_TOGGLEBIT

	int movementDir; // a number 0 to 7 that represents the reletive angle
	                 // of movement to the view angle (axial and diagonals)
	                 // when at rest, the value will remain unchanged
	                 // used to twist the legs during strafing

	vec3_t grapplePoint; // location of grapple to pull towards if PMF_GRAPPLE_PULL

	int eFlags; // copied to entityState_t->eFlags

	int eventSequence; // pmove generated events
	int events[MAX_PS_EVENTS];
	int eventParms[MAX_PS_EVENTS];

	int externalEvent; // events set on player from another source
	int externalEventParm;
	int externalEventTime;

	int clientNum; // ranges from 0 to MAX_CLIENTS-1
	int weapon;
	int weaponstate;

	vec3_t viewangles; // for fixed views
	int viewheight;

	// damage feedback
	int damageEvent; // when it changes, latch the other parms
	int damageYaw;
	int damagePitch;
	int damageCount;

	int stats[MAX_STATS];
	int persistant[MAX_PERSISTANT]; // stats that aren't cleared on death
	int powerups[MAX_POWERUPS];     // level.time that the powerup runs out

	int generic1;
	int loopSound;
	int jumppad_ent; // jumppad entity hit this frame

	// not communicated over the net at all
	int ping;             // server to game info for scoreboard
	int pmove_framecount; // FIXME: don't transmit over the network
	int jumppad_frame;
	int entityEventSequence;
} playerState_t;

#define BUTTON_ATTACK 1
#define BUTTON_ATTACK2 2
#define BUTTON_USE 4
#define BMOVE_W 8
#define BMOVE_A 16
#define BMOVE_S 32
#define BMOVE_D 64
#define BMOVE_J 128
#define BMOVE_C 256

#define PLAYER_MOVE_F   0
#define PLAYER_MOVE_R   1
#define PLAYER_MOVE_U   2

typedef struct usercmd_s {
	int serverTime;
	int angles[2];
	int buttons;
} usercmd_t;

// if entityState->solid == SOLID_BMODEL, modelindex is an inline model number
#define SOLID_BMODEL 0xffffff

typedef enum {
	TR_STATIONARY,
	TR_INTERPOLATE, // non-parametric, but interpolate between snapshots
	TR_LINEAR,
	TR_LINEAR_STOP,
	TR_SINE, // value = base + sin( time / duration ) * delta
	TR_GRAVITY,
	TR_GRAVITY_WATER,
	TR_ROTATING
} trType_t;

typedef struct {
	trType_t trType;
	int trTime;
	int trDuration; // if non 0, trTime + trDuration = stop time
	vec3_t trBase;
	vec3_t trDelta; // velocity, etc
} trajectory_t;

// entityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large

typedef struct entityState_s {
	int number; // entity index
	int eType;  // entityType_t
	int eFlags;

	trajectory_t pos;  // for calculating position
	trajectory_t apos; // for calculating angles

	int time;
	int time2;

	vec3_t origin;
	vec3_t origin2;

	vec3_t scales;

	vec3_t angles;
	vec3_t angles2;

	int otherEntityNum; // shotgun sources, etc
	int otherEntityNum2;

	int groundEntityNum; // -1 = in air

	int constantLight; // r + (g<<8) + (b<<16) + (intensity<<24)
	int loopSound;     // constantly loop this sound

	int modelindex;
	int modelindex2;
	int clientNum; // 0 to (MAX_CLIENTS - 1), for players and corpses
	int frame;

	int solid; // for client side prediction, trap_linkentity sets this properly

	int event; // impulse events -- muzzle flashes, footsteps, etc
	int eventParm;

	// for players
	int powerups;  // bit flags
	int weapon;    // determines weapon and flash model, etc
	int legsAnim;  // mask off ANIM_TOGGLEBIT
	int torsoAnim; // mask off ANIM_TOGGLEBIT, object type for props

	int generic1;
	int generic2;
} entityState_t;

typedef enum {
	CA_UNINITIALIZED,
	CA_DISCONNECTED, // not talking to a server
	CA_CONNECTING,   // sending request packets to the server
	CA_CHALLENGING,  // sending challenge packets to the server
	CA_CONNECTED,    // netchan_t established, getting gamestate
	CA_LOADING,      // only during cgame initialization, never during main loop
	CA_PRIMED,       // got gamestate, waiting for first frame
	CA_ACTIVE        // game views should be displayed
} connstate_t;

#define Square(x) ((x) * (x))

typedef struct qtime_s {
	int tm_sec;   /* seconds after the minute - [0,59] */
	int tm_min;   /* minutes after the hour - [0,59] */
	int tm_hour;  /* hours since midnight - [0,23] */
	int tm_mday;  /* day of the month - [1,31] */
	int tm_mon;   /* months since January - [0,11] */
	int tm_year;  /* years since 1900 */
	int tm_wday;  /* days since Sunday - [0,6] */
	int tm_yday;  /* days since January 1 - [0,365] */
	int tm_isdst; /* daylight savings time flag */
} qtime_t;

// server browser sources
#define AS_LOCAL 0
#define AS_GLOBAL 1
#define AS_FAVORITES 2

#define MAX_PINGREQUESTS 32

#define SAY_ALL 0
#define SAY_TEAM 1
#define SAY_TELL 2

float VectorDistance(const vec3_t v1, const vec3_t v2);
#endif // __Q_SHARED_H
