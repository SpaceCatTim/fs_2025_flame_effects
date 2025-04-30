// Define the pins for the buttons
const int buttonPin1 = 2;
const int buttonPin2 = 3;
const int ledPin = 17;
const int blinkIntervalMs = 500;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  Serial1.begin(115200);
  
  // Set the button pins as inputs
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);

  // led
  pinMode(ledPin, OUTPUT);

  Serial1.print("AT+ADDRESS=5\r\n");        // address 5 = pedals
  delay(100);
  Serial1.print("AT+NETWORKID=18\r\n");
  delay(100);
  Serial1.print("AT+MODE=2,500,60000\r\n"); // smart rx mode [2] [rxtime_ms] [sleeptime_ms]
  delay(1000);
  
  digitalWrite(ledPin, HIGH);
  Serial1.print("AT+SEND=2,5,test1\r\n");
  delay(10);
}

void delaySeconds(uint32_t seconds) {
  while(seconds) {
    seconds--;
    delay(1000);
  }
}

void loop() {
  static int pedalState1 = 0;
  static unsigned long lastPress1 = 0;
  static unsigned long lastRelease1 = 0;
  
  static int pedalState2 = 0;
  static unsigned long lastPress2 = 0;
  static unsigned long lastRelease2 = 0;

  static unsigned int tog = 0;
  static unsigned long lastBlinkMillis = 0;

  // Check if button 1 is pressed
  if (digitalRead(buttonPin1) == HIGH) {
    if(pedalState1 == 0)
    {
      // rising edge
      if(millis() > (lastRelease1 + 100))
      {
        // past debounce time
        pedalState1 = 1;
        lastPress1 = millis();
        Serial1.print("AT+SEND=2,4,p1dn\r\n");
        Serial.println("pedal 1 down");
      }
    }
  }

  // Check if button 1 is released
  if (digitalRead(buttonPin1) == LOW) {
    if(pedalState1 == 1)
    {
      // falling edge
      if(millis() > (lastPress1 + 100))
      {
        // past debounce time
        pedalState1 = 0;
        lastRelease1 = millis();
        Serial1.print("AT+SEND=2,4,p1up\r\n");
        Serial.println("pedal 1 up");
      }
    }
  }

  // Check if button 2 is pressed
  if (digitalRead(buttonPin2) == HIGH) {
    if(pedalState2 == 0)
    {
      // rising edge
      if(millis() > (lastRelease2 + 100))
      {
        // past debounce time
        pedalState2 = 1;
        lastPress2 = millis();
        Serial1.print("AT+SEND=2,4,p2dn\r\n");
        Serial.println("pedal 2 down");
      }
    }
  }

  // Check if button 2 is released
  if (digitalRead(buttonPin2) == LOW) {
    if(pedalState2 == 1)
    {
      // falling edge
      if(millis() > (lastPress2 + 100))
      {
        // past debounce time
        pedalState2 = 0;
        lastRelease2 = millis();
        Serial1.print("AT+SEND=2,4,p2up\r\n");
        Serial.println("pedal 2 up");
      }
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
