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
   * TFT\_eSPI-User-Setup-odroid-go.patch: for ODROID GO display

## Requirements

   * Bodmer TFT\_eSPI lib
   * auxiliary caching httpd: C compiler, POSIX environment
   * optional (for mkfw): odroid-go-firmware

## Screenshot

![Screenshot](https://raw.githubusercontent.com/tx-odroid/PIMwatch/master/PIMwatch-screenshot.jpg)

