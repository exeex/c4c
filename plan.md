# AArch64 LLVM Stack Helper Symbol Runbook

Status: Active
Source Idea: ideas/open/279_aarch64_c_testsuite_llvm_stack_helper_symbols.md

## Purpose

Repair the AArch64 backend failure family where backend-route c-testsuite cases
emit unresolved LLVM stack helper symbols such as `llvm.stacksave`,
`llvm.dynamic_alloca.i8`, and `llvm.stackrestore`.

## Goal

Make the backend route either lower the stack helper family correctly or reject
it at the truthful owner layer without leaving unresolved LLVM helper symbols in
generated output.

## Core Rule

Do not add filename-specific handling for `00207.c`, weaken expectations, or
count `[RUNTIME_UNAVAILABLE]` as pass evidence.

## Read First

- `ideas/open/279_aarch64_c_testsuite_llvm_stack_helper_symbols.md`
- Recent AArch64 backend scan evidence in `test_after.log`, if present
- `tests/c/external/c-testsuite/src/00207.c`
- Stack allocation, dynamic alloca, and intrinsic-lowering paths that feed the
  backend route

## Current Targets

- Representative c-testsuite case:
  `c_testsuite_aarch64_backend_src_00207_c`
- Known unresolved helper names:
  `llvm.stacksave`, `llvm.dynamic_alloca.i8`, `llvm.stackrestore`
- Backend proof should be semantic and independent of the c-testsuite filename.

## Non-Goals

- Do not configure or require an AArch64 runtime runner for this plan.
- Do not reopen completed call-boundary move or `.LBB...` label work unless
  those exact diagnostics return.
- Do not rewrite broad ABI, call lowering, or frontend paths unless tracing
  proves they own the helper-symbol failure.
- Do not change c-testsuite expectations, allowlists, unsupported
  classifications, or stage accounting to claim progress.

## Working Model

The targeted failure is a backend-symbol or lowering ownership gap. The route
must identify where LLVM-flavored stack helper references enter the backend
path, then either lower them into AArch64-supported behavior or produce a
truthful backend diagnostic before assembly/link output contains unresolved
helper symbols.

## Execution Rules

- Keep routine progress in `todo.md`.
- Add focused backend proof before implementing the repair when feasible.
- Prefer a general helper-family rule over exact-string or named-testcase
  handling.
- Preserve stage-specific classification in the AArch64 c-testsuite backend
  route.
- After implementation, run backend tests and the focused AArch64 c-testsuite
  subset selected for this failure family.

## Steps

### Step 1: Reproduce And Classify Helper-Symbol Failure

Goal: prove the current `00207.c` failure and capture the exact owner layer.

Primary target: AArch64 backend c-testsuite subset for `00207.c`.

Actions:

- Run the focused AArch64 backend c-testsuite command for
  `c_testsuite_aarch64_backend_src_00207_c`.
- Record whether the failure is backend emission, assembler/link, or runtime
  unavailable.
- Capture the unresolved helper names and the generated-output location where
  they appear.
- Update `todo.md` with the proof command, classification, and next likely
  source files.

Completion check:

- `todo.md` names the representative failure, helper symbols, classification,
  and a narrow proof command for the next packet.

### Step 2: Trace Helper Origins And Ownership

Goal: identify where stack-save, dynamic-allocation, and stack-restore facts
enter the backend path and which layer should own their lowering or rejection.

Primary target: frontend-to-backend lowering, BIR/MIR preparation, and AArch64
backend symbol emission surfaces discovered in Step 1.

Actions:

- Trace `00207.c` through the backend route far enough to locate helper
  introduction.
- Determine whether the helpers represent supported dynamic stack behavior or
  unsupported semantics that need a truthful diagnostic.
- Record whether the repair belongs before MIR, during AArch64 lowering, or in
  symbol/emitter validation.
- Avoid changing implementation files during this trace-only packet unless the
  supervisor explicitly delegates implementation.

Completion check:

- `todo.md` records the owner layer, relevant functions/files, and the focused
  backend test shape needed before repair.

### Step 3: Add Focused Backend Proof

Goal: create a backend-level test that fails for the old helper-symbol path
without depending on the `00207.c` filename.

Primary target: the narrow backend test file best matching the owner layer from
Step 2.

Actions:

- Add a semantic backend test for the helper-family behavior or diagnostic.
- Ensure the test fails on the old path for unresolved helper emission or for
  missing truthful rejection.
- Run `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Preserve the failing old-path proof in `test_after.log`.

Completion check:

- The focused backend proof exists and fails for the old behavior for the
  intended reason.

### Step 4: Implement General Helper Handling

Goal: repair the helper-symbol family through a general lowering, rewrite, or
truthful rejection rule.

Primary target: the owner files identified by Step 2.

Actions:

- Implement the smallest general rule that handles the helper family.
- Do not key behavior on `00207.c` or one generated symbol occurrence.
- Preserve existing completed AArch64 fixes for call-boundary moves and block
  labels.
- Run backend proof and the focused c-testsuite subset for `00207.c`.

Completion check:

- `^backend_` tests pass.
- The focused helper-symbol backend proof passes.
- The `00207.c` AArch64 backend c-testsuite case no longer emits unresolved
  `llvm.stacksave`, `llvm.dynamic_alloca.i8`, or `llvm.stackrestore`
  references.

### Step 5: Broader AArch64 Backend Scan

Goal: confirm the targeted helper-symbol family is gone from the broader
AArch64 backend scan and classify remaining failures truthfully.

Primary target: `ctest --test-dir build-aarch64-scan --output-on-failure -L aarch64_backend`.

Actions:

- Configure and build the AArch64 backend scan route.
- Run the broad `aarch64_backend` label.
- Count failure classes without treating `[RUNTIME_UNAVAILABLE]` as pass
  evidence.
- Record any new separate backend families as follow-up ideas rather than
  expanding this runbook.

Completion check:

- Broad scan evidence records zero remaining unresolved LLVM stack helper
  symbol diagnostics for this family.
- `todo.md` identifies remaining blockers by truthful owner layer.

### Step 6: Completion Review

Goal: decide whether the source idea criteria are satisfied.

Actions:

- Compare the latest proof against the acceptance criteria in the source idea.
- Confirm focused backend proof exists and the AArch64 subset reclassified the
  targeted failure to the next truthful owner layer.
- Confirm no runtime-unavailable result was counted as pass evidence.
- Ask the plan owner to close only if the source idea itself is complete.

Completion check:

- The supervisor has enough evidence to request plan-owner closure review or
  to split any remaining distinct family into a new open idea.
