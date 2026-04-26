# Current Packet

Status: Active
Source Idea Path: ideas/open/120_bir_raw_label_fallback_cleanup_after_assembler_id_path.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Convert Prepared Pipeline Consumers To Id-First Lookup

## Just Finished

`plan.md` Step 4 converted the owned dynamic-stack prepared consumer to prefer
`BlockLabelId` authority before raw block-label spelling.

Completed work:

- `src/backend/prealloc/prealloc.cpp`: `populate_dynamic_stack_plan` now
  resolves each operation block from the existing BIR `BlockLabelId` through
  prepared names first, and falls back to raw block spelling only when the id is
  invalid or unresolved.
- `tests/backend/backend_prepare_frame_stack_call_contract_test.cpp`: added a
  stale-raw-block fixture whose `label_id` carries the canonical block spelling,
  proving dynamic-stack operation metadata follows id authority.

## Suggested Next

Next cleanup packet: convert the remaining prepared consumers called out by the
runbook, especially regalloc and backend handoff lookup sites that still resolve
block identity from raw spelling.

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
- `clang-format` is not installed in this container, so this packet was kept
  manually formatted.

## Proof

Passed.

Proof command:
`( cmake -S . -B build-backend -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON && cmake --build build-backend --target backend_prepare_frame_stack_call_contract_test c4cll && ctest --test-dir build-backend -j --output-on-failure -R 'backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_vla_goto_stackrestore_cfg' ) > test_after.log 2>&1`

Proof log: `test_after.log`
