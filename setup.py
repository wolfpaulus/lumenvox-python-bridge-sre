#!/usr/bin/python
from distutils.core import setup, Extension

module2 = Extension('lvasrmodule',
sources = ['lvasrmodule.c'],
library_dirs=['/usr/lib64'], 
libraries=['lv_lvspeechport','python2.6'],
include_dirs=['/usr/include'])

setup(name='lvasrmodule', 
version='1.0', 
description = 'Python Interface for LumenVox ASR',
ext_modules=[module2])

