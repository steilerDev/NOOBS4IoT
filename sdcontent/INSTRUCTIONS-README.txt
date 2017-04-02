NOOBS4IoT INSTALLATION INSTRUCTIONS

1. Insert an SD card that is 4GB or greater in size into your computer.
2. Format the SD card using the platform-specific instructions below:
   a. Windows
      i. Download the SD Association's Formatting Tool from https://www.sdcard.org/downloads/formatter_4/eula_windows/
      ii. Install and run the Formatting Tool on your machine
      iii. Set "FORMAT SIZE ADJUSTMENT" option to "ON" in the "Options" menu
      iv. Check that the SD card you inserted matches the one selected by the Tool
      v. Click the "Format" button
   b. Mac
      i. Download the SD Association's Formatting Tool from https://www.sdcard.org/downloads/formatter_4/eula_mac/
      ii. Install and run the Formatting Tool on your machine
      iii. Select "Overwrite Format"
      iv. Check that the SD card you inserted matches the one selected by the Tool
      v. Click the "Format" button
   c. Linux
      i. We recommend using gparted (or the command line version parted)
      ii. Format the entire disk as FAT
3. Extract the files contained in this NOOBS4IoT zip file.
4. Copy the extracted files onto the SD card that you just formatted so that this file is at the root directory of the SD card. Please note that in some cases it may extract the files into a folder, if this is the case then please copy across the files from inside the folder rather than the folder itself.
5. Insert the SD card into your Pi and connect the power supply.

Your Pi will now boot into NOOBS4IoT and should display that he is awaiting incoming connections and showing it's IP address.
Send a payload containing the OS information via a POST command to http://<PI IP Address>/ and see progress on the Pi. After the process is finished, the Pi should automatically boot into the fresh installed OS.

OS should have the layout as obtained by http://downloads.raspberrypi.org/os_list_v3.json (Plus-sign indicates a mandatory entry)
A list of the following entries is expected.
  {
      "description": "LibreELEC is a fast and user-friendly Kodi Entertainment Center distribution.",
      "icon": "http://releases.libreelec.tv/noobs/LibreELEC_RPi/LibreELEC_RPi.png",
      "marketing_info": "http://releases.libreelec.tv/noobs/LibreELEC_RPi/marketing.tar",
      "nominal_size": 1024,
+     "os_info": "http://releases.libreelec.tv/noobs/LibreELEC_RPi/os.json",
+     "os_name": "LibreELEC_RPi",
      "partition_setup": "http://releases.libreelec.tv/noobs/LibreELEC_RPi/partition_setup.sh",
+     "partitions_info": "http://releases.libreelec.tv/noobs/LibreELEC_RPi/partitions.json",
      "release_date": "2016-05-17",
      "supported_hex_revisions": "2,3,4,5,6,7,8,9,d,e,f,10,11,12,14,19,0092",
      "supported_models": [
          "Pi Model",
          "Pi Compute Module",
          "Pi Zero"
      ],
+     "tarballs": [
+         "http://releases.libreelec.tv/noobs/LibreELEC_RPi/LibreELEC_RPi_System.tar.xz",
+         "http://releases.libreelec.tv/noobs/LibreELEC_RPi/LibreELEC_RPi_Storage.tar.xz"
      ]
  },
