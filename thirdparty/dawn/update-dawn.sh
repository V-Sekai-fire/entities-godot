#!/bin/bash -e
# Refresh thirdparty/dawn/ from google/dawn at the pinned commit, applying
# the pruning policy below. Same shape as thirdparty/thorvg/update-thorvg.sh.

GIT_COMMIT=1e897275172a23f27b0022fa6beae3084ed54a9b

pushd "$(dirname "$0")" >/dev/null

find . -mindepth 1 -maxdepth 1 \
  ! -name update-dawn.sh \
  ! -name TAG \
  ! -name README.md \
  -exec rm -rf {} +

mkdir tmp
pushd tmp >/dev/null

git init -q
git remote add origin https://github.com/google/dawn.git
git fetch --depth 1 -q origin "${GIT_COMMIT}"
git checkout -q FETCH_HEAD
rm -rf .git

rm -rf \
  agents build buildtools build_overrides docs infra test testing tools \
  webgpu-cts .github .vscode

if [ -d third_party ]; then
  find third_party -mindepth 1 -maxdepth 1 -type d \
    ! -name spirv-headers \
    ! -name spirv-tools \
    ! -name vulkan-headers \
    ! -name vulkan-utility-libraries \
    ! -name webgpu-headers \
    ! -name emdawnwebgpu \
    ! -name khronos \
    -exec rm -rf {} +
fi

find src \( \
    -type d -name tests -o \
    -type d -name test -o \
    -type d -name unittests -o \
    -type d -name samples -o \
    -type d -name benchmarks -o \
    -type d -name fuzzers -o \
    -type d -name webgpu_cts \
  \) -prune -exec rm -rf {} +

find . -type f \( \
    -name 'BUILD.bazel' -o -name 'BUILD.gn' -o -name '*.gni' -o \
    -name '.clang-tidy' -o -name '.clang-format' -o \
    -name '.clang-format-ignore' -o -name '.gitattributes' -o \
    -name '.gitignore' -o -name '.gitmodules' -o -name '.gn' -o \
    -name '.bazelrc' -o -name '.bazelversion' -o -name '.style.yapf' -o \
    -name '.vpython3' -o -name 'CPPLINT.cfg' -o -name 'DEPS' -o \
    -name 'DIR_METADATA' -o -name 'CODE_OF_CONDUCT.md' -o \
    -name 'CONTRIBUTING.md' -o -name 'codereview.settings' -o \
    -name 'go.mod' -o -name 'go.sum' -o -name 'go_presubmit_support.py' \
  \) -delete

popd >/dev/null

[ -f tmp/LICENSE ] && mv tmp/LICENSE ./LICENSE
[ -f tmp/AUTHORS ] && mv tmp/AUTHORS ./AUTHORS

for d in include src generator scripts third_party CMakeLists.txt CMakeSettings.json; do
  [ -e "tmp/$d" ] && mv "tmp/$d" "./$d"
done

rm -rf tmp

du -sh .

popd >/dev/null
