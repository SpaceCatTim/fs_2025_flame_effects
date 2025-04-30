
#define WINDOW_SIZE         16 // sample window peak-peak detection size
#define PULSE_AVG_SIZE      4   // moving average2 window length
#define PULSE_PERCENT       30  // perecnt tolerance for a new pulse sample to be valid
#define PEAK_MIN_THRSH      60  // previously 35
#define PEAK_MAX_THRSH      200

// Biquad lowpass coefficients - fs = 250Hz, 10 Hz cutoff, q = 0.707 (butterworth)
// #define COEF_LP_B0 	0x01B6L
// #define COEF_LP_B1 	0x01B6L * 2L
// #define COEF_LP_B2 	0x01B6L
// #define COEF_LP_A1 	0x6970L * 2L
// #define COEF_LP_A2 	0xFFFFA649L

// Biquad lowpass coefficients - fs = 250Hz, 15 Hz cutoff, q = 0.707 (butterworth)
#define COEF_LP_B0 	0x0391L
#define COEF_LP_B1 	0x0391L * 2L
#define COEF_LP_B2 	0x0391L
#define COEF_LP_A1 	0x5E6EL * 2L
#define COEF_LP_A2 	0xFFFFB4E0L


// Biquad lowpass coefficients - fs = 250Hz, 20 Hz cutoff, q = 0.707 (butterworth)
// #define COEF_LP_B0 	0x05E8L
// #define COEF_LP_B1 	0x05E8L * 2L
// #define COEF_LP_B2 	0x05E8L
// #define COEF_LP_A1 	0x53ABL * 2L
// #define COEF_LP_A2 	0xFFFFC10CL

// Biquad lowpass coefficients - fs = 250Hz, 30 Hz cutoff, q = 0.707 (butterworth)
// #define COEF_LP_B0 	0x0BB0L
// #define COEF_LP_B1 	0x0BB0L * 2L
// #define COEF_LP_B2 	0x0BB0L
// #define COEF_LP_A1 	0x3EE0L * 2L
// #define COEF_LP_A2 	0xFFFFD380L

// Biquad highpass coefficients - fs = 250Hz, 1 Hz cutoff, q = 0.707 (butterworth)
#define COEF_HP_B0 	0x7DBFL
#define COEF_HP_B1 	0xFFFF8241L * 2L
#define COEF_HP_B2 	0x7DBFL
#define COEF_HP_A1 	0x7DBAL * 2L
#define COEF_HP_A2  0xFFFF8478L


// Biquad highpass coefficients - fs = 250Hz, 0.4 Hz cutoff, q = 0.707 (butterworth)
// #define COEF_HP_B0 	0x7F18L
// #define COEF_HP_B1 	0xFFFF80E8L * 2L
// #define COEF_HP_B2 	0x7F18L
// #define COEF_HP_A1 	0x7F17L * 2L
// #define COEF_HP_A2  0xFFFF81CFL

// Biquad highpass coefficients - fs = 250Hz, 0.3 Hz cutoff, q = 0.707 (butterworth)
// #define COEF_HP_B0 	0x7F35L
// #define COEF_HP_B1 	0xFFFF80CBL * 2L
// #define COEF_HP_B2 	0x7F35L
// #define COEF_HP_A1 	0x7F34L * 2L
// #define COEF_HP_A2  0xFFFF8195L

const int analogPin = A0;     // Analog input pin
const int sampleWindow = 100; // Sample window width in milliseconds
const int ledPin = 17;

int window_filter[WINDOW_SIZE] = {0};
unsigned int window_filter_index = 0;

void setup()
{
  // Initialize serial communication
  Serial.begin(9600);
  Serial1.begin(115200);

  // led
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  
  // setup LoRa
  Serial1.print("AT+ADDRESS=4\r\n");        // address 4 = heartrate
  delay(100);
  Serial1.print("AT+NETWORKID=18\r\n");
  delay(100);
  Serial1.print("AT+MODE=2,500,60000\r\n"); // smart rx mode [2] [rxtime_ms] [sleeptime_ms]
  delay(1000);
  Serial1.print("AT+SEND=2,5,test1\r\n");
  delay(10);
}

void loop()
{
  int peakToPeak;           // Peak-to-peak level
  int signalMax;
  int signalMin;
  unsigned int windowCount;
  int sample = 0;

  static unsigned int  prevPeakToPeak = 0;
  static bool          prevPtpInThrsh = false;
  static unsigned int  PeakToPeakMax = 0;
  static unsigned long lastHeartMillis = 0;
  static unsigned long lastSampleCntMillis = 0;
  static unsigned long sampleCount = 0;
  static unsigned long avgPulseTime = 0;

  digitalWrite(ledPin, HIGH);
  
  // improvement: butterworth filters instead of 16 sample averaging
  sample = analogRead(analogPin);
  sample <<= 3; // scale 8x
  Serial.print(sample);
  Serial.print(", ");
  sample = biquad_lp(sample);
  sample = biquad_hp(sample);
  Serial.print(sample);
  Serial.print(", ");
  sampleCount++;

  // improvement: sliding window (circular buffer)
  windowCount = add_to_window_filt(sample);
  signalMax = get_max(window_filter, window_filter_index, windowCount);
  signalMin = get_min(window_filter, window_filter_index, windowCount);
  peakToPeak = signalMax - signalMin;
  Serial.print(peakToPeak);
  Serial.print(", ");

  Serial.println(peakToPeak);

  // if(millis() >= (lastSampleCntMillis + 1000))
  // {
  //   // for data collection
  //   lastSampleCntMillis = millis();
  //   Serial.print("Samples in hz: ");
  //   Serial.println(sampleCount);
  //   sampleCount = 0;
  // }

  if(prevPtpInThrsh && (peakToPeak < prevPeakToPeak) && (PeakToPeakMax < PEAK_MAX_THRSH))
  {
    // signal has gone within range, did not exceed max since going above min threshold, and has fallen back below min thresh

    if(millis() >= (lastHeartMillis + 350))
    {
      // candidate pulse is at least (avgPulseTime - 100)ms past the last one
      unsigned long pulseTime = millis() - lastHeartMillis;
      lastHeartMillis = millis();
      avgPulseTime = pulse_average(pulseTime);
      if((pulseTime < (avgPulseTime * 3)) && \
         (pulseTime > (avgPulseTime - ((avgPulseTime * PULSE_PERCENT) / 100))))
      {
        Serial1.print("AT+SEND=2,5,heart\r\n");
        digitalWrite(ledPin, LOW);
        delay(180);
        digitalWrite(ledPin, HIGH);
      }
      //Serial.print("Peak to peak: ");
      //Serial.println(peakToPeak);
      //Serial.print("Average pulse ms: ");
      //Serial.println(avgPulseTime);
    }
  }

  if(peakToPeak < PEAK_MIN_THRSH)
  {
    PeakToPeakMax = 0;
    prevPtpInThrsh = false;
  } else {
    if(peakToPeak > PeakToPeakMax)
    {
      PeakToPeakMax = peakToPeak;
    }
    if(peakToPeak < PEAK_MAX_THRSH)
    {
      prevPtpInThrsh = true;
    } else {
      prevPtpInThrsh = false;
    }
  }
  prevPeakToPeak = peakToPeak;
  

  unsigned long loopmillis = millis();
  while(millis() < loopmillis + 4){};
}

#define INT_MIN -32768
#define INT_MAX 32767
int get_max(int buffer[], unsigned int start_index, unsigned int count)
{
  int max = INT_MIN;
  while(count)
  {
    start_index = (start_index - 1) % WINDOW_SIZE;
    //if(start_index) start_index--;
    //else start_index = WINDOW_SIZE - 1;
    if(buffer[start_index] > max)
    {
      max = buffer[start_index];
    }
    count--;
  }
  return max;
}

int get_min(int buffer[], unsigned int start_index, unsigned int count)
{
  int min = INT_MAX;
  while(count)
  {
    start_index = (start_index - 1) % WINDOW_SIZE;
    //if(start_index) start_index--;
    //else start_index = WINDOW_SIZE - 1;
    if(buffer[start_index] < min)
    {
      min = buffer[start_index];
    }
    count--;
  }
  return min;
}

unsigned int add_to_window_filt(int input)
{
  static int count = 0; 

  // Store the new value in the buffer
  window_filter[window_filter_index] = input;

  // Move index forward and wrap around
  window_filter_index = (window_filter_index + 1) % WINDOW_SIZE;

  // // Count how many values we've stored (up to WINDOW_SIZE)
  if (count < WINDOW_SIZE) {
    count++;
  }

  return count;
}

unsigned int pulse_average(unsigned int input)
{
  static unsigned int buffer[PULSE_AVG_SIZE] = {0}; // Stores the last PULSE_AVG_SIZE values
  static int index = 0;                             // Current index in the buffer
  static int count = 0;                             // Number of samples added (up to PULSE_AVG_SIZE)
  static unsigned long sum = 0;                     // Running sum of buffer values

  // Subtract the oldest value from sum
  sum -= buffer[index];
  
  // Store the new value in the buffer
  buffer[index] = input;
  
  // Add the new value to the sum
  sum += input;

  // Move index forward and wrap around
  index = (index + 1) % PULSE_AVG_SIZE;

  // Count how many values we've stored (up to PULSE_AVG_SIZE)
  if (count < PULSE_AVG_SIZE) {
      count++;
  }

  // Return the average
  return sum / count;
}

int biquad_lp(int input)
{
  // implements a 2nd order lowpass filter

  static int adc_biquad_buffer[7] = {0};
  long sum;

  adc_biquad_buffer[0] = input;

  sum =  adc_biquad_buffer[0] * COEF_LP_B0;
  sum += adc_biquad_buffer[1] * COEF_LP_B1;
  sum += adc_biquad_buffer[2] * COEF_LP_B2;
  sum += adc_biquad_buffer[3] * COEF_LP_A1;
  sum += adc_biquad_buffer[4] * COEF_LP_A2;
  sum += (unsigned int)adc_biquad_buffer[5] << 1;
  sum -= adc_biquad_buffer[6];

  // update delay elements
  adc_biquad_buffer[6] = adc_biquad_buffer[5] & 0x7FFF;
  adc_biquad_buffer[5] = sum;

  adc_biquad_buffer[4] = adc_biquad_buffer[3];
  adc_biquad_buffer[3] = (sum >> 15);		// this is the filter output

  adc_biquad_buffer[2] = adc_biquad_buffer[1];
  adc_biquad_buffer[1] = adc_biquad_buffer[0];
  
  return adc_biquad_buffer[3];
}

int biquad_hp(int input)
{
  // implements a 2nd order highpass filter

  static int adc_biquad_buffer[7] = {0};
  long sum;

  adc_biquad_buffer[0] = input;

  sum =  adc_biquad_buffer[0] * COEF_HP_B0;
  sum += adc_biquad_buffer[1] * COEF_HP_B1;
  sum += adc_biquad_buffer[2] * COEF_HP_B2;
  sum += adc_biquad_buffer[3] * COEF_HP_A1;
  sum += adc_biquad_buffer[4] * COEF_HP_A2;
  sum += (unsigned int)adc_biquad_buffer[5] << 1;
  sum -= adc_biquad_buffer[6];				// this one is a subtract

  // update delay elements
  adc_biquad_buffer[6] = adc_biquad_buffer[5] & 0x7FFF;
  adc_biquad_buffer[5] = sum;

  adc_biquad_buffer[4] = adc_biquad_buffer[3];
  adc_biquad_buffer[3] = (sum >> 15);		// this is the filter output

  adc_biquad_buffer[2] = adc_biquad_buffer[1];
  adc_biquad_buffer[1] = adc_biquad_buffer[0];
  
  return adc_biquad_buffer[3];
}
