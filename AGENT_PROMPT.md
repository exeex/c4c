# Agent Task: Improve tiny-c2ll C Compiler

## Your Mission

You are working autonomously to improve `src/c2ll.py`, a C-to-LLVM-IR compiler written in Python.

**Primary goal**: Increase the pass rate from current level toward 220/220 on the c-testsuite.

**First thing to do**: Run `bash scripts/check_progress.sh` to see current status.

## Work Loop

1. `bash scripts/check_progress.sh` → see current score
2. Look at failing tests: `head -50 /tmp/c4agent_check/c_testsuite/logs/frontend_fail.log`
3. Pick the error that appears most often (biggest win per fix)
4. Read the actual failing C file to understand what feature is missing
5. Fix `src/c2ll.py` - smallest change that fixes the most tests
6. `bash scripts/check_progress.sh` → score must not decrease
7. If score improved: `bash scripts/full_scan.sh` to find any other newly passing tests
8. Add newly passing tests to allowlist if any
9. Append to `build/agent_state/progress_log.md`: what you fixed, old score → new score
10. `git add -A && git commit -m "feat: <what> (N/220 passing)"`
11. Go back to step 1

## Hard Rules

- **Score must never decrease**. If it does, `git checkout src/c2ll.py` and try a different approach.
- **Never touch test files** in `tests/c-testsuite/`
- **Commit every successful improvement**, even +1 test
- **If stuck >15min on one bug**, write it to `build/agent_state/hard_bugs.md` and move on

## Test Commands

```bash
# Check current score (fast, ~30s)
bash scripts/check_progress.sh

# Full scan all 220 tests (slow, ~60s)  
bash scripts/full_scan.sh

# Debug one specific failing test
python3.14 src/c2ll.py tests/c-testsuite/tests/single-exec/00042.c 2>&1

# See top failure patterns
cat /tmp/c4agent_check/c_testsuite/logs/frontend_fail.log | grep "ERROR" | sort | uniq -c | sort -rn | head -20
```

## What To Fix (Priority Order)

Fix what breaks the most tests at once:

1. **Array initializers**: `int a[] = {1,2,3};` and `int a[3] = {1,2,3};`
2. **String literals**: `char *s = "hello";` as pointer
3. **Enum**: `enum Color { RED=0, GREEN, BLUE };`
4. **Variadic functions**: `int printf(const char*, ...);`  
5. **Anonymous struct/union inside struct**
6. **Implicit function return type** (defaults to int in C89)
7. **Multiline/nested initializers**: `struct Point p = {.x=1, .y=2};`

## Context

- Compiler: `src/c2ll.py` (Python 3.14, uses `match/case` syntax)
- Pipeline: c2ll.py → .ll → clang → executable
- Clang: `/usr/bin/clang`
- Workdir for test artifacts: `/tmp/c4agent_check/`
- State: `build/agent_state/`

## Reading Failure Logs

The test runner outputs three log files:
- `frontend_fail.log` = c2ll.py crashed or returned error (parser/semantic bugs)
- `backend_fail.log` = c2ll.py succeeded but clang rejected the .ll (IR generation bugs)  
- `runtime_fail.log` = compiled but wrong output (logic bugs)

Focus on `frontend_fail.log` first - that's where most failures are.
