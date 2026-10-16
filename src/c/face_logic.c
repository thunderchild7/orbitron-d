/* The SDK defines _TIME_H_, so <time.h> is empty and struct tm lives in pebble.h.
   This include stays outside an #ifdef the dependency scanner cannot see.
   Host tests pass -DHOST_TEST and use the system time.h instead. */
#ifndef HOST_TEST
#include <pebble.h>
#endif

#include "face_logic.h"

#include <stdio.h>
#include <string.h>

static const char *WEEKDAY_NAMES[NUM_LANG][7] = {
	{"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"},
	{"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"},
	{"Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado"},
	{"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"},
	{"Domenica", "Lunedì", "Martedì", "Mercoledì", "Giovedì", "Venerdì", "Sabato"},
	{"Söndag", "Måndag", "Tisdag", "Onsdag", "Torsdag", "Fredag", "Lördag"},
	{"Søndag", "Mandag", "Tisdag", "Onsdag", "Torsdag", "Fredag", "Lørdag"},
	{"Zondag", "Maandag", "Dinsdag", "Woensdag", "Donderdag", "Vrijdag", "Zaterdag"}
};

static const char *MONTH_NAMES[NUM_LANG][12] = {
	{"Jan.", "Feb.", "Mar.", "Apr.", "May", "June", "July", "Aug.", "Sept.", "Oct.", "Nov.", "Dec."},
	{"Jän.", "Feb.", "März", "Apr.", "Mai", "Juni", "Juli", "Aug.", "Sept.", "Okt.", "Nov.", "Dez."},
	{"Ene", "Feb", "Mar", "Abr", "May", "Jun", "Jul", "Ago.", "Sep", "Oct", "Nov", "Dic"},
	{"Janv.", "Févr.", "Mars", "Avril", "Mai", "Juin", "Juil.", "Août", "Sept.", "Oct.", "Nov.", "Déc."},
	{"Genn.", "Febbr.", "Mar.", "Apr.", "Magg.", "Giugno", "Luglio", "Ag.", "Sett.", "Ott.", "Nov.", "Dic."},
	{"Jan.", "Febr.", "Mars", "April", "Maj", "Juni", "Juli", "Aug.", "Sept.", "Okt.", "Nov.", "Dec."},
	{"Jan.", "Febr.", "Marts", "April", "Maj", "Juni", "Juli", "Aug.", "Sept.", "Okt.", "Nov.", "Dec."},
	{"Jan.", "Feb.", "Maart", "Apr.", "Mei", "Juni", "Juli", "Aug.", "Sept.", "Okt.", "Nov.", "Dec."}
};

static const char *ORDINAL[NUM_LANG][10] = {
	{"th", "st", "nd", "rd", "th", "th", "th", "th", "th", "th"},
	{".", ".", ".", ".", ".", ".", ".", ".", ".", "."},
	{"º", "º", "º", "º", "º", "º", "º", "º", "º", "º"},
	{"er", "", "", "", "", "", "", "", "", ""},
	{"º", "º", "º", "º", "º", "º", "º", "º", "º", "º"},
	{"", "", "", "", "", "", "", "", "", ""},
	{".", ".", ".", ".", ".", ".", ".", ".", ".", "."},
	{".", ".", ".", ".", ".", ".", ".", ".", ".", "."}
};

int face_supported_language(int language) {
	if (language < 0 || language >= NUM_LANG) {
		return LANG_EN;
	}
	return language;
}

int face_supported_layout(int layout, bool is_round) {
	if (is_round && (layout == LAYOUT_D || layout < 0 || layout >= LAYOUT_COUNT)) {
		return LAYOUT_B;
	}
	if (layout < 0 || layout >= LAYOUT_COUNT) {
		return LAYOUT_D;
	}
	return layout;
}

/* First row where a centered span stays inside a circle of this diameter. */
static int circle_fit_y(int diameter, int span) {
	int radius = diameter / 2;
	int half = span / 2 + 2;
	int y;

	for (y = 0; y < radius; y++) {
		int dy = radius - y;

		if (dy * dy + half * half <= radius * radius) {
			return y;
		}
	}
	return 0;
}

static void fill_slot(FaceSlot *slot, int x, int y, int w, int h, int align, bool visible) {
	slot->x = x;
	slot->y = y;
	slot->w = w;
	slot->h = h;
	slot->align = align;
	slot->visible = visible;
}

static void fill_round_frames(const FaceScreen *screen, int layout, FaceFrames *out) {
	int diameter = screen->width < screen->height ? screen->width : screen->height;
	/* Widest pill is bluetooth + battery + vibe, including its padding. */
	int reserved = circle_fit_y(diameter, 129) + 25;
	int time_y = (diameter - 50) / 2;
	int shift = 0;
	bool stacked = layout == LAYOUT_B;

	if (stacked) {
		int block = (91 + 24) - 32;
		int top = (diameter - block) / 2;

		if (top < reserved) {
			top = reserved;
		}
		shift = top - 32;
		time_y = 51 + shift;
	} else if (time_y < reserved) {
		time_y = reserved;
	}

	fill_slot(&out->time, 0, time_y, screen->width, 50, FACE_ALIGN_CENTER, true);
	fill_slot(&out->seconds, 0, 0, screen->width, 20, FACE_ALIGN_CENTER, false);
	fill_slot(&out->day, 0, 32 + shift, screen->width, 22, FACE_ALIGN_CENTER, stacked);
	fill_slot(&out->date, 0, 91 + shift, screen->width, 24, FACE_ALIGN_CENTER, stacked);
}

void face_layout_frames(const FaceScreen *screen, int layout, bool seconds_on,
		FaceFrames *out) {
	int width = screen->width;
	int oy = 0;

	layout = face_supported_layout(layout, screen->is_round);
	if (screen->is_round) {
		fill_round_frames(screen, layout, out);
		return;
	}

	if (width < 144) {
		width = 144;
	}
	if (screen->height > 168) {
		oy = (screen->height - 168) / 2;
	}

	/* Orbitron and Orbitron D share the centered time row. The glyph tops in
	   the store screenshots sit about 12 pixels into this 50-pixel layer. */
	fill_slot(&out->time, 0, 47 + oy, width, 50, FACE_ALIGN_CENTER, true);
	fill_slot(&out->seconds, 0, 88 + oy, width - 8, 20, FACE_ALIGN_RIGHT, false);
	fill_slot(&out->day, 0, 128 + oy, width - 2, 20, FACE_ALIGN_RIGHT, true);
	fill_slot(&out->date, 0, 146 + oy, width - 2, 18, FACE_ALIGN_RIGHT, true);

	if (layout == LAYOUT_B) {
		/* Weekday, time, then date, each centered. Measured from the
		   Orbitron B screenshots: weekday ink at y=42, time at y=63, date at y=98. */
		fill_slot(&out->day, 0, 32 + oy, width, 22, FACE_ALIGN_CENTER, true);
		fill_slot(&out->time, 0, 51 + oy, width, 50, FACE_ALIGN_CENTER, true);
		fill_slot(&out->date, 0, 91 + oy, width, 24, FACE_ALIGN_CENTER, true);
	} else if (layout == LAYOUT_ORBITRON) {
		out->day.visible = false;
		out->date.visible = false;
	} else if (seconds_on) {
		out->seconds.visible = true;
	}
}

const char *face_weekday(int language, int wday) {
	if (language < 0 || language >= NUM_LANG || wday < 0 || wday > 6) {
		return "";
	}
	return WEEKDAY_NAMES[language][wday];
}

const char *face_month(int language, int month) {
	if (language < 0 || language >= NUM_LANG || month < 0 || month > 11) {
		return "";
	}
	return MONTH_NAMES[language][month];
}

const char *face_ordinal(int language, int day) {
	if (language < 0 || language >= NUM_LANG || day < 0) {
		return "";
	}

	if (language == LANG_EN && day >= 11 && day <= 13) {
		return "th";
	}

	/* Day 1 indexes slot 1. French keeps "er" in slot 0, so the 1st has no suffix. */
	if (language == LANG_FR && day == 1) {
		return ORDINAL[language][day];
	}

	return ORDINAL[language][day % 10];
}

void face_format_date(char *day_out, size_t day_len,
		char *date_out, size_t date_len,
		int language, int date_mode, const struct tm *when) {
	const char *day_name = face_weekday(language, when->tm_wday);
	const char *month = face_month(language, when->tm_mon);
	const char *ordinal = face_ordinal(language, when->tm_mday);
	int year = 1900 + when->tm_year;

	if (day_len > 0) {
		snprintf(day_out, day_len, "%s", day_name);
	}
	if (date_len == 0) {
		return;
	}

	if (date_mode == DATE_MODE_US) {
		snprintf(date_out, date_len, "%s, %d%s %d",
				month, when->tm_mday, ordinal, year);
	} else if (date_mode == DATE_MODE_UK) {
		snprintf(date_out, date_len, "%d%s %s %d",
				when->tm_mday, ordinal, month, year);
	} else {
		date_out[0] = '\0';
	}
}

void face_format_clock(char *out, size_t out_len, const struct tm *when, bool is_24h) {
	if (out_len == 0) {
		return;
	}

	strftime(out, out_len, is_24h ? "%R" : "%I:%M", when);
	if (!is_24h && out[0] == '0') {
		memmove(out, out + 1, strlen(out));
	}
}

int face_battery_image_index(int charge_percent, bool is_charging) {
	int step = charge_percent / 10;

	if (step > 10) {
		step = 10;
	}
	return (is_charging ? 11 : 0) + step;
}

bool face_battery_visible(int battery_mode, int charge_percent, bool is_charging) {
	bool battery_is_low = charge_percent <= 20;

	return battery_mode == BATTERY_MODE_ALWAYS
		|| (battery_mode == BATTERY_MODE_IF_LOW && battery_is_low)
		|| is_charging;
}

bool face_bluetooth_hidden(int bluetooth_mode, bool connected) {
	return bluetooth_mode == BLUETOOTH_MODE_NEVER
		|| (bluetooth_mode == BLUETOOTH_MODE_IFOFF && connected);
}

int face_vibe_pulses(int minute, int second, int vibe_minutes, bool hour_changed) {
	int pulses = 0;

	if (vibe_minutes == 0) {
		return 0;
	}
	if ((minute % vibe_minutes) == 0 && second == 0) {
		pulses++;
	}
	if (vibe_minutes == 60 && hour_changed) {
		pulses++;
	}
	return pulses;
}

bool face_line_needs_update(const char *current, const char *next) {
	if (memcmp(current, next, strlen(next)) != 0
			|| (strlen(next) == 0 && strlen(current) != 0)) {
		return true;
	}
	return false;
}

static void blank_icon(FaceIcon *icon, int w, int h) {
	icon->x = 0;
	icon->y = 0;
	icon->w = w;
	icon->h = h;
	icon->visible = false;
}

void face_status_island(const FaceScreen *screen, bool show_bluetooth,
		bool show_battery, bool show_vibe, FaceIsland *out) {
	FaceIcon *slots[3];
	int count = 0;
	int content = 0;
	int cursor;
	int i;
	const int pad_x = 4;
	const int gap = 3;
	/* Odd height so a radius of (height - 1) / 2 fills the capsule exactly. */
	const int height = 25;

	blank_icon(&out->bluetooth, 45, 16);
	blank_icon(&out->battery, 50, 16);
	blank_icon(&out->vibe, 20, 20);
	out->x = 0;
	out->y = 1;
	out->w = 0;
	out->h = height;
	out->visible = false;

	if (show_bluetooth) {
		slots[count++] = &out->bluetooth;
	}
	if (show_battery) {
		slots[count++] = &out->battery;
	}
	if (show_vibe) {
		slots[count++] = &out->vibe;
	}
	if (count == 0) {
		return;
	}

	for (i = 0; i < count; i++) {
		content += slots[i]->w;
		if (i > 0) {
			content += gap;
		}
	}
	out->visible = true;
	out->w = content + pad_x * 2;
	out->x = (screen->width - out->w) / 2;
	if (screen->is_round) {
		int diameter = screen->width < screen->height ? screen->width : screen->height;

		out->y = circle_fit_y(diameter, out->w);
		out->x = (screen->width - out->w) / 2;
	} else if (screen->height > 168) {
		out->y = 1 + (screen->height - 168) / 2;
	}
	cursor = pad_x;
	for (i = 0; i < count; i++) {
		slots[i]->visible = true;
		slots[i]->x = cursor;
		slots[i]->y = (height - slots[i]->h) / 2;
		cursor += slots[i]->w + gap;
	}
}

void face_connlost_frame(int count, bool *icon_on, bool *buzz, bool *keep_going) {
	bool on_off = count & 1;

	*icon_on = on_off;
	*buzz = count < 6 && !on_off;
	*keep_going = count < 50;
}
