#!/bin/bash
# Plan-execution agent loop.
# Usage: ./scripts/run_agent_plan.sh

set -euo pipefail

cd "$(dirname "$0")"
exec ./run_agent_common.sh AGENT_PROMPT_PLAN.md
