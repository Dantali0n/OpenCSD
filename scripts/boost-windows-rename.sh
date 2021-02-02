#!/bin/bash

# Rename libboost.lib files cause Boost found it to hard to have consistent filenames across platforms..
find . -type f | perl -pe 'print $_; s/libboost_([_a-z0-9]*)-[-a-z0-9_]*\.lib/boost_$1.lib/' | xargs -d "\n" -n2 mv -f