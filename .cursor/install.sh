#!/usr/bin/env bash
#
# Cloud Agent install script for CoMaps.
#
# Prepares a full development environment for:
#   * the CoMaps Qt6 desktop application (this repository), and
#   * the companion "explorer" Django backend (repositoryDependency), when present.
#
# It is idempotent: it can run repeatedly against a cached checkout / snapshot.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

echo "==> Installing system packages (Qt6, clang/llvm toolchain, build dependencies)"
export DEBIAN_FRONTEND=noninteractive
sudo apt-get update -y
sudo apt-get install -y \
    build-essential \
    clang \
    llvm \
    cmake \
    ninja-build \
    python3 \
    python3-venv \
    qt6-base-dev \
    qt6-positioning-dev \
    libc++-dev \
    libfreetype-dev \
    libglvnd-dev \
    libgl1-mesa-dev \
    libharfbuzz-dev \
    libicu-dev \
    libqt6svg6-dev \
    libqt6positioning6-plugins \
    libqt6positioning6 \
    libsqlite3-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    zlib1g-dev \
    libstdc++-14-dev \
    optipng \
    jq \
    curl \
    git \
    ca-certificates

echo "==> Ensuring uv (Python package manager) is installed"
if ! command -v uv >/dev/null 2>&1 && [ ! -x "$HOME/.local/bin/uv" ]; then
  curl -LsSf https://astral.sh/uv/install.sh | sh
fi
export PATH="$HOME/.local/bin:$PATH"

echo "==> CoMaps: initializing git submodules"
git submodule update --init --recursive --depth 1

echo "==> CoMaps: configuring repository (styles, strings, World map download)"
./configure.sh

echo "==> CoMaps: building the Release desktop application"
BUILD_DIR="$HOME/omim-build-release"
if [ -f "$BUILD_DIR/CMakeCache.txt" ] && ! grep -q "^CMAKE_HOME_DIRECTORY:INTERNAL=$REPO_ROOT$" "$BUILD_DIR/CMakeCache.txt"; then
  echo "==> CoMaps: existing build cache is for a different source path; removing $BUILD_DIR"
  rm -rf "$BUILD_DIR"
fi
SKIP_MAP_DOWNLOAD=1 ./tools/unix/build_omim.sh -r -p "$HOME" desktop

EXPLORER_DIR="$REPO_ROOT/../explorer"
if [ -d "$EXPLORER_DIR" ]; then
  echo "==> Explorer: setting up the Django backend"
  cd "$EXPLORER_DIR"
  [ -f .env ] || cp .env.example .env
  uv sync
  uv run python manage.py migrate --noinput
else
  echo "==> Explorer repository not present; skipping backend setup"
fi

echo "==> install.sh complete"
