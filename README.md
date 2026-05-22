# RoboVac
The RoboVac is an autonomous vacuum similar to the Roomba! It provides both manual and autonomous control options. The robot was built from electronic components and 3D-printed materials. 

Below is an image of the robot without its outer shell (in its early development):
![RoboVac](https://github.com/user-attachments/assets/6c4c748b-a516-401b-9b95-2bdaee5e0079)
The middle rectangular box contains the fan and waste storage. Ultrasonic sensors are located on the edge, which help detect obstacles. Other components, such as a Bluetooth module, DC motors and battery packs, are also present. The purpose of this project was to provide a cheaper alternative to the Roomba for my parent.

## Before Running
You would need to create your own robot vacuum. This means you will need:
- 3 ultrasonic sensors
- 2 DC motors
- 1 HC-05 Bluetooth module (Make sure to use a voltage divider for this, as it takes in 3.3V)
- 1 9V battery pack
- 1 vacuum fan of your choice
- 3D printed shell

Note which pins for each of these components are documented in the `roboVac.ino` file by referring the pin variables.

## How to Run
- Run `roboVac.ino` as a sketch on an Arduino UNO microcontroller. Make sure the Arduino is properly connected to all components with the pins.
- Optionally, if you want to access the remote control via Bluetooth, run `roboVacApp.py` on your local device. Toggle to manual mode to access the remote control features!
