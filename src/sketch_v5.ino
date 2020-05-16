/*Revised from jcan.ino, all credits go to Dr. Andersen
Add three functions for filtering*/
#define REV_ID "$Id: main.cpp 402 2016-09-11 11:25:36Z jcan $"

#define NUM_LEDS 1  //Numbers of led
#define NUM_SENSORS 3 //Numbers of sensors, for this project, it's 3
#define MAX_STRING_SIZE 40
#define NUM_COMMANDS 10
#define NUM_LOGS 12000*2
#define COUNT_LOGS 0
#define NUM_MEAS 50 //the filter size of the mean value filter
#define TRANSMIT_COUNT 50 //every 50 samples, send it
#define NUM_DATA 450 //the range of data set is 450
#define NUM_SAMPLES 1 //this is the sampling rate
#define STEP 50 // step

// State of flags to do different things
struct Settings
{
  //Note stream and signal: [1] and [7]
  //meas is to activate the mean value filter
  bool states[NUM_COMMANDS] = { false, true, false, false, false, false, true, true, false, false};
  char commands[NUM_COMMANDS][MAX_STRING_SIZE] = { "ledon", "stream", "sample", "debug", "log", "sendlog", "meas", "signal", "mids", "mid-mea"};

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

//unsigned int touchpin[2] = { 15, 17, 19 };


/* Function gets touch value from the microprocessor
 * It multiplies it by a time constant and makes the value flow up or down
 * It also has flags for streaming and outputting values
 * Outputs an offset in the touch value from a reference value depending on the filter applied.
 */
double getTouch(unsigned int pin, int num_sampels, bool stream, Touch* touch_data, bool stream_signal, float fac)
{ 
    //char id_string[] = "Pin";
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
      if(stream_signal || stream){
        touch_data->counter = touch_data->counter + 1;
      }
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
      if(stream && touch_data->counter == TRANSMIT_COUNT){
        //WriteToSerial(pin,touch_data->real_touch,id_string);
      }

      if (noTouch[pin] == 0)
      {
        noTouch[pin] = x;
      }
      else if (x < noTouch[pin] and FAC != 1)
      {
        noTouch[pin] -= 1;
        result = filter(noTouch[pin], x, touch_data, FAC, fac);
      }
      else if (x > noTouch[pin] and FAC != 1)
      {
        noTouch[pin] += 1;
        result = filter(noTouch[pin], x, touch_data, FAC, fac);
      }
      else {
        result = filter(noTouch[pin], x, touch_data, FAC, fac);
      }
      if (stream && touch_data->counter == TRANSMIT_COUNT)
    {
      //char id_string2[] = "Offset";
      //WriteToSerial(pin, result/FAC, id_string2);  
      
      //char id_string3[] = "thresh";
      //WriteToSerial(pin, touch_data->touch_detection, id_string3);

      if (touch_data->glr == false) {
        touch_data->counter = 0;
      }
    }
     if (stream_signal && touch_data->counter == TRANSMIT_COUNT)
    {
    //WriteToSerial(touch_data->counter);
     //WriteToSerial(touch_data->touch_detection);
      touch_data->counter = 0;
    }
    }
    return result / FAC;  // scale it into the original size
  }

  
float e[7] = { 0, 0, 0 };   // e is error
float u[7] = { 0, 0, 0 };   // u is e after transfer function


// Control filter if u[0] = e[0] then the filter is inactive and only a threshold is set.
float filter(float y, float x, Touch* touch_data, int FAC, float fac)
{
  e[0] = x - y;
  // One pole transfer and one zero transfer function time constant = 5
  //u[0] = 0.99999988*u[1] + 0.49999997*e[0] - 0.49999997*e[1];

  // No filter
  u[0] = e[0];

  //u[0] = 0.99999878*u[1] + 6.10500238*e[0] - 6.10500238*e[1];

  if (abs(u[0]) > fac * FAC)
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
void determineTouch(unsigned int pin, int num_sampels, bool data_stream, Touch* touch_data, bool stream_signal, float fac)
{
  touch_data->touch_value = getTouch(pin, num_sampels, data_stream, touch_data, stream_signal, fac);
  
}


void bubble_sort(int* arr, int len) {
    int i, j, temp;
    for (i = 0; i < len - 1; i++)
        for (j = 0; j < len - 1 - i; j++)
            if (*(arr+j) > *(arr+j+1)) {
                temp = *(arr+j);
                *(arr+j) = *(arr+j+1);
                *(arr+j+1) = temp;
            }
}


////////////////////////////////////////////////////
//////////////////////main loop/////////////////////

void loop()
{
  // Command set
  Settings data;
  Touch touch_data[NUM_SENSORS];   // store the signals
  //bool logged_saved = false;   //even save the log
  //int *data_log = (int*)malloc(NUM_LOGS * sizeof(int));
  unsigned int index[NUM_SENSORS] = {15, 17, 19}; // pins 
  float factors[3] = {12.0, 12.0, 12.0}; //the threshold for each sensor
  //char message[100];
  //char buffer_recieved[100];
  //uint32_t t0;
  //uint32_t t1;
  //uint32_t t2;
  //int t1_log;
  //float time_test;
  //int period_log = pow(10, 6) * 7.5;
  //int num_saved_logs = 0;
  int fs[NUM_SENSORS][NUM_MEAS];
  double measurement_val[NUM_SENSORS] = { 0 };
  int max_temp[NUM_MEAS] = {0};
  int min_temp[NUM_MEAS] = {0};
  int l[NUM_MEAS] = {0};
  int n = 0;
  int num_sampels = NUM_SAMPLES;
  //int num_bytes_received = 0;
  //unsigned int period = pow(10, 6) * 2;
  //t0 = micros();   // return the time since the program started, unit microsecond
  //int sample_time = 0;
  char id_string[] = "Pin";
  char id_string1[] = "Offset";
  char id_string3[] = "thresh";
  int counter[NUM_SENSORS] = {0};

  while (true)
  {

    //sprintf(message, 100, "%d", NSCAN);
    //writeToSerial(message);
    //Serial.println("________If there is an obstacle______________");
    for (unsigned int i = 0; i < NUM_SENSORS; i++){
      determineTouch(index[i], num_sampels, data.states[1], &touch_data[i], data.states[7], factors[i]);
      n++;
      //char str[60];
      //snprintf(str, 60, "%s%d:%f", 'State', index[i], touch_data[i].object_detection);
      //Serial.println(str);
    
      //t1 = micros();
      
      // if it's not in debug mode, then when detecting the object, LED on
      if (touch_data[i].object_detection && !data.states[3]) {
        if (!data.states[0])
        {
          data.states[0] = true;
        }      }
       // if not in debug mode, no object detected, LED off
      else if (!data.states[3])
      {
        if (data.states[0]) {
          data.states[0] = false;
        }
      }
    
    // control LED
    switch_led(&data.states[0]);
    // send data to serial port
    if (counter[i]==TRANSMIT_COUNT)
    {
        WriteToSerial(index[i],touch_data[i].real_touch,id_string);
        //WriteToSerial(index[i],touch_data[i].touch_value,id_string1);
        //WriteToSerial(index[i], touch_data[i].touch_detection, id_string3);
        counter[i] = 0;
          //Average Filter
  if (data.states[6])
  {
    fs[i][l[i]%NUM_MEAS] = touch_data[i].real_touch;
    if(l[i]==NUM_MEAS -1)
    {
      /*double mean, sigma, c = 0;
      for (l[i] = 0; l[i] < NUM_MEAS; l[i]++)
           {
             mean += fs[i][l[i]]; // mean value
           }
           mean = mean/NUM_MEAS;
      for (l[i] = 0; l[i] < NUM_MEAS; l[i]++)
           {
             sigma += pow((fs[i][l[i]]-mean),2); // standard deviation
           }
           sigma = sqrt(sigma/NUM_MEAS);
      for (l[i] = 0; l[i] < NUM_MEAS; l[i]++)
           {
             if (fs[i][l[i]]<(mean-2*sigma) || fs[i][l[i]]>(mean+2*sigma) )
             {
              fs[i][l[i]]=0;
              c++;
              }
             }
             mean = 0;
             sigma = 0;
             c = 0;
             */
      for (l[i] = 0; l[i] < NUM_MEAS; l[i]++)
           {
             measurement_val[i] += fs[i][l[i]]; // sum value
           }
            measurement_val[i] = measurement_val[i] / NUM_MEAS;   // mean value
      char id_string2[] = "Average";
      WriteToSerial(index[i],measurement_val[i],id_string2);
      measurement_val[i] = 0;
    }
    else if(l[i]%NUM_MEAS==STEP-1)
    {
        /*double mean, sigma, c = 0;
      for (int k = 0; k < NUM_MEAS; k++)
           {
             mean += fs[i][k]; // mean value
           }
           mean = mean/NUM_MEAS;
      for (int k = 0; k < NUM_MEAS; k++)
           {
             sigma += pow((fs[i][k]-mean),2); // standard deviation
           }
           sigma = sqrt(sigma/NUM_MEAS);
      for (int k = 0; k < NUM_MEAS; k++)
           {
             if (fs[i][k]<(mean-2*sigma) || fs[i][k]>(mean+2*sigma) )
             {
              fs[i][k]=0;
              c++;
              }
             }
             mean = 0;
             sigma = 0;
             c = 0;
             */
      for (int k = 0; k < NUM_MEAS; k++)
           {
             measurement_val[i] += fs[i][k]; // sum value
           }
            measurement_val[i] = measurement_val[i] / NUM_MEAS;   // mean value
      char id_string2[] = "Average";
      WriteToSerial(index[i],measurement_val[i],id_string2);
      measurement_val[i] = 0;
      }
    l[i]++;
  }
  
  // Midian Filter
  
  if (data.states[8])
  {
    fs[i][l[i]%NUM_MEAS] = touch_data[i].real_touch;
    if(l[i]==NUM_MEAS -1)
    {
      bubble_sort(fs[i], NUM_MEAS);
            measurement_val[i] = fs[i][NUM_MEAS/2];
      char id_string2[] = "Midian";
      WriteToSerial(index[i],measurement_val[i],id_string2);
      measurement_val[i] = 0;
    }
    else if(l[i]%NUM_MEAS==STEP-1)
    {
      bubble_sort(fs[i], NUM_MEAS);
            measurement_val[i] = fs[i][NUM_MEAS/2];
      char id_string2[] = "Midian";
      WriteToSerial(index[i],measurement_val[i],id_string2);
      measurement_val[i] = 0;
    }
    l[i]++;
  }
  
  
  // Mid-Average Filter
  if (data.states[9])
  {
    fs[i][l[i]%NUM_MEAS] = touch_data[i].real_touch;
    if(l[i]==0)
    {
      max_temp[i] = touch_data[i].real_touch;
      min_temp[i] = max_temp[i];
    }
    if(touch_data[i].real_touch>max_temp[i])
    {
      max_temp[i] = touch_data[i].real_touch;
    }
    if(touch_data[i].real_touch<min_temp[i])
    {
      min_temp[i] = touch_data[i].real_touch;
    }
    if(l[i]==NUM_MEAS -1)
    {
      for (l[i] = 0; l[i] < NUM_MEAS; l[i]++)
           {
             measurement_val[i] += fs[i][l[i]]; // sum value
           }
            measurement_val[i] = (measurement_val[i]-max_temp[i]-min_temp[i]) / (NUM_MEAS-2);   // mean value
      char id_string2[] = "MidAve";
      WriteToSerial(index[i],measurement_val[i],id_string2);
      measurement_val[i] = 0;
    }
    else if(l[i]%NUM_MEAS==STEP-1)
    {
      for (l[i] = 0; l[i] < NUM_MEAS; l[i]++)
           {
             measurement_val[i] += fs[i][l[i]]; // sum value
           }
            measurement_val[i] = (measurement_val[i]-max_temp[i]-min_temp[i]) / (NUM_MEAS-2);   // mean value
      char id_string2[] = "MidAve";
      WriteToSerial(index[i],measurement_val[i],id_string2);
      measurement_val[i] = 0;
      }
    l[i]++;
  }
      }
      else
      {
        counter[i]++;
        }

  /*
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
    char id_string2[] = "Average";
      WriteToSerial(index[i],measurement_val[i],id_string2);
        //snprintf(message, 100, "Average%d:%f", index[i], measurement_val[i]);
        //WriteToSerial(message);
      }
      else
      {
        l++;
      }

    }
  */

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

// Different ways of writing through usb connection
void WriteToSerial(unsigned int pin, double touchValue, char id_string[])
{
  //Serial.println(pin);
  //delay(5); // Add a delay here, output should be evenly from 3 pins, without the delay, the output may be mainly from the first pin in index
  int m = usb_serial_write_buffer_free();
   if (m > 25 && Serial) {
    char str[60];
    snprintf(str, 60, "%s%d:%f", id_string, pin, touchValue);
    Serial.println(str);
  }
}

void WriteToSerial(float value)
{
  int m = usb_serial_write_buffer_free();
  if (m > 25 && Serial) {
    Serial.println(value);
  }
}

void WriteToSerial(float value, bool arr)
{
  int m = usb_serial_write_buffer_free();
  if (m > 25 && Serial) {
    char str[30];
    snprintf(str, 30, "%f,", value);
    Serial.print(str);
  }
}

void WriteToSerial(int value)
{
  int m = usb_serial_write_buffer_free();
  if (m > 25 && Serial) {
    Serial.println(value);
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
