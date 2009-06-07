
import sys
import time
import signal
import pdb
import time
import os

from ctypes import CDLL, c_char_p
getenv = CDLL("libc.dylib").getenv
getenv.restype = c_char_p
print getenv("COLUMNS")

def winch_handle (a,b):
    #pdb.set_trace ()

    #getenv = CDLL("libc.so.6").getenv

    import ctypes
    reload (ctypes)
    import ctypes
    getenv = ctypes.CDLL("libc.dylib").getenv
    getenv.restype = ctypes.c_char_p

    print getenv("COLUMNS")

    #import os
    #reload(os)
    #import os
    #cmd="echo $COLUMNS >> /Users/patrickmullen/foo.txt"
    #os.execl(cmd,cmd)
    #print os.environ['COLUMNS']#,  os.environ['LINES']
    #print os.getenv ('COLUMNS'),  os.getenv ('LINES')
    #print os.environ.keys ()
    

child_out_r, child_out_w = os.pipe()
child_err_r, child_err_w = os.pipe()

signal.signal (signal.SIGWINCH,winch_handle)
outer_std_err, outer_std_in, outer_std_out =  sys.stderr, sys.stdin, sys.stdin

while True:
    time.sleep (30)

sys.exit ()

child_pid = os.fork()
if (child_pid != 0):
    # We're the parent

    os.close(child_out_w) # the parent won't read the pipes
    os.close(child_err_w)

    os.setpgid(child_pid, 0)

    # Now do the timing...
    #signal.signal(signal.SIGALRM, alarm_handler)

    while True:
        try:
            os.waitpid(child_pid, 0) # make sure the child process gets cleaned up
        except OSError:
            pass#print "OSERROR"
    #signal.alarm(sig_time)
else:
    #signal.signal (signal.SIGWINCH,winch_handle)
    sys.stderr, sys.stdin, sys.stdin = outer_std_err, outer_std_in, outer_std_out 
    os.execl("/bin/zsh","/bin/zsh")
#time.sleep (50)
