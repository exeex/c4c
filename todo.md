Status: Active
Source Idea Path: ideas/open/239_aarch64_intrinsic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Post-Carrier Machine-Node Readiness

# Current Packet

## Just Finished

Completed `plan.md` Step 1 inventory for post-carrier AArch64 intrinsic
machine-node readiness.

- Ready for selected-record definition: `Crc/Crc32W`,
  `VectorMemory/VectorLoad`, and `VectorOperation/VectorAdd`.
- CRC32W readiness facts now exist in BIR and prepared carriers: required
  feature `AArch64Crc`, I32 accumulator/data operand roles, unsigned semantics,
  I32 result, operand/result homes, source callee, and prepared call-plan
  authority.
- Vector-load readiness facts now exist in BIR and prepared carriers: required
  feature `AArch64Neon`, pointer operand role, v16i8 shape, 16-byte read memory
  facts, I128 result, operand/result homes, source callee, and prepared
  call-plan authority.
- Vector-add readiness facts now exist in BIR and prepared carriers: required
  feature `AArch64Neon`, vector lhs/rhs operand roles, v16i8 shape, pure vector
  semantics, I128 result, operand/result homes, source callee, and prepared
  call-plan authority.
- Not ready without dependency work: barrier intrinsics, cache-maintenance
  intrinsics, pause/hint intrinsics, and builtin-address intrinsics. The current
  `IntrinsicFamilyKind`/`IntrinsicOperationKind` surface only covers scalar
  `FAbs`, CRC32W, vector load, and vector add; atomic fences and TLS/global
  address materialization are separate structured routes, not intrinsic-family
  carriers for these remaining source-scope families.
- Existing scalar F32/F64 `FAbs` behavior remains the preservation baseline:
  complete carriers select `ScalarFpUnaryIntrinsicRecord` nodes and print
  `fabs` only from selected records with explicit FPR authority.
- Unsupported x86-only intrinsics remain fail-closed policy: BIR rejects
  accepted AArch64 semantic intrinsic lowering for `llvm.x86.*`, and AArch64
  dispatch tests keep unsupported x86 intrinsic calls as ordinary prepared-call
  behavior rather than selected AArch64 intrinsic nodes.

## Suggested Next

Execute `plan.md` Step 2 for the ready families only: define selected AArch64
intrinsic machine-record shapes for CRC32W, vector load, and vector add,
including feature, operation, operand/result register, vector shape,
memory/access, and provenance fields. Do not add printer assembly text yet, and
do not start barrier/cache/pause-hint/builtin-address support until a separate
carrier-authority dependency is planned.

## Watchouts

- Treat scalar `fabs` selected-machine support as existing behavior to preserve.
- Do not select or print from intrinsic spelling, generic call plans, archived
  scratch registers, or final assembly text.
- Barrier/cache/pause-hint/builtin-address work is blocked on upstream semantic
  and prepared intrinsic carriers; adding name-only records for these families
  would be route drift.
- Keep unsupported x86-only and incomplete-fact paths fail-closed.
- Current AArch64 dispatch lowers any prepared intrinsic carrier through the
  scalar-FAbs-specific builder, so complete CRC/vector carriers intentionally
  diagnose `unsupported_intrinsic_family` and emit return-only blocks until
  Step 2/3 add dedicated selected-record paths.

## Proof

Passed delegated proof; output preserved at `test_after.log`.

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_instruction_dispatch|aarch64_machine_printer|prepared_printer|lir_to_bir_notes)'; } 2>&1 | tee test_after.log`
