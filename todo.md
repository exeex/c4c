Status: Active
Source Idea Path: ideas/open/237_aarch64_binary128_softfloat_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Select Binary128 Soft-Float Helper Nodes

# Current Packet

## Just Finished

Step 4 added the first record-only prepared binary128 soft-float helper
authority slice for F128 add. `PreparedF128RuntimeHelper` now maps an F128
`add` BIR instruction to `__addtf3`, preserves helper identity, source opcode,
result/lhs/rhs value ids, full-width 16-byte F128 carrier references, resource
flags, selected-call ownership flags, and explicit fail-closed diagnostics for
missing ABI marshaling, caller-saved clobber, and live-preservation authority.
No AArch64 machine selection was added.

## Suggested Next

Stay on Step 4 and fill the next exact F128 helper-boundary gap: add explicit
ABI marshaling/unmarshaling authority for the prepared F128 soft-float helper
record, then add caller-saved clobber and live-preservation facts before any
AArch64 dispatch consumes the helper.

## Watchouts

- Do not claim binary128 progress through scalar `F64` lowering.
- Do not add testcase-shaped helper shortcuts or weaken unsupported
  expectations.
- Keep atomic, intrinsic, and inline-assembly AArch64 routes out of this plan.
- F128 q-register transport is modeled as vector-bank full-width storage so it
  does not reopen scalar `F64` lowering.
- The selected `F128TransportRecord` is record-level only; final assembly
  printing remains unsupported until Step 5 adds printer authority.
- Helper-call, arithmetic, casts, constants, and broader F128 preservation
  facts remain outside this packet.
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
- Unsupported F128 arithmetic/cast/comparison operations should remain
  diagnosed until the prepared helper fact family exists; do not add scalar F64
  approximations or dispatch-only callee guesses.
- The new F128 add helper record intentionally keeps
  `owns_terminal_call=no`; the prepared fact is not complete enough for AArch64
  dispatch until ABI marshaling, clobbers, and live preservation become
  structured authority.
- Only F128 add maps to a helper callee in this slice. Sub/mul/div/compare/cast
  helpers remain unsupported until their semantic helper identities and ABI
  facts are added deliberately.

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records)$'; } 2>&1 | tee test_after.log`

Passed, 5/5 tests. Proof log: `test_after.log`.
