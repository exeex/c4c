#!/bin/bash
# Shared agent loop harness.
# Usage: ./scripts/run_agent_common.sh <prompt-file>

set -u

cd "$(dirname "$0")/.."

PROMPT_FILE="${1:-}"

if [ -z "$PROMPT_FILE" ]; then
    echo "[harness] Usage: ./scripts/run_agent_common.sh <prompt-file>" >&2
    exit 1
fi

if [ ! -f "$PROMPT_FILE" ]; then
    echo "[harness] Prompt file not found: $PROMPT_FILE" >&2
    exit 1
fi

echo "[harness] Starting agent loop. Stop with Ctrl+C."
echo "[harness] Prompt → $PROMPT_FILE"
echo "[harness] Logs → build/agent_state/agent_logs/"
mkdir -p build/agent_state/agent_logs

ensure_log_dir() {
    mkdir -p build/agent_state/agent_logs
}

# Extra buffer after reported reset time (default: 10 minutes).
LIMIT_RESUME_BUFFER_SECONDS="${LIMIT_RESUME_BUFFER_SECONDS:-600}"

compute_wait_seconds_from_limit_line() {
    local limit_line="$1"
    local wait_seconds

    wait_seconds=$(python3 - "$limit_line" <<'PY'
import datetime as dt
import re
import sys

line = sys.argv[1] if len(sys.argv) > 1 else ""

time_match = re.search(r"resets?\s+(\d{1,2})(?::(\d{2}))?\s*([ap]m)?", line, re.IGNORECASE)
tz_match = re.search(r"\(([^)]+)\)", line)

if not time_match:
    print(-1)
    raise SystemExit

hour = int(time_match.group(1))
minute = int(time_match.group(2) or "0")
ampm = (time_match.group(3) or "").lower()

if ampm:
    if hour == 12:
        hour = 0
    if ampm == "pm":
        hour += 12

tz = None
if tz_match:
    try:
        from zoneinfo import ZoneInfo
        tz = ZoneInfo(tz_match.group(1).strip())
    except Exception:
        tz = None

now = dt.datetime.now(tz=tz)
target = now.replace(hour=hour, minute=minute, second=0, microsecond=0)
if target <= now:
    target += dt.timedelta(days=1)

print(int((target - now).total_seconds()))
PY
)

    if [ -z "$wait_seconds" ] || [ "$wait_seconds" -lt 0 ] 2>/dev/null; then
        echo 900
    else
        echo "$wait_seconds"
    fi
}

stash_worktree_on_limit() {
    local mode="$1"
    local iter="$2"
    local timestamp="$3"
    local stash_msg

    if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        echo "[harness] Not a git worktree. Skip auto-stash."
        return
    fi

    if [ -z "$(git status --short 2>/dev/null)" ]; then
        echo "[harness] Worktree clean. No stash needed."
        return
    fi

    stash_msg="auto-stash on limit: ${mode} iter=${iter} at ${timestamp}"
    if git stash push -u -m "$stash_msg" >/dev/null 2>&1; then
        echo "[harness] Stashed in-progress work: $stash_msg"
    else
        echo "[harness] Failed to stash in-progress work before sleeping."
    fi
}

ITER=0
while true; do
    ITER=$((ITER + 1))
    COMMIT=$(git rev-parse --short=6 HEAD 2>/dev/null || echo "no-git")
    MODE=$(basename "$PROMPT_FILE" .md | tr '[:upper:]' '[:lower:]')
    LOGFILE="build/agent_state/agent_logs/${MODE}_iter_${ITER}_${COMMIT}.log"
    TMP_LOG=$(mktemp /tmp/claude-harness.XXXXXX.log)
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')

    echo "[harness] === Iteration $ITER at $TIMESTAMP (commit: $COMMIT) ==="
    echo "[harness] Log: $LOGFILE"
    ensure_log_dir
    rm -f test_before.log test_after.log
    echo "[harness] Cleared stale test_before.log/test_after.log."

    claude --dangerously-skip-permissions \
           -p "$(cat "$PROMPT_FILE")" \
           --model opus \
           2>&1 | tee "$TMP_LOG" "$LOGFILE"
    CLAUDE_EXIT=${PIPESTATUS[0]}

    if [ ! -f "$LOGFILE" ] && [ -f "$TMP_LOG" ]; then
        ensure_log_dir
        cp "$TMP_LOG" "$LOGFILE" 2>/dev/null || true
    fi

    echo "[harness] === Iteration $ITER done. Current score: ==="
    cat build/agent_state/last_result.txt 2>/dev/null || echo "(no result yet)"
    echo ""

    LIMIT_SOURCE="$LOGFILE"
    if [ ! -f "$LIMIT_SOURCE" ]; then
        LIMIT_SOURCE="$TMP_LOG"
    fi

    if [ -f "$LIMIT_SOURCE" ] && grep -Eqi "hit your limit|rate limit|usage limit|quota" "$LIMIT_SOURCE"; then
        LIMIT_LINE=$(grep -Ei "hit your limit|rate limit|usage limit|quota|resets?" "$LIMIT_SOURCE" | tail -n 1)
        BASE_WAIT_SECONDS=$(compute_wait_seconds_from_limit_line "$LIMIT_LINE")
        WAIT_SECONDS=$((BASE_WAIT_SECONDS + LIMIT_RESUME_BUFFER_SECONDS))
        RESUME_AT=$(date -v+"${WAIT_SECONDS}"S '+%Y-%m-%d %H:%M:%S' 2>/dev/null || date -d "+${WAIT_SECONDS} seconds" '+%Y-%m-%d %H:%M:%S' 2>/dev/null || echo "unknown")

        echo "[harness] Detected token/rate limit in $LIMIT_SOURCE."
        if [ -n "$LIMIT_LINE" ]; then
            echo "[harness] Limit hint: $LIMIT_LINE"
        fi
        stash_worktree_on_limit "$MODE" "$ITER" "$TIMESTAMP"
        echo "[harness] Sleeping ${WAIT_SECONDS}s (base=${BASE_WAIT_SECONDS}s + buffer=${LIMIT_RESUME_BUFFER_SECONDS}s), resume at ${RESUME_AT}."
        rm -f "$TMP_LOG"
        sleep "$WAIT_SECONDS"
        continue
    fi

    if [ "$CLAUDE_EXIT" -ne 0 ]; then
        echo "[harness] Claude exited with code $CLAUDE_EXIT. Continuing loop."
    fi

    rm -f "$TMP_LOG"
    sleep 5
done
