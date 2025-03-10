/* ----+====***** FormulaRC BOX *****====+----

  Get values from an RC reciever and relay them to the USB Joystick object
  on the Arduino Micro.

  Steering Sensitivity:
  The exponent value can be adjusted to control the sensitivity
  of the steering input.
  * The higher the exponent, the more sensitive the steering will be. Twitchy.
  * The lower the exponent, the less sensitive the steering will be. Sluggish.
  The exponent value should be greater than 1 to have an effect.
  The default value 1.64 works very well for F1 2024.

  Accelerator and brake sensitivity: (Aux3, Channel 6, Joystick Z Axis)
  The curve is a logit based curve. The AUX3 channel can be used to adjust the sensitivity
  of the throttle and brake.
  * The higher the AUX3 value, the more sensitive the throttle and brake will be. Dry conditions.
  * The lower the AUX3 value, the less sensitive the throttle and brake will be. Wet conditions.

  @author: Billy.Brackeen@live.com

  Update: 2024-07 - Added support for Need for Seed (2016) and F1 2024. WIth AUX3 channel at 0,
  the throttle and brake are linear and output through the joystick accelerator, and the buttons are keys sent for NFS.
  With AUX3 channel > 0, the throttle and brake are logarithmic (Adjusted by AUX3 and blended with the curve) and output
  through the joystick throttle and brake and the buttons 1,2,3 are sent for F1 2024. Experimented with XInput, but it was
  not as responsive as the joystick due to most games treating it as an Xbox controller and not a wheel.
*/

// Include libraries
#include <PinChangeInterrupt.h>
#include <ServoInput.h>
#include <Joystick.h>
#include <Keyboard.h>

#define ON LOW
#define OFF HIGH

// Main configuration settings.
bool inputLoggingEnabled = false;  // Enable debug logging
bool serialLoggingEnabled = false; // Enable serial logging
bool printCurve = false;           // Print the adjustedLogitCurve for the throttle and brake on button press
bool dipSwitchEnabled = false;     // Enable dip switch settings
bool steeringExponential = true;   // Enable steering exponential, dip switch 1
bool acceleratorCurve = true;      // Enable accelerator curve, dip switch 2
bool brakeCurve = true;            // Enable brake curve, dip switch 3

// Function prototypes
void write(String message, bool newLine = false);
void write(int message, bool newLine = false);
void write(float message, bool newLine = false);
void write(unsigned long message, bool newLine = false);

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_MULTI_AXIS,
                   4, 0,                // Button Count, Hat Switch Count
                   false, false, false, // X, Y, Z
                   false, false, false, // Rx, Ry, Rz
                   false, true,         // Rudder, Throttle
                   true, true, true);   // Accelerator, Brake, Steering

/* Signal pins for ServoInput MUST be interrupt-capable pins!
 *     Uno, Nano, Mini (328P): 2, 3
 *     Micro, Leonardo (32U4): 0, 1, 2, 3, 7
 *             Mega, Mega2560: 2, 3, 18, 19, 20, 21
 * Reference: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
 *
 * You can use any pin that is not interrupt-capable, it just has a speed impact on that channels ability to see changes.
 */

// Constants for Keyboard commands (Uses asci int)
const int KEY_X = 0x58;     // X
const int KEY_SPACE = 0x20; // Space

// Pulse range for the RC receiver
const int PULSE_MIN = 1000;
const int PULSE_MAX = 2000;

// Ignore anything +/- this value
const int S_DEADZONE = 25;
const int T_DEADZONE = 25;
const int Z_DEADZONE = 50;

// Aux 3 is used as a variable range adjustment for the throttle/brake tuning
const int STEERING_MIN = -1000;
const int STEERING_MAX = 1000;
const int THROTTLE_MIN = -1000;
const int THROTTLE_MAX = 1000;
const int ACCELERATOR_MIN = 0;
const int ACCELERATOR_MAX = 1000;
const int BRAKE_MIN = 0;
const int BRAKE_MAX = 1000;
const int AUX_MIN = 0;
const int AUX_MAX = 1000;

// Signal pins for the RC receiver
const int THROTTLE_SIGNAL_PIN = 0; // MUST be interrupt-capable!
const int STEERING_SIGNAL_PIN = 1; // MUST be interrupt-capable!
const int AUX0_SIGNAL_PIN = 2;     // MUST be interrupt-capable!
const int AUX1_SIGNAL_PIN = 3;     // MUST be interrupt-capable!
const int AUX2_SIGNAL_PIN = 7;     // MUST be interrupt-capable!
const int AUX3_SIGNAL_PIN = 8;     // This is not an interupt-capable. We can use it for the 6th channel. It's performance is not critical.

// Dip switch pins
const int DIP1_PIN = 4;
const int DIP2_PIN = 5;
const int DIP3_PIN = 6;

// Dip switch values
bool dip1 = false;
bool dip2 = false;
bool dip3 = false;

// LED pins
const int RXLED = 17; // The RX LED has a defined Arduino pin
const int TXLED = 30; // The TX LED has a defined Arduino pin

// ServoInputPin objects for each channel
ServoInputPin<THROTTLE_SIGNAL_PIN> throttle(PULSE_MIN, PULSE_MAX); // Throttle Setup (Channel 1)
ServoInputPin<STEERING_SIGNAL_PIN> steering(PULSE_MIN, PULSE_MAX); // Steering Setup (Channel 2)
ServoInputPin<AUX0_SIGNAL_PIN> aux0(PULSE_MIN, PULSE_MAX);         // AUX0 Setup (Channel 3) Button 1
ServoInputPin<AUX1_SIGNAL_PIN> aux1(PULSE_MIN, PULSE_MAX);         // AUX1 Setup (Channel 4) Button 2
ServoInputPin<AUX2_SIGNAL_PIN> aux2(PULSE_MIN, PULSE_MAX);         // AUX2 Setup (Channel 5) Button 3
ServoInputPin<AUX3_SIGNAL_PIN> aux3(PULSE_MIN, PULSE_MAX);         // AUX3 Setup (Channel 6) X Axis

// Variables to hold the current input values per-cycle
int steeringInput = 0;
int lastSteeringInput = 0;
int throttleInput = 0;
int lastThrottleInput = 0;
int auxInput[] = {0, 0, 0, 0}; // B1, B2, B3
bool buttonState[] = {false, false, false};

// Final Values to be sent to the Joystick
int lastSteeringOutput = 0;
int lastAcceleratorOutput = 0;
int lastBrakeOutput = 0;
int lastAux3Output = 0;

// Adjustment factors
float steeringExponent = 1.64; // F1-24 exponent, adjust based on desired sensitivity
float curveAdjustment = 1.0f;  // Example curve adjustment (0.5 to 1.5) 1.0 is neutral

// Custom map function for floating-point values
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setup()
{
  Serial.begin(115200);

  // Set the LED pins as outputs
  pinMode(RXLED, OUTPUT);
  pinMode(TXLED, OUTPUT);

  // Attach to receiver channels
  throttle.attach();
  steering.attach();

  // Attach each AUX channel
  aux0.attach();
  aux1.attach();
  aux2.attach();
  aux3.attach();

  // set the dip switch pins as inputs
  if (dipSwitchEnabled)
  {
    pinMode(DIP1_PIN, INPUT_PULLUP);
    pinMode(DIP2_PIN, INPUT_PULLUP);
    pinMode(DIP3_PIN, INPUT_PULLUP);
  }

  // Configure Joystick
  Joystick.setSteeringRange(STEERING_MIN, STEERING_MAX);
  Joystick.setThrottleRange(ACCELERATOR_MAX, ACCELERATOR_MIN);
  Joystick.setAcceleratorRange(ACCELERATOR_MAX, ACCELERATOR_MIN);
  Joystick.setBrakeRange(BRAKE_MAX, BRAKE_MIN);

  // Initialize the Joystick with autosend set to false
  Joystick.begin(false);

  Serial.print("Waiting for receiver signals.");

  bool ledState = false;

  // Wait for all signals to be ready
  while (!ServoInput.available())
  {
    digitalWrite(RXLED, ledState ? ON : OFF); // Toggle the LED
    digitalWrite(TXLED, ledState ? OFF : ON); // Toggle the LED
    ledState = !ledState;                     // Update the LED state for the next iteration
    Serial.print(".");
    delay(250);
  }

  // Wait for the receiver to be ready
  digitalWrite(RXLED, OFF);
  digitalWrite(TXLED, OFF);
  delay(500);

  // Flash the LED 3 times to indicate that the receiver is ready
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(RXLED, ON);
    digitalWrite(TXLED, ON);
    delay(500);
    digitalWrite(RXLED, OFF);
    digitalWrite(TXLED, OFF);
    delay(500);
  }

  Serial.print("Ready to play!");
}

void loop()
{
  if (dipSwitchEnabled)
  {
    readDipSwitches();
  }
  processSteering();
  processThrottle();
  processAuxChannels();

  // Send the current state of the Joystick
  Joystick.sendState();
  clearLEDs();
  write("\n");
}

void readDipSwitches()
{
  dip1 = digitalRead(DIP1_PIN) == ON;
  dip2 = digitalRead(DIP2_PIN) == ON;
  dip3 = digitalRead(DIP3_PIN) == ON;

  write("Dip1: ");
  write(dip1);
  write(", Dip2: ");
  write(dip2);
  write(", Dip3: ");
  write(dip3);
  write(", ");

  // Update the steering exponential
  steeringExponential = dip1;
  // Update the accelerator curve
  acceleratorCurve = dip2;
  // Update the brake curve
  brakeCurve = dip3;
}

void processSteering()
{
  // Get the steering position -1000-1000 Left to Right
  steeringInput = steering.map(-1000, 1000) * -1; // Flip the values to reverse the servo
  write("S_In: ");
  write(steeringInput);

  // If nothing changed, no need to do anything
  if (lastSteeringInput == steeringInput)
  {
    return;
  }

  lastSteeringInput = steeringInput;

  if (abs(steeringInput) > T_DEADZONE)
    toggleRXLED();

  // See if the steering is outside the deadzone.
  if (abs(steeringInput) > S_DEADZONE)
  {
    adjustSteering(steeringInput);
  }
  else
  {
    adjustSteering(0);
  }
}

float applySteeringExponent(int rawSteeringValue, float exponent)
{
  // Normalize the input to range -1 to 1
  float normalizedInput = rawSteeringValue / 1000.0;

  // Apply the exponential function
  float adjustedValue = (normalizedInput < 0 ? -1 : 1) * pow(abs(normalizedInput), exponent);

  // Denormalize back to the original range
  float denormalizedOutput = adjustedValue * 1000;

  return denormalizedOutput;
}

void adjustSteering(int value)
{
  // If nothing changed, no need to do anything
  if (lastSteeringOutput == value)
  {
    return;
  }

  lastSteeringOutput = value;

  // Apply the steering exponential if enabled
  if (steeringExponential)
  {
    setSteering(applySteeringExponent(value, steeringExponent));
  }
  else
  {
    setSteering(value);
  }
}

void setSteering(int value)
{
  // Log the output to the Serial Monitor to import into Excel as a graph over time.
  // We will need the following columns: Time, Steering, Exponent, Output
  if (serialLoggingEnabled)
  {
    Serial.print("S, ");
    Serial.print(millis());
    Serial.print(", ");
    Serial.print(value);
    Serial.print(", ");
    Serial.println(steeringInput);
  }

  toggleTXLED();
  Joystick.setSteering(value);
}

void processThrottle()
{
  // Get the throttle position -1000-0 Brake 0-1000 Accelerator.
  throttleInput = throttle.map(THROTTLE_MIN, THROTTLE_MAX);
  write(", T_In: ");
  write(throttleInput);

  // If nothing changed, no need to do anything
  if (lastThrottleInput == throttleInput)
  {
    return;
  }

  lastThrottleInput = throttleInput;

  if (abs(throttleInput) > T_DEADZONE)
  {
    toggleRXLED();

    // See if the throttle is outside the deadzone.
    if (throttleInput > T_DEADZONE)
    {
      adjustAccelerator(abs(throttleInput));
      adjustBrake(0);
    }
    else if (throttleInput < (T_DEADZONE * -1))
    {
      adjustAccelerator(0);
      adjustBrake(abs(throttleInput));
    }
    else
    {
      adjustAccelerator(0);
      adjustBrake(0);
    }
  }
  else
  {
    adjustAccelerator(0);
    adjustBrake(0);
  }
}

// Logit function
float logit(float x)
{
  // Ensure x is within (0, 1) to avoid division by zero or negative values under log
  x = max(0.0001f, min(0.9999f, x));
  return log(x / (1 - x));
}

// Adjusted Logit-curve function
float adjustedLogitCurve(float input, float adjustment, float blendFactor)
{
  // Normalize input from 0-1000 to 0 to 1 for logit compatibility
  float normalizedInput = (input / 1000.0f);
  // Ensure the input is within the bounds for the logit function
  normalizedInput = max(0.0001f, min(0.9999f, normalizedInput));
  // Apply logit function directly on normalized input
  float logitOutput = logit(normalizedInput);
  // Adjust the curve based on the adjustment parameter
  float adjustedLogitOutput = logitOutput * adjustment;
  // Map the adjusted logit output back to a suitable range, e.g., 0-1000
  float adjustedOutput = (adjustedLogitOutput + 6) / 12.0f * 1000.0f; // Example mapping
  // Blend the adjusted output with the original input
  float blendedOutput = (blendFactor * adjustedOutput) + ((1.0f - blendFactor) * input);
  // Ensure the result is within the range [0, 1000]
  blendedOutput = max(0.0f, min(1000.0f, blendedOutput));
  return blendedOutput;
}

void printAdjustedLogitCurve()
{
  // Print the adjustedLogitCurve curve for the range 0-1000
  // With the blends from -.5 to 1.5
  Serial.println("-0.5, -0.25, -0.1, i, 0.1, 0.25, 0.5, 0.75, 1.0, 1.25, 1.5");
  for (int i = 0; i <= 1000; i++)
  {
    float adjustedOutput_n50 = adjustedLogitCurve(i, curveAdjustment, -.5);
    float adjustedOutput_n25 = adjustedLogitCurve(i, curveAdjustment, -.25);
    float adjustedOutput_n10 = adjustedLogitCurve(i, curveAdjustment, -.1);
    float adjustedOutput_10 = adjustedLogitCurve(i, curveAdjustment, .1);
    float adjustedOutput_25 = adjustedLogitCurve(i, curveAdjustment, .25);
    float adjustedOutput_50 = adjustedLogitCurve(i, curveAdjustment, .5);
    float adjustedOutput_75 = adjustedLogitCurve(i, curveAdjustment, .75);
    float adjustedOutput_100 = adjustedLogitCurve(i, curveAdjustment, 1.0);
    float adjustedOutput_125 = adjustedLogitCurve(i, curveAdjustment, 1.25);
    float adjustedOutput_150 = adjustedLogitCurve(i, curveAdjustment, 1.5);

    Serial.print(adjustedOutput_n50);
    Serial.print(", ");
    Serial.print(adjustedOutput_n25);
    Serial.print(", ");
    Serial.print(adjustedOutput_n10);
    Serial.print(", ");
    Serial.print(i);
    Serial.print(", ");
    Serial.print(adjustedOutput_10);
    Serial.print(", ");
    Serial.print(adjustedOutput_25);
    Serial.print(", ");
    Serial.print(adjustedOutput_50);
    Serial.print(", ");
    Serial.print(adjustedOutput_75);
    Serial.print(", ");
    Serial.print(adjustedOutput_100);
    Serial.print(", ");
    Serial.print(adjustedOutput_125);
    Serial.print(", ");
    Serial.print(adjustedOutput_150);
    Serial.println();
  }
}

void adjustAccelerator(int value)
{
  // If nothing changed, no need to do anything
  if (lastAcceleratorOutput == value)
  {
    return;
  }

  lastAcceleratorOutput = value;

  // Apply the throttle logarithm if enabled
  if (acceleratorCurve && auxInput[3] > AUX_MIN)
  {
    // map auxInput[3] from its range of 0-1000 to -.1 to 1.5 for the blend factor
    float aux3Adjustment = mapFloat(auxInput[3], 0.0f, 1000.0f, -0.1f, 1.5f);

    // Apply the adjusted Logit-curve to the accelerator input
    float adjustedInput = adjustedLogitCurve(value, curveAdjustment, aux3Adjustment);

    // Map the adjusted input back to 0-1000 range (or any other desired range)
    int outputValue = (int)(adjustedInput + 0.5f); // Round to the nearest integer

    setAccelerator(outputValue, aux3Adjustment);
  }
  else
  {
    setAccelerator(value, 0);
  }
}

void setAccelerator(int value, float adjustment)
{
  if (value > ACCELERATOR_MAX)
  {
    value = ACCELERATOR_MAX;
  }
  else if (value < ACCELERATOR_MIN)
  {
    value = ACCELERATOR_MIN;
  }

  // Log the output to the Serial Monitor to import into Excel as a graph over time.
  // We will need the following columns: Time, Throttle, Aux3
  if (serialLoggingEnabled)
  {
    Serial.print("A, ");
    Serial.print(millis());
    Serial.print(", ");
    Serial.print(value);
    Serial.print(", ");
    Serial.print(throttleInput);
    Serial.print(", ");
    Serial.print(adjustment);
    Serial.print(", ");
    Serial.println(auxInput[3]);
  }

  toggleTXLED();

  if (auxInput[3] > AUX_MIN)
  {
    Joystick.setThrottle(value);
    Joystick.setAccelerator(0);
  }
  else
  {
    Joystick.setThrottle(0);
    Joystick.setAccelerator(value);
  }
}

void adjustBrake(int value)
{

  // If nothing changed, no need to do anything
  if (lastBrakeOutput == value)
  {
    return;
  }

  lastBrakeOutput = value;

  // Apply the throttle logarithm if enabled
  if (brakeCurve && auxInput[3] > AUX_MIN)
  {
    // map auxInput[3] from its range of 0-1000 to -.1 to 1.5 for the blend factor
    float aux3Adjustment = mapFloat(auxInput[3], 0.0f, 1000.0f, -0.1f, 1.5f);

    // Apply the adjusted Logit-curve to the accelerator input
    float adjustedInput = adjustedLogitCurve(value, curveAdjustment, aux3Adjustment);

    // Map the adjusted input back to 0-1000 range (or any other desired range)
    int outputValue = (int)(adjustedInput + 0.5f); // Round to the nearest integer

    setBrake(outputValue, aux3Adjustment);
  }
  else
  {
    setBrake(value, 0);
  }
}

void setBrake(int value, float adjustment)
{
  if (value > BRAKE_MAX)
  {
    value = BRAKE_MAX;
  }
  else if (value < BRAKE_MIN)
  {
    value = BRAKE_MIN;
  }

  // Log the output to the Serial Monitor to import into Excel as a graph over time.
  // We will need the following columns: Time, Throttle, Aux3
  if (serialLoggingEnabled)
  {
    Serial.print("B, ");
    Serial.print(millis());
    Serial.print(", ");
    Serial.print(value);
    Serial.print(", ");
    Serial.print(abs(throttleInput));
    Serial.print(", ");
    Serial.print(adjustment);
    Serial.print(", ");
    Serial.println(auxInput[3]);
  }

  toggleTXLED();

  Joystick.setBrake(value);
}

void processAuxChannels()
{
  // Get Aux Values
  auxInput[0] = aux0.map(AUX_MIN, AUX_MAX);
  auxInput[1] = aux1.map(AUX_MIN, AUX_MAX);
  auxInput[2] = aux2.map(AUX_MIN, AUX_MAX);
  auxInput[3] = aux3.map(AUX_MIN, AUX_MAX);

  // Aux values 0-2 are buttons
  for (int i = 0; i < 4; i++)
  {
    write(", Aux");
    write(i);
    write(": ");
    write(auxInput[i]);

    if (auxInput[i] > AUX_MIN)
    {
      releaseButton(i);
    }
    else
    {
      pressButton(i);
    }
  }

  write(", Aux3: ");
  write(auxInput[3]);

  // Aux value 3 is used for the throttle and brake sensitivity and XAxis
  setAux3(auxInput[3]);
}

void setAux3(int value)
{
  // If nothing changed, no need to do anything
  if (abs(lastAux3Output - value) < Z_DEADZONE)
  {
    return;
  }

  lastAux3Output = value;

  // Log the output to the Serial Monitor to import into Excel as a graph over time.
  // We will need the following columns: Time, Throttle, Aux3
  if (serialLoggingEnabled)
  {
    Serial.print("XRotation, ");
    Serial.print(millis());
    Serial.print(", ");
    Serial.print(value);
    Serial.print(", ");
    Serial.println(auxInput[3]);
  }

  toggleTXLED();

  // Not setting the XAxis to the value of Aux3, as it is used for the throttle and brake sensitivity
}

void pressButton(int button)
{
  if (buttonState[button])
  {
    return;
  }

  buttonState[button] = true;

  toggleTXLED();

  if (printCurve)
  {
    printAdjustedLogitCurve();
  }

  if (serialLoggingEnabled)
  {
    Serial.print("B");
    Serial.print(button);
    Serial.print(": ");
    Serial.print(millis());
    Serial.print(", Press(");
    Serial.print(auxInput[button]);
    Serial.println(")");
  }

  if (auxInput[3] > AUX_MIN)
  {
    Joystick.pressButton(button);
  }
  else
  {
    // Switch statement to handle the different buttons
    switch (button)
    {
    case 0: // Send keyboard command X (Look Back)
      Keyboard.press(KEY_DOWN_ARROW);
      break;
    case 1: // Send keyboard command Space (Emergency Brake)
      Keyboard.press(KEY_SPACE);
      break;
    case 2: // Send keyboard command SPACE (Nitro)
      Keyboard.press(KEY_LEFT_SHIFT);
      break;
    case 3:
      break;
    }
  }
}

void releaseButton(int button)
{
  if (!buttonState[button])
  {
    return;
  }

  buttonState[button] = false;

  toggleTXLED();

  if (serialLoggingEnabled)
  {
    Serial.print("B");
    Serial.print(button);
    Serial.print(": ");
    Serial.print(millis());
    Serial.print(", Release(");
    Serial.print(auxInput[button]);
    Serial.println(")");
  }

  if (auxInput[3] > AUX_MIN)
  {
    Joystick.releaseButton(button);
  }
  else
  {
    // Switch statement to handle the different buttons
    switch (button)
    {
    case 0: // Send keyboard command X (Look Back)
      Keyboard.release(KEY_DOWN_ARROW);
      break;
    case 1: // Send keyboard command Space (Emergency Brake)
      Keyboard.release(KEY_SPACE);
      break;
    case 2: // Send keyboard command SPACE (Nitro)
      Keyboard.release(KEY_LEFT_SHIFT);
      break;
    case 3:
      break;
    }
  }
}

bool rxLedState = false;
bool txLedState = false;
unsigned long lastToggleRX = 0;
unsigned long lastToggleTX = 0;
const unsigned long toggleInterval = 25;

void toggleRXLED()
{
  unsigned long currentMillis = millis();
  if (currentMillis - lastToggleRX >= toggleInterval)
  {
    rxLedState = !rxLedState;
    digitalWrite(RXLED, rxLedState ? ON : OFF);
    lastToggleRX = currentMillis;
  }
}

void toggleTXLED()
{
  unsigned long currentMillis = millis();
  if (currentMillis - lastToggleTX >= toggleInterval)
  {
    txLedState = !txLedState;
    digitalWrite(TXLED, txLedState ? ON : OFF);
    lastToggleTX = currentMillis;
  }
}

// if the tigggleInterval has passed, and no new signal has been received, turn off the LED
void clearLEDs()
{
  unsigned long currentMillis = millis();
  if (rxLedState && currentMillis - lastToggleRX >= toggleInterval)
  {
    rxLedState = false;
    digitalWrite(RXLED, OFF);
    lastToggleRX = currentMillis;
  }
  if (txLedState && currentMillis - lastToggleTX >= toggleInterval)
  {
    txLedState = false;
    digitalWrite(TXLED, ON);
    lastToggleTX = currentMillis;
  }
}

void write(String message, bool newLine = false)
{
  if (!inputLoggingEnabled)
    return;
  Serial.print(message);
  if (newLine)
  {
    Serial.println();
  }
}

void write(int message, bool newLine = false)
{
  if (!inputLoggingEnabled)
    return;
  char formattedMessage[6]; // 5 characters + null terminator
  sprintf(formattedMessage, "%5d", message);
  Serial.print(formattedMessage);
  if (newLine)
  {
    Serial.println();
  }
}

void write(float message, bool newLine = false)
{
  if (!inputLoggingEnabled)
    return;
  char formattedMessage[10];                   // Adjust size as needed for your float formatting
  sprintf(formattedMessage, "%5.2f", message); // Adjust format as needed
  Serial.print(formattedMessage);
  if (newLine)
  {
    Serial.println();
  }
}

void write(unsigned long message, bool newLine = false)
{
  if (!inputLoggingEnabled)
    return;
  char formattedMessage[10];                  // Adjust size as needed for your long formatting
  sprintf(formattedMessage, "%5ld", message); // Adjust format as needed
  Serial.print(formattedMessage);
  if (newLine)
  {
    Serial.println();
  }
}