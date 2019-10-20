#!/bin/bash

ssh s37 'date +\%Y-\%m-\%dT\%H:\%M:\%S;cat /home/demo/tcpu-load.log 2>&1' 2>&1
exit 0
