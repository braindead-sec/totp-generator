# DIY TOTP Generator

A do-it-yourself time-based one-time password generator. You know, for MFA.

![totp-generator](images/totp-generator.jpg)

It's like a cheap open-source [RSA dongle](https://www.tokenguard.com/SecurID-700.asp) that works with any multi-factor authentication service following the [TOTP standard](https://datatracker.ietf.org/doc/html/rfc6238).

Why? Because there are some things I use so frequently (SSH) that it's actually inconvenient to unlock my phone to get the MFA code. With this device mounted on my monitor, I can save *seconds* at a time.

Follow the instructions below to build your own!

### Hardware Components

- Arduino Nano (16MHz ATmega328, 5v) or cheap knock-off
- I2C 128x32 OLED display (SSD1306)
- I2C real-time clock module (DS3231) with battery
- Custom PCB to integrate components

![components](images/components.jpg)

### Hardware Assembly

Soldering required. Fine-point solder tip definitely recommended.

![tools](images/tools.jpg)

1. Attach header pins to OLED (short pins and spacers below, long pins above):

   ![step1-1](images/step1-1.jpg)

   ![step1-2](images/step1-2.jpg)


2. Attach header pins to Nano (note that not all pins are needed). Set the custom PCB in place to make sure the spacing is right - my original design was a hair too wide and set the pins at an angle - but don't solder it in yet. Short pins and spacers should be on the back of the Nano; long pings should protrude through the front:

   ![step2-1](images/step2-1.jpg)

   ![step2-2](images/step2-2.jpg)

   ![step2-3](images/step2-3.jpg)

   ![step2-4](images/step2-4.jpg)
   ![step4-5](images/step2-5.jpg)


3. Attach OLED to custom PCB (insert pins through the holes marked OLED, and don't cover the RTC holes!):

   ![step3-1](images/step3-1.jpg)

   ![step3-2](images/step3-2.jpg)


4. Attach custom PCB to Nano (make sure VIN and TX align on the Nano and custom PCB):

   ![step4-1](images/step4-1.jpg)

   ![step4-2](images/step4-2.jpg)


5. Attach RTC header pins to custom PCB (long pins and spacers below, short pins above):

   ![step5-1](images/step5-1.jpg)

   ![step5-2](images/step5-2.jpg)

6. Slide RTC onto its header pins:

   ![step6](images/step6.jpg)

Assembly is now complete!

![assembled](images/assembled.jpg)

### Software Installation

1. Download [Arduino IDE](https://www.arduino.cc/en/software).

2. [Install required boards](https://support.arduino.cc/hc/en-us/articles/360016119519-How-to-add-boards-in-the-board-manager):
   
   - `Adafruit AVR Boards` by Adafruit
   
3. [Install required libraries](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries):
   - `Adafruit SSD1306` by Adafruit
   - `Adafruit GFX Library` by Adafruit
   - `RTC` by Manjunath CV
   - `TOTP library` by Luca Dentella

4. Clone or export this GitHub repo.

5. Open the Arduino project (`totp-generator.ino`) in Arduino IDE.

6. Connect your fully assembled device to your computer using a USB data cable. The power light should come on, and it may blink if there is a default program installed to make it do that.

   ![connect](images/connect.jpg)

7. From the `Tools` menu in the Arduino IDE, configure these options:
   - Board: Arduino Nano
   - Processor: ATmega 328P
   - Port: whichever one corresponds to the device you just plugged in

   ![arduino-ide-settings](images/arduino-ide-settings.jpg)
   
8. Click the right-arrow button at the top of the program window to compile the code and upload it to the device.

If everything worked properly up to this point, your device should now be showing either "Connect to set clock" or "Connect to set keys" (depending on the initial RTC setting):

![programmed](images/programmed.jpg)

### Software Configuration

1. Install [Python 3](https://www.python.org/downloads/).

2. Install required Python libraries (just `pyserial`):
   * `pip3 install -r requirements.txt`

3. Connect your fully assembled and programmed device to your computer using a USB data cable.

4. Run the Python script:
   * `python3 setup-device.py`

5. Select the USB port corresponding to your device, when prompted.

6. After establishing a serial connection to your device, the script will automatically sync the time from your computer to the RTC.

   ![device-setup](images/device-setup.jpg)

7. For first-time setup, choose option 5 to initialize the device memory.

8. Choose option 3 to add a key to your device. First enter a three-character label for the key, then enter the key in the standard Base32 format. The device currently supports one or two keys at a time, and will adjust the display accordingly. Keys are recorded in the Arduino's built-in EEPROM without encryption (fair warning).
   - [Google Authenticator](https://support.google.com/accounts/answer/1066447): the app should print your key after completing setup, otherwise you can retrieve the code from the first line of your `.google_authenticator` file 
   - [AWS MFA](https://docs.aws.amazon.com/IAM/latest/UserGuide/id_credentials_mfa_enable_virtual.html): use the "Show Secret Key" option during setup

   ![add-key](images/add-key.jpg)

9. Use the other menu options to view the labels assigned to keys stored on the device, delete keys, reset the device memory (wipe all keys), and sync the time again. Choose option 6 when you're done.

Setup is now complete! Here's how it should look:

- One key

  ![one-key](images/one-key.jpg)

- Two keys

  ![two-keys](images/two-keys.jpg)

The display includes a countdown bar to let you know when the code is about to change. It uses a 30-second counter, which is the only time value I've seen used in practice. You can change this value, if necessary, by editing the Arduino code and reprogramming the device.

### Just for Fun ###

if you only intend to use one code, you can modify the Arduino program to use a nicer-looking large font (it's not included by default because of memory constraints).

- Delete this line: `#include <Fonts/FreeMonoBold9pt7b.h>`
- Search for `FreeSansBold9pt7b` and replace all matches with `FreeSansBold18pt7b`
- Delete `display.setTextSize(2);` from the `printCode` function
- Delete all of the code inside the `printTwoCodes` function, so that it just looks like one line: `void printTwoCodes(uint8_t key1, uint8_t key2) {}`
- Compile and upload the new program to your device!