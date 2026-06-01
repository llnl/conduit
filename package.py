#!/bin/env python
# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.


###############################################################################
#
# file: package.py
#
###############################################################################

import subprocess
import sys
import datetime
import os

from os.path import join as pjoin

def create_package(output_file,version):
    scripts_dir = pjoin(os.path.abspath(os.path.split(__file__)[0]),"scripts")
    pkg_script = pjoin(scripts_dir,"git_archive_all.py");
    repo_name = os.path.basename(os.path.dirname(os.path.abspath(__file__)))
    if output_file is None:
         suffix = "tar"
         t = datetime.datetime.now()
         output_file = "%s.%04d.%02d.%02d.%s" % (repo_name,t.year,t.month,t.day,suffix)
    cmd = "python3 " + pkg_script + " --prefix=conduit"
    if not version is None:
        cmd += "-" + version
    cmd +=  " " + output_file
    print("[exe: %s]" % cmd)
    subprocess.call(cmd,shell=True)
    
if __name__ == "__main__":
    ofile   = None
    version = None
    if len(sys.argv) > 1:
        ofile = sys.argv[1]
    if len(sys.argv) > 2:
        version = sys.argv[2]
    create_package(ofile,version)


