# BIR Scalar Signature Control Semantic Producer Admission

Status: Active
Source Idea: ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md

## Purpose

Activate the deferred scalar/signature/control BIR semantic producer lane and
turn it into executable packets without broadening into unrelated producer or
RV64 lowering work.

## Goal

Repair or split BIR scalar-control-flow, function-signature, and scalar-binop
semantic producer admission for the current 20-row lane.

## Core Rule

Prove BIR semantic publication first. Do not treat these rows as RV64 ABI,
local-memory, call-metadata, runtime/intrinsic, expectation, or allowlist work
unless producer inspection proves the source idea must be split.

## Read First

- `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_followups.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_rows.md`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`

## Current Targets

- Scalar-control-flow semantic family rows, including representative
  `src/20000314-3.c`.
- Function-signature semantic family rows, including representative
  `src/20050316-3.c`.
- Scalar-binop semantic family row, including representative `src/960513-1.c`.
- BIR semantic producer/admission code that emits or rejects facts before
  prepared handoff or RV64 object emission.

## Non-Goals

- Do not merge this lane into local-memory, call metadata, runtime/intrinsic,
  bootstrap/global data-shape, or RV64 instruction-fragment work without row
  and code evidence of a shared producer boundary.
- Do not implement named-case shortcuts for `src/960513-1.c` or any other
  representative.
- Do not downgrade expectations, unsupported markers, allowlists, or semantic
  admission checks.
- Do not route function-signature failures to ABI/RV64 lowering until BIR
  publication is proven correct.
- Do not claim broad scalar progress from the single scalar-binop row alone.

## Working Model

- The source evidence groups 10 scalar-control-flow rows, 9
  function-signature rows, and 1 scalar-binop row as a smaller BIR semantic
  producer lane.
- The first executable decision is whether those three topics share one
  producer/admission boundary. If they do not, split the idea before
  implementation instead of forcing one runbook to cover independent routes.
- Each retained topic needs focused BIR coverage before RV64 representative
  proof is accepted as capability progress.

## Execution Rules

- Keep packet progress in `todo.md`; rewrite this runbook only for a real
  route split, lifecycle repair, or proof-driven runbook correction.
- Prefer semantic producer publication or fail-closed admission rules over
  downstream inference.
- Each code-changing step needs fresh build or compile proof plus the narrow
  BIR/RV64 subset selected by the supervisor.
- Broaden validation after multiple retained topics are changed, after a
  shared producer boundary is modified, or before lifecycle closure.
- If inspection proves independent topics, stop implementation and ask the
  plan owner to split the source intent into separate open ideas.

## Steps

### Step 1: Inspect Producer Boundary And Decide Split

Goal: Determine whether scalar-control-flow, function-signature, and
scalar-binop failures share a BIR semantic producer/admission boundary.

Primary targets:
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_followups.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_rows.md`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`

Actions:
- Inspect the 20-row evidence and identify the concrete BIR family labels,
  affected functions, and representative cases.
- Trace each topic to the producer/admission code that creates or rejects its
  semantic facts.
- Decide whether the topics share one repairable boundary or require separate
  source ideas.
- Record the decision and evidence in `todo.md`.

Completion check:
- `todo.md` names the shared boundary with evidence, or records that the lane
  must be split before implementation.

### Step 2: Add Focused BIR Coverage For Retained Topics

Goal: Lock in expected BIR semantic fact publication for each topic that
remains in this active runbook.

Primary targets:
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- Existing BIR test helpers adjacent to semantic admission tests

Actions:
- Add or extend focused tests for scalar-control-flow fact publication.
- Add or extend focused tests for function-signature fact publication.
- Add or extend focused tests for scalar-binop fact publication if it remains
  in this runbook after Step 1.
- Keep tests semantic and producer-oriented; do not encode RV64 lowering
  assumptions as the first proof.

Completion check:
- Focused BIR tests fail before the producer repair or document an already
  correct retained topic, and pass after the implementation step that owns
  that topic.

### Step 3: Repair Semantic Producer Admission

Goal: Publish or admit the missing BIR semantic facts for retained topics
through the real producer boundary.

Primary targets:
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/`
- Any narrow BIR semantic fact carrier or admission helper identified by
  Step 1

Actions:
- Implement the smallest semantic producer repair that covers the retained
  topic family.
- Keep fail-closed behavior for missing or ambiguous facts.
- Avoid downstream RV64 object emission, ABI, allowlist, and expectation
  changes.
- Preserve existing local-memory, call-metadata, runtime/intrinsic, and
  bootstrap/global lanes.

Completion check:
- Focused BIR coverage from Step 2 passes for each retained repaired topic.
- The change removes the relevant semantic admission failure at the BIR layer,
  not by hiding or reclassifying the diagnostic.

### Step 4: Prove RV64 Representatives

Goal: Show that representative RV64 rows advance because BIR facts are
published correctly.

Primary targets:
- `src/20000314-3.c`
- `src/20050316-3.c`
- `src/960513-1.c`
- Current stronger substitutes if the row inventory has changed

Actions:
- Run the supervisor-selected narrow RV64 proof for all retained topic
  representatives.
- If a representative now fails later in RV64 lowering, record the new owner
  boundary instead of expanding this runbook into that downstream work.
- Compare nearby same-family rows where practical so proof is not
  testcase-shaped.

Completion check:
- Representative rows either pass or advance past the original BIR semantic
  admission diagnostic with a clearly recorded downstream owner.

### Step 5: Broader Validation And Closure Decision

Goal: Decide whether the source idea is complete or whether remaining retained
topic work needs another runbook.

Primary targets:
- Focused BIR tests touched by this plan
- Supervisor-selected RV64 semantic admission subset
- Broader backend validation if the producer boundary was shared or widely
  touched

Actions:
- Run the supervisor-selected acceptance proof.
- Confirm no expectations, unsupported markers, allowlists, or downstream
  shortcuts were used as producer progress.
- Record remaining downstream failures or independent-topic leftovers in
  `todo.md` for supervisor/plan-owner routing.

Completion check:
- All retained topics have BIR coverage and representative proof, or the
  remaining work is explicitly routed to separate lifecycle state.
