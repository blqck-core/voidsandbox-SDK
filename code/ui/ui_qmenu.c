// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../qcommon/js_local.h"

sfxHandle_t menu_move_sound;
sfxHandle_t menu_out_sound;
sfxHandle_t menu_buzz_sound;
sfxHandle_t menu_null_sound;

static qhandle_t sliderBar;
static qhandle_t sliderButton_0;
static qhandle_t sliderButton_1;

static void MField_CharEvent(mfield_t *edit, int ch);

static qboolean Menu_CursorInRect(int x, int y, int width, int height) {
	if(uis.cursorx < x || uis.cursory < y || uis.cursorx > x + width || uis.cursory > y + height) return qfalse;
	return qtrue;
}

qboolean Menu_CursorOnItem(int id) {
	if(uis.cursorx < uis.items[id].x || uis.cursory < uis.items[id].y || uis.cursorx > uis.items[id].x + uis.items[id].w || uis.cursory > uis.items[id].y + uis.items[id].h) return qfalse;
	return qtrue;
}

static item_t *UI_FindItem(const char *pickupName) {
	item_t *it;

	for(it = gameInfoItems + 1; it->classname; it++) {
		if(!Q_stricmp(it->pickup_name, pickupName)) return it;
	}

	return NULL;
}

static item_t *UI_FindItemClassname(const char *classname) {
	item_t *it;

	for(it = gameInfoItems + 1; it->classname; it++) {
		if(!Q_stricmp(it->classname, classname)) return it;
	}

	return NULL;
}

#define CB_NONE 0
#define CB_COMMAND 1
#define CB_VARIABLE 2
#define CB_FUNC 3

static void DrawTip(const char *text) {
	ST_DrawRoundedRect((uis.cursorx - BASEFONT_INDENT) + 17, uis.cursory - 21, ST_StringWidth(text, 1.00)+BASEFONT_INDENT*2, BASEFONT_HEIGHT + 4, 0, cgui.colors[JSCOLOR_TIPSHADOW]);
	ST_DrawRoundedRect((uis.cursorx - BASEFONT_INDENT) + 18, uis.cursory - 20, ST_StringWidth(text, 1.00)+BASEFONT_INDENT*2, BASEFONT_HEIGHT + 4, 0, cgui.colors[JSCOLOR_TIP]);
	ST_DrawString(uis.cursorx + 18, uis.cursory - 18, text, 0, cgui.colors[JSCOLOR_TIPTEXT], 1.00);
}

static float SliderProgress(float value, float min, float max) {
    float result;
    float clamp;
    
    if(value < min) clamp = min;
    else if(value > max) clamp = max;
    else clamp = value;
    
    result = (clamp - min) / (max - min);
    
    if(result <= 0.0f) return 0.0f;
    if(result >= 1.0f) return 1.0f;
    
    return result;
}

static float SliderValue(float min, float max, float x, float w) {
    float local_x = (float)uis.cursorx - x;
    
    if(local_x < 0.0f) local_x = 0.0f;
    if(local_x > w) local_x = w;
    
    return min + (local_x / w) * (max - min);
}

static sfxHandle_t UI_GeneralCallback(menuelement_s *item, int key) {
	switch(item->type) {
    case MTYPE_BUTTON:
        if(strlen(item->action)) trap_Cmd(EXEC_INSERT, (item->action));
        break;
    case MTYPE_CHECKBOX:
        if(item->value > 0) item->value = 0;
        else item->value = 1;
        if(strlen(item->action)) cvarSet(item->action, va("%f", item->value));
        break;
	case MTYPE_SLIDER:
        switch(key) {
	    case K_MOUSE1: if(Menu_CursorOnItem(item->id) && !(item->flags & QMF_INACTIVE)) 
	        if(item->mode == EMODE_INT) item->value = (int)(SliderValue(item->min, item->max, item->x+item->corner, item->w-(item->corner*2)));
	        if(item->mode == EMODE_FLOAT) item->value = SliderValue(item->min, item->max, item->x+item->corner, item->w-(item->corner*2)); 
	        break;
	    }
        if(strlen(item->action)) cvarSet(item->action, va("%f", item->value));
        break;
	}
	JS_MenuCallback(item->id);
	return menu_move_sound;
}

static void BText_Init(menuelement_s *t) { t->generic.flags |= QMF_INACTIVE; }

static void Text_Draw(menuelement_s *t) {
	int x;
	int y;
	float *color;

	x = t->generic.x;
	y = t->generic.y;

	if(t->generic.flags & QMF_GRAYED)
		color = color_disabled;
	else
		color = t->color;

	ST_DrawString(x, y, t->string, t->style, color, t->size);
}

static void Button_Draw(menuelement_s *e) {
    float text_y = e->y+(e->h-(BASEFONT_HEIGHT*e->size))*0.5;
    int tcolor;

	tcolor = Menu_CursorOnItem(e->id) ? JSCOLOR_SELECTED : e->colortext;
	
    ST_DrawRoundedRect(e->x, e->y, e->w, e->h, e->corner, cgui.colors[e->colorbg]);
    if(e->style & UI_CENTER) ST_DrawString(e->x+(e->w*0.5), text_y, e->text, UI_CENTER, cgui.colors[tcolor], e->size);
    else if(e->style & UI_RIGHT) ST_DrawString((e->x+e->w)-e->margin, text_y, e->text, UI_RIGHT, cgui.colors[tcolor], e->size);
    else ST_DrawString(e->x+e->margin, text_y, e->text, UI_LEFT, cgui.colors[tcolor], e->size);
}

static void Checkbox_Draw(menuelement_s *e) {
    float text_y = e->y+(e->h-(BASEFONT_HEIGHT*e->size))*0.5;
    int tcolor, scolor;
    
	tcolor = Menu_CursorOnItem(e->id) ? JSCOLOR_SELECTED : e->colortext;
	scolor = e->value ? JSCOLOR_ENABLED : JSCOLOR_DISABLED;
	
    ST_DrawRoundedRect(e->x, e->y, e->w, e->h, e->corner, cgui.colors[e->colorbg]);
    ST_DrawRoundedRect((e->x+e->w)-(e->h*0.90), (e->y+(e->h*0.5))-e->h*0.35, e->h*0.70, e->h*0.70, e->corner, cgui.colors[scolor]);
    ST_DrawString(e->x+e->margin, text_y, e->text, UI_LEFT, cgui.colors[tcolor], e->size);
}

static void Slider_Draw(menuelement_s *e) {
    float text_y = e->y+(e->h-(BASEFONT_HEIGHT*e->size))*0.5;
    int tcolor;
    
	tcolor = Menu_CursorOnItem(e->id) ? JSCOLOR_SELECTED : e->colortext;
	
    ST_DrawRoundedRect(e->x, e->y, e->w, e->h, e->corner, cgui.colors[e->colorbg]);
    ST_DrawRoundedRect(e->x+e->corner, (e->y+e->h)-e->h*0.20, e->w-(e->corner*2), e->h*0.10, 0, cgui.colors[JSCOLOR_DISABLED]);
    ST_DrawRoundedRect(e->x+e->corner, (e->y+e->h)-e->h*0.20, (e->w-(e->corner*2))*SliderProgress(e->value, e->min, e->max), e->h*0.10, 0, cgui.colors[JSCOLOR_ENABLED]);
    ST_DrawString(e->x+e->margin, text_y, e->text, UI_LEFT, cgui.colors[tcolor], e->size);
    if(Menu_CursorOnItem(e->id)) DrawTip(va("%f", e->value));
}

static void Action_Draw(menuelement_s *e) {
    float text_y = e->y+(e->h-(BASEFONT_HEIGHT*e->size))*0.5;
    int tcolor, scolor;
    
	tcolor = Menu_CursorOnItem(e->id) ? JSCOLOR_SELECTED : e->colortext;
	scolor = e->value ? JSCOLOR_ENABLED : JSCOLOR_DISABLED;
	
    ST_DrawRoundedRect(e->x, e->y, e->w, e->h, e->corner, cgui.colors[e->colorbg]);
    ST_DrawRoundedRect((e->x+e->w)-(e->h*0.90), (e->y+(e->h*0.5))-e->h*0.35, e->h*0.70, e->h*0.70, e->corner, cgui.colors[scolor]);
    ST_DrawString(e->x+e->margin, text_y, e->text, UI_LEFT, cgui.colors[tcolor], e->size);
}

static void Bitmap_Init(menuelement_s *b) {
	int x;
	int y;
	int w;
	int h;

	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h = b->height;
	if(w < 0) {
		w = -w;
	}
	if(h < 0) {
		h = -h;
	}

	if(b->generic.flags & QMF_RIGHT_JUSTIFY) {
		x = x - w;
	} else if(b->generic.flags & QMF_CENTER_JUSTIFY) {
		x = x - w / 2;
	}

	b->generic.left = x;
	b->generic.right = x + w;
	b->generic.top = y;
	b->generic.bottom = y + h;

	b->shader = 0;
	b->focusshader = 0;
}

static void Bitmap_Draw(menuelement_s *b) {
	float x;
	float y;
	float w;
	float h;
	vec4_t tempcolor;
	float *color;

	tempcolor[0] = tempcolor[1] = tempcolor[2] = tempcolor[3] = 1.00;

	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h = b->height;

	if(b->generic.flags & QMF_RIGHT_JUSTIFY) {
		x = x - w;
	} else if(b->generic.flags & QMF_CENTER_JUSTIFY) {
		x = x - w / 2;
	}

	// used to refresh shader
	if(b->string && !b->shader) {
		b->shader = trap_R_RegisterShaderNoMip(b->string);
		if(!b->shader && b->errorpic) b->shader = trap_R_RegisterShaderNoMip(b->errorpic);
	}

	if(b->focuspic && !b->focusshader) b->focusshader = trap_R_RegisterShaderNoMip(b->focuspic);

	if(b->generic.flags & QMF_GRAYED) {
		if(b->shader) {
			trap_R_SetColor(color_disabled);
			UI_DrawHandlePic(x, y, w, h, b->shader);
			trap_R_SetColor(NULL);
		}
	} else {
		if(b->shader) UI_DrawHandlePic(x, y, w, h, b->shader);

		if((b->generic.flags & QMF_PULSE) || (b->generic.flags & QMF_PULSEIFFOCUS) && (Menu_CurrentItem() == b)) {
			color = tempcolor;
			color[3] = 0.5 + 0.5 * sin(uis.realtime / PULSE_DIVISOR);

			trap_R_SetColor(color);
			UI_DrawHandlePic(x, y, w, h, b->focusshader);
			trap_R_SetColor(NULL);
		} else if((b->generic.flags & QMF_HIGHLIGHT) || ((b->generic.flags & QMF_HIGHLIGHT_IF_FOCUS) && (Menu_CurrentItem() == b))) {
			UI_DrawHandlePic(x, y, w, h, b->focusshader);
		}
	}
}

static void Slider_Init(menuelement_s *s) {
	int len;

	// calculate bounds
	if(s->string)
		len = strlenru(s->string);
	else
		len = 0;

	s->generic.left = (s->generic.x - 4);
	s->generic.right = (s->generic.x + 4) + (SLIDER_RANGE * BASEFONT_INDENT);
	s->generic.top = s->generic.y;
	s->generic.bottom = s->generic.y + BASEFONT_HEIGHT;
}

static void SpinControl_Init(menuelement_s *s) {
	int len;
	int l;
	const char *str;

	if(s->string)
		len = strlenru(s->string) * BASEFONT_INDENT;
	else
		len = 0;

	s->generic.left = s->generic.x - BASEFONT_INDENT - len;

	len = s->numitems = 0;
	while((str = s->itemnames[s->numitems]) != 0) {
		l = strlenru(str);

		if(l > len) len = l;

		s->numitems++;
	}

	s->generic.top = s->generic.y;
	s->generic.right = s->generic.x + (len + 1) * BASEFONT_INDENT;
	s->generic.bottom = s->generic.y + BASEFONT_HEIGHT;
}

static sfxHandle_t SpinControl_Key(menuelement_s *s, int key) {
	sfxHandle_t sound;

	sound = 0;
	switch(key) {
	case K_MOUSE1:
		s->curvalue++;
		if(s->curvalue >= s->numitems) s->curvalue = 0;
		sound = menu_move_sound;
		break;

	case K_LEFTARROW:
		if(s->curvalue > 0) {
			s->curvalue--;
			sound = menu_move_sound;
		} else
			sound = menu_buzz_sound;
		break;

	case K_RIGHTARROW:
		if(s->curvalue < s->numitems - 1) {
			s->curvalue++;
			sound = menu_move_sound;
		} else
			sound = menu_buzz_sound;
		break;
	}

	if(sound && s->generic.callback) s->generic.callback(s, QM_ACTIVATED);
	//UI_GeneralCallback(s);
	
	return (sound);
}

static void SpinControl_Draw(menuelement_s *s) {
	float *color;
	int x, y;
	qboolean focus;
	int style;

	style = 0;

	x = s->generic.x;
	y = s->generic.y;

	focus = (s->generic.parent->cursor == s->generic.menuPosition);

	if(s->generic.flags & QMF_GRAYED) {
		color = color_disabled;
	} else if(focus) {
		color = color_highlight;
		style |= UI_PULSE;
	} else {
		color = color_white;
	}

	ST_DrawString(x - BASEFONT_INDENT * 2, y, s->string, style | UI_RIGHT, color, 1.00);
	ST_DrawString(x, y, s->itemnames[s->curvalue], style | UI_LEFT, color, 1.00);
}

static void ScrollList_Init(menuelement_s *l) {
	int w;

	l->oldvalue = 0;
	l->curvalue = 0;
	l->top = 0;

	if(!l->columns) l->columns = 1;

	if(l->generic.style <= LST_ICONS) w = (l->width * l->columns) * (BASEFONT_INDENT * l->size);
	if(l->generic.style == LST_GRID) w = l->width * l->columns;

	l->generic.left = l->generic.x;
	l->generic.top = l->generic.y;
	l->generic.right = l->generic.x + w;
	if(l->generic.style <= LST_ICONS) l->generic.bottom = l->generic.y + l->height * (BASEFONT_HEIGHT * l->size);
	if(l->generic.style == LST_GRID) l->generic.bottom = l->generic.y + l->width * l->height;

	if(l->generic.flags & QMF_CENTER_JUSTIFY) {
		l->generic.left -= w / 2;
		l->generic.right -= w / 2;
	}
}

sfxHandle_t ScrollList_Key(menuelement_s *l, int key) {
	static int clicktime = 0;
	int x, y, cursorx, cursory, column, index, clickdelay;

	switch(key) {
	case K_MOUSE1:
	case K_ENTER:
		if(l->generic.flags & QMF_HASMOUSEFOCUS) {
			int item_w, item_h;
			int total_w;

			x = l->generic.x;
			y = l->generic.y;

			if(l->generic.style == LST_GRID) {
				item_w = l->width;
				item_h = l->width;
				total_w = item_w * l->columns;
			} else {
				item_w = (BASEFONT_INDENT * l->size) * l->width;
				item_h = BASEFONT_HEIGHT * l->size;
				total_w = l->width * l->columns * item_w;
			}

			if(Menu_CursorInRect(x, y, total_w, l->height * item_h)) {
				cursorx = (uis.cursorx - x) / item_w;
				cursory = (uis.cursory - y) / item_h;

				column = cursorx;
				index = cursory * l->columns + column;

				if(l->top + index < l->numitems) {
					l->oldvalue = l->curvalue;
					l->curvalue = l->top + index;

					clickdelay = uis.realtime - clicktime;
					clicktime = uis.realtime;

					if(l->oldvalue != l->curvalue) {
						if(l->generic.callback) {
							l->generic.callback(l, QM_GOTFOCUS);
						}
						return menu_move_sound;
					} else {
						if((clickdelay < 350) && !(l->generic.flags & (QMF_GRAYED | QMF_INACTIVE))) {
							//return UI_GeneralCallback(l);
						}
					}
				}
				return menu_null_sound;
			}
		}
		break;

	case K_MWHEELUP:
		if(l->columns <= 1) {
			if(l->top <= 0) {
				l->top = 0;
			} else {
				l->top -= 1;
			}
			return menu_null_sound;
		}
		if(l->top - l->columns >= 0) {
			l->top -= l->columns;
		} else {
			return menu_buzz_sound;
		}

		if(l->generic.callback) {
			l->generic.callback(l, QM_GOTFOCUS);
		}

		return menu_move_sound;

	case K_MWHEELDOWN:
		if(l->columns <= 1) {
			if(l->top + l->height >= l->numitems) {
				int new_top = l->numitems - l->height;
				l->top = new_top > 0 ? new_top : 0;
			} else {
				l->top += 1;
			}
			return menu_null_sound;
		}
		if(l->top + (l->columns * l->height) < l->numitems) {
			l->top += l->columns;
		} else {
			return menu_buzz_sound;
		}

		if(l->generic.callback) {
			l->generic.callback(l, QM_GOTFOCUS);
		}

		return menu_move_sound;
	}

	return (menu_buzz_sound);
}

static void UI_ServerPlayerIcon(const char *modelAndSkin, char *iconName, int iconNameMaxSize) {
	char *skin;
	char model[MAX_QPATH];

	Q_StringCopy(model, modelAndSkin, sizeof(model));
	skin = strrchr(model, '/');
	if(skin) {
		*skin++ = '\0';
	} else {
		skin = "default";
	}

	Com_sprintf(iconName, iconNameMaxSize, "models/players/%s/icon_%s.tga", model, skin);

	if(!trap_R_RegisterShaderNoMip(iconName) && Q_stricmp(skin, "default") != 0) {
		Com_sprintf(iconName, iconNameMaxSize, "models/players/%s/icon_default.tga", model);
	}
}

static void DrawListItemImage(int x, int y, int w, int h, const char *path, menuelement_s *l, const char *itemname) {
	const char *info;
	char pic[MAX_QPATH];
	item_t *it;

	l->generic.shader = trap_R_RegisterShaderNoMip(path);
	if(l->generic.shader) {
		UI_DrawHandlePic(x, y, w, h, l->generic.shader);
		return;
	}

	l->generic.model = trap_R_RegisterModel(path);
	if(l->generic.model) {
		UI_DrawModelElement(x, y, w, h, path, l->range);
		return;
	}

	info = UI_GetBotInfoByName(itemname);
	UI_ServerPlayerIcon(Info_ValueForKey(info, "model"), pic, MAX_QPATH);
	l->generic.shader = trap_R_RegisterShaderNoMip(pic);
	if(l->generic.shader) {
		UI_DrawHandlePic(x, y, w, h, l->generic.shader);
		return;
	}

	it = UI_FindItem(itemname);
	if(it && it->icon && it->classname) {
		UI_DrawHandlePic(x, y, w, h, trap_R_RegisterShaderNoMip(it->icon));
		return;
	}

	it = UI_FindItemClassname(itemname);
	if(it && it->world_model && it->classname) {
		l->generic.model = trap_R_RegisterModel(it->world_model);
		if(l->generic.model) {
			UI_DrawModelElement(x, y, w, h, it->world_model, l->range);
			return;
		}
	}
}

static void UI_DrawListItemSelection(menuelement_s *l, int i, int x, int y, int item_h, int grid_w) {
	int u = x;

	if(i != l->curvalue) return;

	if(l->generic.flags & QMF_CENTER_JUSTIFY) {
		u -= ((l->width * BASEFONT_INDENT * l->size) / 2 + 1);
	}

	if(l->generic.style <= LST_ICONS) {
		ST_DrawRoundedRect(u, y, (l->width * BASEFONT_INDENT) * l->size, item_h, 0, color_select);
	} else if(l->generic.style == LST_GRID) {
		ST_DrawRoundedRect(u, y, grid_w, grid_w, l->corner, color_select);
	}
}

static void ScrollList_Draw(menuelement_s *l) {
	const int item_h = BASEFONT_HEIGHT * l->size;
	const int grid_w = l->width;
	const int icon_w = BASEFONT_HEIGHT * l->size;
	int column, base, i, style;
	float *color;
	const char *item;

	int x = l->generic.x;
	qboolean hasfocus = (l->generic.parent->cursor == l->generic.menuPosition);
	float select_x = 0, select_y = 0;
	int select_i = -1;

	for(column = 0; column < l->columns; column++) {
		int y = l->generic.y;

		for(base = 0; base < l->height; base++) {
			i = (base * l->columns + column) + l->top;
			if(i >= l->numitems) break;

			item = l->itemnames[i];
			style = UI_LEFT;
			if(l->generic.flags & QMF_CENTER_JUSTIFY) style |= UI_CENTER;
			color = l->color;

			switch(l->generic.style) {
			case LST_SIMPLE:
				UI_DrawListItemSelection(l, i, x, y, item_h, grid_w);
				ST_DrawString(x, y, item, style, color, l->size);
				break;

			case LST_ICONS:
				UI_DrawListItemSelection(l, i, x, y, item_h, grid_w);
				ST_DrawString(x + item_h, y, item, style, color, l->size);
				DrawListItemImage(x, y, item_h, item_h, va("%s/%s", l->string, item), l, item);
				break;

			case LST_GRID:
				if(l->padding_x || l->padding_y) {
					if(!l->drawText) UI_DrawListItemSelection(l, i, x, y, item_h, grid_w);
					if(l->drawText) UI_DrawListItemSelection(l, i, x, y + l->padding_y * 0.50, item_h, grid_w);
				}
				DrawListItemImage(x + l->padding_x, y + l->padding_y, grid_w - (l->padding_x * 2), grid_w - (l->padding_y * 2), va("%s/%s", l->string, item), l, item);
				if(l->drawText) ST_DrawString(x + (grid_w * 0.5), (y + grid_w) - (l->padding_y * 0.80), item, UI_CENTER, color, 0.75);

				if(Menu_CursorInRect(x, y, grid_w, grid_w) && hasfocus) {
					select_x = x;
					select_y = y;
					select_i = i;
				}
				if(!l->padding_x && !l->padding_y) UI_DrawListItemSelection(l, i, x, y, item_h, grid_w);
				break;
			}

			y += (l->generic.style == LST_GRID) ? grid_w : item_h;
		}

		x += (l->generic.style == LST_GRID) ? grid_w : (l->width * BASEFONT_INDENT * l->size);
	}

	if(l->generic.style == LST_GRID && hasfocus && select_i >= 0 && l->itemnames[select_i]) {
		const char *item = l->itemnames[select_i];
		DrawTip(item);
	}
}

static void MField_Draw(mfield_t *edit, int x, int y, qboolean focus, vec4_t color, float size) {
	int len, esc;
	int drawLen;
	int prestep;
	char str[MAX_STRING_CHARS];

	drawLen = edit->widthInChars;
	len = strlen(edit->buffer) + 1;
	esc = ST_ColorEscapes(edit->buffer) * 2;

	// guarantee that cursor will be visible
	if(len <= drawLen) {
		prestep = 0;
	} else {
		if(edit->scroll + drawLen > len) {
			edit->scroll = len - drawLen;
			if(edit->scroll < 0) {
				edit->scroll = 0;
			}
		}
		prestep = edit->scroll;
	}

	if(prestep + drawLen > len) drawLen = len - prestep;

	if(drawLen >= MAX_STRING_CHARS) trap_Error("drawLen >= MAX_STRING_CHARS");
	memcpy(str, edit->buffer + prestep, drawLen);
	str[drawLen] = 0;

	ST_DrawString(x, y, str, UI_LEFT, color, size);

	// draw the cursor
	if(!focus) return;

	ST_DrawChar(x + (edit->cursor - prestep - esc) * BASEFONT_INDENT, y, 10, UI_LEFT, color, 1.00);
}

static void MField_Paste(mfield_t *edit) {
	char pasteBuffer[64];
	int pasteLen, i;

	trap_GetClipboardData(pasteBuffer, 64);

	// send as if typed, so insert / overstrike works properly
	pasteLen = strlen(pasteBuffer);
	for(i = 0; i < pasteLen; i++) {
		MField_CharEvent(edit, pasteBuffer[i]);
	}
}

static void MField_Clear(mfield_t *edit) {
	edit->buffer[0] = 0;
	edit->cursor = 0;
	edit->scroll = 0;
}

static void MField_CharEvent(mfield_t *edit, int ch) {
	int len;

	if(ch == 'v' - 'a' + 1) { // ctrl-v is paste
		MField_Paste(edit);
		return;
	}

	if(ch == 'c' - 'a' + 1) { // ctrl-c clears the field
		MField_Clear(edit);
		return;
	}

	len = strlen(edit->buffer);

	if(ch == 'h' - 'a' + 1) { // ctrl-h is backspace
		if(edit->cursor > 0) {
			memmove(edit->buffer + edit->cursor - 1, edit->buffer + edit->cursor, len + 1 - edit->cursor);
			edit->cursor--;
			if(edit->cursor < edit->scroll) {
				edit->scroll--;
			}
		}
		return;
	}

	if(ch == 'a' - 'a' + 1) { // ctrl-a is home
		edit->cursor = 0;
		edit->scroll = 0;
		return;
	}

	if(ch == 'e' - 'a' + 1) { // ctrl-e is end
		edit->cursor = len;
		edit->scroll = edit->cursor - edit->widthInChars + 1;
		if(edit->scroll < 0) edit->scroll = 0;
		return;
	}

	//
	// ignore any other non printable chars
	//
	if(ch == -48) {
		return;
	}

	if((edit->cursor == MAX_EDIT_LINE - 1) || (edit->maxchars && edit->cursor >= edit->maxchars)) return;

	edit->buffer[edit->cursor] = ch;
	if(!edit->maxchars || edit->cursor < edit->maxchars - 1) edit->cursor++;

	if(edit->cursor >= edit->widthInChars) {
		edit->scroll++;
	}

	if(edit->cursor == len + 1) {
		edit->buffer[edit->cursor] = 0;
	}
}

static void MField_KeyDownEvent(mfield_t *edit, int key) {
	int len;

	len = strlen(edit->buffer);

	if(key == K_DEL || key == K_KP_DEL) {
		if(edit->cursor < len) {
			memmove(edit->buffer + edit->cursor, edit->buffer + edit->cursor + 1, len - edit->cursor);
		}
		return;
	}

	if(key == K_RIGHTARROW) {
		if(edit->cursor < len) {
			edit->cursor++;
		}
		if(edit->cursor >= edit->scroll + edit->widthInChars && edit->cursor <= len) {
			edit->scroll++;
		}
		return;
	}

	if(key == K_LEFTARROW) {
		if(edit->cursor > 0) {
			edit->cursor--;
		}
		if(edit->cursor < edit->scroll) {
			edit->scroll--;
		}
		return;
	}
}

static void MenuField_Init(menuelement_s *m) {
	int w, h;

	w = BASEFONT_INDENT * m->size;
	h = BASEFONT_HEIGHT * m->size;

	m->generic.left = m->generic.x - (strlen(m->string) + 1) * w;
	m->generic.top = m->generic.y;
	m->generic.right = m->generic.x + w + m->field.widthInChars * w;
	m->generic.bottom = m->generic.y + h;
}

static void MenuField_Draw(menuelement_s *f) {
	int x;
	int y;
	float *color;
	qboolean focus = qfalse;

	x = f->generic.x;
	y = f->generic.y;

	if(Menu_CurrentItem() == f) {
		focus = qtrue;
	}

	if(f->generic.flags & QMF_GRAYED) {
		color = color_disabled;
	} else {
		color = f->color;
	}

	if(f->string) ST_DrawString(x - BASEFONT_INDENT * 2, y, f->string, UI_RIGHT, color, 1.00);

	MField_Draw(&f->field, x, y, focus, color, 1.00);
}

static sfxHandle_t MenuField_Key(menuelement_s *m, int *key) {
	int keycode;
	static int lastKeypress = 0;

	keycode = *key;

	switch(keycode) {
	case K_ENTER:
	case K_MOUSE1: break;

	case K_TAB:
	case K_DOWNARROW:
	case K_UPARROW: break;

	default:
		if(keycode & K_CHAR_FLAG) {
			keycode &= ~K_CHAR_FLAG;
			MField_CharEvent(&m->field, keycode);
		} else {
			MField_KeyDownEvent(&m->field, keycode);
		}
		break;
	}
	lastKeypress = uis.realtime;

	return (0);
}

static void Menu_CursorMoved(menuframework_s *m) {
	void (*callback)(void *self, int notification);

	if(m->cursor_prev == m->cursor) return;

	if(m->cursor_prev >= 0 && m->cursor_prev < m->nitems) {
		callback = ((menucommon_s *)(m->items[m->cursor_prev]))->callback;
		if(callback) callback(m->items[m->cursor_prev], QM_LOSTFOCUS);
	}

	if(m->cursor >= 0 && m->cursor < m->nitems) {
		callback = ((menucommon_s *)(m->items[m->cursor]))->callback;
		if(callback) callback(m->items[m->cursor], QM_GOTFOCUS);
	}
}

void Menu_SetCursor(menuframework_s *m, int cursor) {
	if(((menucommon_s *)(m->items[cursor]))->flags & (QMF_GRAYED | QMF_INACTIVE)) {
		// cursor can't go there
		return;
	}

	m->cursor_prev = m->cursor;
	m->cursor = cursor;

	Menu_CursorMoved(m);
}

static void Menu_AdjustCursor(menuframework_s *m, int dir) {
	menucommon_s *item = NULL;
	qboolean wrapped = qfalse;

wrap:
	while(m->cursor >= 0 && m->cursor < m->nitems) {
		item = (menucommon_s *)m->items[m->cursor];
		if((item->flags & (QMF_GRAYED | QMF_INACTIVE))) {
			m->cursor += dir;
		} else {
			break;
		}
	}

	if(dir == 1) {
		if(m->cursor >= m->nitems) {
			if(wrapped) {
				m->cursor = m->cursor_prev;
				return;
			}
			m->cursor = 0;
			wrapped = qtrue;
			goto wrap;
		}
	} else {
		if(m->cursor < 0) {
			if(wrapped) {
				m->cursor = m->cursor_prev;
				return;
			}
			m->cursor = m->nitems - 1;
			wrapped = qtrue;
			goto wrap;
		}
	}
}

void Menu_ElementsDraw(void) {
	int i;

	for(i = 0; i < MAX_MENUITEMS; i++) {
		if(uis.items[i].flags & QMF_HIDDEN) continue;

		switch(uis.items[i].type) {
		    case MTYPE_BUTTON: Button_Draw(&uis.items[i]); break;
		    case MTYPE_CHECKBOX: Checkbox_Draw(&uis.items[i]); break;
		    case MTYPE_SLIDER: Slider_Draw(&uis.items[i]); break;
			case MTYPE_FIELD: MenuField_Draw(&uis.items[i]); break;
			case MTYPE_SPINCONTROL: SpinControl_Draw(&uis.items[i]); break;
			case MTYPE_ACTION: Action_Draw(&uis.items[i]); break;
			case MTYPE_BITMAP: Bitmap_Draw(&uis.items[i]); break;
			case MTYPE_SCROLLLIST: ScrollList_Draw(&uis.items[i]); break;
			case MTYPE_TEXT: Text_Draw(&uis.items[i]); break;
		}
		
		if(uis.debug) {
			if(!(uis.items[i].flags & QMF_INACTIVE)) {
				ST_DrawRoundedRect(uis.items[i].x, uis.items[i].y, uis.items[i].w, uis.items[i].h, 0, color_dim);
			}
		}
	}
}

void *Menu_CurrentItem(void) {
	if(uis.currentItem < 0 || uis.currentItem >= MAX_MENUITEMS) return 0;
	return &uis.items[uis.currentItem];
}

sfxHandle_t Menu_DefaultKey(int key) {
	sfxHandle_t sound = 0;
	menuelement_s *item;

	// menu system keys
	switch(key) {
	case K_ESCAPE: UI_PopMenu(); return menu_out_sound;
    case K_F5: UI_HotReloadJS(); return menu_out_sound;
	}

	// route key stimulus to widget
	item = Menu_CurrentItem();
	if(item && !(item->flags & (QMF_INACTIVE))) {
		switch(item->type) {
		case MTYPE_SPINCONTROL: sound = SpinControl_Key(item, key); break;
		case MTYPE_SCROLLLIST: sound = ScrollList_Key(item, key); break;
		case MTYPE_FIELD:
			sound = MenuField_Key(item, &key);
			item->callback(item, QM_ACTIVATED);
			//UI_GeneralCallback(item);
			break;
		}

		if(sound) return sound;
	}

	// default handling
	switch(key) {
	case K_F11: uis.debug ^= 1; break;
	case K_F12: trap_Cmd(EXEC_APPEND, "screenshotJPEG\n"); break;
	case K_MOUSE1: if(Menu_CursorOnItem(item->id) && !(item->flags & QMF_INACTIVE)) return UI_GeneralCallback(item, key); break;
	case K_ENTER: if(!(item->flags & QMF_INACTIVE)) return UI_GeneralCallback(item, key); break;
	}
	
	JS_MenuKey(key);

	return sound;
}

void Menu_Cache(void) {
	uis.cursor = trap_R_RegisterShaderNoMip("menu/assets/3_cursor2");
	uis.rb_on = trap_R_RegisterShaderNoMip("menu/assets/switch_on");
	uis.rb_off = trap_R_RegisterShaderNoMip("menu/assets/switch_off");
	uis.menuWallpapers = trap_R_RegisterShaderNoMip("menu/animbg");
	menu_move_sound = trap_S_RegisterSound("sound/misc/menu2.wav", qfalse);
	menu_out_sound = trap_S_RegisterSound("sound/misc/menu3.wav", qfalse);
	menu_buzz_sound = trap_S_RegisterSound("sound/misc/menu4.wav", qfalse);
	menu_null_sound = -1;
	sliderBar = trap_R_RegisterShaderNoMip("menu/assets/slider2");
	sliderButton_0 = trap_R_RegisterShaderNoMip("menu/assets/sliderbutt_0");
	sliderButton_1 = trap_R_RegisterShaderNoMip("menu/assets/sliderbutt_1");
}

static void UI_FindButtonPic(menuelement_s *e, int pic) {
	switch(pic) {
	case AST_OSLOGO:
		e->string = "menu/sandbox_logo";
		e->focuspic = "menu/sandbox_logo";
		break;
	case AST_MOD:
		e->string = "menu/gamemode_default";
		e->focuspic = "menu/gamemode_default";
		break;
	case AST_LINK:
		e->string = "menu/officialsite";
		e->focuspic = "menu/officialsite";
		break;
	}
}

void UI_FillList(menuelement_s *e, char *location, char *itemsLocation, char *extension, char *names, int namesSize, char **configlist) {
	int i, len, validItems = 0;
	char *configname;
	qboolean skip = qfalse;

	e->string = itemsLocation;
	if(!strcmp(extension, "$image") || !strcmp(extension, "$sound")) {
		e->numitems = FS_List(location, "", names, namesSize);
	} else {
		e->numitems = FS_List(location, extension, names, namesSize);
	}
	e->itemnames = (const char **)configlist;

	if(e->numitems == 0) { // Empty folder
		strcpy(names, "Empty");
		e->numitems = 1;
	} else if(e->numitems > 65536) { // Limit
		e->numitems = 65536;
	}

	configname = names;
	for(i = 0; i < e->numitems; i++) {
		len = strlen(configname);

		skip = qfalse;

		if(!strcmp(extension, "$image")) {
			if(!(Q_stricmp(configname + len - 4, ".png") == 0 || Q_stricmp(configname + len - 4, ".jpg") == 0 || Q_stricmp(configname + len - 4, ".tga") == 0 || Q_stricmp(configname + len - 4, ".bmp") == 0)) skip = qtrue;
		} else if(!strcmp(extension, "$sound")) {
			if(!(Q_stricmp(configname + len - 4, ".wav") == 0 || Q_stricmp(configname + len - 4, ".ogg") == 0 || Q_stricmp(configname + len - 4, ".mp3") == 0)) skip = qtrue;
		}

		if(skip) {
			configname += len + 1;
			continue;
		}

		if(!strcmp(extension, "$image")) {
			if(Q_stricmp(configname + len - 4, ".png") == 0) configname[len - 4] = '\0';
			if(Q_stricmp(configname + len - 4, ".jpg") == 0) configname[len - 4] = '\0';
			if(Q_stricmp(configname + len - 4, ".tga") == 0) configname[len - 4] = '\0';
			if(Q_stricmp(configname + len - 4, ".bmp") == 0) configname[len - 4] = '\0';
		} else if(!strcmp(extension, "$sound")) {
			if(Q_stricmp(configname + len - 4, ".wav") == 0) configname[len - 4] = '\0';
			if(Q_stricmp(configname + len - 4, ".ogg") == 0) configname[len - 4] = '\0';
			if(Q_stricmp(configname + len - 4, ".mp3") == 0) configname[len - 4] = '\0';
		} else {
			if(Q_stricmp(configname + len - strlen(extension), extension) == 0) configname[len - strlen(extension)] = '\0';
		}

		e->itemnames[validItems++] = configname;
		configname += len + 1;
	}

	e->numitems = UI_CountFiles(location, extension);
}

int UI_CountFiles(const char *location, const char *extension) {
	char tempNames[32000];
	int count = 0;

	if(!strcmp(extension, "$image")) {
		count += FS_List(location, ".png", tempNames, sizeof(tempNames));
		count += FS_List(location, ".jpg", tempNames, sizeof(tempNames));
		count += FS_List(location, ".tga", tempNames, sizeof(tempNames));
		count += FS_List(location, ".bmp", tempNames, sizeof(tempNames));
		return count;
	}
	if(!strcmp(extension, "$sound")) {
		count += FS_List(location, ".wav", tempNames, sizeof(tempNames));
		count += FS_List(location, ".ogg", tempNames, sizeof(tempNames));
		count += FS_List(location, ".mp3", tempNames, sizeof(tempNames));
		return count;
	}

	return FS_List(location, extension, tempNames, sizeof(tempNames));
}

void UI_FillListFromArray(menuelement_s *e, char **configlist, char **items, int maxItems) {
	int i;

	e->itemnames = (const char **)configlist;
	e->numitems = 0;

	if(maxItems == 0 || !items) {
		static const char *emptyItem = "Empty";
		e->itemnames[0] = emptyItem;
		e->numitems = 1;
		return;
	}

	if(maxItems > 65536) maxItems = 65536;

	for(i = 0; i < maxItems; i++) {
		if(!items[i]) break;

		configlist[i] = (char *)items[i];
		e->itemnames[i] = configlist[i];
		e->numitems++;
	}
}

void UI_FillListOfItems(menuelement_s *e, char *names, int namesSize, char **configlist) {
	int i, len;
	char *itemName;
	char *out = names;

	e->string = "";
	e->itemnames = (const char **)configlist;

	for(i = 0; i < gameInfoItemsNum - 1; i++) {
		itemName = gameInfoItems[i + 1].pickup_name;

		if(!itemName[0]) continue;

		len = strlen(itemName);
		strcpy(out, itemName);
		e->itemnames[i] = out;
		out += len + 1;
	}
	e->numitems = gameInfoItemsNum - 1;
}

void UI_FillListPlayers(menuelement_s *e, char **configlist, char *names, int namesSize) {
	int i, count = 0;
	char *out = names;
	const char *name;
	int len;

	e->string = "";

	for(i = 0; i < MAX_CLIENTS; i++) {
		char info[MAX_INFO_STRING];
		trap_GetConfigString(CS_PLAYERS + i, info, sizeof(info));

		if(!info[0]) continue;

		name = Info_ValueForKey(info, "n");
		if(!name || !name[0]) continue;

		len = strlen(name);
		if(out + len + 1 >= names + namesSize) break;

		strcpy(out, name);
		Q_CleanStr(out);
		configlist[count] = out;

		out += len + 1;
		count++;
	}

	if(count == 0) {
		static const char *emptyItem = "Empty";
		e->itemnames = &emptyItem;
		e->numitems = 1;
	} else {
		e->itemnames = (const char **)configlist;
		e->numitems = count;
	}
}

int UI_ListPlayerCount(void) {
	int i, count = 0;
	const char *name;

	for(i = 0; i < MAX_CLIENTS; i++) {
		char info[MAX_INFO_STRING];
		trap_GetConfigString(CS_PLAYERS + i, info, sizeof(info));

		if(!info[0]) continue;

		name = Info_ValueForKey(info, "n");
		if(!name || !name[0]) continue;

		count++;
	}
	return count;
}

void UI_SetHitbox(int id, float x, float y, float w, float h) {
	ui.e[id].generic.left = x;
	ui.e[id].generic.right = x + w;
	ui.e[id].generic.top = y;
	ui.e[id].generic.bottom = y + h;
}

int UI_CButton(int id, float x, float y, float w, float h, char *text, int style, float size, int colortext, int colorbg, int corner, int margin, char *action) {
    if(id >= MAX_MENUITEMS) {
        trap_Print("^3Menu item limit exceeded");
        return -1;
    }
    uis.items[id].type = MTYPE_BUTTON;
    uis.items[id].id = id;
	uis.items[id].x = x;
	uis.items[id].y = y;
	uis.items[id].w = w;
	uis.items[id].h = h;
	Q_StringCopy(uis.items[id].text, text, UI_STRINGLENGTH);
	uis.items[id].style = style;
	uis.items[id].size = size;
	uis.items[id].colortext = colortext;
	uis.items[id].colorbg = colorbg;
	uis.items[id].corner = corner;
	uis.items[id].margin = margin;
	Q_StringCopy(uis.items[id].action, action, UI_ACTIONLENGTH);
	return id;
}

int UI_CCheckbox(int id, float x, float y, float w, float h, char *text, float size, int colortext, int colorbg, int corner, int margin, char *action) {
    if(id >= MAX_MENUITEMS) {
        trap_Print("^3Menu item limit exceeded");
        return -1;
    }
    uis.items[id].type = MTYPE_CHECKBOX;
    uis.items[id].id = id;
	uis.items[id].x = x;
	uis.items[id].y = y;
	uis.items[id].w = w;
	uis.items[id].h = h;
	Q_StringCopy(uis.items[id].text, text, UI_STRINGLENGTH);
	uis.items[id].style = 0;
	uis.items[id].size = size;
	uis.items[id].colortext = colortext;
	uis.items[id].colorbg = colorbg;
	uis.items[id].corner = corner;
	uis.items[id].margin = margin;
	Q_StringCopy(uis.items[id].action, action, UI_ACTIONLENGTH);
	
	uis.items[id].value = cvarFloat(uis.items[id].action);
	return id;
}

int UI_CSlider(int id, float x, float y, float w, float h, char *text, float size, int colortext, int colorbg, int corner, int margin, char *action, float min, float max, int mode) {
    if(id >= MAX_MENUITEMS) {
        trap_Print("^3Menu item limit exceeded");
        return -1;
    }
    uis.items[id].type = MTYPE_SLIDER;
    uis.items[id].id = id;
	uis.items[id].x = x;
	uis.items[id].y = y;
	uis.items[id].w = w;
	uis.items[id].h = h;
	Q_StringCopy(uis.items[id].text, text, UI_STRINGLENGTH);
	uis.items[id].style = 0;
	uis.items[id].size = size;
	uis.items[id].colortext = colortext;
	uis.items[id].colorbg = colorbg;
	uis.items[id].corner = corner;
	uis.items[id].margin = margin;
	Q_StringCopy(uis.items[id].action, action, UI_ACTIONLENGTH);
	
	uis.items[id].value = cvarFloat(uis.items[id].action);
	trap_Print(va("^4 Init slider with %f value", cvarFloat(uis.items[id].action)));
	uis.items[id].min = min;
	uis.items[id].max = max;
	uis.items[id].mode = mode;
	return id;
}

void UI_CSpinControl(menuelement_s *e, float x, float y, char *text, const char **list, char *var, void (*callback)(void *self, int event), int callid) {
	e->generic.type = MTYPE_SPINCONTROL;
	e->generic.x = x;
	e->generic.y = y;
	e->string = text;
	e->generic.callback = callback;
	e->generic.callid = callid;
	e->itemnames = list;
	if(var) {
		e->generic.general_cbtype = CB_VARIABLE;
		e->generic.var = var;
	}
	e->color = color_white;
}

void UI_CList(menuelement_s *e, float x, float y, float size, int h, int w, float pad_x, float pad_y, int style, qboolean drawText, int corner, float *color, void (*callback)(void *self, int event), int callid) {
	e->generic.type = MTYPE_SCROLLLIST;
	e->generic.style = style;
	e->generic.x = x;
	e->generic.y = y;
	e->padding_x = pad_x;
	e->padding_y = pad_y;
	e->width = size;
	e->height = h;
	e->columns = w;
	e->generic.callback = callback;
	e->generic.callid = callid;
	e->color = color;
	e->corner = corner;
	e->drawText = drawText;
	e->range = 80.0;
}

void UI_CField(menuelement_s *e, float x, float y, char *text, int w, int maxchars, float *color, char *var, void (*callback)(void *self, int event), int callid) {
	e->generic.type = MTYPE_FIELD;
	e->generic.x = x;
	e->generic.y = y;
	e->string = text;
	e->field.widthInChars = w;
	e->field.maxchars = maxchars;
	e->generic.callback = callback;
	e->generic.callid = callid;
	e->color = color;
	if(var) {
		e->generic.general_cbtype = CB_VARIABLE;
		e->generic.var = var;
	}
}

void UI_CText(menuelement_s *e, float x, float y, char *text, int style, float size) {
	e->generic.type = MTYPE_TEXT;
	e->generic.x = x;
	e->generic.y = y;
	e->size = size;
	e->string = text;
	e->color = color_white;

	if(style == UI_LEFT) {
		e->style = UI_LEFT;
	}
	if(style == UI_CENTER) {
		e->style = UI_CENTER;
	}
	if(style == UI_RIGHT) {
		e->style = UI_RIGHT;
	}
}

void UI_CPicture(menuelement_s *e, float x, float y, float w, float h, int pic, int flags, char *cmd, char *var, void (*func)(void), void (*callback)(void *self, int event), int callid) {
	UI_FindButtonPic(e, pic);

	e->generic.type = MTYPE_BITMAP;
	e->generic.flags = flags;
	e->generic.callback = callback;
	e->generic.callid = callid;
	if(cmd) {
		e->generic.general_cbtype = CB_COMMAND;
		e->generic.cmd = cmd;
	} else if(var) {
		e->generic.general_cbtype = CB_VARIABLE;
		e->generic.var = var;
	} else if(func) {
		e->generic.general_cbtype = CB_FUNC;
		e->generic.func = func;
	}
	e->generic.x = x;
	e->generic.y = y;
	e->width = w;
	e->height = h;
}
