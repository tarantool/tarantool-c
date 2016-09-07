-------------------------------------------------------------------------------
              Documentation for tarantool-c
-------------------------------------------------------------------------------

.. image:: https://travis-ci.org/tarantool/tarantool-c.svg?branch=master
    :target: https://travis-ci.org/tarantool/tarantool-c

===========================================================
                        About
===========================================================

``tarantool-c`` is a client library that implements a C connector for Tarantool
(see https://github.com/tarantool/tarantool).

The ``tarantool-c`` library depends on the ``msgpuck`` library
(see https://github.com/tarantool/msgpuck).

The ``tarantool-c`` library consists of two parts:

  * ``tnt``,
    an `IProto <http://tarantool.org/doc/dev_guide/box-protocol.html>`_/networking
    library
  * ``tntrpl``,
    a library for working with snapshots, xlogs and a replication client
    (this library is not ported to Tarantool 1.6 yet, so it isn't covered in
    this documentation set)

===========================================================
                 Compilation/Installation
===========================================================

You can clone source code from the "tarantool-c" repository at GitHub
and use :program:`cmake`/:program:`make` to compile and install
the ``tarantool-c`` library:

.. code-block:: console

    $ git clone git://github.com/tarantool/tarantool-c.git ~/tarantool-c --recursive
    $ cd ~/tarantool-c
    $ cmake .
    $ make

    #### For testing against installed tarantool:
    $ make test

    #### For installing into system (headers+libraries):
    $ make install

    #### For building documentation using Sphinx:
    $ cd doc
    $ make sphinx-html

Or you can install it as a package:

.. code-block:: console

    #### In Fedora/RHEL/CentOS/Amazon Linux:
    $ yum install tarantool-c tarantool-c-devel

    #### In Ubuntu/Debian:
    $ apt-get install libtarantool-c libtarantool-c-dev

    #### In Mac OS X:
    $ brew install tarantool-c

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
   stream.rst

===========================================================
                         Index
===========================================================

* :ref:`genindex`
