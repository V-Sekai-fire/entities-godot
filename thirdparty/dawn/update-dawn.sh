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

# Populate the submodules Dawn's generator and library actually need. Init
# a curated set only; the Chromium build submodules (buildtools, build,
# tools/*) are dropped below and never fetched. The Khronos-adjacent
# submodules land at third_party/<name>/src (nested one level).
for sub in third_party/spirv-headers/src third_party/spirv-tools/src \
           third_party/vulkan-headers/src third_party/vulkan-utility-libraries/src \
           third_party/webgpu-headers third_party/emdawnwebgpu \
           third_party/khronos third_party/jinja2 third_party/markupsafe \
           third_party/glfw3/src third_party/abseil-cpp; do
  if grep -q "path = ${sub}$" .gitmodules 2>/dev/null; then
    git submodule update --init --depth 1 -q "${sub}" 2>/dev/null || true
  fi
done

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
    ! -name jinja2 \
    ! -name markupsafe \
    ! -name glfw \
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

# Run Dawn's Python generator to produce webgpu.h, dawn_proc_table.h and
# the wire client/server shims so the driver source compiles without a
# separate codegen step in SCons. Requires python3 + jinja2 + markupsafe.
python3 generator/dawn_json_generator.py \
  --dawn-json src/dawn/dawn.json \
  --wire-json src/dawn/dawn_wire.json \
  --targets headers,dawn_headers,cpp_headers,cpp,proc,wire \
  --template-dir generator/templates \
  --output-dir gen

du -sh .

popd >/dev/null
