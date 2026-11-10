#!/usr/bin/env bash
# Byte-for-byte comparison of the Python actor compiler with the C# one, over every actor file in the tree.
# FILES='a.actor.cpp b.actor.h' limits the run to those files (paths relative to the tree root).
# Runs in a container with mono and python3. Mounts: the source tree at /src (read-only), a writable /work.
# Both compilers write to the same relative output path from their own working directory, so the #line
# directives agree with no normalising. Exits non-zero unless different=0 and failed=0.
set -u
S=/src/flow/actorcompiler
if [ ! -x /work/actorcompiler.exe ]; then
  mcs -r:System,System.Core,System.Xml.Linq,System.Data.DataSetExtensions,Microsoft.CSharp,System.Data,System.Xml \
    $S/ActorCompiler.cs $S/ActorParser.cs $S/ParseTree.cs $S/Program.cs $S/Properties/AssemblyInfo.cs \
    -target:exe -out:/work/actorcompiler.exe 2>&1 | grep -v warning | head -5
fi
rm -rf /work/cs /work/py; mkdir -p /work/cs /work/py
same=0; diff=0; fail=0; n=0
cd /src
LIST=${FILES:-$(find . \( -name '*.actor.cpp' -o -name '*.actor.h' \) -not -path './.git/*' | sed 's#^\./##' | sort)}
for f in $LIST; do
  o=$(echo "$f" | sed 's/\.actor\.cpp$/.actor.g.cpp/; s/\.actor\.h$/.actor.g.h/')
  for flags in "--generate-probes" ""; do
    n=$((n+1))
    mkdir -p /work/cs/$(dirname $o) /work/py/$(dirname $o)
    (cd /work/cs; mono /work/actorcompiler.exe /src/$f $o $flags >/dev/null 2>&1); rc1=$?
    (cd /work/py; PYTHONPATH=/src/flow python3 -m actorcompiler_py /src/$f $o $flags >/dev/null 2>&1); rc2=$?
    if [ $rc1 != $rc2 ]; then echo "RC $f [$flags] cs=$rc1 py=$rc2"; fail=$((fail+1)); continue; fi
    if cmp -s /work/cs/$o /work/py/$o; then same=$((same+1)); else diff=$((diff+1)); echo "DIFF $f [$flags]"; fi
    rm -f /work/cs/$o.tmp /work/py/$o.tmp
  done
done
echo "runs=$n identical=$same different=$diff failed=$fail"
[ $diff = 0 ] && [ $fail = 0 ]
