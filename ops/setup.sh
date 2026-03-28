#!/bin/bash
# ops/setup.sh — one-time local setup for the git-triggered agent framework.
# Run once after cloning:  bash ops/setup.sh
set -euo pipefail

REPO_ROOT="$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"

echo "[setup] configuring git hooksPath -> .githooks"
git -C "$REPO_ROOT" config core.hooksPath .githooks

echo "[setup] ensuring hook is executable"
chmod +x "$REPO_ROOT/.githooks/post-commit"

echo "[setup] done — post-commit hook is active"
echo ""
echo "  Trigger manually:  python3 ops/orchestrator.py enqueue-event post-commit --head HEAD"
echo "  View event log:    python3 ops/orchestrator.py show-log"
echo "  View queue:        python3 ops/orchestrator.py show-queue"
