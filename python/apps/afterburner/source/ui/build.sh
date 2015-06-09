#!/usr/bin/env bash

echo 'Creating UI ...'
pyuic4 afterburner.ui > ui_afterburner.py
pyuic4 preferences.ui > ui_preferences.py
