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

/* Orbitron D keeps the day and date at the bottom. Orbitron B stacks them
   around the time. Orbitron is the time alone. */
#define LAYOUT_D 0
#define LAYOUT_B 1
#define LAYOUT_ORBITRON 2
#define LAYOUT_COUNT 3

/* Same values as GTextAlignmentCenter and GTextAlignmentRight. */
#define FACE_ALIGN_CENTER 1
#define FACE_ALIGN_RIGHT 2

/* Integer keys shared with appinfo.json appKeys and phone config messages. */
enum {
	BATTERY_KEY = 0x2,
	VIBEMINUTES_KEY = 0x3,
	LANG_KEY = 0x4,
	DATESTYLE_KEY = 0x5,
	BT_KEY = 0x6,
	CONN_KEY = 0x7,
	SHOW_SECS = 0x8,
	ANIMATE_KEY = 0x9,
	LAYOUT_KEY = 0xA
};

typedef struct {
	int x;
	int y;
	int w;
	int h;
	int align;
	bool visible;
} FaceSlot;

typedef struct {
	FaceSlot time;
	FaceSlot seconds;
	FaceSlot day;
	FaceSlot date;
} FaceFrames;

/* A saved Russian setting (8) and any other unknown value become English. */
int face_supported_language(int language);

/* Rectangle or round face the frames are fitted to. */
typedef struct {
	int width;
	int height;
	bool is_round;
} FaceScreen;

/* Unknown values become Orbitron D. A round face has no Orbitron D layout, so
   that choice and any unknown value become Orbitron B. */
int face_supported_layout(int layout, bool is_round);

/* Text-layer frames. Seconds are drawn only on Orbitron D, which is rectangular. */
void face_layout_frames(const FaceScreen *screen, int layout, bool seconds_on,
		FaceFrames *out);

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

/* Icon frame inside the status pill. x and y are relative to the pill. */
typedef struct {
	int x;
	int y;
	int w;
	int h;
	bool visible;
} FaceIcon;

/* Capsule along the top of the face. Hidden when every icon is off.
   Order inside the pill is Bluetooth, battery, then vibrate.
   On a round face the capsule sits low enough to stay inside the circle. */
typedef struct {
	int x;
	int y;
	int w;
	int h;
	bool visible;
	FaceIcon bluetooth;
	FaceIcon battery;
	FaceIcon vibe;
} FaceIsland;

void face_status_island(const FaceScreen *screen, bool show_bluetooth, bool show_battery,
		bool show_vibe, FaceIsland *out);

#endif
