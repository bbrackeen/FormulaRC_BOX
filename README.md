# FormulaRC_BOX

FormulaRC_BOX is an Arduino-based project designed to allow a familiar remote control driving experience with modern driving simulators. It provides a customizable interface for adjusting the throttle curve, steering sensitivity, and other parameters to suit different driving conditions and games. Generally I have everything configured for a DumboRC X6PM-350 6 channel receiver which is cheap, comfortable, and has a good amount of channels (3 toggle buttons, and 1 range knob that I use for adjusting throttle/brake curves during game). Although any modern RC transmitter should work, along with your own ours of experience if you come from an RC Car background. So keep in mind as I refer to the channels, generally I am speaking of the configuration of the X6PM. Also keep in mind my personal goal is racing in EA F1, so my tuning is based off that platform, but should apply to any platform that takes basic joystick inputs.

## Features

- **Customizable Throttle Curve:** Adjust the throttle response curve to match your driving style or the specific requirements of your car. The throttle curve is based off a logit curve where low throttle stays relatively high, while center to high has the most amount of resolution snapping to full throttle in the very last 90% depending on how you set the AUX value in the curve (AUX value being channel 5). 50% is a good general predictable middle where you can hold traction in high speed corners and snap to full throttle on the way out. Where 10% is more centered and seems to work best with a wet tire configuration where you need a slower ramp up and full throttle almost doesn't exist until nearly full throttle.
- **Steering Sensitivity Adjustment:** Fine-tune the steering sensitivity for precise control at high speeds or in tight corners. The steering exponential allows for very fine control closer to the center and gradually gets higher as you reach full lock. 
- **Steering and Throttle Deadzones:** Keeps long high speed straights dead steady by reducing the noise at zero point, while allowing for gentle corrections.

## Hardware Requirements - Everything found on Amazon

- Arduino Uno/Nano/Mega or any compatible microcontroller board (I used an ATmega32U4 5V/16MHz Module Board)
- Transmitter and Receiver (DumboRC X6PM-350 6 channel receiver)

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
Assemble your hardware according to the schematics provided in the `hardware` folder.
4. **Upload the Sketch:**
Connect your Arduino board to your computer and upload the sketch using the Arduino IDE.

## More Reading 

See the PDF file for my tablet scribble, but it has pictures, and explains some finer details.

- Billy