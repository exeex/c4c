Status: Active
Source Idea Path: ideas/open/251_phase_f3_route45_edge_publication_parity_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove or Block x86 Evidence

# Current Packet

## Just Finished

Completed `plan.md` Step 3, "Prove or Block x86 Evidence", as an
analysis-only evidence packet.

Classification: x86 is blocked for the selected Route 5 CFG-edge publication
source identity.

Concrete x86 evidence found:

- `src/backend/mir/x86/prepared/dispatch.cpp` has
  `consume_edge_publication_move_intent(...)`, which reads
  `ConsumedPlans::shared_function_lookups()` and calls
  `prepare::find_unique_indexed_prepared_edge_publication(...)` by
  predecessor label, successor label, and destination prepared value id. It
  copies prepared source/destination ids, checks prepared lookup status and
  prepared move/home rows, renders x86 source/destination operands, and returns
  `EdgePublicationMoveIntentStatus`.
- `append_edge_publication_move_instruction(...)` only appends the rendered
  x86 `mov` text when that prepared-backed intent is `Available`.
- `src/backend/mir/x86/module/module.cpp` function
  `append_prepared_compare_join_parallel_copy(...)` consumes
  `consume_edge_publication_move_intent(...)` for compare-join parallel-copy
  steps and maps statuses to module behavior or diagnostics.
- `tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp`
  covers the prepared x86 consumer in
  `check_x86_consumed_plans_read_shared_edge_publications`,
  `check_x86_edge_publication_move_intent_accepts_register_source_home`,
  `check_x86_edge_publication_move_intent_accepts_rematerialized_immediate_source_home`,
  `check_x86_edge_publication_move_intent_missing_authority`,
  `check_x86_edge_publication_move_intent_rejects_unsupported_homes`, and
  `check_x86_edge_publication_lowering_appends_shared_move_instruction`.

Route 4/5 consumption check:

- AST-backed caller/callee checks confirm
  `consume_edge_publication_move_intent(...)` calls
  `find_unique_indexed_prepared_edge_publication(...)` and
  `render_edge_publication_source_operand(...)`, but not
  `bir::route5_find_cfg_edge_publication(...)`,
  `mir::find_bir_cfg_edge_publication_source_identity(...)`, or any
  `Route5CfgEdgePublicationRecord` / `BirCfgEdgePublicationSourceIdentity`
  consumer.
- AST-backed callee checks for
  `append_prepared_compare_join_parallel_copy(...)` confirm the module path
  calls prepared parallel-copy/value-home helpers and the x86 prepared intent
  helper, but no Route 5/BIR agreement query.
- Focused search found no Route 5 or BIR CFG-edge publication identity
  references under `src/backend/mir/x86` or in
  `tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp`.
  Route 5 evidence exists in shared/riscv-oriented surfaces such as
  `src/backend/bir/bir.cpp`, `src/backend/mir/query.cpp`,
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, and
  `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`, but
  the x86 consumer does not join against it.

Compatibility-owned x86 rows:

- `EdgePublicationMoveIntentStatus::{MissingSharedLookups,MissingPublication,UnsupportedPublication,UnsupportedSourceHome,UnsupportedDestinationHome,Available}`
  remain x86/prepared compatibility rows, not Route 5 semantic proof.
- x86 `mov` instruction text, operand spelling (`ebx`, `eax`,
  `DWORD PTR [rsp + 56]`, immediates), source/destination home acceptance,
  i32 operand filtering, and module diagnostics such as
  "compare-join edge has no shared prepared edge-publication lookup authority"
  and "compare-join edge parallel-copy step has no shared prepared
  edge-publication fact" remain target policy or prepared compatibility output.

Blocker:

- x86 has prepared edge-publication lookup/status consumers, but no direct or
  indirect Route 5/BIR agreement consumer that compares the prepared
  `PreparedEdgePublication` answer with a same-edge
  `Route5CfgEdgePublicationRecord` or
  `BirCfgEdgePublicationSourceIdentity` and rejects disagreement. Therefore
  x86 cannot prove the selected Route 5 CFG-edge publication source identity
  in this plan step.

## Suggested Next

Execute `plan.md` Step 4, "Recheck Riscv Evidence Against the Same Fact", as
an analysis-only packet for the same selected Route 5 CFG-edge publication
source identity.

## Watchouts

- Keep source idea 251 unchanged unless durable intent truly changes.
- Do not implement an adapter during this blocker map.
- Step 4 should test riscv against the same selected Route 5 CFG-edge source
  identity, not against nearby output formatting or carrier/register choices.
- Keep the x86 result classified as blocked unless a later implementation
  introduces a Route 5/BIR agreement consumer or MIR query facade for x86.
- Preserve prepared edge-publication lookup/status, helper/oracle names,
  fallback publication rows, x86 dispatch/status/module output, riscv status
  fields, and riscv/x86 instruction/output strings as compatibility-owned
  unless direct route/BIR agreement proves otherwise.
- Treat testcase-shaped shortcuts, expectation weakening, helper renames, and
  output rewrites as route drift.

## Proof

No build or test proof required; analysis-only packet. AST/focused inspection
used:

- `c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/x86/prepared/dispatch.cpp consume_edge_publication_move_intent build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/x86/prepared/dispatch.cpp consume_edge_publication_move_intent build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/x86/module/module.cpp append_prepared_compare_join_parallel_copy build/compile_commands.json`
- focused `rg` over x86 prepared/module code and x86 edge-publication tests

Local validation: `git diff --check -- todo.md`.
