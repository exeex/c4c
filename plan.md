# RV64 Text-Only Fail-Closed Output Contract Runbook

Status: Active
Source Idea: ideas/open/307_rv64_text_only_fail_closed_output_contract.md

## Purpose

Repair the RV64 prepared-assembly output contract so a prepared module with an
emit-selected function cannot succeed with linkable-looking `.text`-only
assembly.

Goal: make the no-storage control `src/00094.c` emit a real RV64 `main`
symbol and body, or make unsupported prepared-module features fail with a clear
diagnostic before empty output is accepted as success.

## Core Rule

Repair the prepared-function emission contract semantically. Do not special
case `src/00094.c`, `main`, exact source text, or c-testsuite case names, and
do not fold string literals, aggregate globals, pointer globals, or floating
globals into this idea.

## Read First

- `ideas/open/307_rv64_text_only_fail_closed_output_contract.md`
- `build/rv64_c_testsuite_probe_v2/repair_order.md`
- `build/rv64_c_testsuite_probe_v2/classification.md`
- `build/rv64_c_testsuite_probe_v2/representative_evidence.md`
- `build/rv64_c_testsuite_probe_v2/classification_work/buckets/rv64_text_only_fail_closed.txt`
- `build/rv64_c_testsuite_probe_v2/classification_work/buckets/unused_extern_no_storage.txt`

If these probe artifacts are missing, regenerate only the narrow evidence
needed for this plan under `build/rv64_c_testsuite_probe_v2/` or another
non-root scratch location.

## Current Targets

- Minimal control: `src/00094.c`
- Prepared BIR fact to preserve: `prepared.func @main` exists and requires no
  global storage, string literal, pointer global, floating global, aggregate
  global, external call, or addressing feature.
- Required RV64 assembly signal: `.globl main`, `main:`, and an executable
  return path instead of only `.text`.
- Representative secondary hazards to recheck after the control path is real:
  `src/00024.c`, `src/00025.c`, `src/00045.c`, and `src/00119.c`.

## Non-Goals

- Do not implement string literal storage, external call ABI support, aggregate
  global storage, pointer global storage, floating global storage, scalar
  global storage, libc calls, or broad c-testsuite completion.
- Do not claim all 93 `rv64_text_only_fail_closed` cases are feature-complete
  because the no-storage control emits a function.
- Do not weaken existing backend expectations, mark the control unsupported,
  or accept empty `.text` as a supported output contract.
- Do not rewrite classification artifacts as a substitute for backend
  capability repair.
- Do not leave root-level scratch logs.

## Working Model

The repair order says all 93 undefined-main cases preserve `prepared.func
@main`, but emitted RV64 assembly is currently only:

```asm
    .text
```

The first capability to prove is independent of secondary feature buckets:
`src/00094.c` has no storage values, calls, or addressing demand, so it should
exercise only the prepared-function emission contract. Feature-heavy cases
should become more honest failures after this contract is repaired, not be
absorbed into this plan.

## Execution Rules

- Work in small packets tied to one step at a time.
- Keep `todo.md` as the packet progress and proof log; do not churn this
  runbook for routine executor updates.
- Prefer owner-surface repairs in the RV64 prepared-module/function emission
  path over testcase-shaped checks.
- Unsupported module features must fail closed before successful empty
  function output is reported.
- For code-changing steps, prove with a fresh build or compile check plus the
  delegated narrow backend subset. Escalate to representative rechecks before
  treating the idea as complete.
- Treat expectation downgrades, named-case shortcuts, or `.text`-only success
  as route failure.

## Ordered Steps

### Step 1: Inspect the Text-Only Emission Path

Goal: identify why RV64 prepared assembly succeeds with `.text` only while
prepared BIR still contains an emit-selected function.

Primary targets:

- RV64 prepared-module assembly emission path
- RV64 prepared-function selection and function-body emission path
- Existing diagnostics or unsupported-feature gates for prepared modules
- `src/00094.c` prepared BIR and emitted RV64 assembly

Actions:

- Reproduce or inspect the no-storage control so the current `.text`-only
  output and `prepared.func @main` input facts are visible.
- Trace the prepared-module route from module acceptance through global
  storage handling, function selection, and function body emission.
- Identify whether function emission is skipped, suppressed after an
  unsupported condition, or never selected.
- Record the smallest owner surface for the semantic repair in `todo.md`.

Completion check:

- `todo.md` names the concrete emission gate or missing handoff that produces
  `.text`-only success for the no-storage control, plus the next owner surface
  to change.
- No implementation files need to change for this inspection-only step.

### Step 2: Repair No-Storage Prepared Function Emission

Goal: make supported no-storage prepared functions emit a real RV64 symbol and
body instead of successful empty text.

Primary targets:

- RV64 prepared-function emission owner files
- Any narrow prepared-module orchestration needed to call the function emitter
  after module-level handling

Actions:

- Route emit-selected prepared functions through the existing or intended RV64
  function-body emitter for the supported no-storage case.
- Emit `.globl main`, a `main:` label, and a real return path for
  `src/00094.c` through semantic function emission, not a named-case shortcut.
- Preserve or improve fail-closed handling when unsupported module features are
  present.
- Keep feature-heavy storage, string, pointer, floating, and call support out
  of this step.

Completion check:

- The no-storage control no longer produces `.text` only.
- The changed route is based on prepared-function emission facts rather than
  source filename, function name special casing, or exact testcase shape.
- Fresh build or compile proof covers the touched backend path.

### Step 3: Add Focused Output-Contract Coverage

Goal: lock the minimal RV64 output contract so the old `.text`-only behavior
cannot return unnoticed.

Primary targets:

- Focused backend regression coverage for the no-storage prepared `main` case
- Any existing backend fixture or test harness that verifies emitted RV64 text

Actions:

- Add or update a narrow regression test that fails if a prepared no-storage
  `main` emits only `.text`.
- Assert the observable contract: `.globl main`, `main:`, and a real return
  path or equivalent executable function body signal.
- Avoid broad c-testsuite registration or expectation rewrites for the full
  93-case bucket.
- Keep unsupported secondary feature cases as diagnostics or recheck probes,
  not as passing coverage for this idea.

Completion check:

- The focused regression would have failed on the original `.text`-only
  output.
- The proof command includes the new or updated focused backend test.

### Step 4: Recheck Secondary Representatives and Fail-Closed Behavior

Goal: verify feature-heavy representatives no longer hide behind undefined
`main` and that unsupported module features do not silently erase emitted
functions.

Primary targets:

- `src/00024.c`
- `src/00025.c`
- `src/00045.c`
- `src/00119.c`
- Existing RV64 diagnostics or unsupported-feature reporting paths

Actions:

- Recheck representative secondary cases narrowly after the no-storage control
  emits a function.
- Record each representative's next visible failure mode in `todo.md`.
- Confirm unsupported globals, string address materialization, pointer globals,
  floating globals, or calls do not result in successful empty function output.
- Keep any newly exposed feature repair as follow-up idea work, not as hidden
  scope inside this plan.

Completion check:

- `todo.md` records the representative recheck outcomes and distinguishes
  this output-contract repair from later feature work.
- Empty `.text` success is no longer accepted for a prepared module with an
  emit-selected function.

### Step 5: Closure Readiness Check

Goal: decide whether the source idea is complete and ready for supervisor
review.

Actions:

- Verify the focused no-storage control and regression proof are green.
- Verify representative secondary rechecks have visible next failure modes or
  explicit unsupported diagnostics.
- Verify no tests or expectations were weakened.
- Verify no implementation claims broad string, aggregate, pointer, floating,
  scalar-global, libc-call, or full 93-case feature completion.
- Ask the supervisor for broader validation or reviewer scrutiny if the diff
  touches shared backend emission contracts beyond the narrow RV64 prepared
  route.

Completion check:

- The source idea acceptance criteria are satisfied, and remaining secondary
  feature work is clearly left to later open ideas.
