Status: Active
Source Idea Path: ideas/open/128_aarch64_wide_value_owner_post_contract_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Audit i128 Helper And Call-Boundary Ownership

# Current Packet

## Just Finished

Step 2: Audit i128 Helper And Call-Boundary Ownership completed the read-only
classification of i128 runtime helper ownership against
`src/backend/mir/aarch64/codegen/calls.cpp`.

Classification:

- i128 runtime helper fact ownership:
  `shared-bir-prealloc-contract`. `src/backend/prealloc/i128_runtime_helpers.cpp`
  writes the helper resource, ABI, live-preservation, and selected-call
  ownership facts: `populate_i128_runtime_helper_liveness_policy` builds
  preserved values and sets `live_preservation_policy` plus
  `selected_call_ownership` (lines 941-1073), while
  `populate_i128_runtime_helper_boundary_policy` sets resource policy, clobbers,
  and the div/rem direct register-pair ABI policy (lines 1075-1118). The shared
  predicate `prepared_i128_runtime_helper_has_abi_contract` also encodes div/rem
  ABI/result ownership requirements (lines 1355-1380).
- i128 div/rem helper selection and helper-boundary record construction:
  `aarch64-codegen-consumption`. `i128_ops.cpp` finds a
  `PreparedI128RuntimeHelper` for the current binary instruction, errors when
  it is missing, converts it into an `I128RuntimeHelperBoundaryRecord`, and then
  creates an AArch64 machine instruction (lines 2620-2658). The record builder
  rejects unsupported/missing prepared facts but copies the prepared callee,
  result ownership, resource policy, ABI policy, live-preservation policy, and
  selected-call ownership into the machine record (lines 1977-2051).
- i128 helper resource and ABI validation:
  `aarch64-codegen-consumption`. `i128_ops.cpp` checks that prepared resource
  flags are populated (lines 169-175), delegates the ABI contract decision to
  `prepare::prepared_i128_runtime_helper_has_abi_contract` in record building,
  selection status, and printing (lines 2009-2011, 2474-2478, 943-946), and
  never chooses the helper ABI from scratch. The local
  `has_complete_i128_div_rem_abi_policy` duplicates the div/rem shape but is
  unused in current code, so it is cleanup evidence only, not a shared-policy
  gap.
- i128 preserved value checks:
  `aarch64-codegen-consumption`. `complete_preserved_value_route` and
  `has_complete_live_preservation` only require prepared routes to contain
  callee-saved register or stack-slot details (lines 121-156). The actual route
  construction happens in prealloc through `build_call_preserved_values` and
  `preserved_value_has_complete_route` before the policy is stored on the helper
  (prealloc lines 953-981).
- i128 selected-call ownership checks:
  `aarch64-codegen-consumption`. `has_complete_selected_call_ownership` only
  verifies that all prepared ownership booleans are true (lines 158-167).
  Prealloc computes those booleans from callee identity, resource policy,
  clobbers, ABI bindings, marshaling, and live preservation (prealloc lines
  983-1048), with missing-fact diagnostics attached there (lines 1050-1071).
- i128 runtime helper assembly:
  `aarch64-codegen-consumption`. `print_i128_runtime_helper` validates prepared
  provenance and policy consistency, emits the six prepared lane marshal/unmarshal
  moves, and emits `bl <callee>` from the prepared callee name (lines 921-1015).
  `append_i128_helper_move_line` emits target `mov` instructions using prepared
  carrier-lane and ABI-register bindings (lines 360-380).
- Generic call-boundary ownership in `calls.cpp`:
  `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption`, but
  separate from i128 div/rem helper lowering. Ordinary BIR calls require a
  `PreparedCallPlan` before lowering (lines 6542-6601), copy prepared arguments,
  result, preserved values, clobbers, and callee facts into the call record
  (lines 741-843), materialize selected call-boundary sources/destinations from
  prepared move records (lines 6614-7710), record prepared call-result ABI
  registers (lines 8145-8362), and publish stack-preserved call values from
  prepared preservation routes (lines 8605-8690).

Gap candidates:

- No Step 2 shared-policy gap is justified. The evidence shows i128 div/rem
  helper ABI/resource/preservation/selected-call policy is prepared in
  `src/backend/prealloc/i128_runtime_helpers.cpp`, while `i128_ops.cpp` performs
  completeness checks and target assembly. A cleanup-only candidate exists for
  the unused `has_complete_i128_div_rem_abi_policy` helper in `i128_ops.cpp`,
  but it has no proof route as shared-policy movement and should not become a
  follow-up idea for this audit.

## Suggested Next

Execute Step 3 from `plan.md`: audit i128 pair transport, shift, and compare
record construction against `src/backend/mir/aarch64/codegen/instruction.cpp`
and `src/backend/mir/aarch64/machine_printer.cpp`.

## Watchouts

- This route is analysis-only; do not edit implementation files, tests, or
  build metadata.
- Do not treat `i128_ops.cpp` or `f128.cpp` size as evidence of a boundary gap.
- Do not move AArch64 register spelling, Q-register spelling, lane/shift
  opcode spelling, or helper call assembly into shared BIR/prealloc code.
- Follow-up ideas must be concrete: owner boundary, filenames, proof route, and
  reject signals.
- Step 2 classified i128 helper resource/ABI, preservation, and selected-call
  validation as completeness validation over prepared facts.
- Do not promote the unused `has_complete_i128_div_rem_abi_policy` helper into a
  shared-policy follow-up unless later evidence shows it affects behavior; today
  it is an unused local cleanup note.
- Later overlap checks should inspect f128 memory-backed carrier printable
  address handling against `memory.cpp`, and f128 helper policy matching against
  `calls.cpp`.

## Proof

Proof: analysis-only, no build/test run.
