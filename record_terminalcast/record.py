#!/usr/bin/python
import os
import sys
import signal
from zipfile import ZipFile
import time
import httplib, mimetypes, mimetools, urllib2, cookielib
import optfunc


cj = cookielib.CookieJar()
opener = urllib2.build_opener(urllib2.HTTPCookieProcessor(cj))
urllib2.install_opener(opener)

TC_HOST = 'localhost:8000'


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
    
def record(
    username='',
    password='',
    title='',
    description='',
    tag_list=''
    ):
    "Usage : prog  "
    for required in [username,password,title,description, tag_list]:
        if required == '':
            print "missing option  try -h to list options "
            return

    sig_time = 1
              
    # Okay, let's do this the hard way: create two unnamed
    # pipes for stdout/err. Then, fork this process, and the
    # child becomes the new command and shoves their output
    # onto the pipes, while the parent waits for the child to die.
              
    # Create two named pipes
              
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
    tcast_zip = "%s/tc.zip" % terminalcast_dir
    for f in [tcast_file, tcast_timing, tcast_sound, tcast_ogg, tcast_mp3]:
        print f
        try:  
            os.remove(f)
        except:
            print "file probably didn't exist"

    child_out_r, child_out_w = os.pipe()
    child_err_r, child_err_w = os.pipe()
              
    # Let's fork a child that gets replaced with the test process - krf
    child_pid = os.fork()

    if (child_pid != 0):
        # We're the parent
              
        os.system("tty_rec/ttyrec %s %s" % (tcast_file, tcast_timing))
              
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
              
    a=post_multipart(
        TC_HOST,
        '/terminalcast/add_login/',
        [('username',username),
         ('password',password),
         ('title',title),
         ('description',description),
         ('tag_list',tag_list)],
        [(    
            "zip_file",
            tcast_zip,
            open(tcast_zip).read())])

def dispatcher(
    upload=False,
    record=True):
    if upload:
        optfunc.run(record)
    else:
        print "this is where you would record a terminalcast"
if __name__ == '__main__':
    #optfunc.run(record)b
    optfunc.main([dispatcher,record])
