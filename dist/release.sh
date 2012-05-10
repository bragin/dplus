#!/bin/sh
# release.sh: Script to create a DPlus release tarball.
# Author: Benjamin Johnson <obeythepenguin@users.sourceforge.net>
# Date: 2012/05/10

FILENAME="$1"
BASENAME="$(basename "$FILENAME" .tar.bz2)"

if [ -z "$FILENAME" ]; then
   echo "Usage: $0 /path/to/output-archive.tar.bz2" >&2
   exit 1
fi

tar --exclude=.hg --exclude=.hgignore --exclude=.hgtags \
			--transform="s,^\\./,$BASENAME/," -cjvf "$FILENAME" .
