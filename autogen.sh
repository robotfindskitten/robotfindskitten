#!/bin/bash -x
aclocal 
libtoolize --copy
autoheader 
automake --copy --add-missing
autoconf 
