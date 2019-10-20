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

   * Arduino IDE
   * [Bodmer/TFT\_eSPI](https://github.com/Bodmer/TFT_eSPI)
   * for auxiliary caching httpd: C compiler, POSIX environment
   * optional (for mkfw): [OtherCrashOverride/odroid-go-firmware](https://github.com/OtherCrashOverride/odroid-go-firmware)

## Description of files

   * caching-httpd/: directory with the caching httpd
   * clean.sh: remove file which were generated by create-fw.sh
   * create-fw.sh: create firmware file
   * data/\*.vlw: font files, replace by your own
   * data/pimwatch.ppm; the logo rendered by the sketch
   * get-last-loadlog-lines.sh: wraper script to get values
   * PIMwatch-caching-httpd-dyndns-register.sh: DYNDNS registering
   * PIMwatch.ino: the sketch itself
   * PIMwatch.png: the logo shown in the firmware menu
   * TFT\_eSPI/TFT\_eSPI-User-Setup-odroid-go-display.patch: for ODROID GO display

## Installation

TBD - quick summary: Install/configure ODROID GO environment, install TFT\_eSPI in Arduino/libraries, patch User-Setup.h in it and build PIMwatch.ino with Arduino

