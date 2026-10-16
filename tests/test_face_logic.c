#include "face_logic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0;
static int failures = 0;

static void expect_int(const char *name, int got, int want) {
	tests_run++;
	if (got != want) {
		fprintf(stderr, "FAIL %s: got %d want %d\n", name, got, want);
		failures++;
	}
}

static void expect_str(const char *name, const char *got, const char *want) {
	tests_run++;
	if (strcmp(got, want) != 0) {
		fprintf(stderr, "FAIL %s: got \"%s\" want \"%s\"\n", name, got, want);
		failures++;
	}
}

static struct tm at_time(int year, int month, int mday, int wday,
		int hour, int min, int sec) {
	struct tm when;

	memset(&when, 0, sizeof(when));
	when.tm_year = year - 1900;
	when.tm_mon = month;
	when.tm_mday = mday;
	when.tm_wday = wday;
	when.tm_hour = hour;
	when.tm_min = min;
	when.tm_sec = sec;
	return when;
}

static void test_config_keys(void) {
	expect_int("battery matches appinfo", BATTERY_KEY, 2);
	expect_int("vibe minutes matches appinfo", VIBEMINUTES_KEY, 3);
	expect_int("language matches appinfo", LANG_KEY, 4);
	expect_int("date style matches appinfo", DATESTYLE_KEY, 5);
	expect_int("bluetooth matches appinfo", BT_KEY, 6);
	expect_int("connection loss matches appinfo", CONN_KEY, 7);
	expect_int("seconds matches appinfo showseonds", SHOW_SECS, 8);
	expect_int("animate is the next key", ANIMATE_KEY, 9);
	expect_int("layout matches appinfo", LAYOUT_KEY, 10);
}

static void expect_slot(const char *name, const FaceSlot *slot,
		int x, int y, int w, int h, int align, int visible) {
	expect_int(name, slot->x, x);
	expect_int(name, slot->y, y);
	expect_int(name, slot->w, w);
	expect_int(name, slot->h, h);
	expect_int(name, slot->align, align);
	expect_int(name, slot->visible, visible);
}

static FaceScreen screen_of(int width, int height, bool is_round) {
	FaceScreen screen;

	screen.width = width;
	screen.height = height;
	screen.is_round = is_round;
	return screen;
}

static void test_layouts(void) {
	FaceFrames frames;
	FaceScreen rect = screen_of(144, 168, false);
	FaceScreen emery = screen_of(200, 228, false);
	FaceScreen chalk = screen_of(180, 180, true);
	FaceScreen gabbro = screen_of(260, 260, true);

	expect_int("unknown layout falls back", face_supported_layout(9, false), LAYOUT_D);
	expect_int("orbitron b stays", face_supported_layout(LAYOUT_B, false), LAYOUT_B);
	expect_int("round keeps orbitron b", face_supported_layout(LAYOUT_B, true), LAYOUT_B);
	expect_int("round keeps the time-only layout",
			face_supported_layout(LAYOUT_ORBITRON, true), LAYOUT_ORBITRON);
	expect_int("round disables orbitron d", face_supported_layout(LAYOUT_D, true), LAYOUT_B);
	expect_int("round unknown layout uses b", face_supported_layout(9, true), LAYOUT_B);

	face_layout_frames(&rect, LAYOUT_D, false, &frames);
	expect_slot("d time", &frames.time, 0, 47, 144, 50, FACE_ALIGN_CENTER, 1);
	expect_slot("d seconds hidden", &frames.seconds, 0, 88, 136, 20, FACE_ALIGN_RIGHT, 0);
	expect_slot("d day", &frames.day, 0, 128, 142, 20, FACE_ALIGN_RIGHT, 1);
	expect_slot("d date", &frames.date, 0, 146, 142, 18, FACE_ALIGN_RIGHT, 1);

	face_layout_frames(&rect, LAYOUT_D, true, &frames);
	expect_int("d seconds shown", frames.seconds.visible, 1);

	face_layout_frames(&rect, LAYOUT_B, true, &frames);
	expect_slot("b day", &frames.day, 0, 32, 144, 22, FACE_ALIGN_CENTER, 1);
	expect_slot("b time", &frames.time, 0, 51, 144, 50, FACE_ALIGN_CENTER, 1);
	expect_slot("b date", &frames.date, 0, 91, 144, 24, FACE_ALIGN_CENTER, 1);
	expect_int("b ignores seconds", frames.seconds.visible, 0);

	face_layout_frames(&rect, LAYOUT_ORBITRON, true, &frames);
	expect_slot("orbitron time", &frames.time, 0, 47, 144, 50, FACE_ALIGN_CENTER, 1);
	expect_int("orbitron hides the day", frames.day.visible, 0);
	expect_int("orbitron hides the date", frames.date.visible, 0);
	expect_int("orbitron ignores seconds", frames.seconds.visible, 0);

	face_layout_frames(&rect, 99, false, &frames);
	expect_int("unknown layout uses d day", frames.day.y, 128);
	expect_int("unknown layout keeps the day", frames.day.visible, 1);

	face_layout_frames(&emery, LAYOUT_D, true, &frames);
	expect_slot("emery time", &frames.time, 0, 77, 200, 50, FACE_ALIGN_CENTER, 1);
	expect_slot("emery seconds", &frames.seconds, 0, 118, 192, 20, FACE_ALIGN_RIGHT, 1);
	expect_slot("emery day", &frames.day, 0, 158, 198, 20, FACE_ALIGN_RIGHT, 1);
	expect_slot("emery date", &frames.date, 0, 176, 198, 18, FACE_ALIGN_RIGHT, 1);

	face_layout_frames(&chalk, LAYOUT_D, true, &frames);
	expect_slot("chalk uses the b day", &frames.day, 0, 54, 180, 22, FACE_ALIGN_CENTER, 1);
	expect_slot("chalk uses the b time", &frames.time, 0, 73, 180, 50, FACE_ALIGN_CENTER, 1);
	expect_slot("chalk uses the b date", &frames.date, 0, 113, 180, 24, FACE_ALIGN_CENTER, 1);
	expect_int("chalk hides seconds", frames.seconds.visible, 0);

	face_layout_frames(&chalk, LAYOUT_ORBITRON, true, &frames);
	expect_slot("chalk time only", &frames.time, 0, 65, 180, 50, FACE_ALIGN_CENTER, 1);
	expect_int("chalk time only hides the day", frames.day.visible, 0);

	face_layout_frames(&gabbro, LAYOUT_B, false, &frames);
	expect_slot("gabbro day", &frames.day, 0, 88, 260, 22, FACE_ALIGN_CENTER, 1);
	expect_slot("gabbro time", &frames.time, 0, 107, 260, 50, FACE_ALIGN_CENTER, 1);
	expect_slot("gabbro date", &frames.date, 0, 147, 260, 24, FACE_ALIGN_CENTER, 1);
}

static void test_names(void) {
	expect_str("english saturday", face_weekday(LANG_EN, 6), "Saturday");
	expect_str("german thursday", face_weekday(LANG_DE, 4), "Donnerstag");
	expect_str("spanish wednesday", face_weekday(LANG_ES, 3), "Miércoles");
	expect_str("french sunday", face_weekday(LANG_FR, 0), "Dimanche");
	expect_str("italian wednesday", face_weekday(LANG_IT, 3), "Mercoledì");
	expect_str("swedish monday", face_weekday(LANG_SE, 1), "Måndag");
	expect_str("danish tuesday keeps the swedish spelling", face_weekday(LANG_DK, 2), "Tisdag");
	expect_str("dutch saturday", face_weekday(LANG_NL, 6), "Zaterdag");
	expect_str("former russian index", face_weekday(8, 0), "");
	expect_str("weekday language out of range", face_weekday(NUM_LANG, 0), "");
	expect_str("weekday before sunday", face_weekday(LANG_EN, -1), "");
	expect_str("weekday after saturday", face_weekday(LANG_EN, 7), "");

	expect_str("english march", face_month(LANG_EN, 2), "Mar.");
	expect_str("german march", face_month(LANG_DE, 2), "März");
	expect_str("french august", face_month(LANG_FR, 7), "Août");
	expect_str("italian february", face_month(LANG_IT, 1), "Febbr.");
	expect_str("former russian month", face_month(8, 0), "");
	expect_str("month out of range", face_month(LANG_EN, 12), "");
	expect_str("negative month", face_month(LANG_EN, -1), "");
}

static void test_ordinals(void) {
	static const char *english[32] = {
		"",
		"st", "nd", "rd", "th", "th", "th", "th", "th", "th", "th",
		"th", "th", "th", "th", "th", "th", "th", "th", "th", "th",
		"st", "nd", "rd", "th", "th", "th", "th", "th", "th", "th",
		"st"
	};
	int day;

	for (day = 1; day <= 31; day++) {
		char name[32];

		snprintf(name, sizeof(name), "english day %d", day);
		expect_str(name, face_ordinal(LANG_EN, day), english[day]);
	}

	expect_str("french first uses the empty slot", face_ordinal(LANG_FR, 1), "");
	expect_str("french second", face_ordinal(LANG_FR, 2), "");
	expect_str("german first", face_ordinal(LANG_DE, 1), ".");
	expect_str("spanish eleventh", face_ordinal(LANG_ES, 11), "º");
	expect_str("italian third", face_ordinal(LANG_IT, 3), "º");
	expect_str("swedish first", face_ordinal(LANG_SE, 1), "");
	expect_str("danish fourth", face_ordinal(LANG_DK, 4), ".");
	expect_str("dutch twenty second", face_ordinal(LANG_NL, 22), ".");
	expect_str("former russian ordinal", face_ordinal(8, 31), "");
	expect_str("ordinal language out of range", face_ordinal(9, 1), "");
	expect_str("negative day", face_ordinal(LANG_EN, -1), "");
}

static void test_dates(void) {
	char day[FACE_TEXT_CAP];
	char date[FACE_TEXT_CAP];
	char short_day[4];
	struct tm when;

	when = at_time(2014, 2, 15, 6, 10, 43, 0);
	face_format_date(day, sizeof(day), date, sizeof(date), LANG_EN, DATE_MODE_UK, &when);
	expect_str("screenshot weekday", day, "Saturday");
	expect_str("screenshot uk date", date, "15th Mar. 2014");

	face_format_date(day, sizeof(day), date, sizeof(date), LANG_EN, DATE_MODE_US, &when);
	expect_str("screenshot us date", date, "Mar., 15th 2014");

	when = at_time(2014, 2, 13, 4, 0, 0, 0);
	face_format_date(day, sizeof(day), date, sizeof(date), LANG_DE, DATE_MODE_UK, &when);
	expect_str("german thursday fits", day, "Donnerstag");
	expect_str("german uk date", date, "13. März 2014");

	when = at_time(2014, 0, 1, 3, 0, 0, 0);
	face_format_date(day, sizeof(day), date, sizeof(date), LANG_FR, DATE_MODE_UK, &when);
	expect_str("french new year has no er", date, "1 Janv. 2014");

	when = at_time(2014, 1, 2, 0, 0, 0, 0);
	face_format_date(day, sizeof(day), date, sizeof(date), LANG_IT, DATE_MODE_US, &when);
	expect_str("italian us date", date, "Febbr., 2º 2014");

	when = at_time(2014, 2, 15, 6, 0, 0, 0);
	face_format_date(day, sizeof(day), date, sizeof(date), LANG_EN, 99, &when);
	expect_str("unknown date mode keeps the weekday", day, "Saturday");
	expect_str("unknown date mode clears the date", date, "");

	when = at_time(2014, 2, 15, 6, 0, 0, 0);
	face_format_date(short_day, sizeof(short_day), date, sizeof(date),
			LANG_EN, DATE_MODE_UK, &when);
	expect_str("short weekday buffer truncates", short_day, "Sat");
	expect_int("weekday cap holds the longest name",
			(int)strlen(face_weekday(LANG_DE, 4)) < FACE_TEXT_CAP, 1);
	expect_int("old russian setting falls back to english",
			face_supported_language(8), LANG_EN);
	expect_int("dutch stays dutch", face_supported_language(LANG_NL), LANG_NL);
}

static void test_clock(void) {
	char text[8];
	struct tm when;

	when = at_time(2014, 2, 15, 6, 10, 43, 7);
	face_format_clock(text, sizeof(text), &when, true);
	expect_str("24h afternoon", text, "10:43");

	when = at_time(2014, 2, 15, 6, 0, 5, 0);
	face_format_clock(text, sizeof(text), &when, true);
	expect_str("24h midnight", text, "00:05");

	when = at_time(2014, 2, 15, 6, 9, 5, 0);
	face_format_clock(text, sizeof(text), &when, false);
	expect_str("12h drops the leading zero", text, "9:05");

	when = at_time(2014, 2, 15, 6, 0, 5, 0);
	face_format_clock(text, sizeof(text), &when, false);
	expect_str("12h midnight is 12", text, "12:05");

	when = at_time(2014, 2, 15, 6, 12, 0, 0);
	face_format_clock(text, sizeof(text), &when, false);
	expect_str("12h noon", text, "12:00");

	when = at_time(2014, 2, 15, 6, 15, 7, 0);
	face_format_clock(text, sizeof(text), &when, false);
	expect_str("12h afternoon", text, "3:07");

	when = at_time(2014, 2, 15, 6, 22, 8, 0);
	face_format_clock(text, sizeof(text), &when, true);
	expect_str("24h late", text, "22:08");
}

static void test_battery(void) {
	int percent;

	for (percent = 0; percent <= 100; percent += 10) {
		char name[40];

		snprintf(name, sizeof(name), "discharging %d", percent);
		expect_int(name, face_battery_image_index(percent, false), percent / 10);
		snprintf(name, sizeof(name), "charging %d", percent);
		expect_int(name, face_battery_image_index(percent, true), 11 + percent / 10);
	}

	expect_int("9 percent still uses the empty icon", face_battery_image_index(9, false), 0);
	expect_int("99 percent stays on the 90 icon", face_battery_image_index(99, false), 9);
	expect_int("over 100 stays on the full icon", face_battery_image_index(255, true), 21);

	expect_int("never hides a full battery", face_battery_visible(BATTERY_MODE_NEVER, 100, false), 0);
	expect_int("never still shows a charger", face_battery_visible(BATTERY_MODE_NEVER, 100, true), 1);
	expect_int("low mode shows 20", face_battery_visible(BATTERY_MODE_IF_LOW, 20, false), 1);
	expect_int("low mode hides 21", face_battery_visible(BATTERY_MODE_IF_LOW, 21, false), 0);
	expect_int("low mode shows a charger at 80", face_battery_visible(BATTERY_MODE_IF_LOW, 80, true), 1);
	expect_int("always shows 100", face_battery_visible(BATTERY_MODE_ALWAYS, 100, false), 1);
	expect_int("always shows 0", face_battery_visible(BATTERY_MODE_ALWAYS, 0, false), 1);
}

static void test_bluetooth(void) {
	expect_int("never hides a live link", face_bluetooth_hidden(BLUETOOTH_MODE_NEVER, true), 1);
	expect_int("never hides a dead link", face_bluetooth_hidden(BLUETOOTH_MODE_NEVER, false), 1);
	expect_int("if-off hides a live link", face_bluetooth_hidden(BLUETOOTH_MODE_IFOFF, true), 1);
	expect_int("if-off shows a dead link", face_bluetooth_hidden(BLUETOOTH_MODE_IFOFF, false), 0);
	expect_int("always shows a live link", face_bluetooth_hidden(BLUETOOTH_MODE_ALWAYS, true), 0);
	expect_int("always shows a dead link", face_bluetooth_hidden(BLUETOOTH_MODE_ALWAYS, false), 0);
}

static void test_vibe(void) {
	int minute;

	expect_int("zero interval is quiet", face_vibe_pulses(0, 0, 0, true), 0);
	for (minute = 0; minute < 60; minute++) {
		int want = (minute % 15) == 0 ? 1 : 0;
		char name[40];

		snprintf(name, sizeof(name), "15 minute mark %d", minute);
		expect_int(name, face_vibe_pulses(minute, 0, 15, false), want);
		snprintf(name, sizeof(name), "15 minute mark %d ignores a late second", minute);
		expect_int(name, face_vibe_pulses(minute, 1, 15, false), 0);
	}

	expect_int("hourly interval at midnight fires twice", face_vibe_pulses(0, 0, 60, true), 2);
	expect_int("hourly interval at minute 0 without the hour flag fires once",
			face_vibe_pulses(0, 0, 60, false), 1);
	expect_int("hourly interval mid hour is quiet", face_vibe_pulses(30, 0, 60, false), 0);
	expect_int("five minute interval does not add an hour pulse",
			face_vibe_pulses(0, 0, 5, true), 1);
}

static void test_line_update(void) {
	expect_int("same time stays", face_line_needs_update("10:43", "10:43"), 0);
	expect_int("next minute updates", face_line_needs_update("10:43", "10:44"), 1);
	expect_int("both empty stays", face_line_needs_update("", ""), 0);
	expect_int("clearing a line updates", face_line_needs_update("10:43", ""), 1);
	expect_int("a longer current value that starts with next stays",
			face_line_needs_update("10:00", "10:0"), 0);
	expect_int("12h to 24h width updates", face_line_needs_update("9:00", "09:00"), 1);
}

static void expect_icon(const char *name, const FaceIcon *icon,
		int x, int y, int w, int h, int visible) {
	expect_int(name, icon->x, x);
	expect_int(name, icon->y, y);
	expect_int(name, icon->w, w);
	expect_int(name, icon->h, h);
	expect_int(name, icon->visible, visible);
}

static void test_status_island(void) {
	FaceIsland island;
	FaceScreen rect = screen_of(144, 168, false);
	FaceScreen emery = screen_of(200, 228, false);
	FaceScreen chalk = screen_of(180, 180, true);

	face_status_island(&rect, false, false, false, &island);
	expect_int("empty island hidden", island.visible, 0);
	expect_int("empty island width", island.w, 0);
	expect_int("island hangs from the top", island.y, 1);
	expect_int("island height", island.h, 25);
	expect_int("hidden bluetooth keeps its size", island.bluetooth.w, 45);
	expect_int("hidden bluetooth stays off", island.bluetooth.visible, 0);

	face_status_island(&rect, false, true, false, &island);
	expect_int("battery pill shown", island.visible, 1);
	expect_int("battery pill width", island.w, 58);
	expect_int("battery pill centered", island.x, 43);
	expect_icon("battery in its pill", &island.battery, 4, 4, 50, 16, 1);
	expect_int("bluetooth stays out of the battery pill", island.bluetooth.visible, 0);
	expect_int("vibe stays out of the battery pill", island.vibe.visible, 0);

	face_status_island(&rect, true, false, false, &island);
	expect_int("bluetooth pill width", island.w, 53);
	expect_int("bluetooth pill centered", island.x, 45);
	expect_icon("bluetooth alone", &island.bluetooth, 4, 4, 45, 16, 1);

	face_status_island(&rect, false, false, true, &island);
	expect_int("vibe pill width", island.w, 28);
	expect_int("vibe pill centered", island.x, 58);
	expect_icon("vibe alone", &island.vibe, 4, 2, 20, 20, 1);

	face_status_island(&rect, true, true, true, &island);
	expect_int("full pill width", island.w, 129);
	expect_int("full pill centered", island.x, 7);
	expect_icon("bluetooth leads the pill", &island.bluetooth, 4, 4, 45, 16, 1);
	expect_icon("battery follows bluetooth", &island.battery, 52, 4, 50, 16, 1);
	expect_icon("vibe closes the pill", &island.vibe, 105, 2, 20, 20, 1);

	face_status_island(&emery, false, true, false, &island);
	expect_int("emery battery pill x", island.x, 71);
	expect_int("emery battery pill y", island.y, 31);

	face_status_island(&chalk, true, true, true, &island);
	expect_int("chalk full pill x", island.x, 25);
	expect_int("chalk full pill y", island.y, 29);
	expect_int("chalk full pill width", island.w, 129);

	face_status_island(&chalk, false, true, false, &island);
	expect_int("chalk battery pill y", island.y, 6);
	expect_int("chalk battery pill x", island.x, 61);
}

static void test_connection_loss(void) {
	int count;
	int buzzes = 0;

	for (count = 0; count < 51; count++) {
		bool icon_on = false;
		bool buzz = false;
		bool keep_going = false;
		char name[40];

		face_connlost_frame(count, &icon_on, &buzz, &keep_going);
		snprintf(name, sizeof(name), "blink phase %d", count);
		expect_int(name, icon_on, count & 1);
		if (buzz) {
			buzzes++;
		}
		snprintf(name, sizeof(name), "keep going %d", count);
		expect_int(name, keep_going, count < 50);
	}

	expect_int("three buzzes", buzzes, 3);
}

int main(void) {
	test_config_keys();
	test_layouts();
	test_names();
	test_ordinals();
	test_dates();
	test_clock();
	test_battery();
	test_bluetooth();
	test_vibe();
	test_line_update();
	test_status_island();
	test_connection_loss();

	if (failures != 0) {
		fprintf(stderr, "%d of %d checks failed\n", failures, tests_run);
		return EXIT_FAILURE;
	}
	printf("%d checks passed\n", tests_run);
	return EXIT_SUCCESS;
}
