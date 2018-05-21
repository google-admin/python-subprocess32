#!/usr/bin/env python2

import os
import stat
import sys
from setuptools import setup, Extension, Command
from setuptools.command.build_ext import build_ext


class BuildConfigure(Command):
    description = 'Generate _posixsubprocess_config.h via ./configure.'

    def run(self):
        config_h = '_posixsubprocess_config.h'
        status = 'config.status'
        configure = './configure'
        # Here we implement time stamp checking logic like make, manually...
        if os.path.exists(config_h) and os.path.exists(status):
            status_mtime = os.stat(status)[stat.ST_MTIME]
            configure_mtime = os.stat(configure)[stat.ST_MTIME]
            if configure_mtime < status_mtime:
                print(' ' + config_h + ' is already up to date.')
                return
        configure_command = 'sh ' + configure
        if os.system(configure_command):
            raise RuntimeError(configure_command + ' failed.')

    # Abstract methods we don't need but are required to define.
    def initialize_options(self): pass
    def finalize_options(self): pass


class BuildExtensionAfterConfigure(build_ext):
    """Executes ./configure before the actual extension is built."""

    # See https://github.com/python/cpython/blob/2.7/Lib/distutils/cmd.py#L328
    sub_commands = [('build_configure', None)] + build_ext.sub_commands

    def run(self):
        for command in self.get_sub_commands():
            self.run_command(command)
        build_ext.run(self)


def main():
    ext_modules = []
    py_modules = []
    packages = []
    package_dir = {}
    cmdclass = {}
    if sys.version_info[0] == 2:  # PY2
        py_modules.append('subprocess32')
        if os.name == 'posix':
            ext = Extension('_posixsubprocess32', ['_posixsubprocess.c'],
                            depends=['_posixsubprocess_helpers.c',
                                     '_posixsubprocess_config.h'])
            ext_modules.append(ext)
            # Cause ./configure to be run before we build the extension.
            cmdclass['build_configure'] = BuildConfigure
            cmdclass['build_ext'] = BuildExtensionAfterConfigure
    else:  # PY3
        # Install a redirect that makes subprocess32 == subprocess on import.
        packages.append('subprocess32')
        package_dir['subprocess32'] = 'python3_redirect'
        sys.stderr.write('subprocess32 == subprocess on Python 3.\n')

    setup(
      name='subprocess32',
      version='3.5.1',
      description='A backport of the subprocess module from Python 3 for use on 2.x.',
      long_description="""\
This is a backport of the subprocess standard library module from
Python 3.2 - 3.5 for use on Python 2.

It includes bugfixes and some new features.  On POSIX systems it is
guaranteed to be reliable when used in threaded applications.
It includes timeout support from Python 3.3 and the run() API from 3.5
but otherwise matches 3.2's API.

It has not been tested by the author on Windows.

On Python 3, it merely redirects the subprocess32 name to subprocess.""",
      license='PSF license',

      maintainer='Gregory P. Smith',
      maintainer_email='greg@krypto.org',
      url='https://github.com/google/python-subprocess32',

      ext_modules=ext_modules,
      py_modules=py_modules,
      packages=packages,
      package_dir=package_dir,
      cmdclass=cmdclass,

      # We don't actually "support" 3.3+, we just allow installation there as
      # we install a stub redirecting to the standard library subprocess module
      # under the subprocess32 name.
      python_requires='>=2.6, !=3.0.*, !=3.1.*, !=3.2.*, <4',

      classifiers=[
          'Intended Audience :: Developers',
          'Topic :: Software Development :: Libraries',
          'Development Status :: 5 - Production/Stable',
          'License :: OSI Approved :: Python Software Foundation License',
          'Operating System :: MacOS',
          'Operating System :: MacOS :: MacOS X',
          'Operating System :: POSIX',
          'Operating System :: POSIX :: BSD',
          'Operating System :: POSIX :: Linux',
          'Operating System :: POSIX :: SunOS/Solaris',
          'Programming Language :: Python :: 2.6',
          'Programming Language :: Python :: 2.7',
          'Programming Language :: Python :: 2 :: Only',
          'Programming Language :: Python :: Implementation :: CPython',
      ],
    )


if __name__ == '__main__':
    main()
