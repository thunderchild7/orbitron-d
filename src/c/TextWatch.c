#include <pebble.h>
#include <ctype.h>
#include <stdlib.h>
#include "face_logic.h"

#define DEBUG 0
#define BUFFER_SIZE 44

#define CONNLOST_MODE_IGNORE  0
#define CONNLOST_MODE_WARN    1
#define ANIMATE_MODE_OFF      0
#define ANIMATE_MODE_ON       1

#define SECONDS_MODE_OFF  0
#define SECONDS_MODE_ON   1
	
static int mVibeMinutes = 0;	// Vibrate every X minutes

static int battery_mode   = BATTERY_MODE_NEVER;
static int date_mode      = DATE_MODE_UK;
static int language		= LANG_EN;
static int bluetooth_mode = BLUETOOTH_MODE_NEVER;
static int connlost_mode  = CONNLOST_MODE_IGNORE;
static int animate_mode   = ANIMATE_MODE_ON;

static int secondsMode = SECONDS_MODE_OFF;
static int layout_mode = LAYOUT_D;

static bool is_round(void) {
	return PBL_IF_ROUND_ELSE(true, false);
}

static FaceScreen watch_screen(void) {
	FaceScreen screen;

	screen.width = PBL_DISPLAY_WIDTH;
	screen.height = PBL_DISPLAY_HEIGHT;
	screen.is_round = is_round();
	return screen;
}

Window *window;

static GFont date_font;
static GFont day_font;
static GFont time_font;
static GFont second_font;


static GBitmap *battery_images[22];
static BitmapLayer *battery_layer;

static GBitmap *bluetooth_images[4];
static BitmapLayer *bluetooth_layer;

static Layer *island_layer;
static Layer *island_rim;
static GBitmap *vibe_image;
static BitmapLayer *vibe_layer;
static bool phone_connected = true;
static bool connlost_showing = false;
static int battery_percent = 100;
static bool battery_charging = false;

static void apply_island(void);

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

static AppSync sync;
static uint8_t sync_buffer[256];

static char line2Str[2][BUFFER_SIZE];
static char line4Str[2][BUFFER_SIZE];

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

void setDate(struct tm *pbltime)
{
	static char day_text[FACE_TEXT_CAP];
	static char date_text[FACE_TEXT_CAP];
	time_t now;

	if (pbltime == NULL) {
		now = time(NULL);
		pbltime = localtime(&now);
	}

	face_format_date(day_text, sizeof(day_text), date_text, sizeof(date_text),
			language, date_mode, pbltime);
	text_layer_set_text(date, date_text);
	text_layer_set_text(day, day_text);
}


// Animation handler
void animationStoppedHandler(struct Animation *animation, bool finished, void *context)
{
  Layer *current = (Layer *)context;
  GRect rect = layer_get_frame(current);
  rect.origin.x = rect.size.w;
  layer_set_frame(current, rect);
}

// Animate line
void makeAnimationsForLayers(Line *line, TextLayer *current, TextLayer *next)
{
  GRect rect = layer_get_frame((Layer*)next);
  int slide = rect.size.w;

  rect.origin.x -= slide;

  destroy_property_animation(&(line->nextAnimation));

  line->nextAnimation = property_animation_create_layer_frame((Layer*)next, NULL, &rect);
  animation_set_duration((Animation*)line->nextAnimation, 400);
  animation_set_curve((Animation*)line->nextAnimation, AnimationCurveEaseOut);

  animation_schedule((Animation*)line->nextAnimation);

  GRect rect2 = layer_get_frame((Layer*)current);
  rect2.origin.x -= slide;

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

  return face_line_needs_update(currentStr, nextValue);
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

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed);

static bool seconds_are_drawn(void) {
  FaceFrames frames;
  FaceScreen screen = watch_screen();

  face_layout_frames(&screen, layout_mode, secondsMode == SECONDS_MODE_ON, &frames);
  return frames.seconds.visible;
}

static void place_slot(TextLayer *layer, const FaceSlot *slot) {
  layer_set_frame((Layer *)layer, GRect(slot->x, slot->y, slot->w, slot->h));
  text_layer_set_text_alignment(layer, (GTextAlignment)slot->align);
  layer_set_hidden((Layer *)layer, !slot->visible);
}

static void apply_layout(void) {
  FaceFrames frames;
  FaceScreen screen = watch_screen();
  GRect current;
  GRect next;
  bool current_on_screen;

  if (day == NULL || date == NULL || line4 == NULL || line2.currentLayer == NULL) {
    return;
  }

  face_layout_frames(&screen, layout_mode, secondsMode == SECONDS_MODE_ON, &frames);
  current = layer_get_frame((Layer *)line2.currentLayer);
  next = layer_get_frame((Layer *)line2.nextLayer);
  current_on_screen = abs(current.origin.x - frames.time.x) <= abs(next.origin.x - frames.time.x);
  current.origin.y = frames.time.y;
  current.size.w = frames.time.w;
  current.size.h = frames.time.h;
  next.origin.y = frames.time.y;
  next.size.w = frames.time.w;
  next.size.h = frames.time.h;
  if (current_on_screen) {
    current.origin.x = frames.time.x;
    next.origin.x = frames.time.x + frames.time.w;
  } else {
    next.origin.x = frames.time.x;
    current.origin.x = frames.time.x + frames.time.w;
  }
  layer_set_frame((Layer *)line2.currentLayer, current);
  layer_set_frame((Layer *)line2.nextLayer, next);
  text_layer_set_text_alignment(line2.currentLayer, (GTextAlignment)frames.time.align);
  text_layer_set_text_alignment(line2.nextLayer, (GTextAlignment)frames.time.align);
  place_slot(line4, &frames.seconds);
  place_slot(day, &frames.day);
  place_slot(date, &frames.date);
}

static void refresh_tick(void) {
  tick_timer_service_unsubscribe();
  if (seconds_are_drawn()) {
    tick_timer_service_subscribe(SECOND_UNIT, handle_minute_tick);
  } else {
    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
    if (line4 != NULL) {
      text_layer_set_text(line4, "");
    }
  }
}

void display_time(struct tm* pbltime) {

  //  check for empty time

  time_t now;
  if (pbltime == NULL) {
    now = time(NULL);
    pbltime = localtime(&now);
  }

  static char time_text[] = "00:00";

  face_format_clock(time_text, sizeof(time_text), pbltime, clock_is_24h_style());
  if (needToUpdateLine(&line2, line2Str, time_text)) {
    updateLineTo(&line2, line2Str, time_text);
  }
 if (seconds_are_drawn())
 {
    display_second(NULL);
 }
}



void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed);

void handle_second_tick(struct tm *tick_time, TimeUnits units_changed);


void lost_connection_warning(void *);

/* Largest dx with dx^2 + dy^2 <= radius^2. Diameter is 2 * radius + 1. */
static int capsule_dx(int radius, int dy) {
  int dx = radius;

  if (dy < 0) {
    dy = -dy;
  }
  while (dx > 0 && (dx * dx + dy * dy) > (radius * radius)) {
    dx--;
  }
  return dx;
}

static void draw_row(GContext *ctx, int y, int left, int right) {
  if (right < left) {
    return;
  }
  if (left == right) {
    graphics_draw_pixel(ctx, GPoint(left, y));
    return;
  }
  graphics_draw_line(ctx, GPoint(left, y), GPoint(right, y));
}

/* White capsule, then a black capsule one pixel inside. Icons paint next. */
static void island_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int radius = (bounds.size.h - 1) / 2;
  int left_c = radius;
  int right_c = bounds.size.w - 1 - radius;
  int y;

  if (bounds.size.w < radius * 2 || radius < 2) {
    return;
  }

  graphics_context_set_stroke_color(ctx, GColorWhite);
  for (y = 0; y <= radius * 2; y++) {
    int dx = capsule_dx(radius, y - radius);
    draw_row(ctx, y, left_c - dx, right_c + dx);
  }

  graphics_context_set_stroke_color(ctx, GColorBlack);
  for (y = 1; y <= radius * 2 - 1; y++) {
    int dx = capsule_dx(radius - 1, y - radius);
    draw_row(ctx, y, left_c - dx, right_c + dx);
  }
}

/* Icons are child layers, so redraw the ring after them. */
static void island_rim_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int radius = (bounds.size.h - 1) / 2;
  int left_c = radius;
  int right_c = bounds.size.w - 1 - radius;
  int y;

  if (bounds.size.w < radius * 2 || radius < 2) {
    return;
  }

  graphics_context_set_stroke_color(ctx, GColorWhite);
  for (y = 0; y <= radius * 2; y++) {
    int dx = capsule_dx(radius, y - radius);
    int outer_left = left_c - dx;
    int outer_right = right_c + dx;

    if (y == 0 || y == radius * 2) {
      draw_row(ctx, y, outer_left, outer_right);
    } else {
      int idx = capsule_dx(radius - 1, y - radius);
      draw_row(ctx, y, outer_left, left_c - idx - 1);
      draw_row(ctx, y, right_c + idx + 1, outer_right);
    }
  }
}

static void place_icon(BitmapLayer *layer, const FaceIcon *icon) {
  layer_set_frame(bitmap_layer_get_layer(layer), GRect(icon->x, icon->y, icon->w, icon->h));
  layer_set_hidden(bitmap_layer_get_layer(layer), !icon->visible);
}

static void apply_island(void) {
  FaceIsland island;
  FaceScreen screen = watch_screen();
  bool show_bluetooth;
  bool show_battery;

  if (island_layer == NULL) {
    return;
  }

  show_bluetooth = connlost_showing
      || !face_bluetooth_hidden(bluetooth_mode, phone_connected);
  show_battery = face_battery_visible(battery_mode, battery_percent, battery_charging);
  face_status_island(&screen, show_bluetooth, show_battery, mVibeMinutes > 0, &island);
  layer_set_hidden(island_layer, !island.visible);
  if (island.visible) {
    layer_set_frame(island_layer, GRect(island.x, island.y, island.w, island.h));
    if (island_rim != NULL) {
      layer_set_frame(island_rim, GRect(0, 0, island.w, island.h));
    }
  }
  place_icon(bluetooth_layer, &island.bluetooth);
  place_icon(battery_layer, &island.battery);
  place_icon(vibe_layer, &island.vibe);
}

void handle_bluetooth(bool connected) {
  phone_connected = connected;
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_images[connected ? 1 : 0]);
  if (was_connected && !connected && connlost_mode == CONNLOST_MODE_WARN) {
    was_connected = connected;
    lost_connection_warning((void*) 0);
    return;
  }
  was_connected = connected;
  connlost_showing = false;
  apply_island();
}

void lost_connection_warning(void *data) {
  int count = (int) data;
  bool icon_on = false;
  bool buzz = false;
  bool keep_going = false;

  face_connlost_frame(count, &icon_on, &buzz, &keep_going);
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_images[icon_on ? 1 : 0]);
  connlost_showing = keep_going;
  if (buzz)
    vibes_short_pulse();
  if (keep_going) {
    apply_island();
    app_timer_register(300, lost_connection_warning, (void*) (count+1));
  } else {
    handle_bluetooth(bluetooth_connection_service_peek());
  }
}

void handle_battery(BatteryChargeState charge_state) {
  battery_percent = charge_state.charge_percent;
  battery_charging = charge_state.is_charging;
  bitmap_layer_set_bitmap(battery_layer, battery_images[
      face_battery_image_index(charge_state.charge_percent, charge_state.is_charging)]);
  apply_island();
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


static void sync_tuple_changed_callback(const uint32_t key, const Tuple * new_tuple, const Tuple * old_tuple, void *context) {
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "TUPLE! %lu : %d", key, new_tuple->value->uint8);
	if(new_tuple==NULL) {
		return;
	}
    switch (key) {

		case VIBEMINUTES_KEY:
			mVibeMinutes = new_tuple->value->int32;
		    persist_write_int(VIBEMINUTES_KEY, mVibeMinutes);
			apply_island();
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
			language = face_supported_language(new_tuple->value->uint8);
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
		
		case SHOW_SECS:
			secondsMode = new_tuple->value->uint8;
		    persist_write_int(SHOW_SECS, secondsMode);
			apply_layout();
			refresh_tick();
			break;

		case LAYOUT_KEY:
			layout_mode = face_supported_layout(new_tuple->value->uint8, is_round());
			persist_write_int(LAYOUT_KEY, layout_mode);
			apply_layout();
			refresh_tick();
			break;
		
		case ANIMATE_KEY:
			animate_mode = new_tuple->value->uint8;
			persist_write_int(ANIMATE_KEY, animate_mode);
	}
}
void init() {

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
		language = face_supported_language(persist_read_int(LANG_KEY));
		
	}
	if (persist_exists(BT_KEY)) {
		// If so, read it in to a variable
		bluetooth_mode = persist_read_int(BT_KEY);
	}
	if (persist_exists(CONN_KEY)) {
		// If so, read it in to a variable
		connlost_mode = persist_read_int(CONN_KEY);
	}
	
	if (persist_exists(SHOW_SECS)) {
		// If so, read it in to a variable
		secondsMode = persist_read_int(SHOW_SECS);
		persist_write_int(SHOW_SECS, secondsMode);
	}

	if (persist_exists(LAYOUT_KEY)) {
		layout_mode = persist_read_int(LAYOUT_KEY);
	}
	layout_mode = face_supported_layout(layout_mode, is_round());
	
	if (persist_exists(ANIMATE_KEY)) {
		// If so, read it in to a variable
		animate_mode = persist_read_int(ANIMATE_KEY);
	}
	Tuplet initial_values[] = {
	TupletInteger(VIBEMINUTES_KEY, mVibeMinutes),
	TupletInteger(DATESTYLE_KEY, date_mode),
	TupletInteger(BATTERY_KEY, battery_mode),
	TupletInteger(LANG_KEY, language),
	TupletInteger(BT_KEY, bluetooth_mode),
	TupletInteger(CONN_KEY, connlost_mode),
	TupletInteger(SHOW_SECS, secondsMode),
	TupletInteger(ANIMATE_KEY, animate_mode),
	TupletInteger(LAYOUT_KEY, layout_mode)
	};

	app_message_open(256, 256);
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

	island_layer = layer_create(GRect(0, 1, 144, 25));
  layer_set_update_proc(island_layer, island_update_proc);
  layer_add_child(window_get_root_layer(window), island_layer);

  battery_layer = bitmap_layer_create(GRect(0, 0, 50, 16));
  layer_add_child(island_layer, bitmap_layer_get_layer(battery_layer));

  bluetooth_images[0] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_OFF);  
  bluetooth_images[1] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_ON);  
  bluetooth_layer = bitmap_layer_create(GRect(0, 0, 45, 16));
  layer_add_child(island_layer, bitmap_layer_get_layer(bluetooth_layer));

  vibe_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_VIBRATION_ON);
  vibe_layer = bitmap_layer_create(GRect(0, 0, 20, 20));
  bitmap_layer_set_bitmap(vibe_layer, vibe_image);
  layer_add_child(island_layer, bitmap_layer_get_layer(vibe_layer));

  island_rim = layer_create(GRect(0, 0, 144, 25));
  layer_set_update_proc(island_rim, island_rim_proc);
  layer_add_child(island_layer, island_rim);

  apply_layout();
  // Configure time on init so we don't have a blank face
  display_time(NULL);
  setDate(NULL);
  refresh_tick();

  battery_state_service_subscribe(&handle_battery);
  handle_battery(battery_state_service_peek());
  bluetooth_connection_service_subscribe(&handle_bluetooth);
  handle_bluetooth(bluetooth_connection_service_peek());
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
  bitmap_layer_destroy(battery_layer);
  for (int i = 0; i < 22; i++)
    gbitmap_destroy(battery_images[i]);
  bitmap_layer_destroy(bluetooth_layer);
  for (int i = 0; i < 2; i++)
    gbitmap_destroy(bluetooth_images[i]);
  bitmap_layer_destroy(vibe_layer);
  gbitmap_destroy(vibe_image);
  layer_destroy(island_rim);
  layer_destroy(island_layer);
	
  window_destroy(window);
}

// Time handler called every minute by the system
void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  display_time(tick_time);
  if (units_changed & DAY_UNIT) {
    setDate(tick_time);
  }
	{
		int pulses = face_vibe_pulses(tick_time->tm_min, tick_time->tm_sec,
				mVibeMinutes, (units_changed & HOUR_UNIT) != 0);
		while (pulses > 0) {
			vibes_double_pulse();
			pulses--;
		}
	}
}


int main(void) {

  init();
  app_event_loop();
  deinit();

}
