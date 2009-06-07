
This is the recorder for terminalcast
getting this to run on other platform , hell even OS X will be tricky, primarily because of sound recording.


on OS X  this currently works


get JACK
http://www.jackosx.com


sudo port install lame
sudo port install ecasound
sudo port install vorbis-tools
wget http://ecasound.seul.org/download/ecasound-2.6.0.tar.gz
tar xzvf ecasound-2.6.0.tar.gz 
cd ecasound-2.6.0
./configure
make
sudo make install
