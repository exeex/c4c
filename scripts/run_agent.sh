#!/bin/bash
# Issue-fixing agent loop.
# Usage: ./scripts/run_agent.sh

set -euo pipefail

cd "$(dirname "$0")"
exec ./run_agent_common.sh AGENT_PROMPT_ISSUE.md
