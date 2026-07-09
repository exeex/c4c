Status: Active
Source Idea Path: ideas/open/652_prepared_incoming_stack_formal_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish One Explicit Incoming Stack Formal Fact

# Current Packet

## Just Finished

Step 2 published explicit incoming stack formal authority in producer/prealloc
metadata without wiring RV64 consumption.

Fact shape:
`bir::CallArgAbiInfo::incoming_stack_offset_bytes` is an optional caller-stack
incoming byte offset. `PreparedFormalPublicationPlan::incoming_stack_offset_bytes`
copies that fact for `IncomingStackToHome` publications and keeps it distinct
from `PreparedValueHome::offset_bytes`, which remains the callee local
spill-slot/home offset.

Producer derivation:
`src/backend/bir/lir_to_bir/module.cpp::apply_rv64_ordinary_c_stack_pressure_to_abi`
now assigns incoming stack offsets during RV64 ABI stack-pressure publication,
after deciding which ordinary C scalar lanes are stack-passed. It clears the
field for non-stack lanes and computes offsets from target ABI size/alignment
policy in the BIR producer layer, not in RV64 object emission.

Publication gate:
`src/backend/prealloc/formal_publications.cpp::plan_prepared_formal_publication`
now requires `incoming_stack_offset_bytes` before an `IncomingStackToHome`
formal publication is available. Missing local home offset still reports
`MissingStackOffset`; local-home-only authority now reports
`MissingIncomingStackOffset`.

Focused coverage:
`tests/backend/bir/backend_prealloc_formal_publications_test.cpp` proves an
available stack formal with incoming offset `8` and local home offset `40`,
plus negative local-home-only coverage. The x86 prepared query fixture was
updated to preserve the new shared fact in its synthetic stack formal plan.

## Suggested Next

Execute Step 3: wire RV64 object-route stack-passed scalar formal loading to
consume `PreparedFormalPublicationPlan::incoming_stack_offset_bytes` only when
the formal publication is available. Keep missing-authority diagnostics for
local-home-only or ambiguous facts, and add RV64 coverage proving the load
source comes from the explicit prepared fact rather than from formal-order or
frame-size reconstruction.

## Watchouts

- Do not reintroduce RV64 helpers that compute incoming offsets by walking
  `function.params`, applying ABI size/alignment, or adding
  `stack_frame_bytes`.
- Do not use `PreparedValueHome::offset_bytes` as incoming authority; it is
  still the callee local home offset.
- RV64 object emission currently still fails closed with
  `unsupported_param_home: RV64 object route requires explicit prepared incoming stack formal authority before consuming stack-passed scalar formal homes`.
- The new BIR ABI field is producer-owned. Step 3 should consume it through
  formal-publication/prepared data, not by recomputing the ABI stack layout in
  RV64.

## Proof

Validation command written to `test_after.log`:
`rm -f test_after.log && (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_formal_publications|backend_prepare_frame_stack_call_contract|backend_prepared_lookup_helper|backend_prepared_object_consumer_contract|backend_call_boundary_effect_plan)$') > test_after.log 2>&1`

Result: PASS, 5/5 focused tests passed.
