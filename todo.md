# Current Packet

Status: Active
Source Idea Path: ideas/open/120_bir_raw_label_fallback_cleanup_after_assembler_id_path.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Convert Prepared Pipeline Consumers To Id-First Lookup

## Just Finished

`plan.md` Step 4 converted the owned out-of-SSA prepared consumers to prefer
`BlockLabelId` authority before raw block-label spelling.

Completed work:

- `src/backend/prealloc/out_of_ssa.cpp`: ordinary phi lowering, reducible phi
  materialization traversal, join-transfer edge publication, select-materialized
  join discovery, and continuation metadata block lookups now prefer existing
  BIR block ids translated through prepared names before raw block-label
  spelling.
- `tests/backend/backend_prepare_phi_materialize_test.cpp`: added a loop phi
  fixture with stale raw block and terminator spelling but authoritative
  `BlockLabelId` fields, proving loop-carry classification, edge transfer ids,
  parallel-copy bundle lookup, and predecessor store insertion follow ids.

## Suggested Next

Next cleanup packet: convert the remaining prepared consumers called out by the
runbook, especially dynamic-stack, regalloc, and backend handoff lookup sites
that still resolve block identity from raw spelling.

## Watchouts

- Preserve BIR dump spelling unless a contract change is explicitly approved.
- Treat invalid or unresolved `BlockLabelId` values as proof gaps, not as a
  reason to add broader raw string matching.
- Do not expand into MIR migration unless the supervisor opens a separate
  initiative.
- Avoid testcase-overfit cleanup that only makes one known case pass.
- Do not remove raw label spelling fields yet. They are still the compatibility
  payload for printer output, unresolved-id fallback, and existing downstream
  code not covered by this packet.
- `c4c-clang-tool-ccdb` was used against `build-backend/compile_commands.json`
  to inventory `src/backend/prealloc/out_of_ssa.cpp` helper signatures before
  editing.

## Proof

Passed.

Proof command:
`( cmake -S . -B build-backend -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON && cmake --build build-backend --target backend_prepare_phi_materialize_test backend_prepare_authoritative_join_ownership_test backend_prepare_structured_context_test backend_prepared_printer_test && ctest --test-dir build-backend -j --output-on-failure -R 'backend_prepare_(phi_materialize|authoritative_join_ownership|structured_context)|backend_prepared_printer' ) > test_after.log 2>&1`

Proof log: `test_after.log`
