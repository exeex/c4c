Status: Active
Source Idea Path: ideas/open/237_aarch64_binary128_softfloat_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Select Binary128 Soft-Float Helper Nodes

# Current Packet

## Just Finished

Step 4 started with the smallest real helper-boundary check and resolved it as
blocked, not as a code slice. I inspected the prepared and AArch64 dispatch
surfaces for helper calls: `PreparedF128Carrier`, `PreparedI128RuntimeHelper`,
`append_i128_runtime_helper_mappings`, `populate_i128_runtime_helper_*`,
`make_prepared_i128_runtime_helper_boundary_record`, and the dispatch path that
selects only prepared i128 div/rem helpers. The current prepared F128 authority
is carrier-only: named full-width q-register or memory-backed 16-byte storage
facts. There is no genuine prepared binary128 soft-float helper boundary yet.

Exact missing prepared helper authority: a structured F128 soft-float helper
fact family that maps one binary128 arithmetic operation to a concrete helper
identity/callee, records 16-byte F128 argument and result carriers, models
argument marshaling and result unmarshaling across the helper ABI, publishes
caller-saved clobbers and preserved live values, and exposes selected
call-ownership flags equivalent to the existing i128 helper boundary policy.
Without that fact family, selecting an AArch64 F128 helper node would require
guessing from BIR operation shape or scalar FP behavior instead of consuming
prepared authority.

## Suggested Next

Stay on Step 4 and add the first prepared binary128 soft-float helper authority
slice. The smallest coherent packet is to introduce a record-only prepared F128
helper fact for one operation such as F128 add, with helper identity, full-width
operand/result carrier references, ABI marshaling facts, clobber policy, and
live-preservation ownership before AArch64 dispatch selects a machine node.

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

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records)$'; } 2>&1 | tee test_after.log`

Passed, 5/5 tests. Proof log: `test_after.log`.
