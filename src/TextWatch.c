#include <pebble.h>
#include <ctype.h>
//#include "num2words-en.h"

#define DEBUG 0
#define BUFFER_SIZE 44

// keys for app message and storage
#define BATTERY_MODE   2
#define DATE_MODE      2
#define BLUETOOTH_MODE 4
#define CONNLOST_MODE  6


#define BATTERY_MODE_NEVER    0
#define BATTERY_MODE_IF_LOW   1
#define BATTERY_MODE_ALWAYS   2
#define DATE_MODE_OFF         0
#define DATE_MODE_UK          0
#define DATE_MODE_US          1
#define LANG_EN               0
#define LANG_DE          1
#define LANG_ES          2
#define LANG_FR          3
#define LANG_IT          4
#define LANG_SE          5
#define LANG_DK          6
#define LANG_NL          7
#define LANG_RU          8
#define DATE_MODE_LAST        6
#define BLUETOOTH_MODE_NEVER  0
#define BLUETOOTH_MODE_IFOFF  1
#define BLUETOOTH_MODE_ALWAYS 2
#define GRAPHICS_MODE_NORMAL  0
#define GRAPHICS_MODE_INVERT  1
#define CONNLOST_MODE_IGNORE  0
#define CONNLOST_MODE_WARN    1

#define SECONDS_MODE_OFF  0
#define SECONDS_MODE_ON   1
	
#define NUM_LANG          9
	
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))
	
static int mInvert = GRAPHICS_MODE_NORMAL;			// Invert colours (0/1)
static int mVibeMinutes = 0;	// Vibrate every X minutes

static int battery_mode   = BATTERY_MODE_NEVER;
static int date_mode      = DATE_MODE_UK;
static int language		= LANG_EN;
static int bluetooth_mode = BLUETOOTH_MODE_NEVER;
static int connlost_mode  = CONNLOST_MODE_IGNORE;

static int secondsMode = SECONDS_MODE_OFF;

Window *window;

static GFont *date_font;
static GFont *day_font;
static GFont *time_font;
static GFont *second_font;


static GBitmap *battery_images[22];
static BitmapLayer *battery_layer;

static GBitmap *bluetooth_images[4];
static BitmapLayer *bluetooth_layer;

typedef struct {
  TextLayer *currentLayer;
  TextLayer *nextLayer;
  PropertyAnimation *currentAnimation;
  PropertyAnimation *nextAnimation;
} Line;

// there used to be more lines
Line line2;
TextLayer *line4;

TextLayer *date;
TextLayer *day;

enum {
    INVERT_KEY = 0x1,
	BATTERY_KEY = 0x2,
    VIBEMINUTES_KEY = 0x3,
	LANG_KEY = 0x4,
	DATESTYLE_KEY = 0x5,
	BT_KEY = 0x6,
	CONN_KEY = 0x7,
	NUM_CONFIG_KEYS = 0x7
};

const char* WEEKDAY_NAMES[NUM_LANG][7] = { // 3 chars, 1 for utf-8, 1 for terminating 0
  {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}, // english
  {"Sonntag",  "Montag",  "Dienstag",  "Mittwoch",  "Donnerstag",  "Freitag",  "Samstag" },  // german
  {"Domingo", "Lunes", "Martes", "Miécoles", "Jueves", "Viernes", "Sábado"},  // spanish
  {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"}, // french
  {"Domenica", "Lunedì", "Martedì", "Mercoledì", "Giovedì", "Venerdì", "Sabato"}, // italian 
  {"Söndag", "Måndag", "Tisdag", "Onsdag", "Torsdag", "Fredag", "Lördag"},  // swedish
  {"Søndag", "Mandag", "Tisdag", "Onsdag", "Torsdag", "Fredag", "Lørdag"},  // danish / norwegian
  {"Zondag","Maandag","Dinsdag","Woensdag","Donderdag","Vrijdag","Zaterdag"},  // dutch
  {"Воскресенье","Понедельник","Вторник","Среда","Четверг","Пятница","Суббота"},  // russian
};

const char* MON_NAMES[NUM_LANG][12] = { // 3 chars, 1 for utf-8, 1 for terminating 0
  {"Jan.", "Feb.", "Mar.", "Apr.", "May", "June", "July","Aug.","Sept.","Oct.","Nov.","Dec."},  // english
  {"Jän.",  "Feb.","März","Apr.","Mai","Juni","Juli","Aug.","Sept.","Okt.","Nov.","Dez." },  // german
  {"Ene", "Feb", "Mar", "Abr", "May", "Jun", "Jul","Ago.","Sep","Oct","Nov","Dic"},  // spanish
  {"Janv.", "Févr.", "Mars", "Avril", "Mai", "Juin", "Juil.","Août","Sept.","Oct.","Nov.","Déc."},  // french
  {"Genn.", "Febbr.", "Mar.", "Apr.", "Magg.", "Giugno", "Luglio","Ag.","Sett.","Ott.","Nov.","Dic."},  // italian
  {"Jan.", "Febr.", "Mars", "April", "Maj", "Juni", "Juli","Aug.","Sept.","Okt.","Nov.","Dec."}, //swedish
  {"Jan.", "Febr.", "Marts", "April", "Maj", "Juni", "Juli","Aug.","Sept.","Okt.","Nov.","Dec."}, // danish / norwegian
  {"Jan.", "Feb.", "Maart", "Apr.", "Mei", "Juni", "Juli","Aug.","Sept.","Okt.","Nov.","Dec."}, // dutch
  {"янв", "фев", "мар", "апр", "май", "июн", "июл","авг","сен","окт","ноя","дек"}, // russian
};

const char* ORDINAL[NUM_LANG][10] = {
	{"th","st","nd","rd","th","th","th","th","th","th"}, // english
	{".",".",".",".",".",".",".",".",".","."},  // german
	{"º","º","º","º","º","º","º","º","º","º"},  // spanish
	{"er","","","","","","","","",""}, // french
	{"º","º","º","º","º","º","º","º","º","º"},  	// italian	
	{"","","","","","","","","",""},	//swedish
	{".",".",".",".",".",".",".",".",".","."},		// danish / norwegian	
	{".",".",".",".",".",".",".",".",".","."},		// dutch
	{"","","","","","","","","",""},	// russki

};

static AppSync sync;
static uint8_t sync_buffer[128];

static char line2Str[2][BUFFER_SIZE];
static char line4Str[2][BUFFER_SIZE];

static InverterLayer* inverter_layer;


static bool was_connected = true;

static void destroy_property_animation(PropertyAnimation **prop_animation) {
  if (*prop_animation == NULL) {
    return;
  }

  if (animation_is_scheduled((Animation*) *prop_animation)) {
    animation_unschedule((Animation*) *prop_animation);
  }

  property_animation_destroy(*prop_animation);
  *prop_animation = NULL;
}

const char* get_ordinal(int day)
{
	switch (language) {
	case LANG_EN: 
	if (day >= 11 && day <= 13) {
        return "th";
    } else if (day % 10 == 1) {
        return ORDINAL[language][day%10];
    } else if (day % 10 == 2) {
        return ORDINAL[language][day%10];
    } else if (day % 10 == 3) {
        return ORDINAL[language][day%10];
    } else {
        return ORDINAL[language][day%10];
    };
	break;
	case LANG_FR:
		if (day == 1) {
        return ORDINAL[language][day];
    } else {
        return ORDINAL[language][day%10];
    } ;
	break;
	default :
	return ORDINAL[language][day%10];
}
	
}
//Handle Date
void setDate(struct tm* pbltime)
{

	static char dayString[] = "Wednesday";
    static char dateString[] = "99th September 9999";
  
	//  check for empty time
     time_t now;
     if (pbltime == NULL) {
       now = time(NULL);
       pbltime = localtime(&now);
    }
	
	if (date_mode == DATE_MODE_UK){	

	   snprintf(dateString,sizeof(dateString),   "%d%s %s %d",pbltime->tm_mday,get_ordinal(pbltime->tm_mday),MON_NAMES[language][pbltime->tm_mon],1900 + pbltime->tm_year);

	}
	if (date_mode == DATE_MODE_US) {
	   snprintf(dateString,sizeof(dateString),   "%s, %d%s %d",MON_NAMES[language][pbltime->tm_mon],pbltime->tm_mday,get_ordinal(pbltime->tm_mday),1900 + pbltime->tm_year);


    }

  snprintf(dayString,sizeof(dateString), "%s", WEEKDAY_NAMES[language][pbltime->tm_wday]);
//	dayString = WEEKDAY_NAMES[language][pbltime->tm_wday];
  //   APP_LOG(APP_LOG_LEVEL_DEBUG, "datestring! %s ", dateString);

  text_layer_set_text(date, dateString);
  text_layer_set_text(day, dayString);
}

// Animation handler
void animationStoppedHandler(struct Animation *animation, bool finished, void *context)
{
  Layer *current = (Layer *)context;
  GRect rect = layer_get_frame(current);
  rect.origin.x = 144;
  layer_set_frame(current, rect);
}

// Animate line
void makeAnimationsForLayers(Line *line, TextLayer *current, TextLayer *next)
{
  GRect rect = layer_get_frame((Layer*)next);
  rect.origin.x -= 144;

  destroy_property_animation(&(line->nextAnimation));

  line->nextAnimation = property_animation_create_layer_frame((Layer*)next, NULL, &rect);
  animation_set_duration((Animation*)line->nextAnimation, 400);
  animation_set_curve((Animation*)line->nextAnimation, AnimationCurveEaseOut);

  animation_schedule((Animation*)line->nextAnimation);

  GRect rect2 = layer_get_frame((Layer*)current);
  rect2.origin.x -= 144;

  destroy_property_animation(&(line->currentAnimation));

  line->currentAnimation = property_animation_create_layer_frame((Layer*)current, NULL, &rect2);
  animation_set_duration((Animation*)line->currentAnimation, 400);
  animation_set_curve((Animation*)line->currentAnimation, AnimationCurveEaseOut);

  animation_set_handlers((Animation*)line->currentAnimation, (AnimationHandlers) {
      .stopped = (AnimationStoppedHandler)animationStoppedHandler
      }, current);

  animation_schedule((Animation*)line->currentAnimation);
}

// Update line
void updateLineTo(Line *line, char lineStr[2][BUFFER_SIZE], char *value)
{
  TextLayer *next, *current;

  GRect rect = layer_get_frame((Layer*)line->currentLayer);
  current = (rect.origin.x == 0) ? line->currentLayer : line->nextLayer;
  next = (current == line->currentLayer) ? line->nextLayer : line->currentLayer;

  // Update correct text only
  if (current == line->currentLayer) {
    memset(lineStr[1], 0, BUFFER_SIZE);
    memcpy(lineStr[1], value, strlen(value));
    text_layer_set_text(next, lineStr[1]);
  } else {
    memset(lineStr[0], 0, BUFFER_SIZE);
    memcpy(lineStr[0], value, strlen(value));
    text_layer_set_text(next, lineStr[0]);
  }

  makeAnimationsForLayers(line, current, next);
}

// Check to see if the current line needs to be updated
bool needToUpdateLine(Line *line, char lineStr[2][BUFFER_SIZE], char *nextValue)
{
  char *currentStr;
  GRect rect = layer_get_frame((Layer*)line->currentLayer);
  currentStr = (rect.origin.x == 0) ? lineStr[0] : lineStr[1];

  if (memcmp(currentStr, nextValue, strlen(nextValue)) != 0 ||
      (strlen(nextValue) == 0 && strlen(currentStr) != 0)) {
    return true;
  }
  return false;
}

void display_time(struct tm* pbltime) {

  //  check for empty time

  time_t now;
  if (pbltime == NULL) {
    now = time(NULL);
    pbltime = localtime(&now);
  }

  static char time_text[] = "00:00";

  char *time_format;


  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  strftime(time_text, sizeof(time_text), time_format, pbltime);

  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }
  if (needToUpdateLine(&line2, line2Str, time_text)) {
    updateLineTo(&line2, line2Str, time_text);
  }

}

void display_second(struct tm* pbltime) {

  //  check for empty time

  time_t now;
  if (pbltime == NULL) {
    now = time(NULL);
    pbltime = localtime(&now);
  }


  static char second_text[] = "00";

  char *second_format;


  second_format = "%S";
  strftime(second_text, sizeof(second_text), second_format, pbltime);
  text_layer_set_text(line4, second_text);
  //if (needToUpdateLine(&line4, line4Str, second_text)) {
  //  updateLineTo(&line4, line4Str, second_text);
  //}

}

static void remove_invert() {
    if (inverter_layer != NULL) {
		layer_remove_from_parent(inverter_layer_get_layer(inverter_layer));
		inverter_layer_destroy(inverter_layer);
		inverter_layer = NULL;
    }
}

static void set_invert() {
    if (!inverter_layer) {
		inverter_layer = inverter_layer_create(GRect(0, 0, 144, 168));
		layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(inverter_layer));
		layer_mark_dirty(inverter_layer_get_layer(inverter_layer));
    }
}
  
void lost_connection_warning(void *);

void handle_bluetooth(bool connected) {
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_images[connected ? 1 : 0]);
  layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer),
    bluetooth_mode == BLUETOOTH_MODE_NEVER ||
    (bluetooth_mode == BLUETOOTH_MODE_IFOFF && connected));
  if (was_connected && !connected && connlost_mode == CONNLOST_MODE_WARN)
      lost_connection_warning((void*) 0);
  was_connected = connected;
}

void lost_connection_warning(void *data) {
  int count = (int) data;
  bool on_off = count & 1;
  // blink icon
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_images[on_off ? 1 : 0]);
  layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), false);
  // buzz 3 times
  if (count < 6 && !on_off)
    vibes_short_pulse();
  if (count < 50) // blink for 15 seconds
    app_timer_register(300, lost_connection_warning, (void*) (count+1));
  else // restore bluetooth icon
    handle_bluetooth(bluetooth_connection_service_peek());
}

void handle_battery(BatteryChargeState charge_state) {
	bitmap_layer_set_bitmap(battery_layer, battery_images[
    (charge_state.is_charging ? 11 : 0) + min(charge_state.charge_percent / 10, 10)]);
  bool battery_is_low = charge_state.charge_percent <= 20;
  bool showBattery = battery_mode == BATTERY_MODE_ALWAYS
    || (battery_mode == BATTERY_MODE_IF_LOW && battery_is_low)
    || charge_state.is_charging;

  layer_set_hidden(bitmap_layer_get_layer(battery_layer), !showBattery);

}

// Configure for the time line
void configureLightLayer(TextLayer *textlayer)
{
  text_layer_set_font(textlayer, time_font);
  text_layer_set_text_color(textlayer, GColorWhite  );
  text_layer_set_background_color(textlayer, GColorBlack );
  text_layer_set_text_alignment(textlayer, GTextAlignmentCenter);
}

// Configure the first line of text
void configureBoldLayer(TextLayer *textlayer)
{
  text_layer_set_font(textlayer, second_font);
  text_layer_set_text_color(textlayer, GColorWhite);
  text_layer_set_background_color(textlayer, GColorClear);
  text_layer_set_text_alignment(textlayer, GTextAlignmentRight);
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed);

void handle_second_tick(struct tm *tick_time, TimeUnits units_changed);

static void sync_tuple_changed_callback(const uint32_t key, const Tuple * new_tuple, const Tuple * old_tuple, void *context) {
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "TUPLE! %lu : %d", key, new_tuple->value->uint8);
	if(new_tuple==NULL || new_tuple->value==NULL) {
		return;
	}
    switch (key) {

		case INVERT_KEY:
		    if (mInvert) {				
			    remove_invert();
			}
			mInvert = new_tuple->value->uint8;
		    persist_write_int(INVERT_KEY, mInvert);
			if (mInvert) {
				set_invert();
			}
			break;

		case VIBEMINUTES_KEY:
			mVibeMinutes = new_tuple->value->int32;
		    persist_write_int(VIBEMINUTES_KEY, mVibeMinutes);
			break;
		
		case DATESTYLE_KEY:
			date_mode = new_tuple->value->uint8;
		    persist_write_int(DATESTYLE_KEY, date_mode);		
		    setDate(NULL);
		    break;
		case BATTERY_KEY:
			battery_mode = new_tuple->value->uint8;
			persist_write_int(BATTERY_KEY, battery_mode);
		    handle_battery(battery_state_service_peek());
		    break;
		
		case LANG_KEY:
			language = new_tuple->value->uint8;
			persist_write_int(LANG_KEY, language);
		    setDate(NULL);
			break;
		case BT_KEY:
			bluetooth_mode = new_tuple->value->uint8;
			persist_write_int(BT_KEY, bluetooth_mode);
		    handle_bluetooth(bluetooth_connection_service_peek());
		    break;
		case CONN_KEY:
			connlost_mode = new_tuple->value->uint8;
			persist_write_int(CONN_KEY, connlost_mode);
		    handle_bluetooth(bluetooth_connection_service_peek());
		    break;
	}
}
void init() {

	if (persist_exists(INVERT_KEY)) {
		// If so, read it in to a variable
		mInvert = persist_read_int(INVERT_KEY);
		persist_write_int(INVERT_KEY, mInvert);

	}
	if (persist_exists(VIBEMINUTES_KEY)) {
		// If so, read it in to a variable
		mVibeMinutes = persist_read_int(VIBEMINUTES_KEY);
	}
	if (persist_exists(DATESTYLE_KEY)) {
		// If so, read it in to a variable
		date_mode = persist_read_int(DATESTYLE_KEY);
	}
	if (persist_exists(BATTERY_KEY)) {
		// If so, read it in to a variable
		battery_mode = persist_read_int(BATTERY_KEY);
	}
	if (persist_exists(LANG_KEY)) {
		// If so, read it in to a variable
		language = persist_read_int(LANG_KEY);
		
	}
	if (persist_exists(BT_KEY)) {
		// If so, read it in to a variable
		bluetooth_mode = persist_read_int(BT_KEY);
	}
	if (persist_exists(CONN_KEY)) {
		// If so, read it in to a variable
		connlost_mode = persist_read_int(CONN_KEY);
	}
	Tuplet initial_values[NUM_CONFIG_KEYS] = {
	TupletInteger(INVERT_KEY, mInvert),
	TupletInteger(VIBEMINUTES_KEY, mVibeMinutes),
	TupletInteger(DATESTYLE_KEY, date_mode),
	TupletInteger(BATTERY_KEY, battery_mode),
	TupletInteger(LANG_KEY, language),
	TupletInteger(BT_KEY, bluetooth_mode),
	TupletInteger(CONN_KEY, connlost_mode)
	};

	app_message_open(128, 128);
	app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values,
			ARRAY_LENGTH(initial_values), sync_tuple_changed_callback, NULL, NULL);
	
  window = window_create();
  window_stack_push(window, true);
  window_set_background_color(window, GColorBlack );
  
    // FOnts
    date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORBITRON_BOLD_14));	
    day_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORBITRON_BOLD_15));	
    second_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORBITRON_BOLD_16));	
	time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WWDIGITAL_BOLD_40));	

  // 1st line layers
  line2.currentLayer = text_layer_create(GRect(0, 47, 144, 50));
  line2.nextLayer = text_layer_create(GRect(144, 47, 144, 50));
  configureLightLayer(line2.currentLayer);
  configureLightLayer(line2.nextLayer);

// seconds layer
	  // 3rd layers
  line4 = text_layer_create(GRect(0, 88, 136, 20));
  //line4 = text_layer_create(GRect(0, 88, 128, 20));
  configureBoldLayer(line4);
  //configureBoldLayer(line4.nextLayer);
	
  //date & day layers
  date = text_layer_create(GRect(0, 146, 142, 18));
  text_layer_set_text_color(date,  GColorWhite  );
  text_layer_set_background_color(date,  GColorClear );
	text_layer_set_font(date, date_font);
  text_layer_set_text_alignment(date, GTextAlignmentRight);
  day = text_layer_create(GRect(0, 128, 142, 20));
  text_layer_set_text_color(day,  GColorWhite );
  text_layer_set_background_color(day,  GColorClear);
  text_layer_set_font(day, day_font);
  text_layer_set_text_alignment(day, GTextAlignmentRight);
	
  // Load layers
  Layer *window_layer = window_get_root_layer(window);
  layer_add_child(window_layer, (Layer*)line2.currentLayer);
  layer_add_child(window_layer, (Layer*)line2.nextLayer);

  layer_add_child(window_layer, (Layer*)line4);
  //layer_add_child(window_layer, (Layer*)line4.nextLayer);
	
  layer_add_child(window_layer, (Layer*)date);
  layer_add_child(window_layer, (Layer*)day);
     battery_images[0] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_0 );  
     battery_images[1] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_10);  
     battery_images[2] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_20 );  
     battery_images[3] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_30);  
	 battery_images[4] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_40 );  
     battery_images[5] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_50);  
	 battery_images[6] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_60 );  
     battery_images[7] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_70);  
	 battery_images[8] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_80 );  
     battery_images[9] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_90);  
	 battery_images[10] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_100 );  
     battery_images[11] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGING_0);  
     battery_images[12] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGING_10);  
     battery_images[13] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGING_20);  
     battery_images[14] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGING_30);  
     battery_images[15] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGING_40);  
     battery_images[16] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGING_50);  
     battery_images[17] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGING_60);  
     battery_images[18] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGING_70);  
     battery_images[19] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGING_80);  
     battery_images[20] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGING_90);  
	 battery_images[21] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGING_100);  

	battery_layer = bitmap_layer_create(GRect(93, 3, 50, 16));
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(battery_layer));

  bluetooth_images[0] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_OFF);  
  bluetooth_images[1] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_ON);  
  bluetooth_layer = bitmap_layer_create(GRect(1, 3, 45, 20));
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(bluetooth_layer));

  display_time(NULL);
  setDate(NULL);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  // Configure time on init so we don't have a blank face
	
 if (secondsMode == SECONDS_MODE_ON)
 {
  display_second(NULL);
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
 }

	battery_state_service_subscribe(&handle_battery);
  handle_battery(battery_state_service_peek());
  bluetooth_connection_service_subscribe(&handle_bluetooth);
  handle_bluetooth(bluetooth_connection_service_peek());
  if (mInvert) {
	set_invert();
  }
}

void deinit() {
	  battery_state_service_unsubscribe();
   app_sync_deinit(&sync);
  tick_timer_service_unsubscribe();

  layer_destroy((Layer*)line2.currentLayer);
  layer_destroy((Layer*)line2.nextLayer);
  layer_destroy((Layer*)line4);
  //layer_destroy((Layer*)line4.nextLayer);
  destroy_property_animation(&line2.currentAnimation);
  destroy_property_animation(&line2.nextAnimation);

  //destroy_property_animation(&line4.currentAnimation);
  //destroy_property_animation(&line4.nextAnimation);


  layer_destroy((Layer*)date);
  layer_destroy((Layer*)day);
  inverter_layer_destroy(inverter_layer);
	  bitmap_layer_destroy(battery_layer);
  for (int i = 0; i < 22; i++)
    gbitmap_destroy(battery_images[i]);
  bitmap_layer_destroy(bluetooth_layer);
  for (int i = 0; i < 2; i++)
    gbitmap_destroy(bluetooth_images[i]);
	
  window_destroy(window);
}

// Time handler called every minute by the system
void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  display_time(tick_time);
  if (units_changed & DAY_UNIT) {
    setDate(tick_time);
  }
	if (mVibeMinutes != 0) {
         if ((tick_time->tm_min % mVibeMinutes == 0 ) && (tick_time->tm_sec==0))
	     {
			vibes_double_pulse();	
	     }
	     if ((mVibeMinutes == 60 ) && (units_changed & HOUR_UNIT))
	    {
			vibes_double_pulse();	
	    }
	};
}

// Time handler called every second by the system
void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  display_second(tick_time);
}

int main(void) {

  init();
  app_event_loop();
  deinit();

}
