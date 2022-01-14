#!/bin/bash
wireshark -Xlua_script:masterbusDissector.lua $*
