# 138 Call-plan lookup ownership cleanup

## Goal

Move or re-export call-plan lookup indexes through call-domain ownership while
preserving the existing prepared call semantics and cheap per-function aggregate
construction.

## Why This Exists

The Step 3 classification in the BIR/prealloc prepared query surface audit
found that call plans, outgoing stack areas, prior-preservation lookups, and
call-boundary effects are good shared semantic facts, but their lookup API is
owned by the broad `prepared_lookups.*` facade instead of the call domain.

The concrete smell is `PreparedCallPlanLookups` and its helpers:

- outgoing stack argument area index
- prior preserved value index
- preservation republication index
- `find_latest_indexed_prior_preserved_value`
- `find_dominating_indexed_prior_preserved_value`
- `find_unique_indexed_prior_preserved_value_source`
- `find_indexed_prepared_outgoing_stack_argument_area`
- `find_indexed_prepared_call_boundary_effects`

These are call-domain facts. Future x86/riscv work should not need to learn a
global prepared-lookup facade to find ABI and preservation lookup ownership.

## In Scope

- Call lookup declarations and definitions in:
  - `src/backend/prealloc/prepared_lookups.hpp`
  - `src/backend/prealloc/prepared_lookups.cpp`
  - `src/backend/prealloc/calls.hpp`
  - `src/backend/prealloc/call_plans.cpp`
  - `src/backend/prealloc/module.hpp`
- Construction wiring for `PreparedFunctionLookups::call_plans`, as long as
  the aggregate remains cheap for consumers.
- Known call-domain consumers in:
  - `src/backend/prealloc/call_plans.cpp`
  - `src/backend/prealloc/prepared_printer/calls.cpp`
  - `src/backend/prealloc/prepared_printer/functions.cpp`
  - `src/backend/mir/aarch64/codegen/traversal.cpp`
  - `src/backend/mir/aarch64/codegen/dispatch.cpp`
  - `src/backend/mir/aarch64/codegen/calls.cpp`
  - `src/backend/mir/aarch64/codegen/globals.cpp`

## Out Of Scope

- Changing ABI classification, call lowering, call-boundary machine
  instructions, or AArch64 register handling.
- Reworking `PreparedCallPlan`, argument/result planning, variadic wrapper
  behavior, or preservation semantics.
- Moving target-local call emission policy into prealloc.
- Splitting runtime-helper planning; runtime-helper ownership was not a
  separate Step 3 candidate.

## Acceptance Criteria

- Call-plan lookup types and query helpers are declared from a call-domain
  owner (`calls.hpp`, a paired call lookup header, or another explicitly
  call-owned boundary) rather than only from `prepared_lookups.hpp`.
- `PreparedFunctionLookups` can still aggregate call-plan lookups for hot
  consumers without making callers rebuild indexes.
- AArch64 call lowering still consumes prepared call facts; it does not
  duplicate preservation, outgoing-stack, or call-boundary discovery.
- The cleanup is mechanical/API-ownership oriented unless a narrowly justified
  call-domain helper extraction is necessary to break dependency cycles.

## Proof Route

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`
- Escalate to full `ctest --test-dir build -j --output-on-failure` if call
  semantics, ABI binding fields, preservation route calculation, or
  call-boundary effect construction changes.

## Reviewer Reject Signals

- Reject if the diff changes AArch64 call emission policy, register spelling,
  ABI expectation behavior, or call-boundary instruction selection.
- Reject if prior-preservation lookups are replaced with local scans in
  `src/backend/mir/aarch64/codegen/calls.cpp`.
- Reject if outgoing-stack or preservation behavior is proven only by weakened
  expectations, unsupported tests, or printer-output rewrites.
- Reject if `PreparedFunctionLookups` loses cheap call lookup aggregation and
  consumers are forced to rebuild lookup maps manually.
- Reject if the patch claims capability progress while only renaming helpers or
  moving comments.

## Closure Note

Closed after the active runbook moved call-plan lookup declarations to the
call-owned boundary, kept `PreparedFunctionLookups` aggregate construction
cheap, updated known consumers, and passed the backend regression guard.
