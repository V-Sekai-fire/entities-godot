#!/bin/bash -e
# Refresh thirdparty/tint/ from floooh/tint-extract at the pinned commit.
# tint-extract is a maintained subset of Dawn's Tint (WGSL compiler)
# containing only the SPIR-V reader + WGSL writer, used by the WebGPU
# rendering device driver for shader translation. Same shape as
# thirdparty/thorvg/update-thorvg.sh.

GIT_COMMIT=308db188c0110349ddbde72011203e9b8cbda337

pushd "$(dirname "$0")" >/dev/null

find . -mindepth 1 -maxdepth 1 \
  ! -name update-tint.sh \
  ! -name TAG \
  ! -name README.md \
  -exec rm -rf {} +

mkdir tmp
pushd tmp >/dev/null

git init -q
git remote add origin https://github.com/floooh/tint-extract.git
git fetch --depth 1 -q origin "${GIT_COMMIT}"
git checkout -q FETCH_HEAD
rm -rf .git

popd >/dev/null

[ -f tmp/LICENSE ] && mv tmp/LICENSE ./LICENSE
[ -f tmp/dawn.ref ] && mv tmp/dawn.ref ./dawn.ref

for d in include src; do
  [ -e "tmp/$d" ] && mv "tmp/$d" "./$d"
done

rm -rf tmp

du -sh .

popd >/dev/null
