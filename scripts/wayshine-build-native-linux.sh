#!/usr/bin/env bash
set -euo pipefail

repo_url="${WAYSHINE_REPO_URL:-https://github.com/neokura/WayShine}"
publisher_name="${WAYSHINE_PUBLISHER_NAME:-WayShine}"
publisher_website="${WAYSHINE_PUBLISHER_WEBSITE:-${repo_url}}"
publisher_issue_url="${WAYSHINE_PUBLISHER_ISSUE_URL:-${repo_url}/issues}"

exec scripts/linux_build.sh \
  --publisher-name="$publisher_name" \
  --publisher-website="$publisher_website" \
  --publisher-issue-url="$publisher_issue_url" \
  "$@"
