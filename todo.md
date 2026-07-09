Status: Active
Source Idea Path: ideas/open/634_large_selected_pointer_offset_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define Scratch And Clobber Materialization Contract

# Current Packet

## Just Finished

Step 3 of idea `634` added the RV64 consumer-side classifier `rv64_large_selected_pointer_offset_materialization_status` beside the existing narrow `prepared_pointer_value_base_offset` path.

The Step 3 contract recognizes only selected pointer-value local-memory rows that have default address space, nonvolatile access, pointer-value base/name, base-plus-offset addressing, matching width/alignment, a resolvable base GPR, a non-12-bit offset, and available compatible requested-range facts. When those facts are present, the classifier currently returns `MissingScratchClobberAuthority` because no explicit local-memory-row scratch/clobber publication exists yet.

The existing immediate path is preserved: narrow selected pointer offsets still use `prepared_pointer_value_base_offset`, while large selected pointer offsets reject that path and classify as missing scratch/clobber authority only after range facts are present. Large selected pointer rows without requested-range facts remain `NotApplicable` and fail closed before scratch policy.

## Suggested Next

Step 4 should publish explicit scratch/clobber authority for large selected pointer-value local-memory materialization, or consume an already-approved equivalent contract if one exists. Once that authority is explicit, wire the classifier into the local-memory diagnostic/emission path and materialize only rows that satisfy the Step 3 facts plus scratch/clobber safety.

## Watchouts

The Step 3 slice intentionally does not admit large selected pointer-offset rows and does not change external expectations. `object_emission.cpp` still emits the generic local-memory rejection until Step 4 owns diagnostic/emission consumption. Do not bypass the new classifier with filename predicates, exact-offset predicates, or implicit use of `t0`; Step 4 needs an explicit scratch/clobber publication or approved existing authority.

## Proof

Ran the delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

`test_after.log` shows `backend_riscv_object_emission` passed, 1/1 tests, 0 failures.
