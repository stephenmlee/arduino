#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_UART.h"

//Xmas Tree pins
#define dc   6  // Blue
#define cs   4  // Green
#define rst  7  // Orange
// SCL 13 White 
// SDA 11 Purple

// Bluefruit config
#include "BluefruitConfig.h"
#include <SoftwareSerial.h>

SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);
Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN, BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);
 
//Colour definitions:
#define RED      0xF800
#define GREEN    0x07E0
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF
#define BLACK    0x0000
 
Adafruit_SSD1351 tft = Adafruit_SSD1351(cs, dc, rst);

int LOOP_INTERVAL = 50;

int pushbutton_pin = 3;
int button_state = 0;
int button_countup = 0;

int flux_pin = 5;  // Yellow
bool fluxing = false;
int time_circuits_on = 0; // 0 = Off, 1 = Flux box on, 2 = Pause, 3 = Enable Always-On Mode
int flux_countdown = 0;

int xmas_tree_on = true;
int xmas_tree_level[16];
int xmas_tree_max[16];
int xmas_tree_max_countdown[16];
int xmas_matrix[16][32];
int xmas_min = 4;
int xmas_noise = 2;
int xmas_x_offset = 15;

int time_travel_countdown;


void setup() {
  pinMode(flux_pin, OUTPUT);
  digitalWrite(flux_pin, HIGH);
  pinMode(pushbutton_pin, INPUT_PULLUP);

  ble.begin(false);
  setup_xmas_tree();

}


void loop() 
{
  button_state = digitalRead(pushbutton_pin);
  read_bluefruit();

  if (!fluxing) 
  {
    startup_sequence();
  }
  else 
  {
    refresh_xmas_tree();
    if (button_state == LOW)
    {
      countup();
    }
    if (button_countup >= 1000) 
    {
      disarm_time_circuits();
    }
    if (button_state == HIGH and button_countup > 0) 
    {
      initiate_time_travel();
    }
  }

  delay(LOOP_INTERVAL);    
                  
}


/*
 * Control Circuits
 */
void countup()
{
  if (button_countup == 0) 
  {
    digitalWrite(flux_pin, LOW);
  }
  button_countup += LOOP_INTERVAL;
}


/*
 * Time Circuits On!
 */
void startup_sequence() {
  if (button_state == LOW && time_circuits_on == 0) {
      start_time_circuits();
  }

  if(flux_countdown <= 0)
  {
    next_time_circuit_state();
  }
  else 
  {
    flux_countdown -= LOOP_INTERVAL;
  }  
}


void start_time_circuits() {
      digitalWrite(flux_pin, LOW);
      time_circuits_on = 1;
      flux_countdown = 500;

      power_on_xmas_tree();
}


void next_time_circuit_state() {
  if(time_circuits_on == 1)
  {
    digitalWrite(flux_pin, HIGH);
    time_circuits_on = 2;
    flux_countdown = 500;
  }
  else if (time_circuits_on == 2)
  {
    digitalWrite(flux_pin, LOW);
    time_circuits_on = 3;
    flux_countdown = 2000;
  }
  else if (time_circuits_on == 3)
  {
    digitalWrite(flux_pin, HIGH);
    fluxing = true;
    clear_xmas_tree();
  }
}
 

/*
 * Disarm Time Circuits
 */
void disarm_time_circuits()
{
  digitalWrite(flux_pin, HIGH);
  clear_xmas_tree();
  xmas_tree_on = false;
  if (button_state == HIGH)
  {
    time_circuits_on = 0;
    button_countup = 0;
    fluxing = false;
  }
}


/* 
 *  Travel Through Time!
 */
void initiate_time_travel() 
{
  digitalWrite(flux_pin, HIGH);
  button_countup = 0;
  time_travel_countdown = 3000;
  xmas_noise = 8;
}


/*
 * Xmas Tree
 */
void setup_xmas_tree()
{
  Serial.begin(9600);
  tft.begin();
  tft.fillScreen(BLACK);
  tft.setRotation(2) ;
  clear_xmas_tree();
}


void power_on_xmas_tree()
{
  
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(25+(xmas_x_offset*4),55);
  tft.println("LEE");
  tft.setCursor(4+(xmas_x_offset*4),65);
  tft.println("Industries");

  time_travel_countdown = 0;
  for (int x=0; x<16; x++)
  {
      xmas_tree_level[x] = 1;
      xmas_tree_max[x] = 1;
      xmas_tree_max_countdown[x] = 1000;
      for (int y=0; y<32; y++)
      {
        xmas_matrix[x][y] = 0;
      }
  }

  xmas_tree_on = true;
}


void refresh_xmas_tree()
{
  if (xmas_tree_on)
  {
    set_xmas_levels();
    for (int x=0; x<16; x+=1)
    {
      for (int y=0; y<32; y+=1)
      {
        xmas_refresh_position(x, y);
      }
    }       
  }
}


void set_xmas_levels()
{
  if (time_travel_countdown > 0)
  {
    xmas_increase_min_level();
  }
  for (int x=0; x<16; x++)
  {
    int current_level = xmas_tree_level[x];
    if (random(8)<5 || time_travel_countdown > 0)
    {
      xmas_random_noise(x, current_level);
    }
    else
    {
      // Levels naturally fall each tick - this is heavy!
      xmas_reduce_level(x, current_level);
    }
    xmas_max_level_indicator(x);
  }
}


// Apply some random noise to the xmas display matrix within certain min/max bounds
void xmas_random_noise(int x, int current_level)
{
  xmas_tree_level[x] = random(current_level, current_level + xmas_noise + 1);

  // Clip movement to avoid very large deltas
  xmas_tree_level[x] = max(min(xmas_tree_level[x], xmas_min + xmas_noise), current_level - 2);
  
  // Every now and again add an additional noise spike
  if (random(15) == 1)
  {
    xmas_tree_level[x] += (random(2) + 1);
  }
  if (random(250) == 1)
  {
    xmas_tree_level[x] += (random(8));
  }

  // Ensure we stay on the screen
  xmas_tree_level[x] = min(xmas_tree_level[x], 31);
}


void xmas_reduce_level(int x, int current_level)
{
  xmas_tree_level[x] = max(xmas_min, current_level - 1);
}


void xmas_increase_min_level()
{
    xmas_min = min(xmas_min+3, 31);
    if (time_travel_countdown == LOOP_INTERVAL)
    {
      xmas_min = 4;
      xmas_noise = 2;
    }
    time_travel_countdown -= LOOP_INTERVAL; 
}


void xmas_max_level_indicator(int x)
{
  if (xmas_tree_level[x] > xmas_tree_max[x])
  {
    xmas_tree_max[x] = xmas_tree_level[x];
    xmas_tree_max_countdown[x] = 1500;
  }
  else if (xmas_tree_max_countdown[x] <= 0)
  {
    // Max level indicator is falling down
    xmas_tree_max[x] -= 1;
  }
  xmas_tree_max_countdown[x] -= LOOP_INTERVAL;
}


void xmas_refresh_position(int x, int y)
{
  if (xmas_tree_level[x] < xmas_tree_max[x] && y==xmas_tree_max[x])
  {
    xmas_draw_max_level_indicator(x, y);
  }
  else if (y<=xmas_tree_level[x])
  {
    xmas_fill_position(x, y); 
  }
  else {
    xmas_reset_position(x, y);
  }
}


void xmas_clear_position(int x, int y)
{
  tft.fillRect((x+xmas_x_offset)*4, (32 - y)*4, 4, 4, BLACK);
}


void xmas_draw_max_level_indicator(int x, int y)
{
  xmas_clear_position(x, y);
  tft.fillRect((x+xmas_x_offset)*4, (32 - y)*4, 3, 1, xmas_colour(y));
  xmas_matrix[x][y] = 2; 
}


void xmas_fill_position(int x, int y)
{
  if (xmas_matrix[x][y] != 1)
  {
    xmas_clear_position(x, y);
    xmas_fill_rect(x, y, xmas_colour(y));
    xmas_matrix[x][y] = 1;
  }
}


void xmas_fill_rect(int x, int y, int colour)
{
  tft.fillRect((x+xmas_x_offset)*4, (32 - y)*4, 3, 3, colour);
}


void xmas_reset_position(int x, int y)
{
  if (xmas_matrix[x][y] > 0)
  {
    xmas_clear_position(x, y);
    xmas_matrix[x][y] = 0;
  }
}


int xmas_colour(int y)
{
  if (y < 22)
  {
    return GREEN;
  }
  else if (y < 30)
  {
    return YELLOW;
  }
  else return RED;
}

void clear_xmas_tree()
{
  if (xmas_tree_on)
  {
    tft.fillScreen(BLACK);
  }
}

void read_bluefruit()
{
  String data = String("");

  while ( ble.available() )
  {
    int c = ble.read();
    data += (char)c;
  }

  data.trim();
  data.toUpperCase();
  if ( data == "T" ) 
  {
    // Echo back to bluefruit
    if(!fluxing) 
    {
      ble.print("Time Circuits On!\n");
    }
    else 
    {
      ble.print("Initiating Time Travel Sequence!!\n");
    }
    button_state = LOW;
  }
}
