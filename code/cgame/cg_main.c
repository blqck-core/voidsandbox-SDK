// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

static void CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum);
static void CG_Shutdown(void);

int realVidWidth;
int realVidHeight;

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
intptr_t vmMain(int command, int arg0, int arg1, int arg2) {
	switch(command) {
	case CG_INIT: CG_Init(arg0, arg1, arg2); return 0;
	case CG_SHUTDOWN: CG_Shutdown(); return 0;
	case CG_CONSOLE_COMMAND: return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME: CG_DrawActiveFrame(arg0, arg1); return 0;
	case GETVMCONTEXT: VMContext(&vmargs, &vmresult); return 0;
	case VMCALL: VMCall(arg0); return 0;
	default: err("cgame.qvm: unknown command"); break;
	}
	return -1;
}

cg_t cg;
cgs_t cgs;
centity_t cg_entities[MAX_GENTITIES];
weaponInfo_t cg_weapons[WEAPONS_NUM];
itemInfo_t cg_items[MAX_ITEMS];

int mod_jumpheight;
int mod_gravity;

static void CG_CreateCvars(void) {
	cgs.localServer = cvarInt("sv_running");

	cvarRegister("model", "beret/default", CVAR_USERINFO | CVAR_ARCHIVE);
	cvarRegister("headmodel", "beret/default", CVAR_USERINFO | CVAR_ARCHIVE);
	cvarRegister("legsmodel", "beret/default", CVAR_USERINFO | CVAR_ARCHIVE);
	cvarRegister("team_model", "beret/default", CVAR_USERINFO | CVAR_ARCHIVE);
	cvarRegister("team_headmodel", "beret/default", CVAR_USERINFO | CVAR_ARCHIVE);
	cvarRegister("team_legsmodel", "beret/default", CVAR_USERINFO | CVAR_ARCHIVE);
}

void QDECL CG_PrintfChat(qboolean team, const char *msg, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	if(team) {
		CG_AddToGenericConsole(text, &cgs.teamChat);
	} else {
		CG_AddToGenericConsole(text, &cgs.chat);
	}
	trap_Print(text);
}

char *CG_Argv(int arg) {
	static char buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof(buffer));

	return buffer;
}

static void CG_RegisterSounds(void) {
	int i;
	char name[MAX_QPATH];
	const char *soundName;

	if(cgs.gametype >= GT_TEAM) {
		cgs.media.redLeadsSound = trap_S_RegisterSound("sound/feedback/redleads.wav", qtrue);
		cgs.media.blueLeadsSound = trap_S_RegisterSound("sound/feedback/blueleads.wav", qtrue);
		cgs.media.teamsTiedSound = trap_S_RegisterSound("sound/feedback/teamstied.wav", qtrue);
		cgs.media.hitTeamSound = trap_S_RegisterSound("sound/feedback/hit_teammate.wav", qtrue);

		cgs.media.redScoredSound = trap_S_RegisterSound("sound/teamplay/voc_red_scores.wav", qtrue);
		cgs.media.blueScoredSound = trap_S_RegisterSound("sound/teamplay/voc_blue_scores.wav", qtrue);

		cgs.media.captureYourTeamSound = trap_S_RegisterSound("sound/teamplay/flagcapture_yourteam.wav", qtrue);
		cgs.media.captureOpponentSound = trap_S_RegisterSound("sound/teamplay/flagcapture_opponent.wav", qtrue);

		cgs.media.returnYourTeamSound = trap_S_RegisterSound("sound/teamplay/flagreturn_yourteam.wav", qtrue);
		cgs.media.returnOpponentSound = trap_S_RegisterSound("sound/teamplay/flagreturn_opponent.wav", qtrue);
	}

	cgs.media.tracerSound = trap_S_RegisterSound("sound/weapons/machinegun/buletby1.wav", qfalse);
	cgs.media.selectSound = trap_S_RegisterSound("sound/weapons/change.wav", qfalse);
	cgs.media.useNothingSound = trap_S_RegisterSound("sound/items/use_nothing.wav", qfalse);
	cgs.media.gibSound = trap_S_RegisterSound("sound/player/gibsplt1.wav", qfalse);
	cgs.media.gibBounce1Sound = trap_S_RegisterSound("sound/player/gibimp1.wav", qfalse);
	cgs.media.gibBounce2Sound = trap_S_RegisterSound("sound/player/gibimp2.wav", qfalse);
	cgs.media.gibBounce3Sound = trap_S_RegisterSound("sound/player/gibimp3.wav", qfalse);

	cgs.media.teleInSound = trap_S_RegisterSound("sound/world/telein.wav", qfalse);
	cgs.media.teleOutSound = trap_S_RegisterSound("sound/world/teleout.wav", qfalse);
	cgs.media.respawnSound = trap_S_RegisterSound("sound/items/respawn1.wav", qfalse);

	cgs.media.noAmmoSound = trap_S_RegisterSound("sound/weapons/noammo.wav", qfalse);

	cgs.media.talkSound = trap_S_RegisterSound("sound/player/talk.wav", qfalse);
	cgs.media.landSound = trap_S_RegisterSound("sound/player/land1.wav", qfalse);

	cgs.media.notifySound = trap_S_RegisterSound("sound/notify.wav", qfalse);
	cgs.media.undoSound = trap_S_RegisterSound("sound/undo.wav", qfalse);

	cgs.media.hitSound = trap_S_RegisterSound("sound/feedback/hitde.wav", qfalse);
	cgs.media.hitSoundHighArmor = trap_S_RegisterSound("sound/feedback/hithi.wav", qfalse);
	cgs.media.hitSoundLowArmor = trap_S_RegisterSound("sound/feedback/hitlo.wav", qfalse);

	cgs.media.watrInSound = trap_S_RegisterSound("sound/player/watr_in.wav", qfalse);
	cgs.media.watrOutSound = trap_S_RegisterSound("sound/player/watr_out.wav", qfalse);
	cgs.media.watrUnSound = trap_S_RegisterSound("sound/player/watr_un.wav", qfalse);

	cgs.media.jumpPadSound = trap_S_RegisterSound("sound/world/jumppad.wav", qfalse);

	for(i = 0; i < 4; i++) {
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/step%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/boot%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_BOOT][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/flesh%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_FLESH][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/mech%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_MECH][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/energy%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_ENERGY][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/splash%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/clank%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound(name, qfalse);
	}

	for(i = 0; i <= 10; i++) {
		Com_sprintf(name, sizeof(name), "sound/vehicle/engine%i.ogg", i);
		cgs.media.carengine[i] = trap_S_RegisterSound(name, qfalse);
	}

	for(i = 1; i < MAX_SOUNDS; i++) {
		soundName = CG_ConfigString(CS_SOUNDS + i);
		if(!soundName[0]) break;
		if(soundName[0] == '*') continue; // custom sound
		cgs.gameSounds[i] = trap_S_RegisterSound(soundName, qfalse);
	}

	cgs.media.medkitSound = trap_S_RegisterSound("sound/items/use_medkit.wav", qfalse);
	cgs.media.quadSound = trap_S_RegisterSound("sound/items/damagequad.wav", qfalse);
	cgs.media.sfx_ric1 = trap_S_RegisterSound("sound/weapons/machinegun/ric1.wav", qfalse);
	cgs.media.sfx_ric2 = trap_S_RegisterSound("sound/weapons/machinegun/ric2.wav", qfalse);
	cgs.media.sfx_ric3 = trap_S_RegisterSound("sound/weapons/machinegun/ric3.wav", qfalse);
	cgs.media.sfx_rockexp = trap_S_RegisterSound("sound/weapons/rocket/rocklx1a.wav", qfalse);
	cgs.media.sfx_plasmaexp = trap_S_RegisterSound("sound/weapons/plasma/plasmx1a.wav", qfalse);
	cgs.media.sfx_proxexp = trap_S_RegisterSound("sound/weapons/proxmine/wstbexpl.wav", qfalse);
	cgs.media.sfx_nghit = trap_S_RegisterSound("sound/weapons/nailgun/wnalimpd.wav", qfalse);
	cgs.media.sfx_nghitflesh = trap_S_RegisterSound("sound/weapons/nailgun/wnalimpl.wav", qfalse);
	cgs.media.sfx_nghitmetal = trap_S_RegisterSound("sound/weapons/nailgun/wnalimpm.wav", qfalse);
	cgs.media.sfx_chghit = trap_S_RegisterSound("sound/weapons/vulcan/wvulimpd.wav", qfalse);
	cgs.media.sfx_chghitflesh = trap_S_RegisterSound("sound/weapons/vulcan/wvulimpl.wav", qfalse);
	cgs.media.sfx_chghitmetal = trap_S_RegisterSound("sound/weapons/vulcan/wvulimpm.wav", qfalse);

	cgs.media.wstbimplSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpl.wav", qfalse);
	cgs.media.wstbimpmSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpm.wav", qfalse);
	cgs.media.wstbimpdSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpd.wav", qfalse);
	cgs.media.wstbactvSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbactv.wav", qfalse);

	cgs.media.hgrenb1aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb1a.wav", qfalse);
	cgs.media.hgrenb2aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb2a.wav", qfalse);
}

static void CG_RegisterGraphics(void) {
	int i;

	// clear any references to old media
	memset(&cg.refdef, 0, sizeof(cg.refdef));
	trap_R_ClearScene();

	CG_LoadingString(cgs.mapname, -1);

	trap_R_LoadWorldMap(cgs.mapname);

	// precache status bar pics
	CG_LoadingString("game media", 0.70);

	cgs.media.viewBloodShader = trap_R_RegisterShader("viewBloodBlend");

	cgs.media.smokePuffShader = trap_R_RegisterShader("smokePuff");
	cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader("shotgunSmokePuff");
	cgs.media.nailPuffShader = trap_R_RegisterShader("nailtrail");
	cgs.media.blueProxMine = trap_R_RegisterModel("models/weaphits/proxmineb.md3");
	cgs.media.plasmaBallShader = trap_R_RegisterShader("sprites/plasma1");
	cgs.media.flameBallShader = trap_R_RegisterShader("sprites/flameball");
	cgs.media.antimatterBallShader = trap_R_RegisterShader("sprites/antimatter");
	cgs.media.bloodTrailShader = trap_R_RegisterShader("bloodTrail");

	cgs.media.waterBubbleShader = trap_R_RegisterShader("waterBubble");

	cgs.media.tracerShader = trap_R_RegisterShader("gfx/misc/tracer");
	cgs.media.selectShader = trap_R_RegisterShader("gfx/2d/select");

	for(i = 0; i < NUM_CROSSHAIRS; i++) {
		if(i < 10) cgs.media.crosshairShader[i] = trap_R_RegisterShader(va("gfx/2d/crosshair%c", 'a' + i));
	}

	cgs.media.noammoShader = trap_R_RegisterShader("icons/noammo");

	// powerup shaders
	cgs.media.quadShader = trap_R_RegisterShader("powerups/quad");
	cgs.media.quadWeaponShader = trap_R_RegisterShader("powerups/quadWeapon");
	cgs.media.battleSuitShader = trap_R_RegisterShader("powerups/battleSuit");
	cgs.media.battleWeaponShader = trap_R_RegisterShader("powerups/battleWeapon");
	cgs.media.invisShader = trap_R_RegisterShader("powerups/invisibility");
	cgs.media.regenShader = trap_R_RegisterShader("powerups/regen");
	cgs.media.hastePuffShader = trap_R_RegisterShader("hasteSmokePuff");

	cgs.media.ptexShader[0] = trap_R_RegisterShader("trans");
	cgs.media.ptexShader[1] = trap_R_RegisterShader("powerups/quad");

	cgs.media.dustPuffShader = trap_R_RegisterShader("hasteSmokePuff");

	if(cgs.gametype >= GT_TEAM) {
		cgs.media.redQuadShader = trap_R_RegisterShader("powerups/blueflag");
	}

	cgs.media.machinegunBrassModel = trap_R_RegisterModel("models/weapons2/shells/m_shell.md3");
	cgs.media.shotgunBrassModel = trap_R_RegisterModel("models/weapons2/shells/s_shell.md3");

	cgs.media.gibAbdomen = trap_R_RegisterModel("models/gibs/abdomen.md3");
	cgs.media.gibArm = trap_R_RegisterModel("models/gibs/arm.md3");
	cgs.media.gibChest = trap_R_RegisterModel("models/gibs/chest.md3");
	cgs.media.gibFist = trap_R_RegisterModel("models/gibs/fist.md3");
	cgs.media.gibFoot = trap_R_RegisterModel("models/gibs/foot.md3");
	cgs.media.gibForearm = trap_R_RegisterModel("models/gibs/forearm.md3");
	cgs.media.gibIntestine = trap_R_RegisterModel("models/gibs/intestine.md3");
	cgs.media.gibLeg = trap_R_RegisterModel("models/gibs/leg.md3");
	cgs.media.gibSkull = trap_R_RegisterModel("models/gibs/skull.md3");
	cgs.media.gibBrain = trap_R_RegisterModel("models/gibs/brain.md3");

	cgs.media.sparkShader = trap_R_RegisterShaderNoMip("spark");

	cgs.media.bloodExplosionShader = trap_R_RegisterShader("bloodExplosion");

	cgs.media.bulletFlashModel = trap_R_RegisterModel("models/weaphits/bullet.md3");
	cgs.media.ringFlashModel = trap_R_RegisterModel("models/weaphits/ring02.md3");
	cgs.media.dishFlashModel = trap_R_RegisterModel("models/weaphits/boom01.md3");
	cgs.media.teleportEffectModel = trap_R_RegisterModel("models/powerups/pop.md3");

	cgs.media.lsmkShader1 = trap_R_RegisterShader("leismoke1");
	cgs.media.lsmkShader2 = trap_R_RegisterShader("leismoke2");
	cgs.media.lsmkShader3 = trap_R_RegisterShader("leismoke3");
	cgs.media.lsmkShader4 = trap_R_RegisterShader("leismoke4");

	cgs.media.lsplShader = trap_R_RegisterShader("leisplash");

	// OpenSandbox UI
	cgs.media.errIcon = trap_R_RegisterShaderNoMip("menu/erricon");
	cgs.media.notifyIcon = trap_R_RegisterShaderNoMip("menu/notifyicon");
	cgs.media.undoIcon = trap_R_RegisterShaderNoMip("menu/undoicon");

	memset(cg_items, 0, sizeof(cg_items));
	memset(cg_weapons, 0, sizeof(cg_weapons));

	// wall marks
	cgs.media.bulletMarkShader = trap_R_RegisterShader("gfx/damage/bullet_mrk");
	cgs.media.burnMarkShader = trap_R_RegisterShader("gfx/damage/burn_med_mrk");
	cgs.media.holeMarkShader = trap_R_RegisterShader("gfx/damage/hole_lg_mrk");
	cgs.media.energyMarkShader = trap_R_RegisterShader("gfx/damage/plasma_mrk");
	cgs.media.shadowMarkShader = trap_R_RegisterShader("markShadow");
	cgs.media.bloodMarkShader = trap_R_RegisterShader("bloodMark");

	// explosion effect
	cgs.media.rocketExplosionShader = trap_R_RegisterShader("rocketExplosion");

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	for(i = 1; i < cgs.numInlineModels; i++) {
		char name[10];
		vec3_t mins, maxs;
		int j;

		Com_sprintf(name, sizeof(name), "*%i", i);
		cgs.inlineDrawModel[i] = trap_R_RegisterModel(name);
		trap_R_ModelBounds(cgs.inlineDrawModel[i], mins, maxs);
		for(j = 0; j < 3; j++) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * (maxs[j] - mins[j]);
		}
	}

	// register all the server specified models
	for(i = 1; i < MAX_MODELS; i++) {
		const char *modelName;

		modelName = CG_ConfigString(CS_MODELS + i);
		if(!modelName[0]) break;
		cgs.gameModels[i] = trap_R_RegisterModel(modelName);
	}

	cgs.media.railCoreShader = trap_R_RegisterShader("railCore");
}

static void CG_RegisterClients(void) {
	int i;

	for(i = 0; i < MAX_CLIENTS; i++) {
		const char *clientInfo;

		clientInfo = CG_ConfigString(CS_PLAYERS + i);
		if(!clientInfo[0]) continue;
		CG_LoadingClient(i);
		CG_UpdateClientInfo(i);
	}
}

static void CG_ImportModelsOBJ(void) {
	char dirlist[32000];
	int numfiles = FS_List("props", ".obj", dirlist, sizeof(dirlist));
	int i;
	char *file;

	file = dirlist;
	for(i = 0; i < numfiles; i++) {
		char nameWithoutExt[128];
		Q_snprintf(nameWithoutExt, sizeof(nameWithoutExt), "props/%.*s", (int)(strlen(file) - 4), file);
		trap_ImportOBJ(nameWithoutExt);
		CG_LoadingString(va("import OBJ: %s", nameWithoutExt), 0.30);
		file += strlen(file) + 1;
	}
}

const char *CG_ConfigString(int index) {
	iferr(index < 0 || index >= MAX_CONFIGSTRINGS);
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[index];
}

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
static void CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum) {
	const char *s;
	
	// init cvars and commands
	ST_RegisterCvars();
	CG_CreateCvars();
	CG_InitConsoleCommands();

	// clear state
	memset(&cgs, 0, sizeof(cgs));
	memset(&cg, 0, sizeof(cg));
	memset(cg_entities, 0, sizeof(cg_entities));
	memset(cg_weapons, 0, sizeof(cg_weapons));
	memset(cg_items, 0, sizeof(cg_items));

	CG_LoadingString("game core", 0.10);

	cg.clientNum = clientNum;
	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	// load a few needed things before we do any screen updates
	ST_InitCGUI("default");

	CG_LoadingString("sourcetech.script", 0.20);

	cgs.redflag = cgs.blueflag = -1; // For compatibily, default to unset for
	cgs.flagStatus = -1;

	// get the gamestate from the client system
	trap_GetGameState(&cgs.gameState);
	s = CG_ConfigString(CS_LEVEL_START_TIME);
	cgs.levelStartTime = atoi(s);
	CG_ImportModelsOBJ();
	CG_ParseServerinfo();

	// load the new map
	CG_LoadingString("collision map", 0.40);
	trap_CM_LoadMap(cgs.mapname);
	cg.loading = qtrue;
	CG_LoadingString("sounds", 0.50);
	CG_RegisterSounds();
	CG_LoadingString("graphics", 0.60);
	CG_RegisterGraphics();
	CG_LoadingString("clients", 0.80);
	CG_RegisterClients(); // if low on memory, some clients will be deferred
	cg.loading = qfalse;  // future players will be deferred
	CG_LoadingString("entities", 0.90);
	CG_InitLocalEntities();
	CG_InitMarkPolys();

	// Make sure we have update values (scores)
	CG_SetConfigValues();
	CG_LoadingString("Loaded!", 1.00);
	CG_LoadingString("", 0.0);
	trap_S_ClearLoopingSounds(qtrue);

	// remove the last loading update
	cg.infoScreenText[0] = 0;
	cg.infoScreenValue = 0.0;

	trap_Cmd(EXEC_INSERT, "exec scripts/tools/create.cfg\n");
	trap_Cmd(EXEC_INSERT, va("weapon %i\n", WP_TOOLGUN));
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
static void CG_Shutdown(void) {
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
}
