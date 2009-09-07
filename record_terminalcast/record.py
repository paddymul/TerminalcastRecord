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

cj = cookielib.CookieJar()
opener = urllib2.build_opener(urllib2.HTTPCookieProcessor(cj))
urllib2.install_opener(opener)

TC_DIR = "~/.terminalcast/"
LOCAL_TC_HOST = 'localhost:8000'
#TC_HOST = 'lispnyc.org:8003'
TC_HOST = 'terminalcast.paddymullen.com'


def post_multipart(host, selector, fields, files):
    """
    Post fields and files to an http host as multipart/form-data.
    fields is a sequence of (name, value) elements for regular form fields.
    files is a sequence of (name, filename, value) elements for data to be uploaded as files
    Return the server's response page.
    """
    content_type, body = encode_multipart_formdata(fields, files)
    headers = {'Content-Type': content_type,
               'Content-Length': str(len(body))}
    r = urllib2.Request("http://%s%s" % (host, selector), body, headers)
    return urllib2.urlopen(r).read()

def encode_multipart_formdata(fields, files):
    """
    fields is a sequence of (name, value) elements for regular form fields.
    files is a sequence of (name, filename, value) elements for data to be uploaded as files
    Return (content_type, body) ready for httplib.HTTP instance
    """
    BOUNDARY = mimetools.choose_boundary()
    CRLF = '\r\n'
    L = []
    for (key, value) in fields:
        L.append('--' + BOUNDARY)
        L.append('Content-Disposition: form-data; name="%s"' % key)
        L.append('')
        L.append(value)
    for (key, filename, value) in files:
        L.append('--' + BOUNDARY)
        L.append('Content-Disposition: form-data; name="%s"; filename="%s"' % (key, filename))
        L.append('Content-Type: %s' % get_content_type(filename))
        L.append('')
        L.append(value)
    L.append('--' + BOUNDARY + '--')
    L.append('')
    body = CRLF.join(L)
    content_type = 'multipart/form-data; boundary=%s' % BOUNDARY
    return content_type, body

def get_content_type(filename):
    return mimetypes.guess_type(filename)[0] or 'application/octet-stream'

class TerminalCast(object):
    def __init__(self, opt_dict={}, zip_file=False):
        pass
    
    def dele(self):
        
        for f in [tcast_file, tcast_timing, tcast_sound, tcast_ogg, tcast_mp3]:
            print f
            try:  
                os.remove(f)
            except:
                print "file probably didn't exist"


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
    import pdb
    print __file__
    #os.system("%s/tty_rec/ttyrec " % __file__)
    #pdb.set_trace()
        
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

def record(
    username='',
    password='',
    title='',
    description='',
    tag_list='asdfss'
    ):
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
              
        #os.system("tty_rec/ttyrec %s %s" % (tcast_file, tcast_timing))
        os.system("ttyrec %s %s" % (tcast_file, tcast_timing))
              
        os.kill (child_pid, signal.SIGTERM)
        print "called os kill"
              
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
        cmd="/usr/local/bin/ecasound"
        args="ecasound -i jack,system -o %s" % tcast_sound
        os.execl (cmd, *args.split(" "))
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
    upload_terminalcast(tcast_zip, description_dict, username, password)


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



    
def upload_terminalcast(tcast_zip, tcast_desc, username,password, host=TC_HOST):
    print "host", host
    a=post_multipart(
        host,
        '/terminalcast/add_login/',
        [('username',username),
         ('password',password),
         ('title',tcast_desc['title']),
         ('description',tcast_desc['description']),
         ('tag_list',tcast_desc['tag_list'])],
        [(    
            "zip_file",
            tcast_zip,
            open(tcast_zip).read())])
def my_main():
    optfunc.main([upload,ls,record])
if __name__ == '__main__':
    my_main()
    
    #optfunc.main([upload_saved_terminalcast,record])
    #optfunc.main([upload,ls,record])
    #optfunc.main(record)
