#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="Mate Games"

(test -f $srcdir/configure.in \
  && test -d $srcdir/gnomine \
  && test -d $srcdir/swell-foop) || {
    echo -n "**Error**: Directory \"\`$srcdir\'\" does not look like the"
    echo " top-level mate directory"
    exit 1
}

which mate-autogen.sh || {
    echo "You need to install mate-common from the MATE CVS"
    exit 1
}

REQUIRED_AUTOMAKE_VERSION=1.9 
REQUIRED_MATE_DOC_UTILS_VERSION=0.10.0 
REQUIRED_GETTEXT_VERSION=0.12
REQUIRED_INTLTOOL_VERSION=0.40.4

. mate-autogen.sh
