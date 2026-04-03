Status: Active
Source Idea: ideas/open/04_backend_ir_to_bir_scaffold.md
Activated from: ideas/open/04_backend_ir_to_bir_scaffold.md

# Backend IR To BIR Scaffold Runbook

## Purpose

Establish the first explicit `bir` scaffold on top of the cleaned-up
`lir -> backend_ir` boundary without disturbing the current production backend
path.

## Goal

Create a flag-gated `LIR -> BIR -> backend emission` skeleton that proves one
tiny end-to-end backend case while keeping the existing backend path as the
default.

## Core Rule

Bias toward architectural proof, not feature breadth. Land the smallest
coherent BIR slice that demonstrates naming, wiring, and validation without
starting a broad backend migration.

## Read First

- `ideas/open/04_backend_ir_to_bir_scaffold.md`
- `src/backend/backend.hpp`
- `src/backend/backend.cpp`
- `src/backend/lowering/lir_to_backend_ir.*`
- `src/backend/ir.hpp`
- `tests/backend/*`

## Scope

- New `bir` source surfaces and naming
- An explicit opt-in path from LIR into BIR
- Minimal BIR data model and validation/printing support
- One narrow backend test path that exercises the BIR flow end-to-end

## Non-Goals

- No broad BIR instruction coverage
- No default-path cutover
- No large backend test-suite migration
- No deletion of transitional `backend_ir` or compatibility entrypoints

## Execution Rules

- Keep the existing backend path as the default production flow throughout.
- Treat the new BIR path as explicitly gated and developer-facing.
- Add new-path tests in BIR-oriented files instead of expanding the legacy
  adapter-heavy test files into a mixed framework.
- Prefer compatibility shims and wrappers over invasive renames in the first
  scaffold slice.
- If execution reveals a separate initiative, record it under `ideas/open/`
  instead of broadening this runbook.

## Step 1: Establish the BIR type scaffold

- Introduce the smallest coherent `bir` model needed for one tiny function path:
  module, function, block, return terminator, and minimal scalar values.
- Add any tiny validation or printing helpers needed to keep the scaffold
  inspectable and testable.
- Keep the new source surfaces clearly named `bir`.

Completion check:
- [ ] A distinct `bir` layer exists in source form.
- [ ] The initial BIR model is intentionally minimal and coherent.
- [ ] The new naming is explicit rather than hidden behind legacy files.

## Step 2: Add a flag-gated LIR-to-BIR entrypoint

- Introduce an explicit opt-in path from LIR into BIR lowering.
- Keep the old backend flow as the default and ensure the new path is selected
  only when requested.
- Make the relationship between transitional `backend_ir` code and the new BIR
  path obvious in the implementation.

Completion check:
- [ ] An explicit flag or equivalent narrow switch selects the BIR flow.
- [ ] The old backend path remains the default production path.
- [ ] The new and old ownership boundaries are easy to trace in code.

## Step 3: Prove one end-to-end BIR backend case

- Choose one intentionally tiny backend case such as return-immediate or
  single-block integer add/return.
- Add new BIR-oriented tests in a separate location from the legacy adapter
  suites.
- Wire BIR lowering and backend emission only far enough to make that one case
  pass end-to-end.

Completion check:
- [ ] One backend test case passes end-to-end through the BIR path.
- [ ] The first new-path tests live in a BIR-oriented location.
- [ ] Assertions describe the new path in BIR terms where practical.

## Step 4: Validate scaffold stability

- Re-run the touched backend tests and a backend-scoped regression slice.
- Confirm the default path stays regression-free while the BIR path remains
  explicitly opt-in.
- Record any intentionally deferred coverage boundaries for the follow-on BIR
  enablement plan.

Completion check:
- [ ] Existing default-path backend tests remain regression-free in backend scope.
- [ ] Deferred follow-on work is documented as bounded future scope.
- [ ] The scaffold is ready for broader BIR enablement work without immediate
      architectural rework.
