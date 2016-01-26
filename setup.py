#!/usr/bin/python

import os
import sys
from distutils.core import setup, Extension


def main():
    if sys.version_info[0] != 2:
        sys.stderr.write('This backport is for Python 2.x only.\n')
        sys.exit(1)

    ext = Extension('_posixsubprocess', ['_posixsubprocess.c'],
                    depends=['_posixsubprocess_helpers.c'])
    if os.name == 'posix':
        ext_modules = [ext]
    else:
        ext_modules = []

    setup(
      name='subprocess32',
      version='3.2.7+scale1',
      description='A backport of the subprocess module from Python 3.2/3.3 for use on 2.x.',
      long_description="""
This is a backport of the subprocess standard library module from
Python 3.2 & 3.3 for use on Python 2.
It includes bugfixes and some new features.  On POSIX systems it is
guaranteed to be reliable when used in threaded applications.
It includes timeout support from Python 3.3 but otherwise matches
3.2's API.  It has not been tested on Windows.""",
      license='PSF license',

      maintainer='Gregory P. Smith',
      maintainer_email='greg@krypto.org',
      url='https://github.com/google/python-subprocess32',

      ext_modules=ext_modules,
      py_modules=['subprocess32'],

      classifiers=[
          'Intended Audience :: Developers',
          'Topic :: Software Development :: Libraries',
          'Development Status :: 5 - Production/Stable',
          'License :: OSI Approved :: Python Software Foundation License',
          'Operating System :: POSIX',
          'Operating System :: POSIX :: BSD',
          'Operating System :: POSIX :: Linux',
          'Operating System :: POSIX :: SunOS/Solaris',
          'Programming Language :: Python :: 2.4',
          'Programming Language :: Python :: 2.5',
          'Programming Language :: Python :: 2.6',
          'Programming Language :: Python :: 2.7',
          'Programming Language :: Python :: 2 :: Only',
          'Programming Language :: Python :: Implementation :: CPython',
      ],
    )


if __name__ == '__main__':
    main()
