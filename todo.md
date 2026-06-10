Status: Active
Source Idea Path: ideas/open/169_route3_semantic_memory_access_cache_split.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Candidate Route 3 Consumers

# Current Packet

## Just Finished

Step 1 of `plan.md` completed as an analysis-only inventory.

Selected semantic consumer family: AArch64 floating-point same-block
global-load value materialization. The first Route 3 migration should switch
the `LoadGlobalInst` producer branch in
`src/backend/mir/aarch64/codegen/fp_value_materialization.cpp` from the
prepared same-block global-load semantic lookup to
`mir::find_bir_same_block_global_load_access_identity`, while keeping
`PreparedMemoryAccess` as the target-addressing/address-policy source passed
to `emit_prepared_fp_global_load_to_register`.

Target files for Step 2/3:
- `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp`
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Positive proof cases already nearby:
- Same-block global load identity matches the prepared oracle for a prior
  `LoadGlobalInst` and indexes through Route 3 in
  `backend_aarch64_prepared_memory_operand_records_test.cpp`.
- Shared prepared lookup helper coverage compares prepared and BIR
  same-block global-load identity for `%from_global`.

Negative proof cases already nearby:
- Same-block global-load lookup fails closed before the producer instruction.
- Same-block global-load lookup rejects root type mismatch.
- Route 3 lookup rejects a non-global/root mismatch and string-load shape.

Rejected candidates:
- Direct `PreparedMemoryAccess` consumers in AArch64 `memory.cpp`,
  `globals.cpp`, `frame_slot_address.cpp`, x86 lowering, prealloc call plans,
  publication plans, and variadic entry plans are target-addressing,
  layout/ABI, stack-slot, relocation/TLS, or memory-operand-formation bound;
  migrating those first would violate the Route 3 target-neutral scope.
- AArch64 `dispatch_value_materialization.cpp` same-block global-load
  materialization is already partially BIR-backed and still needs prepared
  fallback/address-policy handling, so it is less bounded as the first
  migration.
- AArch64 `alu.cpp` unpublished load-local source operands depend on
  stack-layout offsets, value homes, source-producer lookups, and frame-slot
  memory operand formation, so they are not a clean first Route 3 consumer.
- AArch64 indirect-callee load-local stored-value source in `calls.cpp`
  crosses call-plan, direct-global select-chain, and source-publication logic;
  defer until the smaller global-load migration proves the pattern.
- `PreparedFunctionLookups` aggregate fields are aggregate-only exposure for
  this step; contraction belongs after a selected consumer migrates and
  residual public uses are re-scanned.

## Suggested Next

Execute Step 2 for the selected FP same-block global-load family: add or
confirm oracle coverage that compares BIR Route 3 and prepared answers for the
FP materialization use case, including prior-global-load positive coverage and
fail-closed coverage for before-producer, root type mismatch, non-global root,
and unsupported/string-load shapes.

## Watchouts

- Step 3 should use Route 3 only for semantic same-block global-load identity;
  do not copy `PreparedAddress`, `PreparedMemoryAccess`, symbol relocation,
  GOT/direct policy, offsets, base-plus-offset legality, or memory operand
  formation into BIR.
- Preserve prepared access lookup as the target-addressing oracle/source until
  a later contraction step proves it private.
- Avoid using the already partially migrated
  `dispatch_value_materialization.cpp` path as proof for the FP consumer unless
  the FP branch itself is exercised.

## Proof

Not run; analysis-only packet. Proposed narrow proof for Step 2/3:
`cmake --build build --target backend_aarch64_prepared_memory_operand_records backend_prepared_lookup_helper && ctest --test-dir build -R '^(backend_aarch64_prepared_memory_operand_records|backend_prepared_lookup_helper)$' --output-on-failure`.
