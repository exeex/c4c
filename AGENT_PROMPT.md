# Agent Task: Build C++ Frontend for tiny-c2ll (Rust-Parity First)

## Mission

You are working autonomously to build a C++-based frontend in `src/frontend_c/` that compiles C to **LLVM IR text**.

Current Python frontend (`src/frontend/c2ll.py`) is the stage0 oracle and must remain working.

Primary goal:

1. Implement C++ frontend modules incrementally (`lexer -> parser -> sema -> ir_builder -> driver`)
2. Follow `ref/claudes-c-compiler/src/frontend` Rust architecture and phase boundaries as the primary reference.
3. Keep output as LLVM IR string (debug-first).
4. Reach practical bootstrap flow:
   - stage0: Python frontend remains the behavior oracle for parity checks.
   - stage1: clang/clang++ builds runnable C++ frontend binary.
   - stage2: C++ frontend passes smoke + allowlist goals.
5. After C++ frontend is stable, plan and execute a controlled backport path to pure C.

## First Steps (every new iteration)

1. Read `plan.md` section `C++ Frontend First Plan (Rust Parity, Then Pure-C Backport)`.
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
- Keep architecture and naming close to `ref/claudes-c-compiler/src/frontend` unless there is a clear project-specific reason not to.

## Implementation Constraints

- Language split (current phase):
  - Core compiler logic is allowed in C++ under `src/frontend_c/`.
  - Keep module boundaries explicit (`lexer / parser / sema / ir_builder / driver`).
  - C++ usage limits:
    - `class` is allowed, but only `public` members (no `private`/`protected` sections).
    - Class inheritance is forbidden.
    - `virtual` is forbidden.
    - Operator overloading is allowed.
    - `std::vector` is allowed.
    - `std::string` is allowed.
    - Prefer smart pointers to model ownership close to Rust:
      - Default to `std::unique_ptr` for single ownership.
      - Use `std::shared_ptr` only when shared ownership is truly required.
      - Avoid raw `new`/`delete` in compiler core code.
  - Avoid overusing templates/metaprogramming; prefer straightforward, C-backportable code style.
  - New abstractions should include a brief note on how they can later map back to pure C.
- IR Builder:
  - Phase-1: string-based LLVM IR emission only.
  - Keep emitter interface clean so LLVM API backend can be added later.

## References

- `/tmp/ref/amacc` (minimalist compiler structure and self-hosting mindset)
- `ref/claudes-c-compiler/src/frontend` (Rust reference architecture to mirror)
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
- Commit created with milestone label (e.g., `feat(frontend_cxx): M1 lexer token stream skeleton`).
