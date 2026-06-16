# Backend Remaining AArch64 Load Local-Memory Failures Runbook

Status: Active
Source Idea: ideas/open/293_backend_remaining_aarch64_load_local_memory_failures.md

## Purpose

Triage and repair the remaining `^backend_` red tests after the 286-292 prepared/interface cleanup sequence.

Goal: classify the three current AArch64 backend failures, repair one coherent semantic BIR `load local-memory` family if possible, and leave precise follow-up work when the failures split.

## Core Rule

Repair semantic BIR local-memory admission generally. Do not make named-fixture, payload-size, function-name, expected-output, or target-specific shortcuts.

## Read First

- `ideas/open/293_backend_remaining_aarch64_load_local_memory_failures.md`
- Existing tests for:
  - `backend_codegen_route_aarch64_byval_payload_8_to_13_stack_overflow`
  - `backend_codegen_route_aarch64_byval_payload_9_to_14_stack_overflow`
  - `backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value`
- Backend semantic BIR admission and local-memory load lowering code
- Recent 286/288/291/292 changes if needed to preserve their behavior

## Current Targets

- Reproduce the exact three-test subset.
- Capture the first rejected LIR/BIR shape for the byval stack-overflow family.
- Capture the first rejected LIR/BIR shape for the pointer-value writeback family.
- Determine whether the two families share one admission gap.
- Repair the smallest coherent semantic rule that admits a general local-memory load shape.
- Add focused backend proof for the admitted shape without relying only on the named failing fixtures.

## Non-Goals

- Do not perform broad AArch64 ABI or prepared call-plan rewrites.
- Do not retire public prepared lookup surfaces.
- Do not update required or forbidden assembly snippets as capability progress.
- Do not bypass semantic BIR through prepared-only state, printed BIR, rendered call-argument text, or target-specific acceptance.
- Do not weaken opaque compatibility gates from ideas 289/290 or fail-closed behavior from ideas 286-292.
- Do not claim full backend health without a fresh `^backend_` proof.

## Working Model

The current backend sweep is 177/180 passing. The remaining three failures stop before assembly emission because semantic BIR rejects a `load local-memory` shape:

- the two byval payload stack-overflow cases fail in `same_bytes`
- the pointer-value named scalar writeback case fails in `main`

Treat these as failure-family evidence, not as names to match in code.

## Execution Rules

- Start from reproduction and shape capture before editing implementation code.
- Prefer one coherent semantic admission rule over separate local patches when the captured shapes support that.
- If the byval and pointer-value failures split, repair one coherent family and record the other as follow-up rather than broadening the active runbook by guesswork.
- Each code-changing step needs fresh build or compile proof plus the delegated focused test subset.
- Escalate to `ctest --test-dir build -j --output-on-failure -R '^backend_'` for close or milestone proof.
- Keep routine progress and packet proof in `todo.md`.

## Ordered Steps

### Step 1: Reproduce and classify the three failures

Goal: capture the exact failing semantic shapes before changing code.

Primary target: the three named backend tests from the source idea.

Actions:

- Run the exact three-test subset delegated by the supervisor.
- Capture the first rejected LIR/BIR shape for the two byval stack-overflow failures.
- Capture the first rejected LIR/BIR shape for the pointer-value writeback failure.
- Decide whether the failures share one local-memory admission gap or represent separate families.

Completion check:

- `todo.md` records the proof command, pass/fail outcome, and a concise family classification for all three tests.

### Step 2: Locate the semantic BIR admission boundary

Goal: identify the smallest backend rule that rejects the captured shape.

Primary target: semantic BIR validation/admission code for local-memory loads.

Actions:

- Trace the captured shape from lowering into the semantic BIR rejection point.
- Identify the existing local-memory load shapes that are already admitted.
- Compare the rejected shape against nearby accepted shapes to define the missing general rule.
- Confirm the route does not rely on rendered `alignstack(...)`, prepared-only data, printed BIR, or compatibility-gate weakening.

Completion check:

- `todo.md` names the rejecting helper or rule, the intended generalization, and any split-family follow-up needed before implementation.

### Step 3: Implement one coherent semantic admission repair

Goal: admit the smallest general local-memory load shape for one coherent failure family.

Primary target: backend semantic BIR admission/lowering code identified in Step 2.

Actions:

- Implement the semantic admission change for the captured family.
- Keep the rule shape-based and capability-oriented, not fixture-oriented.
- Preserve existing fail-closed behavior for unsupported shapes.
- Avoid broad AArch64 ABI, prepared call-plan, or public lookup surface rewrites.

Completion check:

- The code builds.
- The delegated focused subset improves monotonically for the repaired family.
- No expectation downgrade or named-case shortcut is present in the diff.

### Step 4: Add focused backend proof for the admitted shape

Goal: prove the repaired shape independently enough to reject testcase overfit.

Primary target: backend tests covering semantic BIR local-memory load admission.

Actions:

- Add or adjust focused proof for the admitted local-memory load shape.
- Prefer a shape-level or nearby-feature test over relying only on the original named failing fixture.
- Keep assembly expectations aligned with real backend capability; do not rewrite expectations to mask semantic BIR failure.

Completion check:

- The new or updated focused proof fails before the semantic repair and passes after it, or `todo.md` records why existing nearby proof is sufficient.
- The delegated focused subset remains improved.

### Step 5: Run backend milestone proof and record remaining scope

Goal: establish whether the source idea is complete, partially repaired, or needs follow-up ideas.

Primary target: full backend CTest bucket.

Actions:

- Run `ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Record the pass/fail count and the status of all three original failures.
- If any failure family remains outside the repaired rule, create or request a precise follow-up idea instead of silently expanding this runbook.
- Prepare closure only if source-idea acceptance criteria are satisfied.

Completion check:

- `todo.md` records the backend proof result and remaining pass/fail count.
- All three original failures are classified.
- The source idea can be closed only if acceptance criteria and regression guard expectations are met.
