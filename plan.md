# Phase E3 Route 6 x86 Scalar i32 Argument-Source Route-Debug Follow-Up

Status: Active
Source Idea: ideas/open/232_phase_e3_route6_x86_scalar_i32_argument_source_route_debug_follow_up.md

## Purpose

Activate the Route 6 follow-up for one x86 scalar `i32` argument-source
route-debug row while preserving `ConsumedPlans`, prepared call/debug
authority, x86 wrapper behavior, direct-call/helper-oracle families, and
expected strings.

## Goal

Make the selected route-debug row use Route 6 scalar argument-source agreement
only when it agrees with prepared call-plan facts, and fail closed to existing
prepared/`ConsumedPlans` behavior for absent, invalid, duplicate, conflicting,
or mismatched route evidence.

## Core Rule

Route 6 may explain the selected x86 scalar `i32` argument-source route-debug
row only after prepared call argument agreement. `ConsumedPlans`, prepared call
plans, ABI/call wrapper policy, direct-call/helper-oracle behavior, wrapper
output, and expected strings remain authoritative everywhere else.

## Read First

- The source idea named in the metadata above.
- `ideas/closed/214_route6_x86_scalar_source_route_debug_row.md`
- `ideas/closed/213_route6_call_source_consumer.md`
- `ideas/closed/205_route6_call_use_source_adapter.md`
- `ideas/closed/226_phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`
- `docs/bir_prealloc_fusion/phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`
- `src/backend/mir/x86/x86.hpp`
- `src/backend/mir/x86/debug/debug.cpp`
- `src/backend/mir/x86/module/module.cpp`
- `src/backend/mir/x86/prepared/dispatch.cpp`
- `src/backend/mir/query.cpp`
- `src/backend/mir/query.hpp`
- `tests/backend/bir/backend_x86_route_debug_test.cpp`
- `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`
- nearby x86 handoff boundary, prepared printer/debug, direct-call, and
  helper-oracle tests that expose the same scalar argument-source behavior

## Current Targets

- One x86 scalar `i32` argument-source route-debug row.
- Route 6 scalar argument-source agreement through
  `find_consumed_scalar_i32_call_argument_source(...)` and the Route 6
  call-use source index.
- Positive agreement plus absent, invalid, duplicate/conflict, mismatch, and
  `ConsumedPlans` compatibility paths.
- Byte-stable x86 wrapper behavior, ABI/call wrapper policy, prepared
  call/debug output, direct-call/helper-oracle families, route-debug text,
  expected strings, and nearby same-feature cases.
- The aggregate `backend_x86_handoff_boundary` compare-join stack-backed
  parameter-home semantic failure is tracked separately in
  `ideas/open/234_phase_e3_x86_compare_join_stack_home_handoff_follow_up.md`;
  it must not be folded into this Route 6 route-debug row slice.

## Non-Goals

- Do not migrate broad x86 call wrapper, ABI/call policy, call lowering,
  prepared call printer/debug, direct-call/helper-oracle families, or
  `ConsumedPlans` ownership.
- Do not change route-debug row spelling, wrapper output, helper-oracle
  strings, expected strings, baselines, supported/unsupported contracts, or
  public fallback behavior.
- Do not add testcase-shaped matching, fixture-name shortcuts, or
  named-row-only logic to claim Route 6 progress.
- Do not start E4 wrapper migration, draft 155, E5, aggregate prepared
  retirement, or broad prepared diagnostic/oracle replacement.

## Working Model

- `ConsumedPlans` already owns the x86 compatibility boundary and exposes
  shared prepared lookups plus the Route 6 call-use source index.
- `find_consumed_scalar_i32_call_argument_source(...)` is the narrow agreement
  boundary for scalar `i32` argument-source Route 6 facts.
- The x86 route-debug path emits Route 6 scalar argument rows only when the
  consumed route evidence agrees with prepared call argument facts.
- Module lowering may consume the same agreement metadata for diagnostics, but
  ABI placement, wrapper kind, call emission, direct-call policy, helper
  oracle behavior, and final output remain prepared or target-owned.

## Execution Rules

- Start by identifying the exact route-debug row and current positive/fallback
  coverage before editing implementation.
- Keep prepared behavior authoritative for absent route evidence, invalid
  references, duplicate/conflicting facts, source/prepared mismatch,
  `ConsumedPlans` compatibility, wrapper paths, direct-call/helper-oracle
  paths, and public fallback.
- Prefer existing Route 6 call-use source indexing and prepared agreement
  helpers over adding a new diagnostic channel.
- Preserve route-debug text, prepared call/debug output, wrapper output,
  expected strings, baselines, and supported/unsupported contracts unless the
  supervisor explicitly approves a different contract.
- Every code-changing step needs fresh build or compile proof plus the
  supervisor-delegated test subset. Broader validation is required before
  acceptance if the implementation touches shared x86 wrapper, module
  lowering, prepared dispatch, `ConsumedPlans`, MIR query, direct-call, or
  helper-oracle behavior beyond the selected row.
- If a broader aggregate handoff proof exposes compare-join stack-home
  prepared-module consumer semantics unrelated to the selected route-debug row,
  record or follow the separate handoff idea instead of expanding this runbook.

## Steps

### Step 1: Locate The Row And Agreement Boundary

Goal: identify the selected scalar `i32` argument-source route-debug row, the
Route 6 agreement boundary it can use, and the existing tests that prove
positive and fallback behavior.

Primary targets:

- `src/backend/mir/x86/debug/debug.cpp`
- `src/backend/mir/x86/x86.hpp`
- `src/backend/mir/query.cpp`
- `src/backend/mir/query.hpp`
- `tests/backend/bir/backend_x86_route_debug_test.cpp`
- `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`

Actions:

- Inspect the x86 route-debug row emitted for the selected scalar `i32`
  argument-source call.
- Trace the agreement boundary through `ConsumedPlans`,
  `find_consumed_scalar_i32_call_argument_source(...)`, and the Route 6
  call-use source index/query helpers.
- Inventory existing positive, absent, invalid, duplicate/conflict, mismatch,
  compatibility, wrapper, direct-call, helper-oracle, and expected-string
  coverage.
- Record the exact row, minimal Step 2 target files, and fallback/proof gaps in
  `todo.md` before implementation edits.

Completion check:

- `todo.md` names the selected route-debug row, the agreement boundary, the
  minimal implementation targets, and the fallback/proof gaps for Step 2.
- No code, route-debug text, expected-string, baseline, wrapper, direct-call,
  helper-oracle, or prepared call/debug behavior change is made in this step
  unless the executor is explicitly delegated to start implementation.

### Step 2: Use Route 6 Agreement Only Under Prepared Compatibility

Goal: make the selected route-debug row consume Route 6 scalar source metadata
only when it agrees with prepared call argument facts and `ConsumedPlans`
compatibility.

Primary targets:

- The route-debug row path found in Step 1
- `src/backend/mir/x86/x86.hpp`
- `src/backend/mir/x86/debug/debug.cpp`
- `src/backend/mir/x86/module/module.cpp`
- Existing Route 6 query helpers in `src/backend/mir/query.*`

Actions:

- Reuse existing Route 6 call-use source status, record lookup, and prepared
  agreement metadata where possible.
- Require an available prepared scalar `i32` call argument fact before Route 6
  metadata can be used for the selected route-debug row.
- Fail closed to existing prepared/`ConsumedPlans` authority for absent,
  invalid, duplicate/conflicting, mismatch, compatibility, direct-call,
  helper-oracle, wrapper, and public fallback paths.
- Keep row spelling, prepared call/debug text, wrapper output, helper-oracle
  strings, expected strings, and baselines byte-stable.

Completion check:

- The selected positive row is explained by Route 6 scalar argument-source
  metadata after prepared agreement.
- Every non-agreement path retains existing prepared/`ConsumedPlans`
  behavior.
- The slice builds and the delegated narrow proof passes without expectation
  or baseline rewrites.

### Step 3: Prove Fallback And Nearby Same-Feature Stability

Goal: prove the implementation is semantic and not shaped around one known
fixture, while keeping compare-join stack-home handoff semantics out of this
Route 6 route-debug row slice.

Primary targets:

- `tests/backend/bir/backend_x86_route_debug_test.cpp`
- `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`
- prepared printer/debug, direct-call, and helper-oracle tests selected by the
  supervisor
- the separate handoff idea for aggregate compare-join stack-home failures:
  `ideas/open/234_phase_e3_x86_compare_join_stack_home_handoff_follow_up.md`

Actions:

- Add or tighten focused tests only where coverage is missing for the selected
  row and fallback matrix.
- Cover positive Route 6 agreement plus absent route evidence, invalid
  reference, duplicate/conflict, prepared mismatch, and `ConsumedPlans`
  compatibility fallback.
- Check nearby same-feature scalar source cases so the change does not depend
  on one fixture shape.
- Preserve x86 wrapper behavior, ABI/call wrapper policy, prepared call/debug
  output, direct-call/helper-oracle families, route-debug row text, expected
  strings, baselines, and supported/unsupported contracts.
- Treat the broader `backend_x86_handoff_boundary` compare-against-zero
  compare-join lane with stack-backed parameter-home failure as separate
  prepared-module consumer handoff work when the selected row proof and
  fallback matrix remain green.

Completion check:

- Focused tests or explicit proof cover the positive row and every required
  fallback path.
- Nearby same-feature coverage demonstrates this is not a named-case shortcut.
- No wrapper, ABI/call policy, direct-call/helper-oracle, expected-string,
  baseline, or supported/unsupported change is part of the diff.
- Any aggregate handoff failure outside the selected route-debug row is linked
  to a separate open idea before Step 3 is treated as complete.

### Step 4: Validate And Prepare Acceptance Notes

Goal: prove the completed route-debug row slice and leave clear handoff state
for supervisor review.

Primary targets:

- Build or compile target chosen by the supervisor
- Delegated narrow x86 route-debug and `ConsumedPlans` compatibility tests
- Any broader validation the supervisor requests because of touched surfaces
- `todo.md`

Actions:

- Run the exact supervisor-delegated proof command and record the command and
  result in `todo.md`.
- If the change touched shared x86 wrapper, module lowering, prepared dispatch,
  `ConsumedPlans`, MIR query, direct-call, helper-oracle, or prepared
  call/debug behavior beyond the selected row, ask the supervisor to choose
  broader validation before acceptance.
- Summarize the row changed, retained prepared/`ConsumedPlans` authority,
  same-feature proof, and any residual risk.

Completion check:

- Fresh build or compile proof exists.
- Delegated narrow tests pass, and any supervisor-requested broader validation
  passes.
- `todo.md` records the implementation summary, proof commands, retained
  authority, and residual risks.
