#!/bin/bash
# Agent loop: runs Claude Code in infinite loop
# Usage: ./scripts/run_agent.sh
# Stop with Ctrl+C

cd "$(dirname "$0")/.."

echo "[harness] Starting agent loop. Stop with Ctrl+C."
echo "[harness] Logs → build/agent_state/agent_logs/"
mkdir -p build/agent_state/agent_logs

ITER=0
while true; do
    ITER=$((ITER + 1))
    COMMIT=$(git rev-parse --short=6 HEAD 2>/dev/null || echo "no-git")
    LOGFILE="build/agent_state/agent_logs/iter_${ITER}_${COMMIT}.log"
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')

    echo "[harness] === Iteration $ITER at $TIMESTAMP (commit: $COMMIT) ==="
    echo "[harness] Log: $LOGFILE"

    # Run Claude Code with the agent prompt
    claude --dangerously-skip-permissions \
           -p "$(cat AGENT_PROMPT.md)" \
           --model claude-sonnet-4-6 \
           2>&1 | tee "$LOGFILE"

    # Show current score after each iteration
    echo "[harness] === Iteration $ITER done. Current score: ==="
    cat build/agent_state/last_result.txt 2>/dev/null || echo "(no result yet)"
    echo ""

    # Brief pause between iterations
    sleep 5
done
