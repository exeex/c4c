Status: Active
Source Idea Path: ideas/open/237_aarch64_binary128_softfloat_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select Binary128 Memory And Copy Nodes

# Current Packet

## Just Finished

Step 3 added a narrow structured F128 memory-transport selection slice. The
record builder now fails closed unless prepared F128 carrier facts prove the
complete 16-byte size and 16-byte alignment authority, and focused record
coverage proves a `StoreToMemory` transport preserves q-register carrier
identity, vector-bank/vector-class authority, occupied q-register names, and a
16-byte structured memory operand. Dispatch coverage now proves an F128
frame-slot store selects an `F128TransportRecord` with the prepared carrier and
memory facts, while an incomplete prepared carrier reports
`incomplete_prepared_f128_carrier` and emits only the return node.

## Suggested Next

Delegate the next packet to either add the smallest remaining Step 3 copy-node
transport case if the plan expects copy coverage beyond load/store memory
transport, or review Step 3 sufficiency before moving to helper-boundary work.

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
- No explicit non-memory copy node was found or added in this packet; if Step 3
  requires more than memory transport, keep the next slice focused on the
  established move/copy authority boundary.

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records)$'; } 2>&1 | tee test_after.log`

Passed, 5/5 tests. Proof log: `test_after.log`.
