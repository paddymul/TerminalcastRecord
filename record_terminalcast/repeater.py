#!/usr/bin/python
import sys

sys.stderr.write('repeater.py: starting\n')

while True:
   next_line = sys.stdin.readline()
   if not next_line:
       break
   sys.stdout.write(next_line)
   sys.stdout.flush()

sys.stderr.write('repeater.py: exiting\n')
