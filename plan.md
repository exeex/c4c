# RV64 Prepared Object Shape Diagnostics Runbook

Status: Active
Source Idea: ideas/open/355_rv64_prepared_object_shape_diagnostics.md

## Purpose

Replace the coarse RV64 object-route `unsupported prepared module shape`
failure with structured rejection reasons that consume the shared prepared
object consumer contract and add target-specific RV64 evidence.

## Goal

Make representative prepared-shape failures report stable semantic buckets
from backend/object-route diagnostics, without claiming capability repair.

## Core Rule

Do not define shared BIR/prepared semantics inside RV64 object emission. RV64
diagnostics may consume shared prepared-object taxonomy and append
target-specific evidence only.

## Read First

- `ideas/open/355_rv64_prepared_object_shape_diagnostics.md`
- `ideas/closed/359_bir_prepared_object_consumer_contract_completion.md` for
  the completed shared consumer-contract dependency
- `src/backend/backend.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_module_emit.cpp`

## Current Scope

- Structured RV64 prepared-object rejection reasons.
- Representative diagnostic coverage for the prepared-shape buckets named by
  the source idea.
- Log-visible evidence that lets the gcc torture backend scan bucket failures
  without re-dumping prepared BIR.

## Non-Goals

- Do not implement backend capability repairs for CFG, globals, strings,
  ABI, local memory widths, or varargs.
- Do not weaken the RV64 gcc torture runner or mark cases unsupported.
- Do not map testcase names directly to diagnostic buckets.
- Do not build a target-only semantic classifier that scans prepared BIR to
  rediscover facts owned by the shared consumer contract.

## Working Model

RV64 object admission should return or collect structured rejection detail
instead of collapsing every unsupported prepared shape into `std::nullopt` plus
one generic diagnostic. Shared prepared-object contract diagnostics should
identify target-independent causes; RV64 code should translate those into
user-facing object-route evidence where needed.

## Execution Rules

- Keep source intent in the idea file unchanged unless a durable route issue is
  discovered.
- Use `todo.md` for packet status and proof notes.
- Keep implementation slices narrow enough for build plus focused backend
  tests.
- Preserve the generic diagnostic as a prefix if needed, but make the specific
  bucket visible in the same failing command.
- Treat expectation downgrades, scan filtering, and testcase-shaped matching
  as route failures.

## Step 1: Inspect Existing Rejection Paths

Goal: identify where prepared-object rejection loses diagnostic detail.

Primary targets:

- `src/backend/backend.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_module_emit.cpp`
- existing shared prepared-object consumer contract helpers and diagnostics

Actions:

- Trace the path from `--codegen obj --target riscv64-linux-gnu` through
  prepared module admission and RV64 object emission.
- Identify every `std::nullopt`, boolean false, or generic diagnostic path
  that can produce `unsupported prepared module shape`.
- Record which paths already have shared consumer-contract diagnostics and
  which need RV64 target evidence.

Completion check:

- The executor can name the minimal diagnostic insertion points and the shared
  taxonomy helpers to consume before editing behavior.

## Step 2: Add Structured RV64 Rejection Reasons

Goal: make RV64 prepared-object failures carry stable stage-oriented buckets.

Primary targets:

- RV64 prepared object admission and object emission helpers
- shared prepared-object diagnostic/result types when already available

Actions:

- Replace bare unsupported returns in the RV64 object route with structured
  rejection detail or an equivalent diagnostic collector.
- Consume shared taxonomy for block-entry copies, select carriers, value-home
  transfers, frame ownership, and missing prepared contract pieces.
- Add RV64-specific buckets only for target evidence, such as unsupported
  module data, unsupported target opcode, local memory width, declaration entry
  handling, or target ABI hook gaps.
- Keep the old generic text as context if callers or tests rely on it.

Completion check:

- Representative failing object commands include a specific bucket in addition
  to any generic prefix, and existing passing RV64 object cases still pass.

## Step 3: Add Representative Diagnostic Tests

Goal: prove the diagnostics are semantic and stable across the named buckets.

Representative cases:

- `src/20000113-1.c`: multi-block control flow.
- `src/20000224-1.c`: global load/store and global address data.
- `src/20000112-1.c`: string constants.
- `src/20001203-1.c`: non-i32 local memory widths.
- `src/20030216-1.c`: declaration control-flow entry.

Actions:

- Add focused tests that exercise backend diagnostics without relying on
  testcase-name matching.
- Prefer narrow compiler/backend tests where they isolate the prepared shape
  better than full gcc torture execution.
- Ensure assertions match stable bucket names, not incidental source line text
  or one-off full diagnostic formatting.

Completion check:

- Focused tests fail before the diagnostic change and pass after it, with no
  expectation downgrades or skipped coverage.

## Step 4: Prove Scan-Bucket Visibility

Goal: show the full or representative gcc torture backend scan can bucket
prepared-shape failures from logs.

Actions:

- Run the narrow command for at least one representative case:

```sh
build/c4cll -I tests/c/external/gcc_torture --codegen obj \
  --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000113-1.c \
  -o /tmp/rv64-prepared-shape.o
```

- Run the supervisor-delegated proof command, expected to be:

```sh
CASE_TIMEOUT_SEC=20 MAX_CASES=0 scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

- Escalate to broader backend CTest coverage if shared diagnostic plumbing or
  backend entry points were touched.

Completion check:

- The scan logs contain actionable prepared-object buckets without requiring
  `--dump-prepared-bir`, and touched backend tests remain green.

## Final Acceptance Check

- Representative cases each report a stable semantic rejection bucket.
- The full scan can be bucketed from logs without re-dumping prepared BIR.
- Existing passing RV64 object runtime cases still pass.
- RV64 diagnostics consume or mirror the shared 359 taxonomy instead of
  inventing divergent target-only semantics.
- The completed slice is reviewed for testcase-overfit and expectation
  downgrades before commit.
