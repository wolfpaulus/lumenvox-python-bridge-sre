# lumenvox-python-bridge-sre
Python C bridge, to access LumenVox Speech Recognition Server from Python

The LumenVox TTS implementation provides command line tools to access the engine.
A real test performed by the /usr/bin/SimpleTTSClient tool, producing a PCM encoded wave file, 
can be run as well. Running this as the regular ec2-user allows for an more easy transfer back 
to client computer ...


While the SimpleTTSClient command line tool is helpful to verify the successful installation and 
configuration of the TTS Engine, it’s not the ideal component to be integrated into a typical TTS 
workflow. Moreover, PCM wave is not the ideal audio file format to be send to
and to be consumed by a Web or mobile client.
The next steps show a straight forward implementation of a bridge module implemented in C that 
converts the PCM samples into an MP3 file and also provides and easy to program interface to be 
imported into Python programs, like the Tornado Web server.

Since the C code needs to be compiled and then imported as a module into the Python library system, 
C-Compiler, Make, Git, and python-devel.x86_64, all need to be installed first, like so:
To install the LAME2 MP3 encoder libraries via YUM, an additional repository is required, i.e. 
RPMforge3, like so.
Verify that the package has been downloaded, then import the package, and finally install the lame and 
lame-devel packages:
￼sudo su
yum install gcc gcc-c++ autoconf automake git python-devel.x86_64
wget http://packages.sw.be/rpmforge-release/rpmforge-
release-0.5.2-2.el6.rf.x86_64.rpm
rpm --import http://apt.sw.be/RPM-GPG-KEY.dag.txt
￼￼
rpm -K rpmforge-release-0.5.2-2.el6.rf.*.rpm
rpm -i rpmforge-release-0.5.2-2.el6.rf.*.rpm
yum install lame lame-devel

After cloning the code from this repository, we can make the bridge module and install it, 
to be used by python programs:

cd lumenvox/python-bridge
make
...
Compiling lvmodule.c...
gcc -std=c99 -std=gnu99  -I/usr/include/python2.6 -MMD -MP -I/usr/include  -o
lvmodule.o -c lvmodule.c
Linking ...
g++ -s -Wl,-rpath=/usr/lib64,-Bsymbolic  -L/usr/lib64 -o lvmodule lvmodule.o
-llv_lvspeechport -lmp3lame -lpython2.6


python setup.py install
exit

./test.py is a very simple python test script that sends a text string to the TTS engine 
and streams out the resulting mp3 encoded voice sound.

#!/usr/bin/python
from array import array
import sys
import lv
mp3.tofile(sys.stdout)

