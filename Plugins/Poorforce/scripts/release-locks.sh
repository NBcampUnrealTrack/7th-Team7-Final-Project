#!/usr/bin/env bash
# release-locks.sh — Upstash Redis DEL을 호출해서 Poorforce 락을 해제한다.
#
# 사용법:
#   환경변수 UPSTASH_URL, UPSTASH_TOKEN 설정 후
#   ./release-locks.sh key1 key2 key3
#   또는
#   cat keys.txt | ./release-locks.sh
#
# 키 포맷 예시: "gy:asset:Foo/Bar"
#
# CI에서 사용 시:
#   - secrets에 UPSTASH_URL, UPSTASH_TOKEN 등록
#   - PR 머지 시 변경된 .uasset/.umap 경로를 키로 변환해서 전달

set -euo pipefail

if [[ -z "${UPSTASH_URL:-}" ]]; then
    echo "error: UPSTASH_URL not set" >&2
    exit 1
fi

if [[ -z "${UPSTASH_TOKEN:-}" ]]; then
    echo "error: UPSTASH_TOKEN not set" >&2
    exit 1
fi

release_one() {
    local key="$1"
    if [[ -z "$key" ]]; then
        return 0
    fi

    local response
    response=$(curl -sS -X POST "$UPSTASH_URL/" \
        -H "Authorization: Bearer $UPSTASH_TOKEN" \
        -H "Content-Type: application/json" \
        -d "[\"DEL\", \"$key\"]")

    if echo "$response" | grep -q '"error"'; then
        echo "FAILED: $key — $response" >&2
        return 1
    fi

    echo "released: $key"
}

failed=0

if [[ $# -gt 0 ]]; then
    for key in "$@"; do
        release_one "$key" || failed=$((failed + 1))
    done
else
    while IFS= read -r key; do
        release_one "$key" || failed=$((failed + 1))
    done
fi

if [[ $failed -gt 0 ]]; then
    echo "$failed key(s) failed to release" >&2
    exit 1
fi
