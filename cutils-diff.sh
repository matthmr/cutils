#!/usr/bin/sh

here=${0%/*}

case $1 in
  '--help'|'-h'|'')
    echo "\
Usage:       cutils-diff.sh [-o] AGAINST [DIFF_OPTS]"
    echo "\
Description: Diff files from another cutils implementation against this"
    echo "\
Options:
  -o: AGAINST is OURS (reverse diff)"
    exit 0 ;;
esac

reverse=false
diff_opts=""

for arg in $@; do
  case $arg in
    '-o') reverse=true ;;
    *)
      if [[ -z $against ]]; then
        against=$arg
      else
        diff_opts+=" $arg"
      fi ;;
  esac
done

if $reverse; then
  _here=$here
  here=$against
  against=$_here
fi

diff -u -r $here $against -x '*.o' -x '.git*' -x 'README.md' $diff_opts
