# Backend Regex Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md
Activated from: remaining open umbrella inventory after focused idea 308 closed

## Purpose

Continue the backend-regex umbrella inventory after the closed focused AArch64
owners, using the post-308 evidence as the starting point for the next
classification pass.

## Goal

Classify the remaining backend-regex failures well enough to split the next
focused semantic repair idea, or record why no focused owner is ready.

## Core Rule

This umbrella runbook is inventory and classification only. Do not implement
repairs or improve counts by changing expectations, allowlists, unsupported
classifications, CTest registration, timeout policy, runner behavior,
proof logs, or testcase-specific matching.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `test_before.log`, noting that the current canonical proof log is the
  focused post-308 proof for `00189.c`, not a fresh broad backend-regex
  inventory
- `tests/c/external/c-testsuite/src/00189.c`
- Generated AArch64 artifacts under
  `build/c_testsuite_aarch64_backend/src/00189.c.s`
- Closed-owner boundaries 285 through 308 only as historical context when a
  residual appears to overlap a completed owner

## Current Scope

- Main-build backend regex inventory from `/workspaces/c4c/build`
- Local backend/unit/CLI failures if any remain in the backend regex scope
- `c_testsuite_aarch64_backend_*` compile, assembler, runtime, mismatch,
  crash, timeout, and output-storm residuals
- Candidate focused owner splits for semantic backend capability gaps
- The `00189.c` runtime segmentation fault, which returned to this umbrella
  after idea 308 removed the old externally binding data-symbol PIC relocation
  failure

## Non-Goals

- No implementation edits under this umbrella.
- No expectation, allowlist, unsupported, registration, timeout, runner,
  proof-log, or CTest-registration edits.
- No reopening closed owners 285 through 308 from failing counts alone.
- No monolithic repair owner for all `ctest -R backend` failures.
- No broad runtime rerun without timeout handling and stale-process cleanup.

## Working Model

- Idea 308 closed the old `00189.c` linker failure:
  `R_AARCH64_ADR_PREL_PG_HI21` against `stdout@@GLIBC_2.17`. Current
  generated assembly uses GOT materialization for `stdout`.
- The remaining `00189.c` result is `RUNTIME_NONZERO exit=Segmentation fault`.
  The current assembly shows call-argument/call-target corruption candidates:
  `fred` moves the first argument through `w0`, `main` loads `stdout` and a
  function pointer, then overwrites argument/callee registers before
  `blr x21`.
- Treat `00189.c` as a runtime/call-argument classification packet, not as a
  reopened extern-data-symbol/PIC owner.
- The latest broad backend-regex inventory before post-308 focused work
  recorded 352 selected tests, 306 passed, and 46 failed, all under
  `c_testsuite_aarch64_backend_*`. Treat that as historical context until a
  supervisor-delegated fresh broad capture or accepted broad log is available.
- Separate source buckets before proposing owner splits: local backend tests,
  AArch64 compile/machine-printer failures, assembler failures, semantic
  `lir_to_bir` admission failures, runtime nonzero exits, runtime mismatches
  or crashes, timeouts, and output storms.
- A focused split must name a semantic owner and the evidence for that owner.
  Shared filenames, shared pass-count movement, or one diagnostic string alone
  is not enough.

## Execution Rules

- Keep routine classification notes in `todo.md`.
- If a focused semantic owner is found, create a separate `ideas/open/*.md`
  owner and switch lifecycle state before implementation begins.
- If the best result is still uncertain, leave idea 295 active and record the
  blocking evidence gap in `todo.md`.
- Preserve closed-owner boundaries. Reopen or supersede a closed owner only
  with generated-code or proof evidence that contradicts its closure boundary.
- For any broad runtime proof, use bounded commands with stale-process cleanup
  as directed by the supervisor.

## Ordered Steps

### Step 1: Classify 00189 Runtime/Call-Argument Crash

Goal: establish the semantic owner, if any, for the post-308 `00189.c`
runtime segmentation fault.

Primary targets: `tests/c/external/c-testsuite/src/00189.c`,
`build/c_testsuite_aarch64_backend/src/00189.c.s`, and `test_before.log`

Actions:

- Confirm the old non-PIC relocation failure is absent from current generated
  assembly and proof output.
- Inspect the generated `00189.c` call sequence for argument preparation,
  function-pointer call target preservation, varargs setup, and clobbered
  caller-saved registers.
- Compare the failure shape against parked runtime/call-boundary buckets from
  idea 295 before naming a focused owner.
- Do not edit implementation, tests, expectations, runner policy, CTest
  registration, or proof logs.

Completion check:

- `todo.md` records whether `00189.c` is a focused call-argument/function-
  pointer owner candidate, belongs to a broader runtime bucket, or needs a
  narrower supervisor-approved probe before splitting.

### Step 2: Reconcile With Remaining Runtime Buckets

Goal: decide whether the `00189.c` evidence shares a semantic owner with other
runtime nonzero, mismatch, or crash residuals.

Primary target: `build/c_testsuite_aarch64_backend/`

Actions:

- Compare generated assembly or narrow diagnostics for likely call-boundary
  neighbors before grouping them with `00189.c`.
- Keep runtime mismatch, runtime nonzero, crash, timeout, and output-storm
  cases separate unless generated-code evidence proves shared ownership.
- Compare likely overlaps against closed owners 285 through 308 before
  proposing any reopen or related split.

Completion check:

- `todo.md` contains a bucketed runtime/call-boundary note with explicit parked
  buckets for uncertain cases.

### Step 3: Choose the Next Focused Owner

Goal: decide whether a tractable semantic repair family is ready to split.

Actions:

- Prefer the crispest bucket with a shared semantic backend capability and
  narrow proof surface.
- Reject testcase-shaped owners, exact filename matching, instruction-string
  matching, and expectation-only progress.
- If no owner is ready, record the missing evidence and the next narrow probe
  needed for classification.

Completion check:

- Either a focused owner candidate is named with tests, semantic scope,
  boundaries, and proof command guidance, or `todo.md` explains why the
  umbrella must stay active for more classification.

### Step 4: Split and Switch Before Implementation

Goal: create lifecycle state for implementation only after classification.

Actions:

- When Step 3 identifies a focused semantic owner, create a new
  `ideas/open/*.md` file with goal, scope, out-of-scope items, acceptance
  criteria, and reviewer reject signals.
- Add a durable deactivation note to idea 295 summarizing the split decision,
  proof scope, and remaining parked buckets.
- Switch active lifecycle state from this umbrella to the focused owner before
  any implementation work begins.

Completion check:

- Active lifecycle state no longer points to idea 295 when implementation is
  ready to start, and idea 295 preserves the remaining inventory context.
