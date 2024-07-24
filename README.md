# FormulaRC_BOX

FormulaRC_BOX is an Arduino-based project designed to allow a familiar remote control driving experience with modern driving simulators. It provides a customizable interface for adjusting the throttle curve, steering sensitivity, and other parameters to suit different driving conditions and games. Generally, everything is configured for a DumboRC X6PM-350 6 channel receiver which is cheap, comfortable, and has a good amount of channels (3 toggle buttons, and 1 range knob that I use for adjusting throttle/brake curves during the game). Although any modern RC transmitter should work, along with your own hours of experience if you come from an RC Car background. So keep in mind as I refer to the channels, generally I am speaking of the configuration of the X6PM. Also keep in mind my personal goal is racing in EA F1, so my tuning is based on that platform, but should apply to any platform that takes basic joystick inputs.

## Features

- **Customizable Throttle Curve:** Adjust the throttle response curve to match your driving style or the specific requirements of your car. The throttle curve is based on a logit curve where low throttle stays relatively high, while center to high has the most amount of resolution snapping to full throttle in the very last 90% depending on how you set the AUX value in the curve (AUX value being channel 5). 50% is a good general predictable middle where you can hold traction in high-speed corners and snap to full throttle on the way out. Where 10% is more centered and seems to work best with a wet tire configuration where you need a slower ramp-up and full throttle almost doesn't exist until nearly full throttle.
- **Steering Sensitivity Adjustment:** Fine-tune the steering sensitivity for precise control at high speeds or in tight corners. The steering exponential allows for very fine control closer to the center and gradually gets higher as you reach full lock.
- **Steering and Throttle Deadzones:** Keeps long high-speed straights dead steady by reducing the noise at the zero point, while allowing for gentle corrections.
- **Runtime Dip Switchs:** There are 3 dip switches configured for runtime to turn the steering/Accelerator/Brake curves On/Off. If you don't like the way the curves work, tune them yourself, or turn them off.

## Hardware Requirements - Everything found on Amazon

- Arduino Uno/Nano/Micro/Mega or any compatible microcontroller board (I used an ATmega32U4 5V/16MHz Module Board)
- Transmitter and Receiver (DumboRC X6PM-350 6 channel receiver)
- 3 position Dip Switch
- Female to Female Breadboard Jumper Wires

## Libraries Used - All found on the Arduino IDE Library Installer

- PinChangeInterrupt.h
- ServoInput.h
- Joystick.h

## Installation

1. **Clone the Repository:**
Clone the FormulaRC_BOX repository by running the following command in your terminal or command prompt:
git clone https://github.com/bbrackeen/FormulaRC_BOX.git

2. **Open the Project:**
Open the `FormulaRC_BOX.ino` file in the Arduino IDE or your preferred development environment.
3. **Connect the Hardware:**
Assemble your hardware according to the schematics provided in the [PDF file](https://github.com/bbrackeen/FormulaRC_BOX/blob/main/Formula%20RC%20BOX.pdf) or your specific hardware.
4. **Upload the Sketch:**
Connect your Arduino board to your computer and upload the sketch using the Arduino IDE.
5. **Have Fun!**

## Configuration

- **Main Configuration Settings** You can configure the behavior of the software by modifying the variables at the top of the .ino source file. If you don't want dip switches, and want to compile and push with everything set the way you want. Do it.

## More Reading

See the [PDF file](https://github.com/bbrackeen/FormulaRC_BOX/blob/main/Formula%20RC%20BOX.pdf) for my tablet scribble. It has pictures and explains some finer details.

## Aknowledgements

- https://www.micropanoply.com/arduino/arduino-joystick First project I found that got me started in this.
