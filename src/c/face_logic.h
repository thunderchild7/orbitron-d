#ifndef FACE_LOGIC_H
#define FACE_LOGIC_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/* Weekday and date lines. Longest stored name is Donnerstag. */
#define FACE_TEXT_CAP 48

#define NUM_LANG 8

#define LANG_EN 0
#define LANG_DE 1
#define LANG_ES 2
#define LANG_FR 3
#define LANG_IT 4
#define LANG_SE 5
#define LANG_DK 6
#define LANG_NL 7

#define DATE_MODE_UK 0
#define DATE_MODE_US 1

#define BATTERY_MODE_NEVER  0
#define BATTERY_MODE_IF_LOW 1
#define BATTERY_MODE_ALWAYS 2

#define BLUETOOTH_MODE_NEVER 0
#define BLUETOOTH_MODE_IFOFF 1
#define BLUETOOTH_MODE_ALWAYS 2

/* Integer keys shared with appinfo.json appKeys and phone config messages. */
enum {
	BATTERY_KEY = 0x2,
	VIBEMINUTES_KEY = 0x3,
	LANG_KEY = 0x4,
	DATESTYLE_KEY = 0x5,
	BT_KEY = 0x6,
	CONN_KEY = 0x7,
	SHOW_SECS = 0x8,
	ANIMATE_KEY = 0x9
};

/* A saved Russian setting (8) and any other unknown value become English. */
int face_supported_language(int language);

/* Empty string when language or day-of-week is out of range. */
const char *face_weekday(int language, int wday);

/* Empty string when language or month (0-11) is out of range. */
const char *face_month(int language, int month);

/*
 * English teens are "th". French day 1 reads ordinal slot 1, which is empty;
 * the "er" suffix is stored in slot 0 and is not used.
 */
const char *face_ordinal(int language, int day);

/* UK is "15th Mar. 2014". US is "Mar., 15th 2014". Any other mode clears the date. */
void face_format_date(char *day_out, size_t day_len,
		char *date_out, size_t date_len,
		int language, int date_mode, const struct tm *when);

/* 24-hour uses HH:MM. 12-hour strips a leading zero from 01-09. */
void face_format_clock(char *out, size_t out_len, const struct tm *when, bool is_24h);

/* Index into the 22-image battery table. 0-10 discharging, 11-21 charging. */
int face_battery_image_index(int charge_percent, bool is_charging);

/* Charging is shown in every mode. Low means 20 percent or under. */
bool face_battery_visible(int battery_mode, int charge_percent, bool is_charging);

/* Hidden when the icon is off, or when it shows only while disconnected and the link is up. */
bool face_bluetooth_hidden(int bluetooth_mode, bool connected);

/*
 * How many double-pulses this tick fires.
 * Interval 60 also pulses on the hour tick, so midnight with that interval fires twice.
 */
int face_vibe_pulses(int minute, int second, int vibe_minutes, bool hour_changed);

/* False when next is empty and current is empty, or when current begins with next. */
bool face_line_needs_update(const char *current, const char *next);

/* count 0, 2, 4 buzz. Blinking continues while count is below 50. */
void face_connlost_frame(int count, bool *icon_on, bool *buzz, bool *keep_going);

#endif
