# Agent Task: Build C Frontend for tiny-c2ll (Self-Hosting Path)

## Mission

You are working autonomously to build a C-based frontend in `src/frontend_c/` that compiles C to **LLVM IR text**.

Current Python frontend (`src/frontend/c2ll.py`) is the stage0 oracle and must remain working.

Primary goal:

1. Implement C frontend modules incrementally (`lexer -> parser -> sema -> ir_builder -> driver`)
2. Keep output as LLVM IR string (debug-first)
3. Reach bootstrap flow:
   - stage0: existing Python compiler can compile C frontend sources
   - stage1: clang links runnable C frontend binary
   - stage2: C frontend compiles itself (self-host smoke)

## First Steps (every new iteration)

1. Read `plan.md` section `C Rewrite Frontend Plan (Self-Hosting, LLVM IR Text Output First)`.
2. Check repo status: `git status --short`.
3. Pick exactly one smallest milestone slice to complete.

## Work Loop

1. Implement one focused change in `src/frontend_c/` (or supporting build/test wiring).
2. Run the smallest relevant test/smoke command.
3. If pass, record progress in `build/agent_state/progress_log.md`.
4. Commit with clear milestone-oriented message.
5. Continue with next smallest slice.

## Hard Rules

- Do not break existing Python frontend path (`src/frontend/c2ll.py`) while bootstrapping.
- Do not edit `tests/c-testsuite/` contents.
- Keep changes minimal and verifiable; avoid large untested rewrites.
- If blocked for >15 min, write blocker and next hypothesis to `build/agent_state/hard_bugs.md`, then switch to another slice.
- Prefer deterministic commands and explicit file paths in logs.

## Implementation Constraints

- Language split:
  - Core compiler logic in C files under `src/frontend_c/`.
  - `utils.cpp` allowed only for complex helper utilities (e.g., string/path helpers).
  - `utils.cpp` must export C ABI (`extern "C"`) for C callers.
- IR Builder:
  - Phase-1: string-based LLVM IR emission only.
  - Keep emitter interface clean so LLVM API backend can be added later.

## References

- `/tmp/ref/amacc` (minimalist compiler structure and self-hosting mindset)
- `/tmp/ref/claudes-c-compiler` (phase boundaries and pipeline contracts)
- Existing oracle implementation:
  - `src/frontend/lexer.py`
  - `src/frontend/parser.py`
  - `src/frontend/sema.py`
  - `src/frontend/c2ll.py`

## Suggested Commands

```bash
# Existing regression baseline
python3.14 tests/run_tests.py --compiler src/frontend/c2ll.py --workdir /tmp/tiny-c2ll-test --clang /usr/bin/clang

# c-testsuite allowlist baseline
bash scripts/check_progress.sh

# Full scan (when needed)
bash scripts/full_scan.sh
```

## Exit Criteria for This Prompt

- New or improved milestone artifacts exist in `src/frontend_c/` with passing smoke checks, and
- Progress is documented in `build/agent_state/progress_log.md`, and
- Commit created with milestone label (e.g., `feat(frontend_c): M1 lexer token stream skeleton`).
