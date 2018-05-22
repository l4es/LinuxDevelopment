#!/bin/sh

# Filter "-pthread" option from newer gcc versions
# as it's currently unsupported in gccxml.
EOO=0
while [[ $1 ]]; do
if ! ((EOO)); then
  case "$1" in
    -pthread)
      shift
      ;;
    --)
      EOO=1
      OPTIONS="$OPTIONS $1"
      shift
      ;;
    *)
      OPTIONS="$OPTIONS $1"
      shift
      ;;
  esac
else
  OPTIONS="$OPTIONS $1"
  shift
fi
done

gccxml $OPTIONS
