Status: Active
Source Idea Path: ideas/open/239_aarch64_intrinsic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select Scalar And Control Intrinsic Machine Records

# Current Packet

## Just Finished

Step 3: `Select Scalar And Control Intrinsic Machine Records`.

Consumed only complete prepared scalar FP unary F32/F64 `fabs` intrinsic
carriers into explicit AArch64 selected intrinsic machine records.

The selected record preserves the structured carrier family, operation,
operand/result type, operand/result register authority, source callee,
side-effect and feature facts, and complete-carrier provenance. A matching
ordinary `PreparedCallPlan` alone remains insufficient to select an intrinsic.

Fail-closed coverage now proves incomplete intrinsic carriers and carriers
without FPR operand/result authority do not fall back to ordinary call lowering
or fabricate selected intrinsic records. It also proves a structured
F128/x86-only `llvm.fabs.f128`-shaped scalar FP unary carrier remains
unsupported at AArch64 dispatch and does not select an intrinsic or fall back to
ordinary call lowering.

## Suggested Next

Execute Step 4 by selecting CRC/vector intrinsic machine records only for
families whose prepared carriers are complete. If CRC/vector carriers still
lack feature, width, lane, or register authority, record the dependency instead
of selecting from names or ordinary calls.

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
- CRC, vector, barrier/cache/hint, and builtin-address families still need
  structured semantic feature/operand facts before this route can accept them.
- Step 3 intentionally selected only scalar FP unary F32/F64 `fabs`; barrier,
  cache, hint, builtin-address, CRC, vector, F128, and x86-only intrinsic
  families remain unsupported/fail-closed in this packet.
- Step 5 still owns final AArch64 assembly printer emission for selected
  intrinsic records.

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(instruction_dispatch|target_instruction_records)|backend_prepared_printer'; } 2>&1 | tee test_after.log`

Result: passed, `3/3` selected tests. Proof log: `test_after.log`.
