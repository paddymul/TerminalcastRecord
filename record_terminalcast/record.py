!#!/usr/bin/python

import os, os.path
import subprocess

terminalcast_dir = os.path.expanduser("~/.terminalcast")
try:
    os.mkdir(terminalcast_dir)
except:
    pass

tcast_file = "%s/tcast_data" % terminalcast_dir
tcast_timing = "%s/timing.js" % terminalcast_dir
tcast_sound = "%s/sound.wav" % terminalcast_dir
tcast_mp3 = "%s/sound.mp3" % terminalcast_dir
tcast_ogg = "%s/sound.ogg" % terminalcast_dir

#a=subprocess.Popen(("ecasound -i jack,system -o %s" % tcast_sound).split(" "))
os.system("tty_rec/ttyrec %s %s" % (tcast_file, tcast_timing))

#os.system("kill %i"%  a.pid)
#a,b,c = os.popen2("lame -V 9 %s %s >/dev/null" % (tcast_sound, tcast_mp3))
