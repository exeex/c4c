Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Diagnose Unchanged Store Local-Memory Representative Failure

# Current Packet

## Just Finished

Step 8 follow-up - Diagnose unchanged store local-memory representative
failure completed as an inspection-only packet for `src/20001026-1.c`.

The failing C source lowers the representative statement
`args->d = real_value_from_int_cst(args->type, args->i)` to this LLVM/LIR shape
inside `build_real_from_int_cst_1`:
`%t6 = call %struct._anon_0 ...`, `%t8 = getelementptr %struct.brfic_args, ptr
%t7, i32 0, i32 2`, then `store %struct._anon_0 %t6, ptr %t8`.

Concrete remaining boundary: aggregate-field store publication for a
pointer-addressed aggregate destination. This is not the direct local-slot
scalar publication boundary repaired by Step 6. The source aggregate value is
already expected to be represented by local aggregate slots, and the destination
GEP is represented by `pointer_value_addresses_` as a runtime pointer base plus
the `struct brfic_args.d` byte offset. However, the aggregate store branch in
`lower_memory_store_inst` currently requires `store.ptr` to be a
`local_aggregate_slots_` key before it considers aggregate-copy lowering; for
`%t8`, that lookup fails and the function returns `false` before pointer
provenance or route-consumer publication can admit the store.

Classification: aggregate-field store publication / pointer-addressed
aggregate store admission gap. It is not direct local-slot scalar publication,
not scalar store admission classification, and not an RV64/MIR-side inference
problem.

## Suggested Next

Proceed with a narrow semantic BIR packet that adds the symmetric
pointer-addressed aggregate-store path. The focused BIR test should model a
`20001026-1`-style aggregate return value stored through a runtime pointer GEP
to an aggregate field and assert that lowering emits scalar leaf
`StoreLocalInst` records with `MemoryAddress::BaseKind::PointerValue`,
requested ranges at the destination base offset plus each aggregate leaf
offset, and provenance tied to the runtime pointer base.

Suggested focused test name:
`expect_pointer_addressed_aggregate_field_store_publishes_leaf_stores`.

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, and RV64/MIR inference.
Do not infer prepared/route compatibility from the RV64 row alone.

The `src/20001026-1.c` representative is still failing at the same admitted
family label as before Step 8, so the next packet needs a local semantic
admission diagnosis before any expectation, allowlist, runtime comparison, or
prepared-object route work.

On RISC-V, the field GEP may remain a `PointerAddress` record with a base
pointer value plus byte offset rather than a separately materialized `%t8`
pointer value. The implementation should consume `PointerAddress` and add its
stored byte offset to each aggregate leaf offset; do not infer the destination
from RV64/MIR behavior or introduce a named-case shortcut.

Remaining neighboring representative families are still intentionally left for
later packets: load
`src/20000314-1.c`, GEP `src/20000717-4.c`, scalar/local-memory
`src/20000519-1.c`, and alloca `src/20050604-1.c`.

## Proof

Inspection packet only; no code/tests changed, so no new build proof was run.

Commands/logs inspected:
- `build/rv64_gcc_c_torture_backend/src_20001026-1.c/case.log`
- `./build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20001026-1.c`
- `./build/c4cll --codegen llvm --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20001026-1.c`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/aggregate.cpp`

Existing proof log preserved: `test_after.log`.
