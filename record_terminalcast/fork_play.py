
import os, sys
import time

print "I'm going to fork now - the child will write something to a pipe, and the parent will read it back"

r, w = os.pipe() # these are file descriptors, not file objects
child_out=""
pid = os.fork()
if pid:
    # we are the parent
    os.close(w) # use os.close() to close a file descriptor
    r = os.fdopen(r) # turn r into a file object
    print "parent: reading"
    #txt = r.readline ()
    time.sleep (1)
    print "I got tired of waiting so now parent is closing"
    #print "parent: got it; text =", txt
    os.kill (pid,9)
    #sys.exit (0)
    #os.waitpid(pid, 0) # make sure the child process gets cleaned up
else:
    # we are the child
    os.close(r)
    w = os.fdopen(w, 'w')
    child_out= "beginning to sleep 10 from child"
    #print "beginning to sleep 10 from child"
    os.system ("sleep 3")
    
    #print "child: writing"

print child_out
