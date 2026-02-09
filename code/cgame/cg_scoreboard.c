// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

#define SCOREBOARD_X (0 - cgui.wideoffset)

#define SB_WIDTH (ICON_SIZE + 16 + ((BASEFONT_INDENT * 35) + (BASEFONT_INDENT * 6)) * 1.20)
#define SB_INTER_HEIGHT 16
#define SB_X (320 - SB_WIDTH / 2)

static void CG_DrawClientScore(int y, score_t *score) {
	char nameText[36];
	char scoreText[8];
	clientInfo_t *ci;

	if(score->client < 0 || score->client >= cgs.maxclients) return;

	if(score->client == cg.snap->ps.clientNum) {
		float hcolor[4];
		int rank;

		if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cgs.gametype >= GT_TEAM) {
			rank = -1;
		} else {
			rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
		}
		hcolor[0] = cvarFloat("cg_crosshairColorRed");
		hcolor[1] = cvarFloat("cg_crosshairColorGreen");
		hcolor[2] = cvarFloat("cg_crosshairColorBlue");
		hcolor[3] = 0.40;

		ST_DrawRoundedRect(SB_X + 4, y, SB_WIDTH - 8, ICON_SIZE, 0, hcolor);
	}

	ci = &cgs.clientinfo[score->client];
	CG_DrawHead(SB_X + 4, y, ICON_SIZE, ICON_SIZE, score->client);

	// draw the score line
	if(ci->team == TEAM_SPECTATOR) {
		Com_sprintf(nameText, sizeof(nameText), "%s", ci->name);
		Com_sprintf(scoreText, sizeof(scoreText), "SPECT");
	} else {
		Com_sprintf(nameText, sizeof(nameText), "%s", ci->name);
		Com_sprintf(scoreText, sizeof(scoreText), "%i", score->score);
	}

	ST_DrawString(SB_X + 8 + ICON_SIZE, y + 2, nameText, UI_LEFT | UI_DROPSHADOW, color_white, 1.20);
	ST_DrawString((SB_X + 8 + ICON_SIZE) + (BASEFONT_INDENT * 35) * 1.20, y + 2, scoreText, UI_LEFT | UI_DROPSHADOW, color_white, 1.20);
}

static int CG_TeamScoreboard(int y, team_t team, int maxClients, int lineHeight) {
	int i, count;
	score_t *score;
	qboolean localFinded;
	clientInfo_t *ci;

	count = 0;
	localFinded = qfalse;
	for(i = 0; i < cg.numScores && count < maxClients; i++) {
		score = &cg.scores[i];
		ci = &cgs.clientinfo[score->client];
		if(team != ci->team) continue;
		if(score->client == 0) {
			if(localFinded) continue;
			localFinded = qtrue;
		}
		CG_DrawClientScore(y + lineHeight * count, score);
		count++;
	}

	return count;
}

qboolean CG_DrawScoreboard(void) {
	int y, n1, n2;
	int maxClients;
	int lineHeight;

	if(!(cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD || cg.predictedPlayerState.pm_type == PM_INTERMISSION)) return qfalse;

	ST_DrawRoundedRect(320 - (SB_WIDTH / 2), 50 - 8, SB_WIDTH, SB_INTER_HEIGHT * 26, 6, color_dim);

	y = 50;
	lineHeight = SB_INTER_HEIGHT;
	if(cgs.gametype >= GT_TEAM) {
		maxClients = 24;
		y -= 5;
		y += lineHeight / 2;

		if(cg.teamScores[0] >= cg.teamScores[1]) {
			n1 = CG_TeamScoreboard(y, TEAM_RED, maxClients, lineHeight);
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_TeamScoreboard(y, TEAM_BLUE, maxClients, lineHeight);
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n2;
		} else {
			n1 = CG_TeamScoreboard(y, TEAM_BLUE, maxClients, lineHeight);
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_TeamScoreboard(y, TEAM_RED, maxClients, lineHeight);
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n2;
		}
		n1 = CG_TeamScoreboard(y, TEAM_SPECTATOR, maxClients, lineHeight);
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

	} else {
		maxClients = 25;
		n1 = CG_TeamScoreboard(y, TEAM_FREE, maxClients, lineHeight);
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
		n2 = CG_TeamScoreboard(y, TEAM_SPECTATOR, maxClients - n1, lineHeight);
		y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
	}

	return qtrue;
}
