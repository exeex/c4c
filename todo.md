Status: Active
Source Idea Path: ideas/open/237_aarch64_binary128_softfloat_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Binary128 Full-Width Carrier Facts

# Current Packet

## Just Finished

Step 2 added structured F128 full-width carrier authority for 16-byte
memory/load transport. Prealloc now publishes `PreparedF128Carrier` records for
full-width q-register homes and memory-backed frame slots, the prepared printer
emits those facts, and AArch64 dispatch consumes the carrier to select a
structured `F128TransportRecord` for the focused frame-slot load instead of the
Step 1 `missing_prepared_f128_carrier` guard.

## Suggested Next

Delegate the next Step 2 packet to add focused prepared F128 memory-backed
carrier coverage from a real prepared module path, then decide whether helper
boundary preservation facts are ready or should be split into the Step 4 helper
packet.

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

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch)$'; } 2>&1 | tee test_after.log`

Passed. Proof log: `test_after.log`.
