# NOOBS4IoT

This hard fork of [NOOBS (c) 2013, Raspberry Pi All rights reserved](https://github.com/raspberrypi/noobs) ([forked on 6th of September 2016](https://github.com/steilerDev/TOSCA4IoT_RPi/commit/7854f404d7653233f0d4c4d729ceed1e10ab9e88)) is intended to allow the usage of Raspberry Pi's in the context of remote provisioning. This project is part of the TOSCA project at the [Institute for the architecture of application systems (IAAS) at the University of Stuttgart](http://www.iaas.uni-stuttgart.de/).

A full list of all third party libraries, frameworks and participant can be found [in the wiki](https://github.com/steilerDev/NOOBS4IoT/wiki/Acknowledgement)

# Installation

1. Insert an SD card that is 4GB or greater in size into your computer.
2. Format the SD card using the platform-specific instructions below:
    1. Windows
        1. Download the SD Association's Formatting Tool from https://www.sdcard.org/downloads/formatter_4/eula_windows/
        2. Install and run the Formatting Tool on your machine
        3. Set "FORMAT SIZE ADJUSTMENT" option to "ON" in the "Options" menu
        4. Check that the SD card you inserted matches the one selected by the Tool
        5. Click the "Format" button
    2. Mac
        1. Download the SD Association's Formatting Tool from https://www.sdcard.org/downloads/formatter_4/eula_mac/
        2. Install and run the Formatting Tool on your machine
        3. Select "Overwrite Format"
        4. Check that the SD card you inserted matches the one selected by the Tool
        5. Click the "Format" button
    3. Linux
        1. We recommend using gparted (or the command line version parted)
        3. Format the entire disk as FAT
3. Extract the files contained in this NOOBS4IoT zip file.
4. Copy the extracted files onto the SD card that you just formatted so that this file is at the root directory of the SD card. Please note that in some cases it may extract the files into a folder, if this is the case then please copy across the files from inside the folder rather than the folder itself.
5. Insert the SD card into your Pi and connect the power supply.

Please check out [the wiki](https://github.com/steilerDev/NOOBS4IoT/wiki/Installation) for more information.

# Usage

## Initial boot
The Pi will initially boot into the recovery system (until an OS is installed). After preparing the SD Card it should display that he is awaiting incoming connections and showing it's IP address.

Now the REST API (shown below) can be used.

## Post-Install Boots
After an/multiple OS were installed the Pi will boot into the default partition. The default partition can be modified by mounting the settings partition (`/dev/mmcblk0p5`) and specifying a new partition in the `default_boot_partition` file.

Alternatively you can mount the systems partition (`/dev/mmcblk0p1`) and add `runinstaller` to the `recovery.cmdline` file. After a reboot, the Pi will start into recovery mode and allow you to use the existing API to change the Pi. If `no-webserver` is also added to the file, the recovery system will boot into a local-only mode, presenting a self-explanatory CLI menu, having similiar functionalities like the REST API.

## REST API
When the Pi boots into the recovery system, without `no-webserver`, it will display it's IP address and await conections. The API is shown below:

1. Send a payload containing the OS information via a POST command to http://<PI IP Address>/ and see progress on the Pi. After the process is finished, the Pi should automatically boot into the fresh installed OS. OS should have the layout as obtained by `http://downloads.raspberrypi.org/os_list_v3.json` (Plus-sign indicates a mandatory entry)
A list of the following entries is expected.
```
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
```
