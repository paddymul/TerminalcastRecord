#!/usr/bin/python
#Copyright Patrick Mullen 2009
import os
import sys
import signal
from zipfile import ZipFile
import time
import httplib, mimetypes, mimetools, urllib2, cookielib
import optfunc
import cPickle
import posixpath

TC_DIR = "~/.terminalcast/"
LOCAL_TC_HOST = 'localhost:8000'
#TC_HOST = 'lispnyc.org:8003'
#TC_HOST = 'terminalcast.paddymullen.com'
TC_HOST = 'terminalcast.com'

def get_empty_directory():
    terminalcast_dir = os.path.expanduser(TC_DIR)
    try:
        os.mkdir(terminalcast_dir)
    except:
        pass
    for i in range(1,1000):
        terminalcast_dir = os.path.expanduser("%s%d" % (TC_DIR, i))
        
        if posixpath.exists(terminalcast_dir):
            continue
        else:
            os.mkdir(terminalcast_dir)
            return terminalcast_dir
def ls():
    print "LS"
    for i in range(1,1000):
        terminalcast_dir = os.path.expanduser("%s%d" % (TC_DIR, i))
        if posixpath.exists(terminalcast_dir):
            tcast_desc = "%s/desc.pckl" % terminalcast_dir
            try:
                description_dict = cPickle.load(open(tcast_desc))
                print i
                print description_dict
                print "-"*80

            except EOFError, AttributeError:
                print "error"

class SoundRecorders(object):
    @staticmethod
    def jack(tcast_sound):
        cmd="/usr/local/bin/ecasound"
        args="ecasound -i jack,system -o %s" % tcast_sound
        os.execl (cmd, *args.split(" "))

    @staticmethod
    def mac_afrecord(tcast_sound):
        cmd="/usr/bin/sound_recorders/afrecord"
        args="afrecord -d LEI16 -f WAVE %s" % tcast_sound
        #os.execl (cmd, *args.split(" "))
        os.system("/usr/bin/sound_recorders/afrecord -d LEI16 -f WAVE %s" % tcast_sound)
        print " afrecord finished "
        
SOUND_RECORDING_FUNC = SoundRecorders.jack

def record(
    username='',    password='',    title='',
    description='',    tag_list='asdfss'    ):
    "Usage : prog  "
    for required in [username,password,title,description]:
        if required == '':
            print "missing option  try -h to list options "
            return
              
    # Okay, let's do this the hard way: create two unnamed
    # pipes for stdout/err. Then, fork this process, and the
    # child becomes the new command and shoves their output
    # onto the pipes, while the parent waits for the child to die.
              
    # Create two named pipes
    terminalcast_dir = get_empty_directory()
    tcast_file = "%s/tcast_data" % terminalcast_dir
    tcast_timing = "%s/timing.js" % terminalcast_dir
    tcast_sound = "%s/sound.wav" % terminalcast_dir
    tcast_mp3 = "%s/sound.mp3" % terminalcast_dir
    tcast_ogg = "%s/sound.ogg" % terminalcast_dir
    tcast_zip = "%s/tc.zip" % terminalcast_dir
    tcast_desc = "%s/desc.pckl" % terminalcast_dir
    description_dict = dict(
        title=title, description=description, tag_list=tag_list)
    cPickle.dump(description_dict, open(tcast_desc,"w"))

    child_out_r, child_out_w = os.pipe()
    child_err_r, child_err_w = os.pipe()
              
    # Let's fork a child that gets replaced with the test process - krf
    child_pid = os.fork()

    if (child_pid != 0):
        # We're the parent
              
        #this works because in setup.py we copy ttyrec to /usr/bin
        #bad form I know, I'm not quite sure of a better way
        os.system("ttyrec %s %s" % (tcast_file, tcast_timing))
        print "called os kill on %d " %  child_pid
        os.system("kill %d" % child_pid)
        print "called os kill on %d " %  child_pid

        os.kill (child_pid, signal.SIGTERM)
        print "called os kill on %d " %  child_pid
        
        try:  
            child_pid, child_status = os.waitpid(child_pid, 0); # wait forchild
        except OSError:
            print("OSError... did the executable time out?")
            child_status = -1 # this is the same as a timeout for now
        except ValueError:
            print("Timed out")
            # kill it!
            os.kill(child_pid, 9)
            child_status = -1 # timeout

        print "\n\n\n  otherprocess finished \n\n\n"
              
    else:     
        # We're the child
        os.setpgid(0, 0) # now the child is it's group leader (?)
        SOUND_RECORDING_FUNC(tcast_sound)
    print "done"
              
    os.system("lame -V 9 %s %s " % (tcast_sound, tcast_mp3))
    #os.system("oggenc2 -q2 --resample 10000 foo.wav -o fooq2-10k.ogg")
    os.system("oggenc -q2 --resample 10000 %s -o  %s" % (tcast_sound, tcast_ogg))

    zf = ZipFile( tcast_zip ,'w')
    zf.write(tcast_file, "tcast_data")
    zf.write(tcast_timing, "timing.js")
    zf.write(tcast_ogg, "tcast_sound.ogg")
    zf.write(tcast_mp3, "tcast_sound.mp3")
    zf.close ()
    upload_terminalcast(tcast_zip, description_dict, username, password, host=TC_HOST)

from  upload import upload_terminalcast
def upload(number='', username='', password='', host=TC_HOST):
    if not host == TC_HOST:
        host = LOCAL_TC_HOST
    for required in [number,username,password]:
        if required == '':
            print "missing option  try -h to list options "
            return
    number=int(number)
    terminalcast_dir = os.path.expanduser("%s%d/" % (TC_DIR, number))
    tcast_zip = "%s/tc.zip" % terminalcast_dir
    tcast_desc = "%s/desc.pckl" % terminalcast_dir
    description_dict = cPickle.load(open(tcast_desc))
    upload_terminalcast(tcast_zip, description_dict, username, password, host=host)

def my_main():
    optfunc.main([upload,ls,record])

if __name__ == '__main__':
    my_main()
    
    #optfunc.main([upload_saved_terminalcast,record])
    #optfunc.main([upload,ls,record])
    #optfunc.main(record)
