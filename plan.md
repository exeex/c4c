# RV64 Object Route ABI and Width Edge Coverage Runbook

Status: Active
Source Idea: ideas/open/358_rv64_object_route_abi_width_edges.md

## Purpose

Drive the remaining small RV64 prepared-object ABI and width edge buckets
through layer-correct repairs or structured unsupported boundaries.

## Goal

Classify and repair the in-scope mixed edge cases without moving upstream
prepared semantics into RV64 object emission and without testcase-shaped
shortcuts.

## Core Rule

Consume prepared BIR/MIR facts that already exist; do not reconstruct missing
call, CFG, local-memory, or ABI semantics in target object emission.

## Read First

- `ideas/open/358_rv64_object_route_abi_width_edges.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `tests/c/external/gcc_torture/allowlist.txt`
- Representative logs under `build/rv64_gcc_c_torture_backend/` when present

## Current Targets

- `src/20010119-1.c`: general call lowering shape.
- `src/20001203-1.c`: i64/i8 local stack memory stores.
- `src/20030216-1.c`: declaration control-flow entry.
- `src/20030125-1.c`: FPR/floating-point value shape.
- `src/920410-1.c`: local memory addressing/home edge.

## Non-Goals

- Do not route `src/20030914-2.c` or `src/920908-1.c` back into this idea.
- Do not implement variadic, vararg, or `va_arg` semantics here.
- Do not replace the whole object-route architecture.
- Do not implement data-section/global/string support here.
- Do not weaken gcc_torture runtime checks or mark cases unsupported as
  progress.

## Working Model

- Declaration/control-flow entries should be represented as declarations or
  undefined symbols, not emitted as empty text functions.
- Local memory width and home support must follow prepared memory metadata and
  value-home transfer plans.
- Simple call and FPR ABI support is target ABI work only when prepared
  call/value facts already exist.
- Each representative may either execute correctly or advance to a later
  structured unsupported boundary owned by another idea.

## Execution Rules

- Keep changes small and bucket-oriented.
- Prefer focused backend tests before representative gcc_torture probes.
- Use structured diagnostics for still-unsupported shapes.
- Reject changes that hard-code testcase names or source patterns.
- Record representative outcomes in `todo.md`; only update this runbook for a
  true route change.

## Steps

### Step 1: Audit Mixed Edge Boundaries

Goal: identify the current first unsupported boundary for each representative
case and route it to the correct layer.

Primary targets:

- `build/rv64_gcc_c_torture_backend/<case>/case.log`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- prepared BIR/MIR dumps when needed

Actions:

- Run or load the five representative cases from the source idea.
- Record the current diagnostic or runtime result for each case.
- Decide which case is the safest first implementation packet.
- Identify any representative that should be split into a separate idea rather
  than repaired under 358.

Completion check:

- `todo.md` names the first implementation packet, the expected focused test
  target, and the representative proof command.

### Step 2: Repair Declaration Entry Handling

Goal: ensure undefined declarations are represented as symbols/declarations
without emitting invalid empty text bodies.

Primary targets:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Actions:

- Consume existing prepared declaration/function-body facts.
- Add or tighten structured diagnostics for unsupported declaration shapes.
- Verify `src/20030216-1.c` moves past the declaration-control-flow boundary
  or reaches a later structured boundary.

Completion check:

- Focused object-emission tests pass.
- The representative proof documents the new boundary or successful execution.

### Step 3: Repair Local Memory Width and Home Edges

Goal: lower local memory loads/stores for i8/i16/i64/pointer widths and
supported local-memory homes according to prepared metadata.

Primary targets:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Actions:

- Use semantic width and home metadata, not source names.
- Preserve sign/zero extension behavior required by existing prepared values.
- Verify `src/20001203-1.c` and `src/920410-1.c` move past their current
  local-memory boundaries or reach later structured diagnostics.

Completion check:

- Focused backend tests cover each new supported width/home class.
- Representative probes document successful execution or later boundaries.

### Step 4: Route Simple Call and FPR ABI Edges

Goal: support only call and FPR shapes whose prepared semantics are explicit,
and classify the rest with precise diagnostics or follow-up ideas.

Primary targets:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Actions:

- Inspect `src/20010119-1.c` and `src/20030125-1.c` prepared facts.
- Implement narrow ABI support only when argument, result, and register-class
  facts are already available.
- Create or request a new source idea if the missing behavior is upstream.

Completion check:

- Focused backend tests pass.
- Representative outcomes are documented without weakening expectations.

### Step 5: Milestone Validation and Closure Decision

Goal: prove the mixed-edge idea is complete or identify the remaining
non-overlapping follow-up work.

Actions:

- Run the five-case representative allowlist:

```sh
tmp=$(mktemp); printf '%s\n' \
  src/20010119-1.c \
  src/20001203-1.c \
  src/20030216-1.c \
  src/20030125-1.c \
  src/920410-1.c > "$tmp"; \
CASE_TIMEOUT_SEC=20 ALLOWLIST="$tmp" scripts/check_progress_rv64_gcc_c_torture_backend.sh; \
rc=$?; rm -f "$tmp"; exit $rc
```

- Run the focused backend tests used by prior steps.
- Decide whether idea 358 can close or whether remaining boundaries require
  separate ideas.

Completion check:

- Every representative either executes correctly or has a documented later
  structured unsupported bucket.
- No progress claim depends on skipped runtime checks, expectation downgrades,
  or testcase-specific matching.
