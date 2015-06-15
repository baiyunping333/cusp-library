# Introduction #

This page lists some instructions on how to build & run Cusp's unit tests.  The tests have
been built & tested on Windows, Ubuntu, and Mac OS X, but these instructions assume a Debian-based OS like Ubuntu.

# Prerequisites #

Cusp's build process depends on [Scons](http://www.scons.org).  If you are on a
Debian-based OS, you can install Scons with

`$ sudo apt-get install scons`

The testing infrastructure isn't included in the zip file on the
website, so you'll need to checkout the source tree from the
repository with [Mercurial](http://mercurial.selenic.com/).  If you are on a Debian-based OS, you can install Mercurial with

`$ sudo apt-get install mercurial`

To checkout the current development version of Cusp use

`hg clone https://cusp-library.googlecode.com/hg/ cusp`

Cusp also depends on the [Thrust](http://thrust.googlecode.com/) template library.  The Scons scripts assume that Thrust is installed in a standard include directory, such as `/usr/local/cuda/include/`.

# Unit Tests #

You can build the unit tester simply by changing to the testing
directory and running scons:

`$ cd cusp/testing`

`$ scons`

To run all tests,

`$ ./tester`

You can also specify a list of tests to run by name as arguments to tester.  The test driver also provides a verbose mode which prints additional information:

`$ ./tester -v`