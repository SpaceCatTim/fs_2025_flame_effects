
#define WINDOW_SIZE 64  // moving average window length

const int audioPin = A0;     // Analog input pin for audio
const int sampleWindow = 10; // Sample window width in milliseconds
const int ledPin = 17;
const int blinkIntervalMs = 500;
const float booshThreshV = 1.5;  // sets amplitude thereshold

float average(float input);

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  Serial1.begin(115200);

  // led
  pinMode(ledPin, OUTPUT);

  Serial1.print("AT+ADDRESS=6\r\n");        // address 6 = the void
  delay(100);
  Serial1.print("AT+NETWORKID=18\r\n");
  delay(100);
  Serial1.print("AT+MODE=2,500,60000\r\n"); // smart rx mode [2] [rxtime_ms] [sleeptime_ms]
  delay(1000);
  digitalWrite(ledPin, HIGH);
  Serial1.print("AT+SEND=2,5,test1\r\n");
  delay(10);
}

void loop() {
  unsigned long startMillis = millis();  // Start of sample window
  unsigned int peakToPeak = 0;           // Peak-to-peak level
  unsigned int signalMax = 0;
  unsigned int signalMin = 1023;
  unsigned int sample;
  static unsigned int tog = 0;
  static unsigned long lastPrintMillis = 0;
  static unsigned long lastBooshMillis = 0;
  static unsigned long lastBlinkMillis = 0;

  // collect data for 10 ms
  while (millis() - startMillis < sampleWindow) {
    sample = analogRead(audioPin);
    if (sample < 1023) {
      if (sample > signalMax) {
        signalMax = sample;
      }
      if (sample < signalMin) {
        signalMin = sample;
      }
    }
  }

  peakToPeak = signalMax - signalMin;
  float voltage = (peakToPeak * 5.0) / 1023.0; // Convert to voltage (assuming 5V ref)

  //Serial.print("Amplitude (Vpp): ");
  //Serial.println(voltage, 3); // Print with 3 decimal places

  float avg = average(voltage);

  if(millis() >= (lastPrintMillis + 250))
  {
    lastPrintMillis = millis();
    Serial.print("Average Amplitude (Vpp): ");
    Serial.println(avg, 3); // Print with 3 decimal places
  }

  if(avg > booshThreshV)
  {
    if(millis() >= (lastBooshMillis + 10000))
    {
      Serial.println("Boosh!");
      lastBooshMillis = millis();
      Serial1.print("AT+SEND=2,5,boosh\r\n");
    }
  }

  if(millis() >= (lastBlinkMillis + blinkIntervalMs))
  {
    lastBlinkMillis = millis();
    if(tog == 0)
    {
      tog = 1;
      digitalWrite(ledPin, LOW);
    } else {
      tog = 0;
      digitalWrite(ledPin, HIGH);
    }
  }
}

float average(float input) {
    static float buffer[WINDOW_SIZE] = {0}; // Stores the last WINDOW_SIZE values
    static int index = 0;                   // Current index in the buffer
    static int count = 0;                   // Number of samples added (up to WINDOW_SIZE)
    static float sum = 0;                   // Running sum of buffer values

    // Subtract the oldest value from sum
    sum -= buffer[index];
    
    // Store the new value in the buffer
    buffer[index] = input;
    
    // Add the new value to the sum
    sum += input;

    // Move index forward and wrap around
    index = (index + 1) % WINDOW_SIZE;

    // Count how many values we've stored (up to WINDOW_SIZE)
    if (count < WINDOW_SIZE) {
        count++;
    }

    // Return the average
    return sum / count;
}
