import os
import sys
import signal
from zipfile import ZipFile
import time
_alarm = "request timed out"

sig_time = 1

cmd="sh1.sh"

###########################################################

def alarm_handler(a , b):
    #signal.alarm(0)
    raise ValueError("The __year lookup type requires an integer argument")


###########################################################

# Okay, let's do this the hard way: create two unnamed
# pipes for stdout/err. Then, fork this process, and the
# child becomes the new command and shoves their output
# onto the pipes, while the parent waits for the child to die.

# Create two named pipes
child_out_r, child_out_w = os.pipe()
child_err_r, child_err_w = os.pipe()

# Let's fork a child that gets replaced with the test process - krf
child_pid = os.fork()

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

if (child_pid != 0):
    # We're the parent

    os.system("tty_rec/ttyrec %s %s" % (tcast_file, tcast_timing))

    #os.close(child_out_w) # the parent won't read the pipes
    #os.close(child_err_w)

    #

    # Now do the timing...
    #signal.signal(signal.SIGALRM, alarm_handler)

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
    """
    os.setpgid(child_pid, 0)
    signal.signal(signal.SIGTERM, alarm_handler)
    signal.alarm(0)
    signal.siginterrupt (signal.SIGTERM,False)

    signal.alarm(0) # clean up alarm
    """


else:
    # We're the child
    os.setpgid(0, 0) # now the child is it's group leader (?)
    cmd="/usr/local/bin/ecasound"
    args="ecasound -i jack,system -o %s" % tcast_sound
    #os.system (cmd)
    #args2="%s %s" % ( cmd, args)
    #os.system (args2)
    os.execl (cmd, *args.split(" "))
    """


    # Redirect error to our pipes
    os.close(child_out_r) # the child won't read the pipes
    os.close(child_err_r)
    os.dup2(child_out_w, sys.stdout.fileno())
    os.dup2(child_err_w, sys.stderr.fileno())

    # replace the process
    print "before sleep in child "
    child_out = os.fdopen ( child_err_w,'w')
    child_out.write ("from child")
    child_out.flush ()
    time.sleep (8)
    print "after sleep 8"
    #os.execl(cmd, cmd) #strange, but execl->execv needs a secondarg...
    # the child is no more... (assuming success)
    """
print "done"
zf = ZipFile("%s/tc.zip" % terminalcast_dir,"w")
zf.write(tcast_file, "tcast_data")
zf.write(tcast_timing, "timing.js")
zf.close ()
#print "Status = ", child_status
#print "Child stdout [",os.read(child_out_r,999),"]"
#print "Child stderr [",os.read(child_err_r,999),"]"

