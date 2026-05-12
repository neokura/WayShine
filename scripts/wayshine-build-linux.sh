#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  scripts/wayshine-build-linux.sh [target...]

Builds WayShine with the Linux Dockerfiles shipped by the imported Sunshine
snapshot. If no target is provided, the default Linux matrix is built.

Targets:
  ubuntu-22.04
  ubuntu-24.04
  ubuntu-26.04
  debian-trixie

Environment:
  WAYSHINE_PLATFORM       Docker platform, default: linux/amd64
  WAYSHINE_BUILD_TARGET   Docker target, default: sunshine
  WAYSHINE_LOAD_IMAGE     Use --load with buildx, default: 1
  WAYSHINE_NO_CACHE       Set to 1 to disable cache
  WAYSHINE_LINUX_BUILD_ARGS
                         Extra arguments passed to scripts/linux_build.sh
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi

default_targets=(ubuntu-22.04 ubuntu-24.04 ubuntu-26.04 debian-trixie)
targets=("$@")
if [[ ${#targets[@]} -eq 0 ]]; then
  targets=("${default_targets[@]}")
fi

platform="${WAYSHINE_PLATFORM:-linux/amd64}"
build_target="${WAYSHINE_BUILD_TARGET:-sunshine}"
load_image="${WAYSHINE_LOAD_IMAGE:-1}"
no_cache="${WAYSHINE_NO_CACHE:-0}"
linux_build_args="${WAYSHINE_LINUX_BUILD_ARGS:-}"

branch="$(git rev-parse --abbrev-ref HEAD)"
commit="$(git rev-parse HEAD)"
version="$(awk -F= '/^upstream_tag=/{print $2}' UPSTREAM.lock 2>/dev/null || true)"
version="${version:-dev}"

if docker buildx version >/dev/null 2>&1; then
  builder=(docker buildx build)
  if [[ "$load_image" == "1" ]]; then
    builder+=(--load)
  fi
else
  echo "docker buildx is not available; using docker build fallback." >&2
  echo "Install the buildx plugin for reliable cross-platform builds." >&2
  builder=(docker build)
fi

if [[ "$no_cache" == "1" ]]; then
  builder+=(--no-cache)
fi

for target in "${targets[@]}"; do
  dockerfile="docker/${target}.dockerfile"
  if [[ ! -f "$dockerfile" ]]; then
    echo "Unknown Linux build target: ${target}" >&2
    echo "Expected Dockerfile at ${dockerfile}" >&2
    exit 1
  fi

  image="wayshine:${target}"
  echo "Building ${image} from ${dockerfile} on ${platform}"
  "${builder[@]}" \
    --platform "$platform" \
    --target "$build_target" \
    --build-arg "BRANCH=${branch}" \
    --build-arg "BUILD_VERSION=${version}" \
    --build-arg "COMMIT=${commit}" \
    --build-arg "WAYSHINE_LINUX_BUILD_ARGS=${linux_build_args}" \
    --tag "$image" \
    --file "$dockerfile" \
    .
done
