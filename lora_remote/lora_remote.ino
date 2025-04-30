// Define the pins for the buttons
const int buttonPin1 = 2;
const int buttonPin2 = 3;
const int ledPin = 17;

const long ledInterval = 500;           // interval at which to blink led in milliseconds

void setup() {
  // Initialize serial communication
  Serial1.begin(115200);
  
  // Set the button pins as inputs
  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);

  // led
  pinMode(ledPin, OUTPUT);

  Serial1.print("AT+ADDRESS=1\r\n");
  delay(100);
  Serial1.print("AT+NETWORKID=18\r\n");
  delay(100);
  Serial1.print("AT+MODE=2,500,60000\r\n"); // smart rx mode [2] [rxtime_ms] [sleeptime_ms]
  delay(1000);
  
  digitalWrite(ledPin, HIGH);
  Serial1.print("AT+SEND=2,5,test1\r\n");
}

void loop()
{
  static bool enabled = false;
  static unsigned long previousLedMillis = 0;

  if(enabled == false)
  {
    digitalWrite(ledPin, HIGH);
  }

  // Check if button 1 is pressed
  if(digitalRead(buttonPin1) == LOW)
  {
    // Button 1 is pressed, send the corresponding message
    Serial1.print("AT+SEND=2,5,boosh\r\n");

    while(digitalRead(buttonPin1) == LOW){};
    delay(300); // Debounce delay
  }

  // Check if button 2 is pressed
  if(digitalRead(buttonPin2) == LOW)
  {

    // Button 2 is pressed, send the corresponding message
    if(enabled == false)
    {
      Serial1.print("AT+SEND=2,6,enable\r\n");
      digitalWrite(ledPin, LOW);
      enabled = true;
    }
    else
    {
      Serial1.print("AT+SEND=2,7,disable\r\n");
      digitalWrite(ledPin, HIGH);
      enabled = false;
    }

    while(digitalRead(buttonPin2) == LOW){};
    delay(300); // Debounce delay
  }
}
