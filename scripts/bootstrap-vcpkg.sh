#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
VCPKG_DIR="$ROOT_DIR/.vcpkg"
VCPKG_COMMIT="93ad765c7b66ab9ce97469ec23a90b15c3a14ee5"

if ! command -v git >/dev/null 2>&1; then
    echo "Git is required to download vcpkg." >&2
    exit 1
fi

if [ ! -d "$VCPKG_DIR/.git" ]; then
    git clone https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
fi

git -C "$VCPKG_DIR" checkout "$VCPKG_COMMIT"
"$VCPKG_DIR/bootstrap-vcpkg.sh" -disableMetrics
