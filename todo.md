# Current Packet

Status: Active
Source Idea Path: ideas/open/120_bir_raw_label_fallback_cleanup_after_assembler_id_path.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Narrow BIR Construction And Validation Fallbacks

## Just Finished

`plan.md` Step 2 converted the owned ordinary BIR backend fixtures to attach
`BlockLabelId` through available `NameTables` while retaining raw strings as
display text and compatibility payloads.

Completed work:

- `tests/backend/backend_prepare_liveness_test.cpp`: added local fixture
  helpers for id-backed blocks, branch targets, conditional branch targets, and
  phi incoming labels. Manual BIR construction in this file now interns block
  labels through `module.names.block_labels` before the prepared pipeline sees
  the module.
- `tests/backend/backend_prepare_block_only_control_flow_test.cpp`: attached
  ids to the ordinary block-only fixture's block headers and branch target.
- `tests/backend/backend_prepare_structured_context_test.cpp`: attached an id
  to the ordinary BIR call-rendering block and preserved the existing
  id-preferred and explicit raw-fallback label fixtures.

Fallback classification after this packet: ordinary owned construction is now
id-backed construction convenience plus rendering compatibility. Remaining
raw-only owned fixtures are explicit raw fallback coverage for invalid or
unresolved label ids, not silent ordinary construction.

## Suggested Next

Next cleanup packet: update prepared liveness and nearby stack-layout lookup
code to prefer BIR `BlockLabelId` for block indexing, successor resolution, and
phi incoming predecessor lookup, retaining raw spelling only as unresolved-id
fallback.

## Watchouts

- Preserve BIR dump spelling unless a contract change is explicitly approved.
- Treat invalid or unresolved `BlockLabelId` values as proof gaps, not as a
  reason to add broader raw string matching.
- Do not expand into MIR migration unless the supervisor opens a separate
  initiative.
- Avoid testcase-overfit cleanup that only makes one known case pass.
- `c4c-clang-tool-ccdb` is installed, but `build/compile_commands.json` did not
  contain the queried backend sources, so this inventory used targeted `rg` and
  source snippets for those files.
- `clang-format` is not installed in this environment; the C++ edits were kept
  manually formatted.
- Do not remove raw label spelling fields yet. They are still the compatibility
  payload for printer output, unresolved-id fallback, and existing downstream
  code not covered by this packet.

## Proof

Passed.

Proof command:
`( cmake -S . -B build-backend -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON && cmake --build build-backend --target backend_prepare_structured_context_test backend_prepare_liveness_test backend_prepare_block_only_control_flow_test && ctest --test-dir build-backend -j --output-on-failure -R 'backend_prepare_(structured_context|liveness|block_only_control_flow)' ) > test_after.log 2>&1`

Proof log: `test_after.log`
