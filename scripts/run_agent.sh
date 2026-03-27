#!/bin/bash
# Agent loop entrypoint.
# Usage: ./scripts/run_agent.sh [--cli {claude|codex|auto}] [prompt-file]

set -euo pipefail

cd "$(dirname "$0")"
exec python3 ./run_agent_common.py "$@"
