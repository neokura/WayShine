#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  scripts/wayshine-merge-upstream.sh <sunshine-tag> [upgrade-branch]

Fetches a Sunshine release tag from the official upstream repository, creates an
upgrade branch from the current branch, and merges that upstream tag.

Example:
  scripts/wayshine-merge-upstream.sh v2026.508.45922
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
  echo "Refusing to start an upstream merge with a dirty working tree." >&2
  exit 1
fi

remote_name="${WAYSHINE_UPSTREAM_REMOTE:-upstream}"
remote_url="${WAYSHINE_UPSTREAM_URL:-https://github.com/LizardByte/Sunshine.git}"

if ! git remote get-url "$remote_name" >/dev/null 2>&1; then
  git remote add "$remote_name" "$remote_url"
fi

git remote set-url --push "$remote_name" no_push
git fetch "$remote_name" "refs/tags/${tag}:refs/tags/${tag}" "refs/heads/master:refs/remotes/${remote_name}/master"

upstream_commit="$(git rev-parse "${tag}^{commit}")"
current_branch="$(git rev-parse --abbrev-ref HEAD)"
upgrade_branch="${2:-upgrade/sunshine-${tag}}"

git switch -c "$upgrade_branch"
git merge --no-ff "$tag" -m "Merge Sunshine ${tag} into WayShine"

cat <<EOF
Merged Sunshine ${tag}
  upstream commit: ${upstream_commit}
  source branch: ${current_branch}
  upgrade branch: ${upgrade_branch}

Next:
  update UPSTREAM.lock
  run scripts/wayshine-build-linux.sh
  open a pull request into main
EOF
