Copyright Patrick Mullen 2009

This is the recorder for terminalcast
getting this to run on other platform , hell even OS X will be tricky, primarily because of sound recording.


on OS X  this currently works

You must have developer tools installed 

sudo port install lame
sudo port install vorbis-tools
sudo python setup.py build install


I am now using a sound recorder based on CoreAudio so you don't need to install a bunch of external tools
