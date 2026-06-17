Status: Active
Source Idea Path: ideas/open/297_rv64_runtime_local_stack_memory_foundation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Focused Backend Validation

# Current Packet

## Just Finished

Step 4 classified `local_dynamic_member_array.c` as out of scope for this
packet because it crosses a helper call and pointer-parameter member indexing.
That would require call/argument and non-local pointer provenance machinery
rather than only local frame-slot base-plus-constant-offset memory.

Step 4 registered `backend_rv64_runtime_packed_local_member_offsets` for
`packed_local_member_offsets.c` with expected qemu exit `8`. The accepted path
stays inside prepared local frame-slot facts: fixed local slots at packed byte
offsets, frame-slot address materialization for the member-array base, and
constant-offset loads selected by the prepared BIR.

rv64 prepared emission now handles I8 local frame-slot stores, unaligned I32
local frame-slot loads/stores bytewise, I32 loads into prepared stack homes,
simple prepared casts/select materialization, and skips only the unused dynamic
pointer add when prepared frame-slot address-materialization metadata owns the
address.

Supervisor review: `review/rv64_step4_route_review.md` found the Step 4 route
on track and not testcase-overfit, with a watch note that the pointer-index skip
must remain limited to prepared frame-slot address materialization.

Step 5 validation ran the active acceptance commands. The rv64 runtime subset
passed with 17 cases. The RISC-V focused subset preserved the known
`backend_riscv_prepared_edge_publication` failure while both
`backend_codegen_route_riscv64_*` tests passed; the same failure was reproduced
with this slice stashed, so it is not introduced by the rv64 local-memory work.

## Suggested Next

Ask the plan owner to decide whether the active runbook and source idea are
complete, or whether the out-of-scope `local_dynamic_member_array.c` call /
pointer-parameter member indexing boundary should remain as a follow-up split.

## Watchouts

- `local_dynamic_member_array.c` remains a split candidate because it involves a
  helper call and pointer-parameter member-array access.
- The new packed support is semantic frame-slot/constant-offset lowering, not
  broad aggregate ABI, dynamic index, call, global, or non-local pointer
  provenance support.
- The unused pointer-add skip is guarded by prepared frame-slot address
  materialization at the same instruction; keep future cases on prepared
  metadata rather than source-shaped matching.
- Broader RISC-V validation currently has a pre-existing
  `backend_riscv_prepared_edge_publication` failure.

## Proof

Ran:

`bash -o pipefail -c "{ cmake --build --preset default && ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure; } 2>&1 | tee test_after.log"`

Result: build succeeded; all 17 `backend_rv64_runtime` tests passed, including
`backend_rv64_runtime_packed_local_member_offsets`.

Proof log: `test_after.log`.

Ran:

`ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`

Result: 2/3 passed. Preserved pre-existing failure:
`backend_riscv_prepared_edge_publication`.
