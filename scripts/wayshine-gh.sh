#!/usr/bin/env bash
set -euo pipefail

repo="${WAYSHINE_GITHUB_REPO:-neokura/WayShine}"

usage() {
  cat <<'EOF'
Usage:
  scripts/wayshine-gh.sh status
  scripts/wayshine-gh.sh ci [ref]
  scripts/wayshine-gh.sh upstream-watch [ref]
  scripts/wayshine-gh.sh ghcr [ref] [version] [linux_build_args]
  scripts/wayshine-gh.sh watch [run-id]

GitHub CLI helper for WayShine's remote checks and builds.

Commands:
  status          Show recent workflow runs.
  ci             Dispatch the CI workflow. Ref defaults to current branch.
  upstream-watch Dispatch the Sunshine release watch workflow.
  ghcr           Dispatch GHCR publication. Ref defaults to current branch.
  watch          Watch a run by id, or the latest run when omitted.

Environment:
  WAYSHINE_GITHUB_REPO  GitHub repository, default: neokura/WayShine
EOF
}

require_gh() {
  if ! command -v gh >/dev/null 2>&1; then
    echo "GitHub CLI is required. Install gh and authenticate with 'gh auth login'." >&2
    exit 1
  fi
}

current_ref() {
  git rev-parse --abbrev-ref HEAD
}

latest_run_id() {
  gh run list --repo "$repo" --limit 1 --json databaseId --jq '.[0].databaseId'
}

require_gh

command_name="${1:-}"
case "$command_name" in
  -h|--help|"")
    usage
    ;;
  status)
    gh run list \
      --repo "$repo" \
      --limit 10 \
      --json databaseId,workflowName,status,conclusion,headBranch,event,createdAt,url \
      --jq '.[] | [.databaseId, .workflowName, .status, (.conclusion // ""), .headBranch, .event, .createdAt, .url] | @tsv'
    ;;
  ci)
    ref="${2:-$(current_ref)}"
    gh workflow run ci.yml --repo "$repo" --ref "$ref"
    ;;
  upstream-watch)
    ref="${2:-$(current_ref)}"
    gh workflow run upstream-release-watch.yml --repo "$repo" --ref "$ref"
    ;;
  ghcr)
    ref="${2:-$(current_ref)}"
    version="${3:-}"
    linux_build_args="${4:---skip-cuda}"
    args=(--repo "$repo" --ref "$ref" -f "linux_build_args=${linux_build_args}")
    if [[ -n "$version" ]]; then
      args+=(-f "version=${version}")
    fi
    gh workflow run ghcr.yml "${args[@]}"
    ;;
  watch)
    run_id="${2:-$(latest_run_id)}"
    if [[ -z "$run_id" || "$run_id" == "null" ]]; then
      echo "No workflow runs found for ${repo}." >&2
      exit 1
    fi
    gh run watch "$run_id" --repo "$repo" --exit-status
    ;;
  *)
    usage >&2
    exit 1
    ;;
esac
