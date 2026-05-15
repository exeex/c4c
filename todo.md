Status: Active
Source Idea Path: ideas/open/239_aarch64_intrinsic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Print Scalar AArch64 Intrinsic Records

# Current Packet

## Just Finished

Step 5: `Print Scalar AArch64 Intrinsic Records`.

Added AArch64 machine-printer support for selected scalar FP unary `fabs`
intrinsic records only. F32 records now print `fabs sN, sM` from selected S
operand/result registers, and F64 records print `fabs dN, dM` from selected D
operand/result registers.

The printer revalidates complete prepared intrinsic carrier provenance, scalar
FP unary `FAbs` operation/family, matching F32/F64 types, prepared call-plan
authority, side-effect-free/feature-free facts, and explicit FPR operand/result
registers before emitting text. Missing register authority, bad register bank,
F128-shaped records, non-selected records, and feature/side-effect records fail
closed with diagnostics instead of fabricated registers or zero-fill output.

## Suggested Next

Execute Step 6 by proving the scalar intrinsic route end to end and asking the
plan owner to decide whether this runbook should be retired, replaced, or kept
open for remaining source-idea scope.

## Watchouts

- Do not claim AArch64 intrinsic support through intrinsic-name string matching or archived scratch-register conventions.
- Keep binary128 helpers, atomics, and inline asm outside this plan.
- Unsupported x86-only intrinsics must remain rejected, trapped, or explicitly diagnosed rather than silently zero-filled.
- Ordinary `PreparedCallPlan` facts are still not sufficient intrinsic
  authority; selected intrinsic support must require a complete prepared
  intrinsic carrier.
- x86/F128 `fabs` currently carries a structured diagnostic but is
  intentionally not a complete intrinsic carrier because binary128 remains
  delegated to the closed binary128 route.
- CRC and vector intrinsic families still need structured semantic and prepared
  carrier facts for feature requirements, width/lane/type contracts,
  operand/result identities, memory operands where applicable, and register
  authority before this route can accept them. That dependency is tracked by
  `ideas/open/241_aarch64_crc_vector_intrinsic_carriers.md`.
- Step 3 intentionally selected only scalar FP unary F32/F64 `fabs`; barrier,
  cache, hint, builtin-address, CRC, vector, F128, and x86-only intrinsic
  families remain unsupported/fail-closed in this packet.
- Step 5 printer support is intentionally limited to selected scalar F32/F64
  `fabs` records. CRC/vector/control-style records still must not be printed
  from names, ordinary call plans, or archived scratch-register conventions.

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(machine_printer|instruction_dispatch|target_instruction_records)|backend_prepared_printer'; } 2>&1 | tee test_after.log`

Result: passed, `4/4` selected tests. Proof log: `test_after.log`.

`git diff --check`: passed.
