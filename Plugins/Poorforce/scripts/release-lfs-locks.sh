#!/usr/bin/env bash
# release-lfs-locks.sh — git lfs unlock 으로 LFS 락 해제
#
# 사용법:
#   ./release-lfs-locks.sh path/to/file1.uasset path/to/file2.uasset
#   또는
#   cat paths.txt | ./release-lfs-locks.sh
#
# 경로는 git repo 루트 기준 상대경로. CI 환경에선 GITHUB_TOKEN 등이 이미 git auth에
# 들어가있어야 동작 (actions/checkout 이 처리). 로컬 사용 시 git 자격증명 필요.
#
# 실패 케이스 (이미 풀려있음 등) 는 출력만 하고 다음 경로로 넘어감 — exit 0 유지.

set -uo pipefail

unlock_one() {
    local path="$1"
    if [[ -z "$path" ]]; then return 0; fi

    # --force: 락 owner != 호출자여도 해제 (CI 컨텍스트에서 PR 머지 후 해제하는 흐름이라 OK)
    local output
    if output=$(git lfs unlock --force "$path" 2>&1); then
        echo "unlocked: $path"
    else
        echo "skip $path — $output" >&2
    fi
}

if [[ $# -gt 0 ]]; then
    for p in "$@"; do unlock_one "$p"; done
else
    while IFS= read -r p; do unlock_one "$p"; done
fi
