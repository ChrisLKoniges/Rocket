#include <pebble.h>
  
#define SCREEN_CENTER_X 72

static int random(int max)
{
  //Credit:  Post edited by FlashBIOS on April 2013
  //Link  :  http://forums.getpebble.com/discussion/4456/random-number-generation
  static long seed = 100;
  seed = (((seed * 214013L + 2531011L) >> 16) & 32767);
  return ((seed % max) + 1);
}

//root window:
static Window *s_main_window;
static Layer *layer_Canvas;

//Background - Backdrop:
static GBitmap *bmBackground_bitmap;
static BitmapLayer *layer_Backdrop;

//TRACK HITS TO MISSES
static int Rocket_Launches = 0;
static int Rocket_Hits = 0;

//EXPLOSION
static GBitmap *bmExplosion_bitmap;
static int Explosion_Starting_X = -17;
static int Explosion_Starting_Y = -17;
static int Explosion_HandW = 16;
static AppTimer *t_Explosion_timer;
static AppTimer *t_Explosion_Explode_timer;
static int Explosions_MSeconds_To_Show_Explosion = 500;
static bool Explosion_Timer_Running = false;
static TextLayer *PercentHit_text_layer;
//JET
static BitmapLayer *layer_Jet;
static GBitmap *bm_Jet;
static int Jet_HandW = 16;
static int iJet_Speed_Choices [10] = { 20, 3, 16, 25, 14, 10, 18, 16, 30, 7 }; //{ 30, 25, 16, 55, 25, 10, 18, 16, 55, 45 };
static unsigned int s_current_Tock_Jet = 10;
static int Jet_Current_x = 0;
static int Jet_y_Starting_Choices [10] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90};
static int Jet_Current_y = 0;
static int Jet_Pixel_Speed = 1; //number of pixels per clock calls
static int Jet_Offscreen_X = 184; //Jet_HandW8 (the actual end of x) + Jet_HandW (the length of the Jet)
static AppTimer *t_Jet_Fly_Launch_timer;
static bool Jet_Timer_Running = false;
//ROCKET
static BitmapLayer *layer_Rocket;
static GBitmap *bmRocketInFlight_bitmap;
static BitmapLayer *layer_Explosion;
static int Rocket_Starting_X = 65;
static int Rocket_Starting_Y = 128;
static int Rocket_Out_Of_Frame_Y = -30;
static int Rocket_Current_X;
static int Rocket_Current_Y;
static int Rocket_Pixel_Speed = 1; //number of pixels per clock calls
static int Rocket_Height = 30;
static int Rocket_Width = 14;
static int iRocket_SpeedNow = 6;
static int iRocket_Speed_Choices [10] = { 44, 45, 40, 35, 30, 25, 20, 15, 10, 5 };
static char *sRocket_Speed_Text [10] = { "150", "200", "250", "300", "350", "400", "450", "500", "550", "600" };
static AppTimer *t_Rocket_Launch_timer;
static bool Rocket_Timer_Running = false;
static TextLayer *Rocket_text_layer;

static void SendCurrentPercent(int PercentNow)
{
  if(PercentNow < 10)
  {
    static char buf_String[] = "12";
    snprintf(buf_String, sizeof(buf_String), "%d", PercentNow);
    buf_String[1] = '%';
    text_layer_set_text(PercentHit_text_layer, buf_String);
  }
  if(PercentNow > 9 && PercentNow < 99)
  {
    static char buf_String[] = "123";
    snprintf(buf_String, sizeof(buf_String), "%d", PercentNow);  
    buf_String[2] = '%';
    text_layer_set_text(PercentHit_text_layer, buf_String);
  }
  if(PercentNow > 99)
  {    
    static char buf_String[] = "1234";    
    snprintf(buf_String, sizeof(buf_String), "%d", PercentNow);    
    buf_String[3] = '%';
    text_layer_set_text(PercentHit_text_layer, buf_String);
  }
}
static int Round(float floatResult)
{
  return (int)(floatResult + 0.5);
}
static void Record_Current_Hit_Percent()
{
  if(Rocket_Launches == 0)
  {
    SendCurrentPercent(0);
  }
  else
  {
    float HitPercent_Float =  (Rocket_Hits * 100)/Rocket_Launches;
    int PercentageOfHits = Round(HitPercent_Float); 
    SendCurrentPercent(PercentageOfHits);     
  }  
}

// *** START ROCKET ***
static void Explosion_Stop_Timer()
{
  Explosion_Timer_Running = false;
}
static void Rocket_Fly()
{  
  if(Rocket_Current_Y < Rocket_Out_Of_Frame_Y)
  {
    Explosion_Stop_Timer();
    Rocket_Current_X = Rocket_Starting_X;
    Rocket_Current_Y = Rocket_Starting_Y;
    GRect frame = GRect(Rocket_Starting_X, Rocket_Starting_Y, Rocket_Width, Rocket_Height);
    layer_set_frame(bitmap_layer_get_layer(layer_Rocket), frame);  
  }
  else
  {
     Rocket_Current_Y -= Rocket_Pixel_Speed;
     GRect frame = GRect(Rocket_Current_X, Rocket_Current_Y, Rocket_Width, Rocket_Height);
     layer_set_frame(bitmap_layer_get_layer(layer_Rocket), frame);  
     if(Rocket_Timer_Running)t_Rocket_Launch_timer = app_timer_register(iRocket_Speed_Choices[iRocket_SpeedNow], Rocket_Fly, NULL);        
  }
}
static void Jet_Set_Position_And_Speed()
{    
  Jet_Current_y = Jet_y_Starting_Choices[random(9)];
  s_current_Tock_Jet = iJet_Speed_Choices[random(9)];  
  Record_Current_Hit_Percent();
}
static void Jet_Fly()
{
  if(Jet_Current_x > (Jet_Offscreen_X + Jet_HandW))
  {
    Jet_Current_x = 0;
    Jet_Set_Position_And_Speed();
  }  

  if(Jet_Timer_Running)
  {
    Jet_Current_x += Jet_Pixel_Speed;
    GRect frame = GRect(Jet_Current_x, Jet_Current_y, Jet_HandW, Jet_HandW);
    layer_set_frame(bitmap_layer_get_layer(layer_Jet), frame);    
    t_Jet_Fly_Launch_timer = app_timer_register(s_current_Tock_Jet, Jet_Fly, NULL);
  }  
}
static void Explosion_Clear_Explosion_Effects()
{
  Jet_Timer_Running = true;
  Jet_Fly();
  GRect frame = GRect(Explosion_Starting_X, Explosion_Starting_Y, Explosion_HandW, Explosion_HandW);
  layer_set_frame(bitmap_layer_get_layer(layer_Explosion), frame);      
}
static void Explosion_Explode()
{
  Explosion_Timer_Running = false;
  //Set the explosion image to be where the Jet image is:  
  layer_set_frame(bitmap_layer_get_layer(layer_Explosion), GRect(Jet_Current_x, Jet_Current_y, Explosion_HandW, Explosion_HandW));    
  
  Jet_Timer_Running = false;
  //Set the Jet image to be off screen:  
  layer_set_frame(bitmap_layer_get_layer(layer_Jet), GRect(-20, -20, Jet_HandW, Jet_HandW));    
  Jet_Current_x = 0;
  Jet_Set_Position_And_Speed();
  
  Rocket_Timer_Running = false;
  //Set the Rocket image to be back at launch position:
  Rocket_Current_X = Rocket_Starting_X;
  Rocket_Current_Y = Rocket_Starting_Y;
  layer_set_frame(bitmap_layer_get_layer(layer_Rocket), GRect(Rocket_Starting_X, Rocket_Starting_Y, Rocket_Width, Rocket_Height)); 

  t_Explosion_Explode_timer = app_timer_register(Explosions_MSeconds_To_Show_Explosion, Explosion_Clear_Explosion_Effects, NULL);
}


static void Explosion_Check_For_Hit()
{
  if(Rocket_Current_X < (Jet_Current_x + Jet_HandW) 
     && 
     (Rocket_Current_X + Rocket_Width) > Jet_Current_x
    &&
    Rocket_Current_Y < (Jet_Current_y + Jet_HandW) 
    &&
    (Rocket_Height + Rocket_Current_Y) > Jet_Current_y
    )
  {
    Rocket_Hits++;
    Explosion_Explode();    
  }
  else
  {
    if(Explosion_Timer_Running)
    {
      t_Explosion_timer = app_timer_register(5, Explosion_Check_For_Hit, NULL);  
    }
  }
}
static void Explosion_Check_Start_Timer()
{
  Explosion_Timer_Running = true;
  t_Explosion_timer = app_timer_register(5, Explosion_Check_For_Hit, NULL);     
}
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  //Start Rocket lift off:  
  Rocket_Timer_Running = true;
  Rocket_Launches++;
  t_Rocket_Launch_timer = app_timer_register(iRocket_Speed_Choices[iRocket_SpeedNow], Rocket_Fly, NULL);  
  Explosion_Check_Start_Timer();
 
}
static void up_click_handler(ClickRecognizerRef recognizer, void *context) 
{
  iRocket_SpeedNow++;
  if(iRocket_SpeedNow == 10)iRocket_SpeedNow = 9;
  text_layer_set_text(Rocket_text_layer, sRocket_Speed_Text[iRocket_SpeedNow]);
}
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    iRocket_SpeedNow--;
  if(iRocket_SpeedNow == -1)iRocket_SpeedNow = 0;
  text_layer_set_text(Rocket_text_layer, sRocket_Speed_Text[iRocket_SpeedNow]);
}
// *** STOP ROCKET ***

// *** START - MAIN PROGRAM SETUP AND TEAR DOWN ***

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  
  // *** CANVAS LAYER ***
  // START - Create Background canvas Layer  
  layer_Canvas = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
  layer_add_child(window_layer, layer_Canvas);
  // DONE - Create Background canvas Layer  

  // *** BACKDROP LAYER ***
  //START - Create Backdrop layer:
  bmBackground_bitmap = gbitmap_create_with_resource(RESOURCE_ID_Backdrop);
  layer_Backdrop = bitmap_layer_create(GRect(0, 69, 144, 84)); 
  bitmap_layer_set_bitmap(layer_Backdrop, bmBackground_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(layer_Backdrop));
  //DONE - Create Backdrop layer:
  
  // *** JET ***
  // START - Create Jet, then set to created BitmapLayer
  bm_Jet = gbitmap_create_with_resource(RESOURCE_ID_Jet);
  Jet_Current_x = 0;
  Jet_Current_y = 0;
  Jet_Timer_Running = true;
  layer_Jet = bitmap_layer_create(GRect(Jet_Current_x, Jet_Current_y, Jet_HandW, Jet_HandW));  
  bitmap_layer_set_bitmap(layer_Jet, bm_Jet);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(layer_Jet));
  bitmap_layer_set_compositing_mode(layer_Jet, GCompOpAnd);// visible
  t_Jet_Fly_Launch_timer = app_timer_register(s_current_Tock_Jet, Jet_Fly, NULL);
  // DONE - Create Jet, then set to created BitmapLayer
  
  // *** START EXPLOSION ***
  bmExplosion_bitmap = gbitmap_create_with_resource(RESOURCE_ID_Explosion);
  layer_Explosion = bitmap_layer_create(GRect(Explosion_Starting_X, Explosion_Starting_Y, Explosion_HandW, Explosion_HandW));
  bitmap_layer_set_bitmap(layer_Explosion, bmExplosion_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(layer_Explosion));
  bitmap_layer_set_compositing_mode(layer_Explosion, GCompOpAnd);// visible
  // *** STOP EXPLOSION ***
  
  // *** START ROCKET ***
  
  // START - Create RocketInFlight, then set to created BitmapLayer
  bmRocketInFlight_bitmap = gbitmap_create_with_resource(RESOURCE_ID_RocketInFlight);
  layer_Rocket = bitmap_layer_create(GRect(Rocket_Starting_X, Rocket_Starting_Y, Rocket_Width, Rocket_Height)); //(0, 0, 144, Jet_HandW8));
  Rocket_Current_X = Rocket_Starting_X;
  Rocket_Current_Y = Rocket_Starting_Y;
  Rocket_Timer_Running = true;
  bitmap_layer_set_bitmap(layer_Rocket, bmRocketInFlight_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(layer_Rocket));
  bitmap_layer_set_compositing_mode(layer_Rocket, GCompOpAnd);// visible
  // STOP - Create RocketInFlight, then set to created BitmapLayer
  
  // START - Set up ROCKET SPEED ADJUSTMENTS
  Rocket_text_layer = text_layer_create((GRect) { .origin = { 110, 135 }, .size = { 20, 14 } });
  text_layer_set_text(Rocket_text_layer, sRocket_Speed_Text[iRocket_SpeedNow]);
  text_layer_set_text_alignment(Rocket_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(Rocket_text_layer));
  // STOP - Set up ROCKET SPEED ADJUSTMENTS

  //START - Set up Hit Percentage
  PercentHit_text_layer = text_layer_create((GRect) { .origin = { 16, 135 }, .size = { 30, 14 } });  
  text_layer_set_text_alignment(PercentHit_text_layer, GTextAlignmentCenter);
  SendCurrentPercent(0);
  layer_add_child(window_layer, text_layer_get_layer(PercentHit_text_layer));
  //STOP - Set up Hit Percentage
  
  // *** STOP ROCKET ***
}
static void main_window_unload(Window *window) {
  gbitmap_destroy(bmBackground_bitmap);
  layer_destroy(layer_Canvas);
  gbitmap_destroy(bmRocketInFlight_bitmap);
  bitmap_layer_destroy(layer_Backdrop);
  bitmap_layer_destroy(layer_Jet);
  text_layer_destroy(Rocket_text_layer);
  bitmap_layer_destroy(layer_Explosion);
  gbitmap_destroy(bmExplosion_bitmap);
  text_layer_destroy(PercentHit_text_layer);
}
static void init(void) {
  // Create main Window
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
      .unload = main_window_unload,    
  });

  window_stack_push(s_main_window, true);
}
static void deinit(void) {
  // Destroy main Window
  window_destroy(s_main_window);
}
int main(void) {
  init();
  app_event_loop();
  deinit();
}
// *** STOP - MAIN PROGRAM SETUP AND TEAR DOWN ***