import os
import sys

# Localization.
if getattr(sys, 'frozen', False):
    APP_DIR = os.path.dirname(dirname(sys.executable))
else:
    APP_DIR     = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
LOCALDIR      = os.path.join(APP_DIR, 'locale')

DOMAIN = 'samuel'
