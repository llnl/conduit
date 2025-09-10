##############################################################################
# pip install recipe for conduit
##############################################################################
#
# Recipe that uses cmake to build conduit for use in python.
#
# Optional Conduit features are enabled via env vars:
#
#  HDF5_DIR  {path to hdf5 install}
#
# [Caveats]
#  - Assumes a suitable cmake (3.9 + ) is in your path
#  - Does not build a relocatable wheel
#  - Windows untested
#
# [Example Usage]
#  pip install .
#  pip install . --user
#  pip install  -v . --user
#  env HDF5_DIR={path/to/hdf5/install} pip install -v . --user
#
#  # for those with certificate woes
#  pip install  --trusted-host pypi.org --trusted-host files.pythonhosted.org  -v . --user
#  env HDF5_DIR={path/to/hdf5/install} pip install  --trusted-host pypi.org --trusted-host files.pythonhosted.org  -v . --user
#
#
# [Ack]
#  https://github.com/openPMD/openPMD-api/blob/dev/setup.py
#
#  Provided helpful pointers to create this setup script.
##############################################################################

import os
import re
import sys
import platform
import subprocess

from os.path import join as pjoin

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

##############################################################################
# NOTE 2024/02/16
# with distutils gone, there is no built-in equivalent to:
# from distutils.version import LooseVersion
#
# The packaging module helps, but it's *not* built in to
# standard python installs and we don't want to add another
# dep for a one line check for cmake.
#
# I removed the cmake version check.
#
##############################################################################

CONDUIT_VERSION = '0.9.5'

class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)

class CMakeBuild(build_ext):
    def run(self):
        # make sure cmake exist with min version
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following " +
                               "extensions: " +
                               ", ".join(e.name for e in self.extensions))

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir =self.get_ext_fullpath(ext.name)
        extdir = os.path.abspath(os.path.dirname(extdir))

        # when off,  will build the main conduit libs as shared
        # and they will be linked into the python modules dynamic libs
        build_shared_libs = "OFF"

        # required for auto-detection of auxiliary "native" libs
        if not extdir.endswith(os.path.sep):
            extdir += os.path.sep
        cmake_args = ['-DPYTHON_MODULE_INSTALL_PREFIX=' + pjoin(extdir),
                      '-DCMAKE_INSTALL_PREFIX=' + pjoin(ext.sourcedir,"_install"),
                      '-DPYTHON_EXECUTABLE=' + sys.executable,
                      '-DENABLE_PYTHON:BOOL=ON',
                      '-DBUILD_SHARED_LIBS:BOOL=' + build_shared_libs,
                      '-DENABLE_TESTS:BOOL=OFF',
                      '-DENABLE_DOCS:BOOL=OFF']

        if HDF5_DIR != "IGNORE":
            cmake_args.append('-DHDF5_DIR=' + HDF5_DIR)

        if ENABLE_MPI != "IGNORE":
            cmake_args.append('-DENABLE_MPI=' + ENABLE_MPI)

        # cmake initial cache file support
        if HOST_CONFIG != "IGNORE":
            cmake_args.extend(['-C',HOST_CONFIG])

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        # TODO: Windows untested
        if platform.system() == "Windows":
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(
            env.get('CXXFLAGS', ''),
            self.distribution.get_version())
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        print(cmake_args)
        subprocess.check_call(['cmake',
                               pjoin(ext.sourcedir,"src")] + cmake_args,
                               cwd=self.build_temp,
                               env=env)

        subprocess.check_call(['cmake', '--build', '.', '--target','install'] + build_args,
                               cwd=self.build_temp,
                               env=env)

#
# pass options via env vars
#
HOST_CONFIG = os.environ.get('HOST_CONFIG', 'IGNORE')
HDF5_DIR    = os.environ.get('HDF5_DIR', 'IGNORE')
ENABLE_MPI  = os.environ.get('ENABLE_MPI', 'IGNORE')

# keyword reference:
# https://packaging.python.org/guides/distributing-packages-using-setuptools
setup(
    name='conduit',
    version=CONDUIT_VERSION,
    author='Cyrus Harrison',
    author_email='cyrush@llnl.gov',
    maintainer='Cyrus Harrison',
    maintainer_email='cyrush@llnl.gov',
    description='Simplified Data Exchange for HPC Simulations '
                '(Python, C++, C, and Fortran)',
    keywords=('yaml json cpp fortran hpc hdf5 scientific-computing'
              ' data-management llnl radiuss'),
    url='https://github.com/llnl/conduit',
    project_urls={
        'Documentation': 'https://llnl-conduit.readthedocs.io/',
        'Source': 'https://github.com/llnl/conduit',
        'Tracker': 'https://github.com/LLNL/conduit/issues',
    },
    ext_modules=[CMakeExtension('conduit_cxx')],
    cmdclass=dict(build_ext=CMakeBuild),
    zip_safe=False,
    python_requires='>=3.6, <3.11')

