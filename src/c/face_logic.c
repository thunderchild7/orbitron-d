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

void face_connlost_frame(int count, bool *icon_on, bool *buzz, bool *keep_going) {
	bool on_off = count & 1;

	*icon_on = on_off;
	*buzz = count < 6 && !on_off;
	*keep_going = count < 50;
}
