// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#define MAX_VIDEO_HANDLES 16
#define MAX_DLIGHTS 1024 // can't be increased, because bit flags are used on surfaces

// renderfx flags
#define RF_THIRD_PERSON 1    // don't draw through eyes, only mirrors (player bodies, chat sprites)
#define RF_FIRST_PERSON 2    // only draw through eyes (view weapon, damage blood blob)
#define RF_DEPTHHACK 4       // for view weapon Z crunching
#define RF_LIGHTING_ORIGIN 8 // use refEntity->lightingOrigin instead of refEntity->origin
#define RF_WRAP_FRAMES 16    // mod the model frames by the maxframes to allow continuous animation without needing to know the frame count
#define RF_CROSSHAIR 32      // for 3D crosshair

// refdef flags
#define RDF_NOWORLDMODEL 1 // used for player configuration screen

typedef struct {
	vec3_t xyz;
	float st[2];
	byte modulate[4];
} polyVert_t;

typedef struct poly_s {
	qhandle_t hShader;
	int numVerts;
	polyVert_t *verts;
} poly_t;

typedef enum {
	RT_MODEL,
	RT_POLY,
	RT_SPRITE,
	RT_BEAM,
	RT_RAIL_CORE,
	RT_RAIL_RINGS,
	RT_LIGHTNING,
	RT_PORTALSURFACE, // doesn't draw anything, just info for portals

	RT_MAX_REF_ENTITY_TYPE
} refEntityType_t;

typedef struct {
	refEntityType_t reType;
	int renderfx;

	qhandle_t hModel; // opaque type outside refresh

	// most recent data
	vec3_t lightingOrigin; // so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)

	vec3_t axis[3];             // rotation vectors
	qboolean nonNormalizedAxes; // axis are not normalized, i.e. they have scale
	float origin[3];            // also used as MODEL_BEAM's "from"
	int frame;                  // also used as MODEL_BEAM's diameter

	// previous data for frame interpolation
	float oldorigin[3]; // also used as MODEL_BEAM's "to"
	int oldframe;
	float backlerp; // 0.0 = current, 1.0 = old

	// texturing
	int skinNum;            // inline skin index
	qhandle_t customSkin;   // NULL for default skin
	qhandle_t customShader; // use one image for the entire thing

	// misc
	byte shaderRGBA[4];      // colors used by rgbgen entity shaders
	float shaderTexCoord[2]; // texture coordinates used by tcMod entity modifiers
	float shaderTime;        // subtracted from refdef time to control effect start times

	// extra sprite information
	float radius;
	float rotation;

	// cameraEyes
	vec3_t eyepos[2]; // looking from
	vec3_t eyelook;   // looking from
} refEntity_t;

#define MAX_RENDER_STRINGS 8
#define MAX_RENDER_STRING_LENGTH 32

typedef struct {
	int x, y, width, height;
	float fov_x, fov_y;
	vec3_t vieworg;
	vec3_t viewaxis[3]; // transformation matrix

	// time in milliseconds for shader effects and other time dependent rendering issues
	int time;

	int rdflags; // RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte areamask[MAX_MAP_AREA_BYTES];

	// text messages for deform text shaders
	char text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];
} refdef_t;

typedef enum {
	TC_NONE,
	TC_S3TC,    // this is for the GL_S3_s3tc extension.
	TC_S3TC_ARB // this is for the GL_EXT_texture_compression_s3tc extension.
} textureCompression_t;

typedef struct {
	char renderer_string[MAX_STRING_CHARS];
	char vendor_string[MAX_STRING_CHARS];
	char version_string[MAX_STRING_CHARS];
	char extensions_string[BIG_INFO_STRING];

	int maxTextureSize;
	int numTextureUnits;

	int colorBits, depthBits, stencilBits;

	qboolean deviceSupportsGamma;
	textureCompression_t textureCompression;
	qboolean textureEnvAddAvailable;

	int vidWidth, vidHeight;
	float windowAspect;
	int displayFrequency;

	qboolean isFullscreen;
} glconfig_t;

extern glconfig_t glconfig;

#ifndef GAME
// sharedsyscalls_client
void trap_UpdateScreen(void);
void trap_S_StartLocalSound(sfxHandle_t sfx, int channelNum);
sfxHandle_t trap_S_RegisterSound(const char *sample, qboolean compressed);
qhandle_t trap_R_RegisterModel(const char *name);
qhandle_t trap_R_RegisterSkin(const char *name);
qhandle_t trap_R_RegisterShader(const char *name);
qhandle_t trap_R_RegisterShaderNoMip(const char *name);
void trap_R_ClearScene(void);
void trap_R_AddRefEntityToScene(const refEntity_t *re);
void trap_R_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts);
void trap_R_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b);
void trap_R_AddLinearLightToScene(const vec3_t start, const vec3_t end, float intensity, float r, float g, float b);
void trap_R_RenderScene(const refdef_t *fd);
void trap_R_SetColor(const float *rgba);
void trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader);
int trap_R_LerpTag(orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, float frac, const char *tagName);
void trap_GetGlconfig(glconfig_t *glconfig);
int trap_Key_GetCatcher(void);
void trap_R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset);
void trap_Key_KeynumToStringBuf(int keynum, char *buf, int buflen);
void trap_Key_GetBindingBuf(int keynum, char *buf, int buflen);
#endif
