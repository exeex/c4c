Status: Active
Source Idea Path: ideas/open/237_aarch64_binary128_softfloat_lowering.md
Source Plan Path: plan.md
Current Step ID: 4.2
Current Step Title: Comparison Helper Boundary

# Current Packet

## Just Finished

Step 4.2 completed the remaining F128 comparison helper materialization
contract. Prepared F128 comparison helpers now publish an explicit
`ScalarCmpResultConsumption` fact that maps the helper CMPtype result to the
source BIR `I1` result through a structured zero-test (`eq_zero`, `ne_zero`,
signed less/less-equal/greater/greater-equal zero). AArch64 helper records now
require that consumption fact, preserve the scalar CMPtype ABI result, expose
the materialized `I1` result register, and mark the BIR result value as defined.
Dispatch now selects real BIR F128 `Eq/Ne/Slt/Sle/Sgt/Sge` comparison helpers
from prepared authority instead of falling through to scalar lowering, while
unmodeled unsigned predicates still fail closed. No scalar `F64` shortcut,
assembly printer implementation, cast, sign-bit, or unrelated helper-family
work was added.

## Suggested Next

Move to Step 4.3 cast helper boundaries, unless the supervisor wants an
independent review of the completed Step 4.2 comparison-helper route first.

## Watchouts

- Do not claim binary128 progress through scalar `F64` lowering.
- Do not add testcase-shaped helper shortcuts or weaken unsupported
  expectations.
- Keep atomic, intrinsic, and inline-assembly AArch64 routes out of this plan.
- F128 q-register transport is modeled as vector-bank full-width storage so it
  does not reopen scalar `F64` lowering.
- The selected `F128TransportRecord` is record-level only; final assembly
  printing remains unsupported until Step 5 adds printer authority.
- Further helper-call operations, casts, constants, and broader F128
  preservation facts remain outside this packet.
- The new real-path F128 coverage uses a byval parameter to force a prepared
  frame-slot home; it does not claim helper-boundary or arithmetic support.
- The new register-carrier coverage uses a normal ABI-passed `f128` parameter
  to prove q-register carrier facts; it does not force register-group overrides
  or testcase-shaped assignments.
- Storage-plan bank text for the ABI home can still report the generic scalar
  FP source bank before `PreparedF128Carrier` normalizes full-width F128
  transport to vector-bank/vector-class authority.
- This packet covered F128 load/store-style memory transport selection, with
  new store proof and existing load proof. It did not add final assembly
  printing or arithmetic/cast/helper-call support.
- The inspected copy boundary was the call-boundary move route:
  `lower_before_call_move`, `lower_after_call_move`,
  `make_call_boundary_move_instruction`, and
  `call_boundary_move_selection_status`. That boundary now handles the narrow
  structured F128 q-register move case; generic parallel copies, memory-backed
  F128 copies, helper-call lowering, final assembly printing, and arithmetic
  remain outside this packet.
- The inspected constant boundary was fail-closed: `StoreLocalInst` with a
  non-named F128 value reaches `lower_f128_transport_instruction`, cannot map
  to a `ValueNameId`, and reports missing prepared F128 carrier authority.
  Adding an AArch64 constant case now would require inventing 128-bit payload
  authority outside the prepared facts.
- Exact missing authority: a prepared full-width F128 constant carrier carrying
  both 64-bit halves or equivalent 16-byte payload provenance, linked to a BIR
  value or storage fact that instruction selection can consume.
- Step 4 cannot reuse `PreparedI128RuntimeHelper` as-is without overfitting:
  its helper family, ABI transition, operand lanes, and selected record
  builder are i128-specific and require GPR low/high lanes, while binary128
  helper calls need F128 full-width carriers and a soft-float ABI contract.
- Unsupported F128 comparison, cast, sign-bit, and other helper-family
  operations should remain diagnosed until their prepared helper identities and
  complete ownership facts exist; do not add scalar F64 approximations or
  dispatch-only callee guesses.
- Only F128 add, sub, mul, and signed binary128 division map to helper callees
  in this slice. Compare/cast/sign-bit and unsigned div/rem helpers remain
  unsupported until their semantic helper identities and ABI facts are added
  deliberately.
- The AArch64 F128 helper boundary is now selected only from a complete
  `PreparedF128RuntimeHelper` with selected-call ownership. Do not reintroduce
  raw BIR-shape, rendered-text, or scalar F64 inference.
- The selected F128 add/sub/mul/div helper boundary is record-level only.
  Final assembly printing remains unsupported until Step 5 adds printer
  authority; do not start Step 5 until the remaining Step 4
  helper-family/helper-identity packets are deliberately handled or explicitly
  deferred by the supervisor.
- Unsigned F128 predicates remain fail-closed until an ordered/unordered
  predicate contract is explicitly modeled.
- The CMPtype-to-`I1` consumption contract is still record-level target MIR
  authority; final instruction printing/encoding is Step 5 work.
- Stack-homed comparison results remain fail-closed at this boundary because
  this packet proves the register materialization path only.

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_aarch64_prepared_branch_records|backend_aarch64_branch_compare_records)$'; } 2>&1 | tee test_after.log`

Passed, 7/7 tests. Proof log: `test_after.log`.
