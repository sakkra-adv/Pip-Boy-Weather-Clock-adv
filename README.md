Pip-Boy Weather Clock - M5Cardputer ADV Edition ☢️
This is a modified and fixed version of the original Pip-Boy Weather Clock by nishad2m8.

THE PROBLEM

The original project was designed for the standard M5Cardputer. On the newer ADV (Advanced) model with the StampS3, the keyboard was completely dead (unresponsive) because the hardware uses a different I2C initialization sequence and pin mapping.

THE FIX (What we did)

Keyboard Wake-up: We successfully implemented the I2C handshake (Address 0x34) and register settings required to initialize the keyboard chip on the ADV hardware.

StampS3 Optimization: Adjusted display brightness and system initialization for the S3-based board.

UK/Europe Time Support: Pre-configured with POSIX time strings to support automatic Daylight Saving Time (GMT/BST) transitions.

KNOWN ISSUES

Audio (WIP): Sound is currently NOT WORKING. The ADV model uses the ES8311 audio codec via I2S, which requires a specific initialization sequence that hasn't been implemented in this version yet. If you are a developer and know how to wake up the ES8311, feel free to contribute!

INSTALLATION

Format your microSD card to FAT32.

Create a file named config.txt in the root directory of the SD card.

Paste the following lines and fill in your details:



```
WIFI_SSID=Your_WiFi_Name
WIFI_PASSWORD=Your_WiFi_Password
TIME_ZONE=GMT0BST,M3.5.0/1,M10.5.0/2
API_KEY=Your_WeatherAPI_Key
LOCATION=Your_City_Name
```


(Note: The TIME_ZONE string above is for the UK. For Poland, use: CET-1CEST,M3.5.0,M10.5.0/3)

CREDITS

Original UI & Logic: nishad2m8

ADV Keyboard Fix & Porting: Sakkra-adv
