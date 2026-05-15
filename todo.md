Status: Active
Source Idea Path: ideas/open/237_aarch64_binary128_softfloat_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Binary128 Full-Width Carrier Facts

# Current Packet

## Just Finished

Step 2 added real prepared-module coverage for F128 full-width register carrier
publication. `backend_prepared_printer` now builds an AArch64 prepared module
whose non-byval `f128` parameter naturally receives ABI q-register home
authority, then verifies the published `PreparedF128Carrier` is
`full_width_register` with size 16, alignment 16, vector-bank/vector-class
authority, `q0` register identity, single-register occupancy, and no missing
facts. The slice also added the missing AArch64 F128 ABI register-name mapping
so inferred F128 register-passed ABI values publish `qN` homes instead of
falling back to no full-width register name.

## Suggested Next

Delegate the next packet to review whether Step 2 is now sufficient as a
carrier-fact milestone, or move to the Step 3/Step 4 helper-boundary work
without adding arithmetic, cast, or final assembly support in this step.

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

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_aarch64_instruction_dispatch)$'; } 2>&1 | tee test_after.log`

Passed, 4/4 tests. Proof log: `test_after.log`.
