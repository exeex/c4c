Status: Active
Source Idea Path: ideas/open/610_rv64_move_bundle_target_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Move-Bundle Residual Diagnostics

# Current Packet

## Just Finished

Completed `plan.md` Step 1: refreshed the current
`unsupported_move_bundle_target_shape` residual diagnostics from
`build/rv64_gcc_c_torture_backend/*/case.log` and classified the current
106-row family by first owner.

Current RV64-ready rows with prepared authority:
- 46 rows are single-move
  `pre_terminator_copies/block_entry/out_of_ssa_parallel_copy` phi joins with
  `phi_join_register_to_register`, `parallel_copy=yes`, register source homes,
  register destination homes, scalar or pointer types, and
  `fragment_status=generic_move_bundle_materialization_failed`. Examples:
  `src/20000314-1.c`, `src/20040309-1.c`, `src/960209-1.c`,
  `src/pr63641.c`.
- 26 rows are larger out-of-SSA edge-preservation register-to-register bundles
  with prepared authority present, but they are not the first packet because
  they require multi-move ordering/materialization. `src/20020206-2.c` is in
  this family: it has `move_count=5`, `authority=out_of_ssa_parallel_copy`,
  `parallel_copy=yes`, complete register-to-register move facts, plus one
  register-to-stack edge-preservation move that should not drive the first
  implementation packet.

Rows excluded from the first code packet:
- 10 block-entry edge-preservation register-to-stack rows are destination
  fan-in or incomplete stack-destination rows, including rows where source or
  destination home/type facts are omitted. Examples: `src/20000706-1.c`,
  `src/20030626-2.c`, `src/pr71631.c`, `src/pr83477.c`.
- 4 phi stack-to-stack rows, 1 phi stack-to-register row, and 2 select
  immediate-materialization rows are not the first RV64 consumer packet because
  they either lack ordinary prepared source/destination home evidence in the
  generic move facts or belong to select-publication immediate handling.
- 2 before-return return-stack-to-register rows are ABI/return move-bundle
  work, not this out-of-SSA block-entry packet.
- 16 `before_instruction/authority=none` rows remain outside this Step 2
  packet. Most are multi-move consumer register-to-stack bundles; one uses a
  pointer-base-plus-offset source home and one uses a floating `double` move.
  They should not be mixed with the block-entry out-of-SSA register-to-register
  implementation.
- `src/pr47337.c` still reports only the coarse
  `unsupported_move_bundle_target_shape` message without event, phase,
  authority, home, type, or move facts; classify it as an evidence gap.

## Suggested Next

Implement Step 2 as a narrow RV64 consumer packet for the single-move
`block_entry/out_of_ssa_parallel_copy/phi_join_register_to_register`
register-to-register family only. Owned implementation surface should be
`src/backend/mir/riscv/codegen/object_emission.cpp`, centered on
`fragment_for_prepared_move_bundle` /
`fragment_for_prepared_out_of_ssa_moves`. The rule should consume only rows
with explicit prepared source register home, destination register home,
`authority=out_of_ssa_parallel_copy`, `parallel_copy=yes`, scalar or pointer
type evidence, and one ordinary move. Exact proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

## Watchouts

- Do not infer missing prepared authority from an encodable RV64 move.
- Keep destination fan-in policy out of this plan.
- Do not touch expectations, unsupported markers, allowlists, timeout files,
  runtime accounting, or unrelated implementation surfaces.
- Reject named-case-only fixes for `src/20020206-2.c` or similar cases.
- Do not use `src/20020206-2.c` as the first proof target; it is useful
  representative evidence, but the current failure is a five-move mixed bundle
  and should wait until the single-move register-to-register rule is proven.
- Keep select-publication immediate materialization, before-return ABI moves,
  before-instruction generic moves, stack destinations, and evidence-gap rows
  out of Step 2.

## Proof

Proof command run for this diagnostic packet:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: passed. Backend subset used: `^backend_`. Log path:
`test_after.log`. The proof is sufficient for this diagnostic-only packet.
