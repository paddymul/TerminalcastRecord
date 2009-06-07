import sys, curses, signal, time

def sigwinch_handler(n, frame):
    curses.endwin()
    a=curses.initscr()
    print "hello"
    sys.stderr.write ( a.getmaxyx ())
    cuses.flash ()
def main(stdscr):
    """just repeatedly redraw a long string to reveal the window boundaries"""
    while 1:
        #stdscr.insstr(0,0,"abcd"*40)

        time.sleep(1)

if __name__=='__main__':
    if len(sys.argv)==2 and sys.argv[1]=="1":
        signal.signal(signal.SIGWINCH, sigwinch_handler)
        curses.wrapper(main)
