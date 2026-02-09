// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

void TeleportPlayer(gentity_t *player, vec3_t origin, vec3_t angles, qboolean noKnockback) {
	gentity_t *tent;

	// use temp events at source and destination to prevent the effect
	// from getting dropped by a second player event
	if(player->client->sess.sessionTeam != TEAM_SPECTATOR) {
		tent = G_TempEntity(player->client->ps.origin, EV_PLAYER_TELEPORT_OUT);
		tent->s.clientNum = player->s.clientNum;

		tent = G_TempEntity(origin, EV_PLAYER_TELEPORT_IN);
		tent->s.clientNum = player->s.clientNum;
	}

	// unlink to make sure it can't possibly interfere with G_KillBox
	trap_UnlinkEntity(player);

	VectorCopy(origin, player->client->ps.origin);
	player->client->ps.origin[2] += 1;

	// spit the player out
	if(!noKnockback) {
		AngleVectors(angles, player->client->ps.velocity, NULL, NULL);
		VectorScale(player->client->ps.velocity, 400, player->client->ps.velocity);
		player->client->ps.pm_time = 160; // hold time
		player->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}

	// toggle the teleport bit so the client knows to not lerp
	player->client->ps.eFlags ^= EF_TELEPORT_BIT;

	// set angles
	SetClientViewAngle(player, angles);

	// kill anything at the destination
	if(player->client->sess.sessionTeam != TEAM_SPECTATOR) {
		G_KillBox(player);
	}

	// save results of pmove
	BG_PlayerStateToEntityState(&player->client->ps, &player->s, qtrue);

	// use the precise origin for linking
	VectorCopy(player->client->ps.origin, player->r.currentOrigin);

	if(player->client->sess.sessionTeam != TEAM_SPECTATOR) {
		trap_LinkEntity(player);
	}
}
