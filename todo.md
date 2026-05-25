Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit AArch64 Call Emission Ownership

# Current Packet

## Just Finished

Step 1 in `plan.md` completed the AArch64 call-emission ownership audit.

AST-backed symbol and caller/callee queries covered
`src/backend/mir/aarch64/codegen/calls*.cpp` and the call headers before
focused text reads. Current surface classification:

- `calls.cpp`: prepared-call lookup and final `CallInstructionRecord` emission.
- `calls_common.cpp`: shared AArch64 sizing/register naming utilities used by
  call emission and frame/stack calculations.
- `calls_argument_sources.cpp`: prepared argument/source/storage conversion
  helpers; mostly prepared-fact consumption with some byval/local-frame
  materialization selection.
- `calls_byval_aggregates.cpp`: AArch64 aggregate/byval lane copy and stack
  copy emission helpers; target-local emission policy remains here.
- `calls_moves.cpp`: prepared boundary-effect move lowering and call-boundary
  machine instruction emission.
- `calls_preservation.cpp`: preservation lookup/liveness helpers plus
  callee-saved preservation population/republication emission; this still
  contains analysis-shaped helpers but the prior run already moved the
  current-call callee-saved storage decision to prepared boundary effects.
- `calls_printing.cpp`: call-boundary instruction record/effect construction
  and machine spelling/printing; it includes non-emission spelling/effect code
  that should remain a later Step 4 candidate.
- `calls_dispatch_bridge.cpp` and `calls_dispatch_bridge.hpp`: dispatch
  compatibility bridge around prepared call emission, including scalar
  producer materialization, indirect callee materialization, missing frame-slot
  argument materialization, stack-preserved value publication, and the
  `lower_call_instruction()` wrapper that delegates to
  `lower_prepared_call_instruction()`.

First consolidation family selected: remove the stack-preserved call-value
publication fallback path in `calls_dispatch_bridge.cpp`. The path now first
uses `prepare::first_indexed_stack_preserved_values_for_call()`, but still
falls back to `first_stack_preserved_values_for_call_fallback()` by scanning
`PreparedCallPlansFunction::calls`, and then falls back again to raw
`call_plan.preserved_values` when no call-plan lookup table is present. That
fallback reconstructs first-stack-preserved ownership locally in AArch64 after
shared prepared lookup authority exists, so it is the first duplicate planning
path to delete or fail closed.

## Suggested Next

Execute Step 2 in `plan.md`: update
`src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp` and
`src/backend/mir/aarch64/codegen/calls_dispatch_bridge.hpp` so
`publish_stack_preserved_call_values()` consumes only shared prepared lookup
facts for the first indexed stack-preserved values. Delete
`first_stack_preserved_values_for_call_fallback()` and remove the raw
`call_plan.preserved_values` fallback from this dispatch bridge path. Keep the
behavior emission-only: if the prepared lookup facts are absent, skip or fail
closed through the existing diagnostic style rather than rederiving the value
set in AArch64.

Focused proof command for that implementation packet:

`bash -lc '{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_call_boundary_effect_plan)$"; } > test_after.log 2>&1'`

## Watchouts

- Do not activate `03_dispatch_responsibility_reduction.md` until the calls
  cleanup has established the required call-emission boundary.
- Do not claim progress from file concatenation, mechanical renames, or test
  weakening.
- Keep source-idea edits out of routine audit findings.
- `calls_printing.cpp` still owns call-boundary effect construction and
  printing/spelling. That looks like the right later Step 4 family, not the
  first Step 2 deletion, because it crosses machine-record/printer ownership.
- `materialize_missing_frame_slot_call_arguments()` and scalar argument
  producers still bridge dispatch state to prepared facts; audit them after the
  stack-preserved fallback is gone, but do not mix them into the first
  implementation packet.

## Proof

`printf 'Audit-only Step 1; no code changed, no build/test run required.\n' > test_after.log`

Audit-only packet; no code changed, so no build/test run was required.
Proof log path: `test_after.log`.
