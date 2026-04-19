# Execution State

Status: Active
Source Idea Path: ideas/open/63_complete_phi_legalization_and_parallel_copy_resolution.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Make Parallel-Copy Resolution Authoritative
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Completed Step 2.2 (`Make Parallel-Copy Resolution Authoritative`) by adding
prepared `parallel_copy_bundles` with explicit per-edge move membership,
ordered resolution steps, and cycle-breaking `SaveDestinationToTemp` markers.
`legalize.cpp` now derives those bundles from authoritative join transfers for
both select-materialized and slot-backed phi carriers, and
`backend_prepare_phi_materialize` now proves both a direct multi-move edge and
a true loop-backedge swap cycle against the published metadata contract.

## Suggested Next

Start Step 3.1 by moving regalloc and immediate edge-move consumers off raw
`bir::PhiInst` reconstruction and onto the new prepared
`parallel_copy_bundles` plus join-transfer contract, beginning with
`append_phi_move_resolution` in `src/backend/prealloc/regalloc.cpp`.

## Watchouts

- Do not treat one additional phi testcase shape as success.
- Keep typed semantic ids as the public identity boundary.
- Do not silently fold branch/join ownership work from idea 62 back into this
  route.
- `prealloc.hpp` still carries transitional enum aliases plus
  `effective_prepared_join_transfer_carrier_kind(...)` so older manual
  prepared-control-flow fixtures that only rewrite `storage_name` or `kind`
  keep working while Step 3.1 migrates consumers to the new surface.
- The new cycle planner keys off authoritative published `bir::Value` names,
  not incidental vector order or later name-table interning.
- `regalloc.cpp` still reconstructs phi moves from surviving `bir::PhiInst` in
  `append_phi_move_resolution`, so the current consumer handoff still lags the
  prepared control-flow contract.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
ran for the Step 2.2 packet and preserved the canonical proof log in
`test_after.log`; `backend_prepare_phi_materialize` stayed green and the subset
returned to the same four baseline failures already present in
`test_before.log`. The remaining baseline reds are
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
