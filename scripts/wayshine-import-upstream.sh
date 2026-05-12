#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  scripts/wayshine-import-upstream.sh <sunshine-tag>

Imports a Sunshine tag into the upstream-snapshot vendor branch without keeping
Sunshine's upstream commit history.

Example:
  scripts/wayshine-import-upstream.sh v2026.508.45922
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi

tag="${1:-}"
if [[ -z "$tag" ]]; then
  usage
  exit 1
fi

if [[ -n "$(git status --porcelain)" ]]; then
  echo "Refusing to import with a dirty working tree." >&2
  exit 1
fi

remote_name="${WAYSHINE_UPSTREAM_REMOTE:-upstream}"
remote_url="${WAYSHINE_UPSTREAM_URL:-https://github.com/LizardByte/Sunshine.git}"
snapshot_branch="${WAYSHINE_SNAPSHOT_BRANCH:-upstream-snapshot}"

if ! git remote get-url "$remote_name" >/dev/null 2>&1; then
  git remote add "$remote_name" "$remote_url"
fi

git fetch --depth=1 "$remote_name" "refs/tags/${tag}"

upstream_commit="$(git rev-parse FETCH_HEAD^{commit})"
upstream_tree="$(git rev-parse FETCH_HEAD^{tree})"
previous_snapshot="$(git rev-parse --verify --quiet "refs/heads/${snapshot_branch}" || true)"

message=$(
  cat <<EOF
Import Sunshine ${tag} snapshot

Upstream: ${remote_url}
Tag: ${tag}
Commit: ${upstream_commit}

This is a clean-history vendor snapshot commit. It records Sunshine's source
tree without retaining Sunshine's upstream commit parents.
EOF
)

if [[ -n "$previous_snapshot" ]]; then
  snapshot_commit="$(printf '%s\n' "$message" | git commit-tree "$upstream_tree" -p "$previous_snapshot")"
else
  snapshot_commit="$(printf '%s\n' "$message" | git commit-tree "$upstream_tree")"
fi

git update-ref "refs/heads/${snapshot_branch}" "$snapshot_commit"

cat <<EOF
Imported ${tag}
  upstream commit: ${upstream_commit}
  snapshot branch: ${snapshot_branch}
  snapshot commit: ${snapshot_commit}

Next:
  git switch main
  git merge --no-ff ${snapshot_branch}
  update UPSTREAM.lock after the merge is resolved
EOF
