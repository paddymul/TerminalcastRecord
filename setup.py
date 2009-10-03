"""
Copyright Patrick Mullen 2009
"""
__author__="Paddy Mullen"
__date__ ="$May 14/2009"

from setuptools import setup, find_packages

import pdb
#pdb.set_trace()
import os
os.system("cd record_terminalcast/tty_rec/ttyrec ; gcc ttyrec ; cd ../../")


os.system("cp record_terminalcast/tty_rec/ttyrec /usr/bin")
os.system("cd  record_terminalcast/sound_recorders/mac_afrecord/AudioFileTools; xcodebuild")
os.system("cp  record_terminalcast/sound_recorders/mac_afrecord/AudioFileTools/build/Debug-Tiger+/afrecord/afrecord record_terminalcast/sound_recorders/bin/")
os.system("mkdir  /usr/bin/sound_recorders")
os.system("cp record_terminalcast/sound_recorders/bin/* /usr/bin/sound_recorders")
setup(name='terminalcast_record',
      version='1.0',
      description='record a terminal session',
      author='Paddy Mullen',
      author_email='paddy@chartwidget.com',
      url='http://demo.chartwidget.com',
      entry_points = {
          'console_scripts':[
              'rec_tcast = record_terminalcast.record:my_main']},
      #data_files=["record_terminalcast/tty_rec/ttyrec"],
      #packages=['record_terminalcast','record_terminalcast.tty_rec']
      packages=['record_terminalcast',
                'record_terminalcast.tty_rec',
                'record_terminalcast.sound_recorders']
     )
