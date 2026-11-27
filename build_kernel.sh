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

for tool in clang ld.lld llvm-ar llvm-nm make depmod tar gzip; do
	command -v "$tool" >/dev/null || {
		echo "ERROR: $tool is required" >&2
		exit 1
	}
done

OUT_DIR="$ROOT/out/a12s"
MODULES_INSTALL_DIR="$OUT_DIR/modules"
MODULES_ARCHIVE="$OUT_DIR/a12s-modules.tar.gz"

MAKE_FLAGS=(
	ARCH=arm64
	LLVM=1
)

mkdir -p "$OUT_DIR"
make "${MAKE_FLAGS[@]}" exynos850_a12s_defconfig
make "${MAKE_FLAGS[@]}" -j"$JOBS" Image.gz exynos/exynos850-a127f.dtb modules

KERNELRELEASE="$(make "${MAKE_FLAGS[@]}" -s kernelrelease)"
make "${MAKE_FLAGS[@]}" INSTALL_MOD_PATH="$MODULES_INSTALL_DIR" modules_install

tar --sort=name --mtime="@${SOURCE_DATE_EPOCH:-0}" \
	--owner=0 --group=0 --numeric-owner \
	--exclude="$MODULES_INSTALL_DIR/lib/modules/$KERNELRELEASE/build" \
	--exclude="$MODULES_INSTALL_DIR/lib/modules/$KERNELRELEASE/source" \
	--transform="s|^/*${MODULES_INSTALL_DIR#/}/lib/||" \
	-cf - "$MODULES_INSTALL_DIR/lib/modules/$KERNELRELEASE" \
	| gzip -n > "$MODULES_ARCHIVE"

ARTIFACTS=(
	arch/arm64/boot/Image.gz
	arch/arm64/boot/dts/exynos/exynos850-a127f.dtb
	"$MODULES_ARCHIVE"
)

for artifact in "${ARTIFACTS[@]}"; do
	test -s "$artifact" || {
		echo "ERROR: missing build artifact: $artifact" >&2
		exit 1
	}
done

echo "Build complete:"
ls -lh "${ARTIFACTS[@]}"
