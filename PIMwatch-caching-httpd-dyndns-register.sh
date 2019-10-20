#!/bin/bash

export PATH=$PATH:/sbin:/opt/local/bin
lynx -dump http://dyndns.example.com/register?$({ ifconfig en7 2>/dev/null; ifconfig en0 2>/dev/null; }|grep 'inet '|awk '{print$2}')
exit 0
