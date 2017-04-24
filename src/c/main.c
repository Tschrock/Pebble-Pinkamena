#include <pebble.h>

static GRect timePos;
static GRect timePos2;
static GRect bgPos;
static GRect batteryPos;
static GRect batteryIconSize;
static GRect bluetoothPos;
static GRect bluetoothIconSize;
static GRect datePos;

static void initVars() {
  timePos = GRect(89, 4, 54, 40);
  timePos2 = GRect(89, 35, 54, 40);
  bgPos = GRect(0, 0, 144, 168);
  batteryPos = GRect(123, 3, 17, 10);
  batteryIconSize = GRect(0, 0, 17, 10);
  bluetoothPos = GRect(3, 3, 13, 12);
  bluetoothIconSize = GRect(0, 0, 13, 12);
  datePos = GRect(0, 147, 144, 25);
}
  

//
// Main Variables
//

// Main Window
static Window *s_main_window;

// Text (Clock)
static TextLayer *s_time_layer;
static TextLayer *s_time_layer2;

// Text (Date)
static TextLayer *s_date_layer;

// Background
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

#ifdef PBL_BW
  static BitmapLayer *s_background_layer2;
  static GBitmap *s_background_bitmap2;
#endif

// Battery
static BitmapLayer *s_battery_layer;
static GBitmap *s_battery_bitmap;
static GBitmap *s_battery_subbitmap;

#ifdef PBL_BW
  static BitmapLayer *s_battery_layer2;
  static GBitmap *s_battery_bitmap2;
  static GBitmap *s_battery_subbitmap2;
#endif

// Bluetooth
static BitmapLayer *s_bluetooth_layer;
static GBitmap *s_bluetooth_bitmap;
static GBitmap *s_bluetooth_subbitmap;

#ifdef PBL_BW
  static BitmapLayer *s_bluetooth_layer2;
  static GBitmap *s_bluetooth_bitmap2;
  static GBitmap *s_bluetooth_subbitmap2;
#endif

#define KEY_DATA 5

//
// Persistant Settings
//

typedef struct Settings {
  bool blackBG;
  bool showDate;
  bool showBattery;
  bool showBluetooth;
  bool vibeDisconnect;
  bool vibeConnect;
} Settings;

#define PERSIST_KEY_SETTINGS 10
static Settings settings;

static void initSettings() {
  if (persist_exists(PERSIST_KEY_SETTINGS)) {
    persist_read_data(PERSIST_KEY_SETTINGS, &settings, sizeof(settings));
  }
  else {
    settings = (Settings) {false, true, true, true, true, true};
  }
}

static void deinitSettings() {
  persist_write_data(PERSIST_KEY_SETTINGS, &settings, sizeof(settings));
}

static void applySettings() {

  GColor bgColor = settings.blackBG ? GColorBlack : GColorWhite;
  GColor fgColor = settings.blackBG ? GColorWhite : GColorBlack;

  text_layer_set_text_color(s_time_layer, fgColor);
  text_layer_set_text_color(s_time_layer2, fgColor);
  text_layer_set_text_color(s_date_layer, fgColor);
  window_set_background_color(s_main_window, bgColor);

  layer_set_hidden(text_layer_get_layer(s_date_layer), !settings.showDate);
  layer_set_hidden(bitmap_layer_get_layer(s_battery_layer), !settings.showBattery);
  layer_set_hidden(bitmap_layer_get_layer(s_bluetooth_layer), !settings.showBluetooth);

  #ifdef PBL_BW
    text_layer_set_background_color(s_date_layer, settings.blackBG ? GColorBlack : GColorClear);
    layer_set_hidden(bitmap_layer_get_layer(s_battery_layer2), !settings.showBattery);
    layer_set_hidden(bitmap_layer_get_layer(s_bluetooth_layer2), !settings.showBluetooth);
  #endif

}


//
// Spritesheet functions
//

static GBitmap* getBatteryIcon(const GBitmap * base_bitmap, const BatteryChargeState batState){
  if(batState.is_charging){
    return gbitmap_create_as_sub_bitmap(base_bitmap, GRect(batteryIconSize.origin.x, 110, batteryIconSize.size.w, batteryIconSize.size.h));
  }
  else {
    int pos = batState.charge_percent / 10;
    //APP_LOG(APP_LOG_LEVEL_INFO, "batState.charge_percent = %i", batState.charge_percent);
    return gbitmap_create_as_sub_bitmap(base_bitmap, GRect(batteryIconSize.origin.x, pos * batteryIconSize.size.h, batteryIconSize.size.w, batteryIconSize.size.h));
  }
}
static GBitmap* getBluetoothIcon(const GBitmap * base_bitmap, const bool blueState){
  if(blueState){
    return gbitmap_create_as_sub_bitmap(base_bitmap, GRect(bluetoothIconSize.origin.x, 0, bluetoothIconSize.size.w, bluetoothIconSize.size.h));
  }
  else {
    return gbitmap_create_as_sub_bitmap(base_bitmap, GRect(bluetoothIconSize.origin.x, bluetoothIconSize.size.h, bluetoothIconSize.size.w, bluetoothIconSize.size.h));
  }
}

static void swapSubBitmap(GBitmap ** oldBitmapPtr, GBitmap * newBitmap, BitmapLayer * bitmapLayer){
  bitmap_layer_set_bitmap(bitmapLayer, newBitmap);  
  gbitmap_destroy(*oldBitmapPtr);
  *oldBitmapPtr = newBitmap;
}


//
// Time Update functions
//

static void update_time() {

  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  static char buffer[] = "00";
  static char buffer2[] = "00";

  if(clock_is_24h_style() == true) {
    strftime(buffer, sizeof("00"), "%H", tick_time);
  } else {
    strftime(buffer, sizeof("00"), "%I", tick_time);
  }

  strftime(buffer2, sizeof("00"), "%M", tick_time);
  

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
  text_layer_set_text(s_time_layer2, buffer2);
  
  static char buffer3[] = "Thu Aug 23 2001";
  strftime(buffer3, sizeof("Thu Aug 23 2001"), "%a %b %e %Y", tick_time);
  
  text_layer_set_text(s_date_layer, buffer3);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

//
// Battery Update functions
//

static void checkBattery_(BatteryChargeState batState) {
  swapSubBitmap(&s_battery_subbitmap, getBatteryIcon(s_battery_bitmap, batState), s_battery_layer);
  #ifdef PBL_BW
    swapSubBitmap(&s_battery_subbitmap2, getBatteryIcon(s_battery_bitmap2, batState), s_battery_layer2);
  #endif
}

static void checkBattery() {
  checkBattery_(battery_state_service_peek());
}

//
// Bluetooth Update functions
//

static bool oldBlthState = false;
static bool oldBlthStateInit = false;

static void checkBluetooth_(bool connected) {
  if (connected) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "Phone is connected!");
    if(settings.vibeConnect && oldBlthStateInit && !oldBlthState){
      vibes_double_pulse();
    }
  } else {
    //APP_LOG(APP_LOG_LEVEL_INFO, "Phone is not connected!");
    if(settings.vibeDisconnect && oldBlthStateInit&& oldBlthState){
      vibes_double_pulse();
    }
  }
  oldBlthState = connected;
  oldBlthStateInit = true;
  
  swapSubBitmap(&s_bluetooth_subbitmap, getBluetoothIcon(s_bluetooth_bitmap, connected), s_bluetooth_layer);
  #ifdef PBL_BW
    swapSubBitmap(&s_bluetooth_subbitmap2, getBluetoothIcon(s_bluetooth_bitmap2, connected), s_bluetooth_layer2);
  #endif
}

static void checkBluetooth() {
  checkBluetooth_(bluetooth_connection_service_peek());
}

//
// AppMessage Functions
//

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Get the first pair
  Tuple *t = dict_read_first(iterator);
  int rtn;

  // Process all pairs present
  while(t != NULL) {
    // Process this pair's key
    switch (t->key) {
      case KEY_DATA:
        rtn = t->value->int32;
        //APP_LOG(APP_LOG_LEVEL_INFO, "KEY_DATA received with value %d", rtn);
        settings.blackBG = (rtn & 1) != 0;
        settings.showDate = (rtn & 2) != 0;
        settings.showBattery = (rtn & 4) != 0;
        settings.showBluetooth = (rtn & 8) != 0;
        settings.vibeDisconnect = (rtn & 16) != 0;
        settings.vibeConnect = (rtn & 32) != 0;
        applySettings();
        break;
    }

    // Get next pair, if any
    t = dict_read_next(iterator);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

//
// Window functions
//

static void main_window_load(Window *window) {
  
  //
  // Background Setup
  //
  
  s_background_layer = bitmap_layer_create(bgPos);

  #ifdef PBL_COLOR
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BG_COLOR);
    bitmap_layer_set_compositing_mode(s_background_layer, GCompOpSet);
  #else
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BG_BAW_BLACK);
    bitmap_layer_set_compositing_mode(s_background_layer, GCompOpClear);
  #endif
    
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
    
  #ifdef PBL_BW
    s_background_layer2 = bitmap_layer_create(bgPos);
    s_background_bitmap2 = gbitmap_create_with_resource(RESOURCE_ID_BG_BAW_WHITE);
    
    bitmap_layer_set_compositing_mode(s_background_layer2, GCompOpOr);
    bitmap_layer_set_bitmap(s_background_layer2, s_background_bitmap2);
  
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer2));
  #endif
  
  
  //
  // Battery Indicator Setup
  //
  
  s_battery_layer = bitmap_layer_create(batteryPos);

  #ifdef PBL_COLOR
    s_battery_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BATT_COLOR);
    bitmap_layer_set_compositing_mode(s_battery_layer, GCompOpSet);
  #else
    s_battery_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BATT_BAW_BLACK);
    bitmap_layer_set_compositing_mode(s_battery_layer, GCompOpClear);
  #endif
  
  s_battery_subbitmap = getBatteryIcon(s_battery_bitmap, battery_state_service_peek());
  bitmap_layer_set_bitmap(s_battery_layer, s_battery_subbitmap);
  
  
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_battery_layer));
    
  #ifdef PBL_BW
    s_battery_layer2 = bitmap_layer_create(batteryPos);

    s_battery_bitmap2 = gbitmap_create_with_resource(RESOURCE_ID_BATT_BAW_WHITE);
    
    bitmap_layer_set_compositing_mode(s_battery_layer2, GCompOpOr);
  
    s_battery_subbitmap2 = getBatteryIcon(s_battery_bitmap2, battery_state_service_peek());
    bitmap_layer_set_bitmap(s_battery_layer2, s_battery_subbitmap2);
  
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_battery_layer2));
  #endif
  
  
  //
  // Bluetooth Indicator Setup
  //
  
  s_bluetooth_layer = bitmap_layer_create(bluetoothPos);

  #ifdef PBL_COLOR
    s_bluetooth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BLTH_COLOR);
    bitmap_layer_set_compositing_mode(s_bluetooth_layer, GCompOpSet);
  #else
    s_bluetooth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BLTH_BAW_BLACK);
    bitmap_layer_set_compositing_mode(s_bluetooth_layer, GCompOpClear);
  #endif
  
  s_bluetooth_subbitmap = getBluetoothIcon(s_bluetooth_bitmap, bluetooth_connection_service_peek());
  bitmap_layer_set_bitmap(s_bluetooth_layer, s_bluetooth_subbitmap);
  
  
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bluetooth_layer));
    
  #ifdef PBL_BW
    s_bluetooth_layer2 = bitmap_layer_create(bluetoothPos);

    s_bluetooth_bitmap2 = gbitmap_create_with_resource(RESOURCE_ID_BLTH_BAW_WHITE);
    
    bitmap_layer_set_compositing_mode(s_bluetooth_layer2, GCompOpOr);
  
    s_bluetooth_subbitmap2 = getBluetoothIcon(s_bluetooth_bitmap2, bluetooth_connection_service_peek());
    bitmap_layer_set_bitmap(s_bluetooth_layer2, s_bluetooth_subbitmap2);
  
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bluetooth_layer2));
  #endif
  
  
  //
  // Time Setup
  //
    
  ResHandle font_handle = resource_get_handle(RESOURCE_ID_ROBOTO_BOLD_40);
  
  // Create time TextLayer
  s_time_layer = text_layer_create(timePos);
  text_layer_set_background_color(s_time_layer, GColorClear);

  // Improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer, fonts_load_custom_font(font_handle));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  // Create time TextLayer2
  s_time_layer2 = text_layer_create(timePos2);
  text_layer_set_background_color(s_time_layer2, GColorClear);

  // Improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer2, fonts_load_custom_font(font_handle));
  text_layer_set_text_alignment(s_time_layer2, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer2));

  
  //
  // Date Setup
  //
  
  ResHandle font_handle2 = resource_get_handle(RESOURCE_ID_ROBOTO_BOLD_18);
  
  // Create time TextLayer
  s_date_layer = text_layer_create(datePos);
  text_layer_set_background_color(s_date_layer, GColorClear);

  // Improve the layout to be more like a watchface
  text_layer_set_font(s_date_layer, fonts_load_custom_font(font_handle2));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));


  // finish up
  applySettings();
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  
  // Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);
  bitmap_layer_destroy(s_background_layer);
  
  gbitmap_destroy(s_battery_subbitmap);
  gbitmap_destroy(s_battery_bitmap);
  bitmap_layer_destroy(s_battery_layer);

  #ifdef PBL_BW
    gbitmap_destroy(s_background_bitmap2);
    bitmap_layer_destroy(s_background_layer2);
    gbitmap_destroy(s_battery_subbitmap2);
    gbitmap_destroy(s_battery_bitmap2);
    bitmap_layer_destroy(s_battery_layer2);
  #endif
}


//
// Main App Setup/Loop
//

static void init() {
  initVars();
  initSettings();
  
  // Create main Window element and assign to pointer
  s_main_window = window_create();
    
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Make sure the time is displayed from the start
  update_time();
  checkBluetooth();
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  bluetooth_connection_service_subscribe(checkBluetooth_);
  battery_state_service_subscribe(checkBattery_);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
  
  deinitSettings();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}