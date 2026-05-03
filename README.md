# Smart-House-Automation-System-Project
# Smart Home Automation System

Arduino Uno-Based Smart Home Automation System with Password Protection, LCD Menu Interface, Real-Time Clock, Temperature Monitoring, Motion Detection, and Lighting Control

# Description
This project presents a smart home automation system designed using Arduino Uno. The system uses a 16x2 LCD display and a 4x4 keypad as the main user interface. Before entering the system menu, the user must log in with a valid password. If the password is entered incorrectly, the red LED turns on and the buzzer gives an alert. If the password is correct, the green LED turns on for a short time and the user gains access to the main menu.

The system includes temperature monitoring, real-time clock display, motion detection, and ambient light control. It also supports user management by allowing new users to be registered and stored in EEPROM memory.

# Objective
The main objective of this project is to design and implement a microcontroller-based smart home automation system using Arduino Uno. The system combines sensor monitoring, password-based access control, menu-driven interaction, and automatic output control in a single embedded application.

This project aims to demonstrate practical use of embedded systems concepts such as:
- digital and analog input/output control
- sensor integration
- EEPROM data storage
- real-time clock communication
- menu-based LCD interface design
- basic automation logic

# Hardware Components
- Arduino Uno
- 16x2 LCD Display
- I2C LCD Module
- 4x4 Keypad
- RTC Module (DS1307 / DS3231)
- LM35 Temperature Sensor
- PIR Motion Sensor
- LDR Sensor
- Red LED
- Green LED
- White LED
- Buzzer
- DC Fan
- NPN Transistor
- Diode
- Resistors
- Breadboard
- Jumper Wires

# System Architecture

## Access Control Module
The system starts with a login screen. The user selects a user number and enters a 4-digit password using the keypad. Incorrect password entry activates the red LED and buzzer. Correct password entry opens the main menu.

## Temperature Monitoring Module
The LM35 sensor measures the ambient temperature. The measured value is displayed on the LCD in degrees Celsius. If the temperature exceeds the predefined threshold, the fan is activated automatically.

## Real-Time Clock Module
The RTC module provides the current local time. When the user selects the clock menu, the system displays the current time on the LCD in real time.

## Lighting Control Module
The lighting section contains two sub-modes:

### Motion Mode
The PIR sensor detects movement. If motion is detected, the white LED turns on. If no motion is detected for a certain delay period, the LED turns off.

### Ambient Light Mode
The LDR sensor detects the light level of the environment. If the environment is dark, the white LED turns on. If sufficient light is detected, the white LED turns off.

## User Management Module
The system allows creation of new users. Passwords are stored in EEPROM memory so that registered users remain saved even after reset or power loss.

# Implementation
The project was implemented in Arduino IDE using C/C++. The software was developed with a modular structure to improve readability, debugging, and future development.

Main software functions include:
- keypad-based password entry
- LCD menu navigation
- RTC communication
- temperature sensing and fan control
- motion sensing and timed LED control
- ambient light sensing
- EEPROM-based user registration and storage

# Working Principle
1. The system powers on and displays the login screen.
2. The user selects a user number and enters a password.
3. If the password is correct, the main menu is displayed.
4. The user can access:
   - Temperature
   - Clock
   - Lighting
   - User Menu
5. Sensor data is processed in real time.
6. LEDs, buzzer, and fan are controlled according to system conditions.

# Code
The project code was developed in Arduino IDE using C/C++. We designed the software in a modular and readable way. It includes password verification, menu navigation, RTC communication, temperature monitoring, lighting control, and EEPROM-based user management.

The code was organized to make each module easier to test separately. Sensor reading, menu drawing, user login control, and output control were handled in different sections of the program for clarity and maintainability.

# Contribution
This project was developed as a team project. We contributed together to the system design, hardware integration, coding, debugging, and testing stages of the project.

Our contribution included:
- designing the smart home automation concept
- planning the menu structure and interface flow
- implementing password-protected access control
- integrating LCD, keypad, RTC, LM35, PIR, and LDR
- programming automatic fan and lighting logic
- implementing new user registration with EEPROM
- testing both simulation and physical prototype stages

# Installation & Setup
1. Clone or download the repository.
2. Open the project in Arduino IDE.
3. Install the required libraries:
   - Wire
   - Keypad
   - RTClib
   - LiquidCrystal_I2C
   - EEPROM
4. Connect the hardware components to Arduino Uno.
5. Upload the code to the Arduino board.
6. Power the system and test all menu functions.

# Usage
- The system starts with a login screen.
- The user selects a user number and enters a password.
- After successful login, the main menu is displayed.
- The temperature menu shows the current temperature and fan status.
- The clock menu shows the local time.
- The lighting menu allows the user to check motion detection and ambient light status.
- The user menu allows registration of a new user.

# Results
The project successfully combines access control, real-time monitoring, and automatic output control in a single embedded system application. The system provides a simple interface, sensor-based automation, and multi-user support through EEPROM storage.

The final implementation demonstrates how Arduino Uno can be used to create a compact and practical smart home automation prototype for educational purposes.

# Future Improvements
Possible future improvements include:
- mobile app or Bluetooth control
- Wi-Fi-based monitoring
- door lock integration with servo motor
- gas and smoke detection
- relay-based room lighting
- data logging to SD card
- more advanced user authorization levels

# Conclusion
This project demonstrates a practical smart home automation system using Arduino Uno. It combines password-based access, sensor integration, real-time display, and automatic control in one embedded design. The project is suitable for microprocessor and embedded systems courses and can be expanded into a more advanced smart home application in the future.
