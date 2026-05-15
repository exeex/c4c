Status: Active
Source Idea Path: ideas/open/237_aarch64_binary128_softfloat_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select Binary128 Memory And Copy Nodes

# Current Packet

## Just Finished

Step 3 finished the remaining copy-transport question by adding a real
structured F128 call-boundary move slice. The existing AArch64
`lower_before_call_move` / `lower_after_call_move` dispatch boundary now admits
q-register call moves only when prepared call plans, move bundles, value homes,
ABI bindings, and complete `PreparedF128Carrier` facts agree on a full-width
16-byte/16-byte vector carrier. The call-boundary move record preserves the
source or destination F128 carrier pointer and stays fail-closed for non-GPR
register moves that lack structured q-register authority. Focused record and
dispatch coverage proves an F128 argument move selects `q2 -> q0` from prepared
carrier facts without adding named-case shortcuts.

## Suggested Next

Delegate review of Step 3 sufficiency before moving to helper-boundary work, or
select the next Step 4 packet if the supervisor accepts memory plus
call-boundary copy transport as enough Step 3 coverage.

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

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records)$'; } 2>&1 | tee test_after.log`

Passed, 5/5 tests. Proof log: `test_after.log`.
