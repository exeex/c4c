Status: Active
Source Idea Path: ideas/open/239_aarch64_intrinsic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Print Scalar AArch64 Intrinsic Records

# Current Packet

## Just Finished

Step 4: `Split Missing CRC And Vector Carrier Dependency`.

Lifecycle repaired after CRC/vector selection blocked. Complete prepared
carriers do not currently exist for CRC, vector memory, or vector operation
intrinsic families, so those families are split to
`ideas/open/241_aarch64_crc_vector_intrinsic_carriers.md`.

The only structured intrinsic carrier family in `bir::IntrinsicFamilyKind` is
`ScalarFpUnary`, and the only intrinsic operation in
`bir::IntrinsicOperationKind` is `FAbs`. `PreparedIntrinsicCarrier` currently
publishes scalar unary fields only: one operand/result type, optional single
operand/result values, side-effect and call-plan provenance, and a placeholder
`requires_feature` bit that is never populated for CRC/vector operations.

`build_intrinsic_carrier` accepts only F32/F64 scalar `fabs` and marks all other
families/operations unsupported. Existing I128/F128 transport carriers preserve
lane/register facts for integer and binary128 transport/helper routes, but they
are not intrinsic carriers and do not publish CRC/vector intrinsic operation,
feature, lane/type, multi-operand, memory, or result-register authority. Step 4
therefore cannot responsibly select CRC/vector machine records without falling
back to intrinsic spelling, ordinary call plans, or archived scratch-register
conventions.

Completed scalar progress is preserved: Step 3 selected only scalar FP unary
F32/F64 `fabs` records from complete prepared carriers. Barrier, cache, hint,
builtin-address, CRC, vector, F128, and x86-only intrinsic families remain
unsupported or fail-closed in this runbook.

## Suggested Next

Execute Step 5 by adding printer support only for the selected scalar FP unary
F32/F64 `fabs` records from Step 3. Do not print CRC/vector or control-style
intrinsics from names, ordinary calls, or archived scratch-register
conventions.

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
- Step 5 still owns final AArch64 assembly printer emission for selected
  scalar intrinsic records.

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(instruction_dispatch|target_instruction_records)|backend_prepared_printer'; } 2>&1 | tee test_after.log`

Result: passed, `3/3` selected tests. Proof log: `test_after.log`.

`git diff --check`: passed.
