// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
static void G_WriteClientSessionData(gclient_t *client) {
	const char *s;
	const char *var;

	s = va("%i %i %i %i", client->sess.sessionTeam, client->sess.spectatorNum, client->sess.spectatorState, client->sess.spectatorClient);

	var = va("session%i", (int)(client - level.clients));
	cvarSet(var, s);
}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData(gclient_t *client) {
	char *s;
	const char *var;
	int spectatorState;
	int sessionTeam;

	var = va("session%i", (int)(client - level.clients));
	s = cvarString(var);

	sscanf(s, "%i %i %i %i", &sessionTeam, &client->sess.spectatorNum, &spectatorState, &client->sess.spectatorClient);

	client->sess.sessionTeam = (team_t)sessionTeam;
	client->sess.spectatorState = (spectatorState_t)spectatorState;
}

/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData(gclient_t *client, char *userinfo) {
	clientSession_t *sess;
	const char *value;

	sess = &client->sess;

	// initial team determination
	if(cvarInt("g_gametype") >= GT_TEAM) {
		sess->sessionTeam = PickTeam(-1);
		BroadcastTeamChange(client, -1);
	} else {
		value = Info_ValueForKey(userinfo, "team");
		if(value[0] == 's') {
			// a willing spectator, not a waiting-in-line
			sess->sessionTeam = TEAM_SPECTATOR;
		} else {
			switch(cvarInt("g_gametype")) {
			default:
			case GT_SANDBOX:
			case GT_FFA: sess->sessionTeam = TEAM_FREE; break;
			}
		}
	}

	sess->spectatorState = SPECTATOR_FREE;

	G_WriteClientSessionData(client);
}

void G_InitWorldSession(void) {
	char *s;
	int gt;

	// restore session from vQ3 session data
	s = cvarString("session");
	gt = atoi(s);

	// if the gametype changed since the last session, don't use any client sessions
	if(cvarInt("g_gametype") != gt) {
		level.newSession = qtrue;
		print("Gametype changed, clearing session data.\n");
	}
}

void G_WriteSessionData(void) {
	int i;

	cvarSet("session", va("%i", cvarInt("g_gametype")));

	for(i = 0; i < level.maxclients; i++) {
		if(level.clients[i].pers.connected == CON_CONNECTED) {
			G_WriteClientSessionData(&level.clients[i]);
		}
	}
}
