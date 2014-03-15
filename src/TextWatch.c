#include <pebble.h>
#include <ctype.h>
//#include "num2words-en.h"

#define DEBUG 0
#define BUFFER_SIZE 44

static int mInvert = 0;			// Invert colours (0/1)
static int mVibeMinutes = 0;	// Vibrate every X minutes

Window *window;

static GFont *date_font;
static GFont *day_font;
static GFont *time_font;

typedef struct {
  TextLayer *currentLayer;
  TextLayer *nextLayer;
  PropertyAnimation *currentAnimation;
  PropertyAnimation *nextAnimation;
} Line;

// there used to be more lines
Line line2;

TextLayer *date;
TextLayer *day;

enum {
    INVERT_KEY = 0x1,
    VIBEMINUTES_KEY = 0x3,
	NUM_CONFIG_KEYS = 0x2
};

static AppSync sync;
static uint8_t sync_buffer[128];

static char line2Str[2][BUFFER_SIZE];

static InverterLayer* inverter_layer;

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

//Handle Date
void setDate(struct tm* pbltime)
{

  //  check for empty time
  time_t now;
  if (pbltime == NULL) {
    now = time(NULL);
    pbltime = localtime(&now);
  }
	
  static char dateString[] = "99th September 9999";
  static char dayString[] = "Wednesday";
  //if (pbltime->tm_mon < 8)
 // {
  switch(pbltime->tm_mday)
  {
    case 1 :
    case 21 :
    case 31 :
      strftime(dateString, sizeof(dateString), "%est %B %Y", pbltime);
      break;
    case 2 :
    case 22 :
      strftime(dateString, sizeof(dateString), "%end %B %Y", pbltime);
      break;
    case 3 :
    case 23 :
      strftime(dateString, sizeof(dateString), "%erd %B %Y", pbltime);
      break;
    default :
      strftime(dateString, sizeof(dateString), "%eth %B %Y", pbltime);
      break;
  }
  //}
	//else {
//		 strftime(dateString, sizeof(dateString), "%e %B %Y", pbltime);
	//}
  strftime(dayString, sizeof(dayString), "%A", pbltime);

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
  

// Configure for the time line
void configureLightLayer(TextLayer *textlayer)
{
  text_layer_set_font(textlayer, time_font);
  text_layer_set_text_color(textlayer, GColorWhite  );
  text_layer_set_background_color(textlayer, GColorBlack );
  text_layer_set_text_alignment(textlayer, GTextAlignmentCenter);
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed);

static void sync_tuple_changed_callback(const uint32_t key, const Tuple * new_tuple, const Tuple * old_tuple, void *context) {
     //APP_LOG(APP_LOG_LEVEL_DEBUG, "TUPLE! %lu : %d", key, new_tuple->value->uint8);
	if(new_tuple==NULL || new_tuple->value==NULL) {
		return;
	}
    switch (key) {

		case INVERT_KEY:
		    if (mInvert) {
			    remove_invert();
			}
			mInvert = new_tuple->value->uint8;
			if (mInvert) {
				set_invert();
			}
			break;

		case VIBEMINUTES_KEY:
			mVibeMinutes = new_tuple->value->int32;

			break;


	}
}
void init() {

	Tuplet initial_values[NUM_CONFIG_KEYS] = {
	TupletInteger(INVERT_KEY, mInvert),
	TupletInteger(VIBEMINUTES_KEY, mVibeMinutes)
	};

	app_message_open(128, 128);
	app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values,
			ARRAY_LENGTH(initial_values), sync_tuple_changed_callback, NULL, NULL);
	
  window = window_create();
  window_stack_push(window, true);
  window_set_background_color(window, GColorBlack );
  
    // FOnts
    date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORBITRON_LIGHT_13));	
    day_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORBITRON_BOLD_14));	
	time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORBITRON_BOLD_42));	

  // 1st line layers
  line2.currentLayer = text_layer_create(GRect(0, 47, 144, 50));
  line2.nextLayer = text_layer_create(GRect(144, 47, 144, 50));
  configureLightLayer(line2.currentLayer);
  configureLightLayer(line2.nextLayer);


  //date & day layers
  date = text_layer_create(GRect(0, 150, 142, 15));
  text_layer_set_text_color(date,  GColorWhite  );
  text_layer_set_background_color(date,  GColorClear );
	text_layer_set_font(date, date_font);
  text_layer_set_text_alignment(date, GTextAlignmentRight);
  day = text_layer_create(GRect(0, 135, 142, 18));
  text_layer_set_text_color(day,  GColorWhite );
  text_layer_set_background_color(day,  GColorClear);
  text_layer_set_font(day, day_font);
  text_layer_set_text_alignment(day, GTextAlignmentRight);


  // Configure time on init so we don't have a blank face
  display_time(NULL);
  setDate(NULL);
	
  // Load layers
  Layer *window_layer = window_get_root_layer(window);
  layer_add_child(window_layer, (Layer*)line2.currentLayer);
  layer_add_child(window_layer, (Layer*)line2.nextLayer);

  layer_add_child(window_layer, (Layer*)date);
  layer_add_child(window_layer, (Layer*)day);


  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

void deinit() {
   app_sync_deinit(&sync);
  tick_timer_service_unsubscribe();

  layer_destroy((Layer*)line2.currentLayer);
  layer_destroy((Layer*)line2.nextLayer);

  destroy_property_animation(&line2.currentAnimation);
  destroy_property_animation(&line2.nextAnimation);

  layer_destroy((Layer*)date);
  layer_destroy((Layer*)day);
  inverter_layer_destroy(inverter_layer);
	
  window_destroy(window);
}

// Time handler called every minute by the system
void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  display_time(tick_time);
  if (units_changed & DAY_UNIT) {
    setDate(tick_time);
  }
  if ((tick_time->tm_min % mVibeMinutes == 0 ) && (tick_time->tm_sec==0))
	{
			vibes_double_pulse();	
	}
	  if ((mVibeMinutes == 60 ) && (units_changed & HOUR_UNIT))
	{
			vibes_double_pulse();	
	}
}

int main(void) {

  init();
  app_event_loop();
  deinit();

}
