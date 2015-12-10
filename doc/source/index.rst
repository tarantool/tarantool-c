-------------------------------------------------------------------------------
                          tarantool-c documentation
-------------------------------------------------------------------------------

.. image:: https://travis-ci.org/tarantool/tarantool-c.svg?branch=master
    :target: https://travis-ci.org/tarantool/tarantool-c

===========================================================
                        About
===========================================================

**Tarantool-c** is a client library written in C for Tarantool.

Tarantool-c depends on `msgpuck <https://github.com/tarantool/msgpuck>`_.

Documentation is avaliable in headers and examples are in tests.

It consinsts of:

* tnt - tarantool IProto/networking library

===========================================================
                 Compilation/Installation
===========================================================

This project using CMake for generating Makefiles:

.. code-block:: console

    $ cmake .
    $ make
    #### For testing against installed tarantool
    $ make test
    #### For building documentation using Doxygen
    $ make doc
    #### For installing into system (headers+libraries)
    $ make install

Or you can install it using yum/apt into your favorite linux distribution
from `tarantool`'s repository

===========================================================
                        Contents
===========================================================

.. toctree::
   :maxdepth: 2

   connection.rst
   msgpackobject.rst
   reply.rst
   request.rst
   request_builder.rst
   schema.rst
   buffering.rst

===========================================================
                  Indices and tables
===========================================================

* :ref:`genindex`
* :ref:`search`

