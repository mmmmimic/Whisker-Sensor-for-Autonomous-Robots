#define REV_ID "$Id: main.cpp 402 2016-09-11 11:25:36Z jcan $"

#define NUM_LEDS 1  //Numbers of led
#define NUM_SENSORS 3 //Numbers of sensors, for this project, it's 3
#define MAX_STRING_SIZE 40
#define NUM_COMMANDS 8
#define NUM_LOGS 12000*2
#define COUNT_LOGS 0
#define NUM_MEAS 50 //the filter size of the mean value filter
#define TRANSMIT_COUNT 50
#define NUM_DATA 450 //the range of GLR is 450
#define NUM_SAMPLES 1 //this is the sampling rate

// State of flags to do different things
struct Settings
{
  //Note stream and signal: [1] and [7]
  //meas is to activate the mean value filter
  bool states[NUM_COMMANDS] = { false, true, false, false, false, false, true, true};
  char commands[NUM_COMMANDS][MAX_STRING_SIZE] = { "ledon", "stream", "sample", "debug", "log", "sendlog", "meas", "signal" };

};


// Define a data structure called Touch
struct Touch
{
  //cusum and glr are both prepared for glr filter
  bool apply_cusum = false; //default false
  bool glr = false;    //default false
  bool object_detection;  //detect if there are obstacles
  float touch_value;
  float real_touch;  //sample value
  int counter = 0;  //counter --> 50
  float touch_detection = 0; //detect touch
};


// Data for GLR or CuSum function(the past samples)
/*
struct Cusum
{
  int index = 0;
  int num_data_present = 0;
  float mean_data = 0;
  float sd_data = 0;
  float h = 0;
  float S[2] = {0};
  float g[2] = {0};
  float sequence[NUM_DATA] = {0};
  float change_time = 0;
  float k = 0;
  float j = 0;
  float loss;
};
*/


////////////////////////////////////////////////////
////////////////////////////////////////////////////

// This function sets up the leds and tells the controller about them
void setup()
{
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  delay(500);
  digitalWrite(13, LOW);  //Initialization blink
  delay(1000);
  Serial.begin(9600);  //bode rate
}


// How many pins is on the microprocessor Teensy 3.2
const int MAX_PIN_NUM = 34;

// Pins with the touch functions initial value and vector.
int noTouch[MAX_PIN_NUM] = { 0 }; // Initial position value

//unsigned int touchpin[2] = { 15, 19 };


/* Function gets touch value from the microprocessor
 * It multiplies it by a time constant and makes the value flow up or down
 * It also has flags for streaming and outputting values
 * Outputs an offset in the touch value from a reference value depending on the filter applied.
 */
double getTouch(unsigned int pin, int num_sampels, Touch* touch_data, float t)
{ 
    char id_string[] = "Pin";
    //v can be accumulated by each data
    float v = 0;
    //x is the final v divided by NUM_MEAS
    float x = 0;
    //for IIC filter
    float result = 0;

    // For scaling and decaying
    int FAC = 100000;

    //Average data
    //int FAC = 4000;

    // dont FAC
    //int FAC = 1;
    if (pin < MAX_PIN_NUM) // make sure the pin series is correct
    {
      if (num_sampels > 1)
      {
        for (int k = 0; k < num_sampels; k++)
        {
          v += touchRead(pin);
          if (k == 0) {
            touch_data->real_touch = v;
          }
        }
        num_sampels = (float)(num_sampels);
        x = v / num_sampels * FAC;

      }
      else
      {
        x = touchRead(pin);
        touch_data->real_touch = x;
        x = x * FAC;
      }

      if (noTouch[pin] == 0)
      {
        noTouch[pin] = x;
      }
      else if (x < noTouch[pin] and FAC != 1)
      {
        noTouch[pin] -= 1;
        result = filter(noTouch[pin], x, touch_data, FAC, t);
      }
      else if (x > noTouch[pin] and FAC != 1)
      {
        noTouch[pin] += 1;
        result = filter(noTouch[pin], x, touch_data, FAC, t);
      }
      else {
        result = filter(noTouch[pin], x, touch_data, FAC, t);
      }
    }
    return result / FAC;  // scale it into the original size
  }

  
float e[7] = { 0, 0, 0 };   // e is error
float u[7] = { 0, 0, 0 };   // u is e after transfer function


// Control filter if u[0] = e[0] then the filter is inactive and only a threshold is set.
float filter(float y, float x, Touch* touch_data, int FAC, float t)
{
  e[0] = x - y;
  // One pole transfer and one zero transfer function time constant = 5
  //u[0] = 0.99999988*u[1] + 0.49999997*e[0] - 0.49999997*e[1];

  // No filter
  u[0] = e[0];

  //u[0] = 0.99999878*u[1] + 6.10500238*e[0] - 6.10500238*e[1];

  if (abs(u[0]) > t * FAC)   //set the threshold to be an array
  {
    touch_data->touch_detection = 1;
    touch_data->object_detection = true;
  }
  else
  {
    touch_data->object_detection = false;
    touch_data->touch_detection = 0;
  }

  e[1] = e[0];
  u[1] = u[0];
  e[2] = e[1];
  u[2] = u[1];
  e[3] = e[2];
  u[3] = u[2];
  e[4] = e[3];
  u[4] = u[3];
  e[5] = e[4];
  u[5] = u[4];
  e[6] = e[5];
  u[6] = u[5];


  return u[0];
}

// Determine if there is a touch on the whisker
void determineTouch(unsigned int pin, int num_sampels, Touch* touch_data, float t)
{
  touch_data->touch_value = getTouch(pin, num_sampels, touch_data, t);
  
}

////////////////////////////////////////////////////
//////////////////////main loop/////////////////////

void loop()
{
  // Command set
  Settings data;
  Touch touch_data[NUM_SENSORS];   // store the signals
  bool logged_saved = false;   //even save the log
  //int *data_log = (int*)malloc(NUM_LOGS * sizeof(int));
  unsigned int index[NUM_SENSORS] = {17, 19, 15};
  char message[100];
  char buffer_recieved[100];
  uint32_t t0;
  uint32_t t1;
  uint32_t t2;
  int t1_log;
  float time_test;
  int period_log = pow(10, 6) * 7.5;
  int num_saved_logs = 0;
  int meas[NUM_SENSORS][NUM_MEAS];
  double measurement_val[NUM_SENSORS] = { 0 };
  int l = 0;
  int n = 0;
  int i = 0;
  int num_sampels = NUM_SAMPLES;
  int num_bytes_received = 0;
  float t[NUM_SENSORS] = {20.0, 15.0, 20.0};
  bool states[NUM_SENSORS] = {false};
  unsigned int period = pow(10, 6) * 2;
  t0 = micros();   // return the time since the program started, unit microsecond
  int sample_time = 0;
  while (true)
  {

    //sprintf(message, 100, "%d", NSCAN);
    //writeToSerial(message);
    Serial.println("______________loop___________________");
    for (unsigned int i = 0; i < NUM_SENSORS; ++i) {
      determineTouch(index[i], num_sampels, &touch_data[i], t[i]);
      n++;
      String ii = (String) index[i];
      Serial.println(ii);
      t1 = micros();
      states[i] = touch_data[i].object_detection;
      String s = states[i];
      Serial.println(s);
      // if it's not in debug mode, then when detecting the object, LED on
      if (touch_data[i].object_detection && !data.states[3]) {
        if (!data.states[0])
        {
          data.states[0] = true;
        }
      }
       // if not in debug mode, no object detected, LED off
      else if (!data.states[3])
      {
        if (data.states[0]) {
          data.states[0] = false;
        }
      }
    // control LED
    switch_led(&data.states[0]);

    if (data.states[6])  // activate the average filter
    {
      meas[i][l] = touch_data[i].real_touch;

      if (l == NUM_MEAS - 1)
      {
        for (l = 0; l < NUM_MEAS; l++)
        {
          measurement_val[i] += meas[i][l]; // sum value
        }
        measurement_val[i] = measurement_val[i] / NUM_MEAS;   // mean value

        l = 0;   // clear l, recount
        snprintf(message, 100, "Measurement average:%f", measurement_val);
        WriteToSerial(message);
      }
      else
      {
        l++;
      }

    }
    }
  }
}

// turn on/off the led
void switch_led(bool* led_on) {
  if (*led_on) {
    digitalWrite(13, HIGH);
  }
  else {
    digitalWrite(13, LOW);
  }
}

void WriteToSerial(char message[])
{
  int m = usb_serial_write_buffer_free();
  if (m > 25 && Serial)
  {
    char str[60];
    snprintf(str, 60, "%s", message);
    Serial.println(str);
  }
}
