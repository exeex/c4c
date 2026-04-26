# Current Packet

Status: Active
Source Idea Path: ideas/open/120_bir_raw_label_fallback_cleanup_after_assembler_id_path.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Convert Prepared Pipeline Consumers To Id-First Lookup

## Just Finished

`plan.md` Step 4 converted the owned prepared liveness and nearby stack-layout
consumers to prefer `BlockLabelId` authority before raw block-label spelling.

Completed work:

- `src/backend/prealloc/liveness.cpp`: liveness block ids now translate from
  existing BIR block ids when valid, successor resolution uses terminator label
  ids first, and phi incoming predecessor lookup uses incoming label ids first.
  Raw spelling remains the invalid or unresolved id fallback.
- `src/backend/prealloc/stack_layout/coordinator.cpp`: prepared memory-access
  block labels now come from existing BIR block ids when valid, with raw spelling
  retained as the fallback boundary.
- `src/backend/prealloc/stack_layout/alloca_coalescing.cpp`: phi incoming block
  lookup now indexes blocks by valid BIR label ids first and consults raw labels
  only when the incoming id is invalid.
- `tests/backend/backend_prepare_liveness_test.cpp`: added an id-authoritative
  phi liveness fixture with stale raw block strings to prove block indexing,
  successor resolution, and phi predecessor uses follow ids.
- `tests/backend/backend_prepare_stack_layout_test.cpp`: added an
  id-authoritative prepared-addressing fixture with stale raw block spelling to
  prove stack-layout memory-access block lookup follows ids.

## Suggested Next

Next cleanup packet: convert the remaining prepared consumers called out by the
runbook, especially dynamic-stack, out-of-SSA, regalloc, and backend handoff
lookup sites that still resolve block identity from raw spelling.

## Watchouts

- Preserve BIR dump spelling unless a contract change is explicitly approved.
- Treat invalid or unresolved `BlockLabelId` values as proof gaps, not as a
  reason to add broader raw string matching.
- Do not expand into MIR migration unless the supervisor opens a separate
  initiative.
- Avoid testcase-overfit cleanup that only makes one known case pass.
- `c4c-clang-tool-ccdb` is installed and was used against
  `build-backend/compile_commands.json` to inventory the owned C++ helper
  signatures before editing.
- Do not remove raw label spelling fields yet. They are still the compatibility
  payload for printer output, unresolved-id fallback, and existing downstream
  code not covered by this packet.

## Proof

Passed.

Proof command:
`( cmake -S . -B build-backend -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON && cmake --build build-backend --target backend_prepare_liveness_test backend_prepare_stack_layout_test && ctest --test-dir build-backend -j --output-on-failure -R 'backend_prepare_(liveness|stack_layout)' ) > test_after.log 2>&1`

Proof log: `test_after.log`
