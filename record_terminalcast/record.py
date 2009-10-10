#!/usr/bin/python
#Copyright Patrick Mullen 2009
import cPickle
import os
import posixpath
import signal
import subprocess
from zipfile import ZipFile
import optfunc
from  upload import upload_terminalcast

TC_DIR = "~/.terminalcast/"
LOCAL_TC_HOST = 'localhost:8000'
#TC_HOST = 'lispnyc.org:8003'
#TC_HOST = 'terminalcast.paddymullen.com'
TC_HOST = 'terminalcast.com'

class JackRecord(object):

    def __init__(self,tcast_sound):
        args = ("/usr/local/bin/ecasound  -i jack,system -o %s" % tcast_sound).split(" ")
        self.sub_proc = subprocess.Popen(args)
    def finish(self):
        self.sub_proc.send_signal(signal.SIGTERM)        

    
class Afrecord(object):

    def __init__(self, tcast_sound):
        args = ("/usr/bin/sound_recorders/afrecord -d LEI16 -f WAVE %s" % tcast_sound).split(" ")
        self.sub_proc = subprocess.Popen(args)
    def finish(self):
        self.sub_proc.send_signal(signal.SIGTERM)        

SOUND_RECORDING_CLASS = Afrecord

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
                print i,description_dict, "\n", "-"*80
            except EOFError, AttributeError:
                print "error"

def TCAST_RECORDING_FUNC(tcast_file, tcast_timing):
    os.system("/usr/bin/ttyrec %s %s" % (tcast_file, tcast_timing))

def post_process_sound(tcast_sound, tcast_mp3, tcast_ogg):
    os.system("lame -V 9 %s %s " % (tcast_sound, tcast_mp3))
    os.system("oggenc -q2 --resample 10000 %s -o  %s" % (tcast_sound, tcast_ogg))

def record(
    username='',    password='',    title='',
    description='',    tag_list='asdfss'    ):
    "Usage : prog  "
    for required in [username,password,title,description]:
        if required == '':
            print "missing option  try -h to list options "
            return


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

    sound_recorder = SOUND_RECORDING_CLASS(tcast_sound)
    TCAST_RECORDING_FUNC(tcast_file, tcast_timing)
    sound_recorder.finish()
    post_process_sound(tcast_sound, tcast_mp3, tcast_ogg)

    zf = ZipFile( tcast_zip ,'w')
    zf.write(tcast_file, "tcast_data")
    zf.write(tcast_timing, "timing.js")
    zf.write(tcast_ogg, "tcast_sound.ogg")
    zf.write(tcast_mp3, "tcast_sound.mp3")
    zf.close ()
    upload_terminalcast(tcast_zip, description_dict, username, password, host=TC_HOST)

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
