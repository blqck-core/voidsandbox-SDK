// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../qcommon/js_local.h"

void CG_UpdateClientInfo(int clientNum) {
	clientInfo_t *ci = &cgs.clientinfo[clientNum];
	const char *configstring = CG_ConfigString(clientNum + CS_PLAYERS);
	const char *v;

	if(!configstring[0]) {
		Q_StringCopy(ci->name, "Disconnected", sizeof(ci->name));
		return; // player disconnected
	}

	v = Info_ValueForKey(configstring, "n");
	Q_StringCopy(ci->name, v, sizeof(ci->name));
	
	v = Info_ValueForKey(configstring, "t");
	ci->team = atoi(v);
	
	v = Info_ValueForKey(configstring, "p");
	ci->playerSkin = trap_R_RegisterShader(v);

	v = Info_ValueForKey(configstring, "v");
	ci->vehicleNum = atoi(v);

	v = Info_ValueForKey(configstring, "i");
	ci->isNPC = atoi(v);

	v = Info_ValueForKey(configstring, "f");
	ci->flashlight = atoi(v);

	ci->infoValid = qtrue;
}

#define SHADOW_DISTANCE 128
static qboolean CG_PlayerShadow(centity_t *cent) {
	vec3_t end, mins = {-15, -15, 0}, maxs = {15, 15, 2};
	trace_t trace;
	float alpha;

	if(cvarInt("cg_shadows") == 0) return qfalse;

	// send a trace down from the player to the ground
	VectorCopy(cent->lerpOrigin, end);
	end[2] -= SHADOW_DISTANCE;

	trap_CM_BoxTrace(&trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID);

	if(trace.fraction == 1.0 || trace.startsolid || trace.allsolid) return qfalse;

	CG_ImpactMark(cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal, cent->pe.legs.yawAngle, alpha, alpha, alpha, 1, qfalse, 24, qtrue);

	return qtrue;
}

void CG_Player(centity_t *cent) {
	clientInfo_t *ci;
	refEntity_t body;
	refEntity_t head;
	/*refEntity_t l_arm;
	refEntity_t l_leg;
	refEntity_t r_arm;
	refEntity_t r_leg;*/
	int clientNum;
	qboolean yourself;
	
	clientNum = cent->currentState.clientNum;
	if(clientNum < 0 || clientNum >= MAX_CLIENTS) CG_Error("Player out of range!");
	ci = &cgs.clientinfo[clientNum];
	
	if(!ci->infoValid) return;
	if(cent->currentState.number == cg.snap->ps.clientNum) yourself = qtrue;

	memset(&body, 0, sizeof(body));
	memset(&head, 0, sizeof(head));

	CG_PlayerShadow(cent);
	
	VectorCopy(cent->lerpOrigin, body.origin);

	body.hModel = trap_R_RegisterModel("models/player/body/default");
	body.customShader = ci->playerSkin;
	body.renderfx = RF_LIGHTING_ORIGIN | RF_FIRST_PERSON;
	trap_R_AddRefEntityToScene(&body);

    head.hModel = trap_R_RegisterModel("models/player/head/default");
	head.customShader = ci->playerSkin;
	head.renderfx = RF_LIGHTING_ORIGIN;
	CG_PositionRotatedEntityOnTag(&head, &body, body.hModel, "tag.head");
	trap_R_AddRefEntityToScene(&head);

	if(cent->currentState.eFlags & EF_DEAD) return;

	if(cent->currentState.number != cg.snap->ps.clientNum) {
		if(ci->team == cg.snap->ps.persistant[PERS_TEAM] && ci->team != TEAM_FREE) CG_Add3DString(cent->lerpOrigin[0], cent->lerpOrigin[1], cent->lerpOrigin[2] + 48, ci->name, UI_DROPSHADOW, color_white, 1.00, 2048, 3072, qfalse);
		else if(ci->team == TEAM_FREE) CG_Add3DString(cent->lerpOrigin[0], cent->lerpOrigin[1], cent->lerpOrigin[2] + 48, ci->name, UI_DROPSHADOW, color_white, 1.00, 512, 768, qtrue);
	}
	
	if(ci->vehicleNum) {
		VectorScale(body.axis[0], 0, body.axis[0]);
		VectorScale(body.axis[1], 0, body.axis[1]);
		VectorScale(body.axis[2], 0, body.axis[2]);
	}

	//CG_AddPlayerWeapon(&torso, NULL, cent, ci->team, ci);
}

void CG_ResetPlayerEntity(centity_t *cent) {
	BG_EvaluateTrajectory(&cent->currentState.pos, cg.time, cent->lerpOrigin);
	BG_EvaluateTrajectory(&cent->currentState.apos, cg.time, cent->lerpAngles);
}
