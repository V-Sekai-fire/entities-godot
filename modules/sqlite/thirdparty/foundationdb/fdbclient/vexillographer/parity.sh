#!/usr/bin/env bash
# Byte-for-byte comparison of vexillographer.py with the C# vexillographer, for the languages the Python port covers
# (c, cpp, python, ruby). java is not ported. Runs in a container with mono and python3: /src is the tree (read-only),
# /work is writable. Exits non-zero on any difference.
set -u
cd /src/fdbclient/vexillographer
mcs -r:System,System.Core,System.Data,System.Xml,System.Xml.Linq c.cs cpp.cs java.cs python.cs ruby.cs vexillographer.cs \
  -target:exe -out:/work/vex.exe 2>&1 | grep -v warning | head -3
rm -rf /work/vcs /work/vpy; mkdir -p /work/vcs /work/vpy
bad=0
cmpf() { if cmp -s "/work/vcs/$1" "/work/vpy/$1"; then echo "$1: identical ($(wc -c < /work/vcs/$1) bytes)"; else echo "$1: DIFFERENT"; bad=1; fi; }
for l in c python ruby cpp; do
  mono /work/vex.exe fdb.options $l /work/vcs/out.$l >/dev/null 2>&1; r1=$?
  python3 vexillographer.py fdb.options $l /work/vpy/out.$l >/dev/null 2>&1; r2=$?
  [ $r1 = $r2 ] || { echo "$l: exit codes differ cs=$r1 py=$r2"; bad=1; }
  if [ $l = cpp ]; then cmpf out.cpp.h; cmpf out.cpp.cpp; else cmpf out.$l; fi
done
python3 vexillographer.py fdb.options java /work/vpy/java >/dev/null 2>&1; echo "java: python exit $? (not ported, must refuse)"
exit $bad
