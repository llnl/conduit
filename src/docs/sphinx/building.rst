.. # Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
.. # Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
.. # other details. No copyright assignment is required to contribute to Conduit.

.. _building:

=================
Building
=================

This page provides details on several ways to build Conduit from source.

For the shortest path from zero to Conduit, see :doc:`quick_start`.

If you are building features that depend on third party libraries we recommend using :ref:`uberenv <building_with_uberenv>` which leverages Spack or :ref:`Spack directly<building_with_spack>`.
We also provide info about :ref:`building for known HPC clusters using uberenv <building_known_hpc>`.
and a :ref:`Docker example <building_with_docker>` that leverages Spack.



Obtain the Conduit source
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Clone the Conduit repo from Github:

.. code:: bash

    git clone --recursive https://github.com/llnl/conduit.git


``--recursive`` is necessary because we are using a git submodule to pull in BLT (https://github.com/llnl/blt).
If you cloned without ``--recursive``, you can checkout this submodule using:

.. code:: bash

    cd conduit
    git submodule init
    git submodule update


Configure a build
~~~~~~~~~~~~~~~~~~~~

Conduit uses CMake for its build system. These instructions assume ``cmake`` is in your path.
We recommend CMake 3.21 or newer, for more details see :ref:`Supported CMake Versions <supported_cmake>`.

``config-build.sh`` is a simple wrapper for the cmake call to configure conduit.
This creates a new out-of-source build directory ``build-debug`` and a directory for the install ``install-debug``.
It optionally includes a ``host-config.cmake`` file with detailed configuration options.


.. code:: bash

    cd conduit
    ./config-build.sh


Build, test, and install Conduit:

.. code:: bash

    cd build-debug
    make -j 8
    make test
    make install



Build Options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The core Conduit library has no dependencies outside of the repo, however Conduit provides optional support for I/O and Communication (MPI) features that require externally built third party libraries.

Conduit's build system supports the following CMake options:

Main CMake Options
^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1

   * - Option
     - Description
     - Default

   * - ``BUILD_SHARED_LIBS``
     - Controls if shared (ON) or static (OFF) libraries are built.
     - *(default = ON)*

   * - ``ENABLE_FORTRAN``
     - Controls if Fortran components of Conduit are built.
     - *(default = ON)*

   * - ``ENABLE_PYTHON``
     - Controls if the Conduit Python module and related tests are built.
     - *(default = OFF)*

   * - ``ENABLE_MPI``
     - Controls if Conduit MPI features are built.
     - *(default = ON)*

   * - ``ENABLE_OPENMP``
     - Controls if Conduit OpenMP features are built.
     - *(default = OFF)*

   * - ``ENABLE_TESTS``
     - Controls if unit tests are built.
     - *(default = ON)*

   * - ``ENABLE_EXAMPLES``
     - Controls if Conduit examples are built.
     - *(default = ON)*

   * - ``ENABLE_UTILS``
     - Controls if Conduit utilities are built.
     - *(default = ON)*

   * - ``ENABLE_DOCS``
     - Controls if the Conduit documentation is built (when sphinx is available).
     - *(default = ON)*

   * - ``ENABLE_COVERAGE``
     - Controls if code coverage compiler flags are used to build Conduit.
     - *(default = OFF)*

   * - ``CONDUIT_ENABLE_TESTS``
     - Extra control for if Conduit unit tests are built. Useful for in cases where Conduit is pulled into a larger CMake project
     - *(default = ON)*


CMake Options for Third-party Library Paths
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1

   * - Name
     - Description

   * - ``HDF5_DIR``
     - Path to a HDF5 install (optional). Controls if HDF5 I/O support is built into *conduit_relay*.

   * - ``ZLIB_DIR``
     - Path to a Zlib install (optional). (Needed for HDF5 support)

   * - ``SILO_DIR``
     - Path to a Silo install (optional). Controls if Silo I/O support is built into *conduit_relay*. Requires HDF5.

   * - ``H5ZZFP_DIR``
     - Path to a H5ZZFP install (optional). Controls if HDF5 ZFP support is built into *conduit_relay*.

   * - ``ZFP_DIR``
     - Path to a ZFP install (optional). Controls if ZFP support is built into *conduit_relay*.

   * - ``PARMETIS_DIR``
     - Path to a Parmetis install (optional). Controls if Parmetis support for *conduit_blueprint* partitioning features is built.

   * - ``CALIPER_DIR``
     - Path to a Caliiper install (optional). Controls if Caliper performance annotation support.

   * - ``ADIAK_DIR``
     - Path to a Adiak install (optional). (Caliper support depends on Adiak)

   * - ``BLT_SOURCE_DIR``
     - Path to a BLT install (default = ``blt``)

Installation Path Options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Conduit's build system provides an **install** target that installs the Conduit libraires, headers, python modules, and documentation. These CMake options allow you to control install destination paths:

.. list-table::
   :header-rows: 1

   * - Option
     - Description

   * - ``CMAKE_INSTALL_PREFIX``
     - Standard CMake install path option (optional).

   * - ``PYTHON_MODULE_INSTALL_PREFIX``
     - Path to install Python modules into (optional).


Additional Build Notes
^^^^^^^^^^^^^^^^^^^^^^

* **Python** - The Conduit Python module builds for both Python 2 and Python 3. To select a specific Python, set the CMake variable PYTHON_EXECUTABLE to path of the desired python binary. When ``PYTHON_MODULE_INSTALL_PREFIX`` is set and ``ENABLE_PYTHON=ON``, Conduit's Python modules will be installed to ``${PYTHON_MODULE_INSTALL_PREFIX}`` directory instead of ``${CMAKE_INSTALL_PREFIX}/python-modules``.

* **MPI** - We use CMake's standard FindMPI logic. To select a specific MPI set the CMake variables ``MPI_C_COMPILER`` and ``MPI_CXX_COMPILER``, or the other FindMPI options for MPI include paths and MPI libraries. To run the mpi unit tests, you may also need change the CMake variables ``MPIEXEC_EXECUTABLE`` and ``MPIEXEC_NUMPROC_FLAG``, so you can use a different launcher, such as srun and set number of MPI tasks used.

* **BLT** - Conduit uses BLT (https://github.com/llnl/blt) as the foundation of its CMake-based build system. It is included as a submodule in Conduit's git repo, and also available in our release tarballs. The ``BLT_SOURCE_DIR`` CMake option defaults to ``src/blt``, where we expect the blt submodule. The most compelling reason to override is to share a single instance of BLT across multiple projects.


Host Config Files
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To handle build options, third party library paths, etc we rely on CMake's initial-cache file mechanism.


.. code:: bash

    cmake -C config_file.cmake


We call these initial-cache files *host-config* files, since we typically create a file for each platform or specific hosts if necessary.


These files use standard CMake commands. To properly seed the cache, CMake *set* commands need to specify ``CACHE`` as follows:

.. code:: cmake

    set(CMAKE_VARIABLE_NAME {VALUE} CACHE PATH "")



.. _building_with_uberenv:

Building Conduit and Third Party Dependencies
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
We use **Spack** (http://software.llnl.gov/spack) to help build Conduit's third party dependencies on OSX and Linux. Conduit builds on Windows as well, but there is no automated process to build dependencies necessary to support Conduit's optional features.

Uberenv (``scripts/uberenv/uberenv.py``) automates fetching spack, building and installing third party dependencies, and can optionally install Conduit as well.  To automate the full install process, Uberenv uses the Conduit Spack package along with extra settings such as Spack compiler and external third party package details for common HPC platforms.


Building Third Party Dependencies for Development
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. note::
  Conduit developers use ``scripts/uberenv/uberenv.py`` to setup third party libraries for Conduit development.
  For info on how to use the Conduit Spack package see :ref:`building_with_spack`.


On OSX and Linux, you can use ``scripts/uberenv/uberenv.py`` to help setup your development environment. This script leverages **Spack** to build all of the external third party libraries and tools used by Conduit. Fortran support is optional and all dependencies should build without a fortran compiler. After building these libraries and tools, it writes an initial *host-config* file and adds the Spack built CMake binary to your PATH so can immediately call the ``config-build.sh`` helper script to configure a conduit build.

.. code:: bash

    #build third party libs using spack
    python3 scripts/uberenv/uberenv.py

    # run the configure helper script and give it the
    # path to a host-config file
    ./config-build.sh uberenv_libs/`hostname`*.cmake



Uberenv Options for Building Third Party Dependencies
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``uberenv.py`` has a few options that allow you to control how dependencies are built:

 ==================== ============================================== ==============================================================
  Option               Description                                     Default
 ==================== ============================================== ==============================================================
  --prefix             Destination directory                          ``uberenv_libs``
  --spec               Spack spec                                     linux: **%gcc**
                                                                      osx: **%clang**
  --spack-env-file     Spack environment file (spack.yaml)            linux: (empty)
                                                                      osx: (empty)
  -k                   Ignore SSL Errors                              **False**
  --install            Fully install conduit, not just dependencies   **False**
  --run_tests          Invoke tests during build and against install  **False**
 ==================== ============================================== ==============================================================

The ``-k`` option exists for sites where SSL certificate interception undermines fetching
from github and https hosted source tarballs. When enabled, ``uberenv.py`` clones spack using:

.. code:: bash

    git -c http.sslVerify=false clone https://github.com/llnl/spack.git

And passes ``-k`` to any spack commands that may fetch via https.


Default invocation on Linux:

.. code:: bash

    python3 scripts/uberenv/uberenv.py --prefix uberenv_libs \
                                       --spec %gcc

Suggested invocation on macOS:

.. code:: bash

    python3 scripts/uberenv/uberenv.py --prefix uberenv_libs \
                                       --spec %clang \
                                       --spack-config-dir scripts/uberenv_configs/spack_configs/envs/darwin/spack.yaml


The uberenv `--install` installs conduit\@develop (not just the development dependencies):

.. code:: bash

    python3 scripts/uberenv/uberenv.py --install


To run tests during the build process to validate the build and install, you can use the ``--run_tests`` option:

.. code:: bash

    python3 scripts/uberenv/uberenv.py --install \
                                       --run_tests

For details on Spack's spec syntax, see the `Spack Specs & dependencies <http://spack.readthedocs.io/en/latest/basic_usage.html#specs-dependencies>`_ documentation.


You use the **--spack-env-file** option to specify a Spack environment file. See the `Spack Environments <https://spack.readthedocs.io/en/latest/environments.html>`_
and `Spack System Packages
<http://spack.readthedocs.io/en/latest/getting_started.html#system-packages>`_
documentation for details.

For macOS, the enries in ``scripts/uberenv_configs/spack_configs/envs/darwin/spack.yaml`` are X-Code's clang and gfortran from https://gcc.gnu.org/wiki/GFortranBinaries#MacOS.

.. note::
    The bootstrapping process ignores ``~/.spack/compilers.yaml`` to avoid conflicts
    and surprises from a user's specific Spack settings on HPC platforms.

When run, ``uberenv.py`` checkouts a specific version of Spack from github as ``{prefix}/spack``.
It then uses Spack to build and install Conduit's dependencies to create a Spack environment at
``{prefix}/spack_env``. Finally, it generates a host-config file at ``{prefix}/{hostname}.cmake``
that specifies the compiler settings and paths to all of the dependencies.


.. .. _building_known_hpc:
..
.. Building with Uberenv on Known HPC Platforms
.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
..
.. To support testing and installing on common platforms, we maintain sets of Spack compiler and package settings
.. for a few known HPC platforms.  Here are the commonly tested configurations:
..
..  ================== ====================== ======================================
..   System             OS                     Tested Configurations (Spack Specs)
..  ================== ====================== ======================================
..   pascal.llnl.gov     Linux: TOSS3          %gcc
..
..                                             %gcc~shared
..   lassen.llnl.gov     Linux: BlueOS         %clang\@coral~python~fortran
..   cori.nersc.gov      Linux: SUSE / CNL     %gcc
..  ================== ====================== ======================================
..
..
.. See ``scripts/spack_build_tests/`` for the exact invocations used to test on these platforms.


.. _building_with_spack:

Building Conduit and its Dependencies with Spack
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

As of 1/4/2017, Spack's develop branch includes a `recipe <https://github.com/LLNL/spack/blob/develop/var/spack/repos/builtin/packages/conduit/package.py>`_ to build and install Conduit.

To install the latest released version of Conduit with all options (and also build all of its dependencies as necessary) run:

.. code:: bash

  spack install conduit

To build and install Conduit's github develop branch run:

.. code:: bash

  spack install conduit@develop


The Conduit Spack package provides several `variants <http://spack.readthedocs.io/en/latest/basic_usage.html#specs-dependencies>`_ that customize the options and dependencies used to build Conduit.
To see these variants, please rune

.. code:: bash

  spack info conduit

Variants are enabled using ``+`` and disabled using ``~``. For example, to build Conduit with the minimum set of options (and dependencies) run:

.. code:: bash

  spack install conduit~python~mpi~hdf5~silo~docs


You can specify specific versions of a dependency using ``^``. For Example, to build Conduit with Python 3,19,19:


.. code:: bash

  spack install conduit+python ^python@3.10.10


.. _supported_cmake:

Supported CMake Versions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
We recommend CMake 3.21 or newer. We test building Conduit with CMake 3.21 and newer 3.2x variants.


Using Conduit in Another Project
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Under ``src/examples`` there are examples demonstrating how to use Conduit in a CMake-based build system (``using-with-cmake``) and via a Makefile (``using-with-make``).


.. _building_with_docker:

Building Conduit in a Docker Container
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Under ``src/examples/docker/ubuntu`` there is an example ``Dockerfile`` which can be used to create an ubuntu-based docker image with a build of the Conduit. There is also a script that demonstrates how to build a Docker image from the Dockerfile (``example_build.sh``) and a script that runs this image in a Docker container (``example_run.sh``). The Conduit repo is cloned into the image's file system at ``/conduit``, the build directory is ``/conduit/build-debug``, and the install directory is ``/conduit/install-debug``.

.. _building_with_pip:

Building Conduit with pip
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Conduit provides a setup.py that allows pip to use CMake to build and install
Conduit and the Conduit Python module. This script assumes that CMake is in your path.

Example Basic Build:

.. code:: bash

    pip install . --user

Or for those with certificate woes:

.. code:: bash

    pip install  --trusted-host pypi.org --trusted-host files.pythonhosted.org . --user


You can enable Conduit features using the following environment variables:

 ================== ========================================== ======================================
  Option              Description                              Default
 ================== ========================================== ======================================
  **HOST_CONFIG**     Path to a host config with cmake options IGNORE
  **HDF5_DIR**        Path to HDF5 install for HDF5 Support    IGNORE
  **ENABLE_MPI**      Build Conduit with MPI  Support          OFF
 ================== ========================================== ======================================


Example Build with MPI and HDF5 Support:

.. code:: bash

    env ENABLE_MPI=ON HDF5_DIR={path/to/hdf5/install} pip install . --user

Example Build with MPI and HDF5 Support using a host config file:

.. code:: bash
    echo 'set(ENABLE_MPI "ON" CACHE BOOL "")'  > conduit-config.cmake
    echo 'set(HDF5_DIR "{path/to/hdf5/install}" CACHE PATH "")' >> conduit-config.cmake
    env HOST_CONFIG=conduit-config.cmake pip install . --user

The specific `HDF5_DIR` and `ENABLE_MPI` are provided for convenience, the `HOST_CONFIG`
option allows you to set any of Conduit's CMake options.

.. Notes for Cray systems
.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
..
.. HDF5 and gtest use runtime features such as ``dlopen``. Because of this, building static on Cray systems commonly yields the following flavor of compiler warning:
..
.. .. code::
..
..    Using 'zzz' in statically linked applications requires at runtime the shared libraries from the glibc version used for linking
..
.. You can avoid related linking warnings by adding the ``-dynamic`` compiler flag, or by setting the CRAYPE_LINK_TYPE environment variable:
..
.. .. code:: bash
..
..   export CRAYPE_LINK_TYPE=dynamic
..
.. `Shared Memory Maps are read only <https://pubs.cray.com/content/S-0005/CLE%206.0.UP02/xctm-series-dvs-administration-guide-cle-60up02-s-0005/dvs-caveats>`_
.. on Cray systems, so updates to data using ``Node::mmap`` will not be seen between processes.
..


Notes for using OpenMPI in a container as root
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
By default OpenMPI prevents the root user from launching MPI jobs. If you are running as root in a container you can use the following env vars to turn off this restriction:

.. code:: bash

    OMPI_ALLOW_RUN_AS_ROOT=1
    OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1







