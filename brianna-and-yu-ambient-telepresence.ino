
#include <Servo.h>
//================================================================
// Hardware definitions. You will need to customize this for your specific hardware.
const int SONAR_TRIGGER_PIN = 8;    // Specify a pin for a sonar trigger output.
const int SONAR_ECHO_PIN    = 7;    // Specify a pin for a sonar echo input.
const int SERVO_PIN = 9;
const int base = 100;
int last_value = -1;

Servo svo;
int servoMapValue = 0;

//================================================================
// Set the serial port transmission rate. The baud rate is the number of bits
// per second.
const long BAUD_RATE = 115200;    

//================================================================
// This function is called once after reset to initialize the program.
void setup()
{
  // Initialize the Serial port for host communication.
  Serial.begin(BAUD_RATE);

  // Initialize the digital input/output pins.  You will need to customize this
  // for your specific hardware.
  pinMode(SONAR_TRIGGER_PIN, OUTPUT);
  pinMode(SONAR_ECHO_PIN, INPUT);
  svo.attach(SERVO_PIN);
}

//================================================================
// This function is called repeatedly to handle all I/O and periodic processing.
// This loop should never be allowed to stall or block so that all tasks can be
// constantly serviced.
void loop()
{
  serial_input_poll();
  hardware_input_poll();
}

//================================================================
// Polling function to process messages received over the serial port from the
// remote Arduino.  Each message is a line of text containing a single integer
// as text.

void serial_input_poll(void)
{
  while (Serial.available()) {
    // When serial data is available, process and interpret the available text.
    // This may be customized for your particular hardware.

    // The default implementation assumes the line contains a single integer
    // which controls the built-in LED state.
    int value = Serial.parseInt();
    if (last_value == -1) {
      last_value = value;
    } else if (abs(value-last_value) > 25) {
      value = last_value;
    } else {
      last_value = value;
    }
    int MapValue = value + base;

    int increment = 2;
    int servoMoveValue = 0;
    int original_pos = svo.read();


    // Drive the servo to the value read in.
    if (MapValue > original_pos){
        while (((svo.read() + increment) <= MapValue) & (servoMoveValue < MapValue - original_pos)){
          servoMoveValue += increment;
          //delay(10);
          svo.write(servoMoveValue + original_pos);
        }
      }
      else{
        while ((svo.read() - increment) >= MapValue & (servoMoveValue < original_pos - MapValue)){
          servoMoveValue -= increment;
          //delay(10);
          svo.write(servoMoveValue + original_pos);
        }
      }

    // Once all expected values are processed, flush any remaining characters
    // until the line end.  Note that when using the Arduino IDE Serial Monitor,
    // you may need to set the line ending selector to Newline.
    Serial.find('\n');
  }
}

//================================================================
// Polling function to read the inputs and transmit data whenever needed.

void hardware_input_poll(void)
{
  // Calculate the interval in milliseconds since the last polling cycle.
  static unsigned long last_time = 0;
  unsigned long now = millis();
  unsigned long interval = now - last_time;
  last_time = now;

  // Poll each hardware device.  Each function returns true if the input has
  // been updated.  Each function directly updates the global output state
  // variables as per your specific hardware.  The input_changed flag will be
  // true if any of the polling functions return true (a logical OR using ||).
  bool input_changed = (poll_sonar(interval));

  // Update the message timer used to guarantee a minimum message rate.
  static long message_timer = 0;
  message_timer -= interval;

  // If either the input changed or the message timer expires, retransmit to the network.
  if (input_changed || (message_timer < 0)) {
    message_timer = 1000;  // one second timeout to guarantee a minimum message rate
    transmit_packet();
  }
}

//================================================================
// Poll the sonar at regular intervals.
bool poll_sonar(unsigned long interval)
{
  static long sonar_timer = 0;
  sonar_timer -= interval;
  if (sonar_timer < 0) {
    sonar_timer = 250; // 4 Hz sampling rate

    // Generate a short trigger pulse.
    digitalWrite(SONAR_TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(SONAR_TRIGGER_PIN, LOW);

    // Measure the echo pulse length.  The ~6 ms timeout is chosen for a maximum
    // range of 100 cm assuming sound travels at 340 meters/sec.  With a round
    // trip of 2 meters distance, the maximum ping time is 2/340 = 0.0059
    // seconds.  You may wish to customize this for your particular hardware.
    const unsigned long TIMEOUT = 5900;
    unsigned long ping_time = pulseIn(SONAR_ECHO_PIN, HIGH, TIMEOUT);

    // The default implementation only updates the data if a ping was observed,
    // the no-ping condition is ignored.
    if (ping_time > 0) {
      // Update the data output and indicate a change.
      servoMapValue = map(ping_time, 0, TIMEOUT, 2, 178);
      if (servoMapValue > 100) {
        servoMapValue = 100;
      }
      int MapValue = servoMapValue + base;

      int increment = 2;
      int servoMoveValue = 0;
      int original_pos = svo.read();
      /*while (svo.read() < servoMapValue + base) {
        svo.write(svo.read() + increment + base);
        delay(200);
      }*/
      if (MapValue > original_pos){
        while (((svo.read() + increment) <= MapValue) & (servoMoveValue < MapValue - original_pos)){
          servoMoveValue += increment;
          //Serial.print("move up ");
          delay(100);
          svo.write(servoMoveValue + original_pos);
          //Serial.println(servoMoveValue);
          //Serial.println(svo.read());
        }
      }
      else{
        while ((svo.read() - increment) >= MapValue & (servoMoveValue < original_pos - MapValue)){
          servoMoveValue -= increment;
          //Serial.print("move down ");
          delay(100);
          svo.write(servoMoveValue + original_pos);
          //Serial.println(servoMoveValue);
          //Serial.println(svo.read());
        }
      }
      return true;
    }
  }
  return false; // No change in state.
}

//================================================================
// Send the current data to the MQTT server over the serial port.  The values
// are clamped to the legal range using constrain().

void transmit_packet(void)
{
  if (servoMapValue > 100) {
    Serial.println(100);
  } else {
    Serial.println(servoMapValue);
  }
}
//================================================================
