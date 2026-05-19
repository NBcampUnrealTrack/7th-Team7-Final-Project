#!/usr/bin/env bash
# release-lfs-locks.sh — git lfs unlock 으로 LFS 락 해제
#
# 사용법:
#   ./release-lfs-locks.sh path/to/file1.uasset path/to/file2.uasset
#   또는
#   cat paths.txt | ./release-lfs-locks.sh
#
# 경로는 git repo 루트 기준 상대경로. CI 환경에선 actor PAT 가 git auth 에
# 들어가있어야 본인 락 해제 가능 (actions/checkout 의 token 으로 주입).
#
# "이미 풀려있음" (no matching lock) 은 idempotent 로 간주, exit 0 유지.
# 그 외 실패(권한/네트워크/다른 사람 락) 는 카운트해서 exit 1 — CI 알림 트리거용.

set -uo pipefail

failed=0

unlock_one() {
    local path="$1"
    if [[ -z "$path" ]]; then return 0; fi

    # --force 없이: PAT 본인 락만 해제. --force 는 본인 락에도 admin 권한을 요구함.
    # 다른 사람 락은 거부되고 → CI 에서 Discord fallback 으로 떨어짐.
    local output
    if output=$(git lfs unlock "$path" 2>&1); then
        echo "unlocked: $path"
    elif echo "$output" | grep -qiE "no matching|unable to find"; then
        echo "already unlocked: $path"
    else
        echo "FAILED: $path — $output" >&2
        failed=$((failed + 1))
    fi
}

if [[ $# -gt 0 ]]; then
    for p in "$@"; do unlock_one "$p"; done
else
    while IFS= read -r p; do unlock_one "$p"; done
fi

if [[ $failed -gt 0 ]]; then
    echo "$failed path(s) failed to unlock" >&2
    exit 1
fi
