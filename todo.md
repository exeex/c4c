Status: Active
Source Idea Path: ideas/open/237_aarch64_binary128_softfloat_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Select Binary128 Soft-Float Helper Nodes

# Current Packet

## Just Finished

Step 3 resolved the remaining constant-transport question as blocked, not as a
code slice. I inspected the AArch64 F128 transport boundary through
`make_prepared_f128_carrier_transport_record`, `f128_transport_selection_status`,
`make_f128_transport_instruction`, `lower_f128_transport_instruction`, and the
existing F128 record/dispatch tests. That boundary only has structured
authority for prepared named F128 carriers plus optional 16-byte memory
operands. I also inspected `bir::Value`, `BirFunctionLowerer::lower_value`,
`build_f128_carrier`, and `PreparedF128Carrier`; BIR immediates currently carry
only 64-bit `immediate` / `immediate_bits`, F128 literals are not parsed into
full 128-bit low/high halves, and prepared F128 carriers are only derived from
named regalloc/storage facts. There is therefore no genuine prepared constant
carrier fact for a structured AArch64 Binary128 constant transport selection
case yet.

The plan owner split the missing full-width F128 constant-carrier authority
into `ideas/open/241_f128_full_width_constant_carriers.md` and narrowed active
Step 3 so it covers load/store/copy transport only. F128 constant transport
remains fail-closed in this runbook until that dependency provides structured
16-byte payload facts.

## Suggested Next

Proceed with Step 4: Select Binary128 Soft-Float Helper Nodes. The smallest
future packet should map one supported helper-boundary case to structured
binary128 helper-call records from prepared facts without reopening constant
transport or printer support.

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

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records)$'; } 2>&1 | tee test_after.log`

Passed, 5/5 tests. Proof log: `test_after.log`.
