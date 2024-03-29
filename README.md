# PIMwatch
Example ODROID GO sketch to show some server monitoring values


## Goal/Purpose

This application is useful for me but it needs to be heavily customized for
different environments to work.
I'm primarily publishing it for educational purposes.

## What does it?

The ODROID GO application gets some "monitoring values" from a caching http
server. The server itself gets the values from the monitored environment via
a wrapper script. A second script is called by the httpd on startup to do a
dyndns registering.

For each monitored server the application gets 3 values: load value and for
the task/thread which consumes most cpu time the cpu% value and its name.
The required format is shown by example in the source of the httpd.

The caching server expects new values every 60 seconds and requests them
every minute by invoking the wrapper script. The ODROID GO application
checks the caching server every two seconds.

## Screenshot

![Screenshot](https://raw.githubusercontent.com/tx-odroid/PIMwatch/master/PIMwatch-screenshot.jpg)

_(The logo with the "mirrored 'y'" as shown here in the screenshot is "borrowed" partially from SAP Hybris. The logos in this repository are different.)_

## Features

   * several WiFi configs can be stored in a file PIMWIFI.TXT on SD card, PIMwatch will try all until the first connecting one
   * support for "DynDNS" in a configurable way to easily connect to the auxiliary HTTP server
   * auxiliary HTTP server caches recent values, is lightweight and can be accessed with "higher" frequency (e.g. interval of one second or less)
   * PPM color bitmap support (many other bitmap solutions don't support color or are not "lightweight")

## Requirements

   * Arduino IDE with ODROID GO development environment
   * Library [Bodmer/TFT\_eSPI](https://github.com/Bodmer/TFT_eSPI)
   * for building and running auxiliary caching httpd: C compiler, POSIX environment
   * optional (for mkfw): [OtherCrashOverride/odroid-go-firmware](https://github.com/OtherCrashOverride/odroid-go-firmware)

## Installation

As a requirement you need the Arduino IDE and the ODROID GO development environment installed like described on
[Getting started with Arduino](https://wiki.odroid.com/odroid_go/arduino/01_arduino_setup).

An alternative and script based installation method for Linux and macOS is provided in
[tx-odroid/odroid-go-install](https://github.com/tx-odroid/odroid-go-install).

### Installation of PIMwatch and TFT Lib

1. Step into your Arduino dir (e.g. `cd ~/Arduino`).
2. Clone PIMwatch: `git clone https://github.com/tx-odroid/PIMwatch.git`
3. Go to libraries dir and get TFT lib: `cd libraries; git clone https://github.com/Bodmer/TFT_eSPI.git`
4. Step into this dir and patch it for ODROID GO display: `cd TFT_eSPI; patch User_Setup.h <../../PIMwatch/TFT_eSPI/TFT_eSPI-User-Setup-odroid-go-display.patch`

### Fonts

The `.vlw` files in data directory were created with
[Processing IDE](https://processing.org/).

### Configure PIMwatch

Open PIMwatch.ino in Arduino and check/modify these variables to match your environment:

   * UseDynDns
   * DynDnsServer
   * AuxHttpdServer
   * AuxHttpdPort (need to match the one in PIMwatch-caching-httpd.c)

Create a file `PIMWIFI.TXT` on the root level of your ODROID GO SD card and add one or more WiFi configs to it. Each config consists of 2 lines: SSID and password.

If you don't use DynDNS then you also need to specify a static URL under which your auxiliary HTTP server will later be accessible.

### Auxiliary HTTP server

#### Edit Scripts

Edit `get-last-loadlog-lines.sh` to match your needs with e.g. an ssh command to get the values from the monitored environment.

The second script `PIMwatch-caching-httpd-dyndns-register.sh` needs to be adjusted for your DynDNS setup. If you don't use DynDNS then simply do an "exit 0" in the script.

#### Build server

Edit PIMwatch-caching-httpd.c and modify the script paths `COMMAND_DYNDNS_REGISTER` and `COMMAND_LOADLOG_LINE`.

Then build the server:

    gcc -O2 -o PIMwatch-caching-httpd PIMwatch-caching-httpd.c || gcc -O2 -o PIMwatch-caching-httpd PIMwatch-caching-httpd.c -lpthread

Start it via `./PIMwatch-caching-httpd`.

### Build PIMwatch

Compile the sketch in Arduino IDE by hitting Ctrl-R (or ⌘-R on macOS).

After that you have two ways of copying the application to the ODROID GO:

1. Flash it via USB cable (attention: the data directory also needs to be uploaded).
2. Create a firmware which includes logos and data dir and store it on SD card.

I recommend the creation of the firmware file. Run `./create-fw.sh` to do this and after that store PIMwatch.fw on SD card in `odroid/firmware/` folder. When turning on ODROID GO hold `B` button pressed until firmware menu appears. Then select PIMwatch and flash it. After flashing switch ODROID GO off and on.

In case of the error that `mkfw` was not yet compiled (`mkfw: No such file or directory`) please go to `Arduino/odroid-go-firmware/tools/mkfw` and run `make`. Then run `./create-fw.sh` again.

## Description of files

   * caching-httpd/: directory with the caching httpd
   * clean.sh: remove files which were generated by create-fw.sh
   * create-fw.sh: create firmware file
   * data/\*.vlw: font files
   * data/pimwatch.ppm; the logo rendered by the sketch
   * get-last-loadlog-lines.sh: wraper script to get values
   * PIMwatch-caching-httpd-dyndns-register.sh: DYNDNS registering
   * PIMwatch.ino: the sketch itself
   * PIMwatch.png: the logo shown in the firmware menu
   * TFT\_eSPI/TFT\_eSPI-User-Setup-odroid-go-display.patch: patch for ODROID GO display

