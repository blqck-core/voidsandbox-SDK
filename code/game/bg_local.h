// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../qcommon/js_local.h"

#define MIN_WALK_NORMAL 0.7f // can't walk on very steep slopes
#define STEPSIZE 18
#define TIMER_LAND 130
#define TIMER_GESTURE (34 * 66 + 50)
#define OVERCLIP 1.001f

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server
typedef struct {
	vec3_t forward, right, up;
	float frametime;

	int msec;

	qboolean walking;
	qboolean groundPlane;
	trace_t groundTrace;

	float impactSpeed;

	vec3_t previous_origin;
	vec3_t previous_velocity;
	int previous_waterlevel;
} pml_t;

extern pmove_t *pm;
extern pml_t pml;

// bg_pmove.c
void PM_AddEvent(int newEvent);
void PM_AddTouchEnt(int entityNum);
void PM_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce);

// bg_slidemove.c
qboolean PM_SlideMove(qboolean gravity);
void PM_StepSlideMove(qboolean gravity);
