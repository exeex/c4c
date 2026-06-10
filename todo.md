Status: Active
Source Idea Path: ideas/open/169_route3_semantic_memory_access_cache_split.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Contract Only Proven-Private Surface

# Current Packet

## Just Finished

Step 4 of `plan.md` completed as an audit-first contraction pass for the
selected AArch64 FP same-block global-load value materialization family.

No prepared same-block global-load surface was contracted. Direct consumer
evidence:

- `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp` no longer calls
  `prepare::find_prepared_same_block_global_load_access` or
  `prepare::find_prepared_global_load_access`; it uses
  `mir::find_bir_same_block_global_load_access_identity` for semantic identity
  and keeps prepared `PreparedMemoryAccess` only as target-addressing/
  address-policy emission data.
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` still
  directly calls `prepare::find_prepared_same_block_global_load_access` through
  `prepared_shape_same_block_scalar_producer` for a remaining AArch64 same-block
  global-load materialization path.
- `src/backend/mir/aarch64/codegen/globals.cpp` still directly calls
  `prepare::find_prepared_global_load_access` through
  `prepared_current_global_load_access` for current-instruction global-load GOT
  materialization policy.
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`
  and `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` still use
  `prepare::find_prepared_same_block_global_load_access` as the prepared oracle
  for BIR/Route 3 equivalence coverage.

Because remaining non-FP codegen consumers still require the public prepared
semantic lookup, and target/addressing payloads must remain public, Step 4 made
an explicit no-contraction decision.

## Suggested Next

Execute Step 5: Validate And Handoff. Run the delegated narrow proof for the
selected FP migration, then record fresh proof results plus the residual
prepared same-block global-load consumers that keep the public helper surface
alive.

## Watchouts

- `PreparedSameBlockGlobalLoadAccess`,
  `prepare::find_prepared_global_load_access`, and
  `prepare::find_prepared_same_block_global_load_access` remain public by direct
  consumer evidence.
- Target/addressing payloads remain public and are still required for
  address-policy and memory operand emission.
- A future contraction packet should first migrate or explicitly retire the
  `dispatch_value_materialization.cpp` and `globals.cpp` prepared consumers;
  the selected FP migration alone is not enough to hide the helper surface.

## Proof

Not run for this audit-only no-contraction packet, per delegated proof rule.

Audit commands used:
- `rg -n "find_prepared_same_block_global_load_access|find_prepared_global_load_access|PreparedSameBlockGlobalLoadAccess" src tests -g '*.hpp' -g '*.cpp'`
- `c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/prealloc/prepared_lookups.cpp find_prepared_global_load_access build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/prealloc/prepared_lookups.cpp find_prepared_same_block_global_load_access build/compile_commands.json`

Proof log: none for this packet; existing `test_after.log` was not refreshed.
