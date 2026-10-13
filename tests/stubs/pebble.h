#ifndef PEBBLE_STUB_H
#define PEBBLE_STUB_H

/* Host stand-in so TextWatch.c can be typechecked without the Pebble SDK. */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

typedef struct Window Window;
typedef struct Layer Layer;
typedef struct TextLayer TextLayer;
typedef struct BitmapLayer BitmapLayer;
typedef struct GBitmap GBitmap;
typedef struct Animation Animation;
typedef struct PropertyAnimation PropertyAnimation;
typedef struct AppTimer AppTimer;
typedef struct AppSync { uint8_t unused; } AppSync;
typedef struct GFont GFontImpl;
typedef GFontImpl *GFont;

typedef struct { int16_t x; int16_t y; } GPoint;
typedef struct { int16_t w; int16_t h; } GSize;
typedef struct { GPoint origin; GSize size; } GRect;

#define GRect(x, y, w, h) ((GRect){{(int16_t)(x), (int16_t)(y)}, {(int16_t)(w), (int16_t)(h)}})
#define ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

typedef enum {
	GColorClear = 0,
	GColorBlack = 1,
	GColorWhite = 2
} GColor;

typedef enum {
	GTextAlignmentLeft = 0,
	GTextAlignmentCenter = 1,
	GTextAlignmentRight = 2
} GTextAlignment;

typedef enum {
	AnimationCurveEaseOut = 1
} AnimationCurve;

typedef enum {
	SECOND_UNIT = 1 << 0,
	MINUTE_UNIT = 1 << 1,
	HOUR_UNIT = 1 << 2,
	DAY_UNIT = 1 << 3
} TimeUnits;

typedef struct {
	uint8_t charge_percent;
	bool is_charging;
	bool is_plugged;
} BatteryChargeState;

typedef union {
	uint8_t uint8;
	int32_t int32;
} TupleValue;

typedef struct {
	TupleValue *value;
} Tuple;

typedef struct {
	uint32_t key;
	int32_t integer;
} Tuplet;

#define TupletInteger(key, integer) ((Tuplet){(uint32_t)(key), (int32_t)(integer)})

typedef void (*AnimationStoppedHandler)(struct Animation *animation, bool finished, void *context);

typedef struct {
	AnimationStoppedHandler stopped;
} AnimationHandlers;

typedef void (*TickHandler)(struct tm *tick_time, TimeUnits units_changed);
typedef void (*BatteryStateHandler)(BatteryChargeState state);
typedef void (*BluetoothConnectionHandler)(bool connected);
typedef void (*AppTimerCallback)(void *data);
typedef void (*AppSyncTupleChangedCallback)(const uint32_t key, const Tuple *new_tuple,
		const Tuple *old_tuple, void *context);

#define RESOURCE_ID_FONT_ORBITRON_BOLD_14 1
#define RESOURCE_ID_FONT_ORBITRON_BOLD_15 2
#define RESOURCE_ID_FONT_ORBITRON_BOLD_16 3
#define RESOURCE_ID_FONT_WWDIGITAL_BOLD_40 4
#define RESOURCE_ID_IMAGE_BATTERY_0 10
#define RESOURCE_ID_IMAGE_BATTERY_10 11
#define RESOURCE_ID_IMAGE_BATTERY_20 12
#define RESOURCE_ID_IMAGE_BATTERY_30 13
#define RESOURCE_ID_IMAGE_BATTERY_40 14
#define RESOURCE_ID_IMAGE_BATTERY_50 15
#define RESOURCE_ID_IMAGE_BATTERY_60 16
#define RESOURCE_ID_IMAGE_BATTERY_70 17
#define RESOURCE_ID_IMAGE_BATTERY_80 18
#define RESOURCE_ID_IMAGE_BATTERY_90 19
#define RESOURCE_ID_IMAGE_BATTERY_100 20
#define RESOURCE_ID_IMAGE_CHARGING_0 30
#define RESOURCE_ID_IMAGE_CHARGING_10 31
#define RESOURCE_ID_IMAGE_CHARGING_20 32
#define RESOURCE_ID_IMAGE_CHARGING_30 33
#define RESOURCE_ID_IMAGE_CHARGING_40 34
#define RESOURCE_ID_IMAGE_CHARGING_50 35
#define RESOURCE_ID_IMAGE_CHARGING_60 36
#define RESOURCE_ID_IMAGE_CHARGING_70 37
#define RESOURCE_ID_IMAGE_CHARGING_80 38
#define RESOURCE_ID_IMAGE_CHARGING_90 39
#define RESOURCE_ID_IMAGE_CHARGING_100 40
#define RESOURCE_ID_IMAGE_BLUETOOTH_OFF 50
#define RESOURCE_ID_IMAGE_BLUETOOTH_ON 51

Window *window_create(void);
void window_stack_push(Window *window, bool animated);
void window_set_background_color(Window *window, GColor color);
void window_destroy(Window *window);
Layer *window_get_root_layer(Window *window);

void layer_add_child(Layer *parent, Layer *child);
void layer_set_frame(Layer *layer, GRect frame);
GRect layer_get_frame(const Layer *layer);
void layer_destroy(Layer *layer);
void layer_set_hidden(Layer *layer, bool hidden);

TextLayer *text_layer_create(GRect frame);
void text_layer_set_text(TextLayer *text_layer, const char *text);
void text_layer_set_font(TextLayer *text_layer, GFont font);
void text_layer_set_text_color(TextLayer *text_layer, GColor color);
void text_layer_set_background_color(TextLayer *text_layer, GColor color);
void text_layer_set_text_alignment(TextLayer *text_layer, GTextAlignment alignment);

GFont fonts_load_custom_font(uint32_t resource);
uint32_t resource_get_handle(uint32_t resource_id);

GBitmap *gbitmap_create_with_resource(uint32_t resource_id);
void gbitmap_destroy(GBitmap *bitmap);
BitmapLayer *bitmap_layer_create(GRect frame);
void bitmap_layer_destroy(BitmapLayer *bitmap_layer);
void bitmap_layer_set_bitmap(BitmapLayer *bitmap_layer, GBitmap *bitmap);
Layer *bitmap_layer_get_layer(BitmapLayer *bitmap_layer);

bool animation_is_scheduled(Animation *animation);
void animation_unschedule(Animation *animation);
void property_animation_destroy(PropertyAnimation *property_animation);
PropertyAnimation *property_animation_create_layer_frame(Layer *layer, GRect *from_frame, GRect *to_frame);
void animation_set_duration(Animation *animation, uint32_t duration_ms);
void animation_set_curve(Animation *animation, AnimationCurve curve);
void animation_schedule(Animation *animation);
void animation_set_handlers(Animation *animation, AnimationHandlers handlers, void *context);

void tick_timer_service_subscribe(TimeUnits units, TickHandler handler);
void tick_timer_service_unsubscribe(void);

void battery_state_service_subscribe(BatteryStateHandler handler);
void battery_state_service_unsubscribe(void);
BatteryChargeState battery_state_service_peek(void);

void bluetooth_connection_service_subscribe(BluetoothConnectionHandler handler);
bool bluetooth_connection_service_peek(void);

void vibes_short_pulse(void);
void vibes_double_pulse(void);
AppTimer *app_timer_register(uint32_t timeout_ms, AppTimerCallback callback, void *data);

bool clock_is_24h_style(void);
void app_event_loop(void);
void app_message_open(uint32_t inbound_size, uint32_t outbound_size);

bool persist_exists(uint32_t key);
int32_t persist_read_int(uint32_t key);
int persist_write_int(uint32_t key, int32_t value);

void app_sync_init(AppSync *s, uint8_t *buffer, unsigned buffer_size,
		const Tuplet *keys, unsigned count,
		AppSyncTupleChangedCallback changed, void *removed, void *error);
void app_sync_deinit(AppSync *s);

#endif
