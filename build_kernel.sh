#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

export ARCH=arm64

JOBS="${JOBS:-$(nproc)}"
TOOLCHAINS_DIR="${TOOLCHAINS_DIR:-$ROOT/../toolchains}"
CLANG_VERSION="${CLANG_VERSION:-clang-r614150}"

if [ -z "${CLANG_DIR:-}" ]; then
	CLANG_DIR="$TOOLCHAINS_DIR/clang/host/linux-x86/$CLANG_VERSION"
	if [ ! -x "$CLANG_DIR/bin/clang" ]; then
		if [ ! -x "$TOOLCHAINS_DIR/fetch-clang.sh" ]; then
			echo "ERROR: toolchain not found at $CLANG_DIR" >&2
			echo "Clone https://github.com/SaneHyuke/toolchains.git to $TOOLCHAINS_DIR or set CLANG_DIR" >&2
			exit 1
		fi
		echo "Downloading $CLANG_VERSION from the toolchains repository..."
		"$TOOLCHAINS_DIR/fetch-clang.sh" "$CLANG_VERSION"
	fi
fi

if [ ! -x "$CLANG_DIR/bin/clang" ]; then
	echo "ERROR: clang not found at $CLANG_DIR/bin/clang" >&2
	exit 1
fi

export PATH="$CLANG_DIR/bin:$PATH"
echo "Using toolchain: $CLANG_DIR"

for tool in clang ld.lld llvm-ar llvm-nm; do
	command -v "$tool" >/dev/null || {
		echo "ERROR: $tool is required" >&2
		exit 1
	}
done

MAKE_FLAGS=(
	ARCH=arm64
	LLVM=1
)

make "${MAKE_FLAGS[@]}" exynos850_a12s_defconfig
make "${MAKE_FLAGS[@]}" -j"$JOBS" Image.gz exynos/exynos850-a127f.dtb

ARTIFACTS=(
	arch/arm64/boot/Image.gz
	arch/arm64/boot/dts/exynos/exynos850-a127f.dtb
)

for artifact in "${ARTIFACTS[@]}"; do
	test -s "$artifact" || {
		echo "ERROR: missing build artifact: $artifact" >&2
		exit 1
	}
done

echo "Build complete:"
ls -lh "${ARTIFACTS[@]}"
