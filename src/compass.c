/*
 * Copyright 2014 Paul Mitchell
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pebble.h"
#include "stdlib.h"

#define BG_COLOR GColorBlack
#define FG_COLOR GColorWhite
#define BEZEL_RADIUS 55
#define BEZEL_WIDTH 8
#define SQUARE_RADIUS 15

Window *window;
Layer *window_layer;
Layer *bezel_layer;
Layer *direction_layer;
Layer *heading_layer;
GPath *square_path;

TextLayer *heading_text_label;
TextLayer *heading_angle_label;
char heading_angle_buffer[8];
int16_t heading_angle = 0;

AppSync sync;
uint8_t sync_buffer[32];

enum LocationKey {
  LOCATION_HEADING_KEY = 0x0,
};

const GPathInfo SQUARE_PATH_POINTS = {
  4,
  (GPoint []) {
    {-SQUARE_RADIUS, -SQUARE_RADIUS},
    {-SQUARE_RADIUS, SQUARE_RADIUS},
    {SQUARE_RADIUS, SQUARE_RADIUS},
    {SQUARE_RADIUS, -SQUARE_RADIUS},
  }
};

static void draw_direction(GContext *ctx, GPoint *center, int32_t degrees, const char *text) {

  int32_t angle = TRIG_MAX_ANGLE * degrees / 360;
  GPoint point = {
    .y = ((int16_t)(-cos_lookup(angle) * (int32_t)BEZEL_RADIUS / TRIG_MAX_RATIO) + center->y),
    .x = ((int16_t)(sin_lookup(angle) * (int32_t)BEZEL_RADIUS / TRIG_MAX_RATIO) + center->x),
  };

  gpath_move_to(square_path, point);
  gpath_rotate_to(square_path, angle);
  gpath_draw_filled(ctx, square_path);

  graphics_draw_text(
    ctx, text,
    fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
    GRect(point.x - 12, point.y - 16, 24, 24),
    GTextOverflowModeWordWrap,
    GTextAlignmentCenter,
    NULL);
}

static void heading_update_proc(Layer *layer, GContext *ctx) {

  if (heading_angle < 0) {
    text_layer_set_text(heading_text_label, "NO");
  } else if (heading_angle < 23) {
    text_layer_set_text(heading_text_label, "N");
  } else if (heading_angle < 68) {
    text_layer_set_text(heading_text_label, "NE");
  } else if (heading_angle < 113) {
    text_layer_set_text(heading_text_label, "E");
  } else if (heading_angle < 158) {
    text_layer_set_text(heading_text_label, "SE");
  } else if (heading_angle < 203) {
    text_layer_set_text(heading_text_label, "S");
  } else if (heading_angle < 248) {
    text_layer_set_text(heading_text_label, "SW");
  } else if (heading_angle < 293) {
    text_layer_set_text(heading_text_label, "W");
  } else if (heading_angle < 338) {
    text_layer_set_text(heading_text_label, "NW");
  } else {
    text_layer_set_text(heading_text_label, "N");
  }

  if (heading_angle < 0) {
    text_layer_set_text(heading_angle_label, "HEADING");
  } else {
    snprintf(heading_angle_buffer, sizeof(heading_angle_buffer), "%d°", heading_angle);
    text_layer_set_text(heading_angle_label, heading_angle_buffer);
  }
}

static void bezel_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  graphics_context_set_fill_color(ctx, FG_COLOR);
  graphics_fill_circle(ctx, center, BEZEL_RADIUS + (BEZEL_WIDTH / 2));
  graphics_context_set_fill_color(ctx, BG_COLOR);
  graphics_fill_circle(ctx, center, BEZEL_RADIUS - (BEZEL_WIDTH / 2));
}

static void direction_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  graphics_context_set_fill_color(ctx, BG_COLOR);
  graphics_context_set_text_color(ctx, FG_COLOR);
  draw_direction(ctx, &center, -heading_angle, "N");
  draw_direction(ctx, &center, -heading_angle + 90, "E");
  draw_direction(ctx, &center, -heading_angle + 180, "S");
  draw_direction(ctx, &center, -heading_angle + 270, "W");
}

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d:%d", dict_error, app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context)
{
  switch (key) {
    case LOCATION_HEADING_KEY:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Heading: %d", new_tuple->value->uint8);
      heading_angle = new_tuple->value->int16;
      layer_mark_dirty(window_layer);
      break;
  }
}

static void window_load(Window *window) {

  window_set_background_color(window, BG_COLOR);

  window_layer = window_get_root_layer(window);  
  GRect bounds = layer_get_bounds(window_layer);

  bezel_layer = layer_create(bounds);
  layer_set_update_proc(bezel_layer, bezel_layer_update_proc);
  layer_add_child(window_layer, bezel_layer);

  direction_layer = layer_create(bounds);
  layer_set_update_proc(direction_layer, direction_layer_update_proc);
  layer_add_child(window_layer, direction_layer);

  heading_layer = layer_create(GRect(27, 42, 90, 75));
  layer_set_update_proc(heading_layer, heading_update_proc);
  layer_add_child(window_layer, heading_layer);

  heading_text_label = text_layer_create(GRect(0, 0, 90, 45));
  text_layer_set_text(heading_text_label, "N");
  text_layer_set_text_color(heading_text_label, FG_COLOR);
  text_layer_set_background_color(heading_text_label, GColorClear);
  text_layer_set_text_alignment(heading_text_label, GTextAlignmentCenter);
  text_layer_set_font(heading_text_label, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  layer_add_child(heading_layer, text_layer_get_layer(heading_text_label));

  heading_angle_label = text_layer_create(GRect(0, 40, 90, 35));
  text_layer_set_text(heading_angle_label, "0°");
  text_layer_set_text_color(heading_angle_label, FG_COLOR);
  text_layer_set_background_color(heading_angle_label, GColorClear);
  text_layer_set_text_alignment(heading_angle_label, GTextAlignmentCenter);
  text_layer_set_font(heading_angle_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(heading_layer, text_layer_get_layer(heading_angle_label));

  Tuplet initial_values[] = {
    TupletInteger(LOCATION_HEADING_KEY, (int16_t) -1),
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);
}

static void window_unload(Window *window) {
  app_sync_deinit(&sync);
  text_layer_destroy(heading_text_label);
  text_layer_destroy(heading_angle_label);
  layer_destroy(bezel_layer);
  layer_destroy(direction_layer);
  layer_destroy(heading_layer);
}

static void init(void) {

  window = window_create();

  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  square_path = gpath_create(&SQUARE_PATH_POINTS);

  const int inbound_size = 64;
  const int outbound_size = 64;
  app_message_open(inbound_size, outbound_size);

  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
  gpath_destroy(square_path);
}

int main(void) {
  init();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
  app_event_loop();
  deinit();
}
