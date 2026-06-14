Status: Active
Source Idea Path: ideas/open/252_phase_f3_edge_publication_source_producer_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove or Block x86 Evidence

# Current Packet

## Just Finished

Completed Step 3, "Prove or Block x86 Evidence", as an analysis-only x86
classification for idea 252.

Classification: x86 is blocked for the selected same-edge CFG publication
source-producer identity relation, not proven and not explicitly
non-applicable.

x86 prepared-publication evidence:

- `src/backend/mir/x86/prepared/dispatch.cpp:55` defines
  `consume_edge_publication_move_intent(...)`. AST-backed callee tracing showed
  that it reads `ConsumedPlans::shared_function_lookups()`, then calls
  `prepare::find_unique_indexed_prepared_edge_publication(...)` on
  `lookups->edge_publications`; it does not read
  `edge_publication_source_producers`, `PreparedEdgePublication` source-producer
  fields, `BirCfgEdgePublicationSourceIdentity`, or
  `bir::Route5CfgEdgePublicationRecord`.
- The same function turns prepared publication status and prepared homes into
  an `EdgePublicationMoveIntent`: `MissingSharedLookups`,
  `MissingPublication`, `UnsupportedPublication`, `UnsupportedSourceHome`,
  `UnsupportedDestinationHome`, or `Available`, plus final `mov` text from
  prepared source/destination homes.
- `src/backend/mir/x86/module/module.cpp:2523` is the production caller inside
  `append_prepared_compare_join_parallel_copy(...)`. It uses the prepared
  move-intent status to emit an i32 `mov` only when
  `prepared_edge_publication_move_has_i32_operands(...)` accepts the operands,
  and otherwise preserves prepared lookup/value-home error behavior or silently
  leaves unsupported publication/home cases as non-emitted target policy.
- `src/backend/mir/x86/prepared/prepared.hpp:154` exposes the
  `EdgePublicationMoveIntentStatus` rows, which are compatibility/output
  surface rows for the x86 prepared edge-publication bridge rather than
  semantic source-producer proof.

Route/BIR evidence gap:

- `src/backend/mir/query.hpp:218` defines
  `BirCfgEdgePublicationSourceIdentity` with the selected semantic fields:
  source value identity, `source_producer`, `source_producer_kind`,
  `source_producer_block_label_id`, and
  `source_producer_instruction_index`.
- `src/backend/mir/query.cpp:690` converts a
  `bir::Route5CfgEdgePublicationRecord` into that MIR identity, including
  `route5_edge_source_producer_to_mir(...)` and optional Route 3
  source-memory access.
- `src/backend/mir/query.cpp:1591` defines
  `find_bir_cfg_edge_publication_source_identity(...)`; targeted caller
  tracing in that translation unit found no caller, and repo search shows its
  consumers are helper/oracle tests and riscv diagnostics, not x86 production
  code.
- Focused x86 search found no use of
  `find_bir_cfg_edge_publication_source_identity`,
  `BirCfgEdgePublicationSourceIdentity`,
  `Route5CfgEdgePublicationRecord`, or
  `edge_publication_source_producers` under `src/backend/mir/x86`.

Rows that remain compatibility-owned for x86:

- Prepared move-intent status rows:
  `MissingSharedLookups`, `MissingPublication`, `UnsupportedPublication`,
  `UnsupportedSourceHome`, `UnsupportedDestinationHome`, and `Available`.
- Prepared publication lookup status, prepared source/destination homes,
  out-of-SSA parallel-copy/move-bundle authority, and the compare-join errors:
  `compare-join edge has no shared prepared edge-publication lookup authority`
  and `compare-join edge parallel-copy step has no shared prepared
  edge-publication fact`.
- x86 operand and output policy: i32-only acceptance,
  register-vs-memory filtering, source/destination operand spelling, and final
  `mov <dest>, <source>` text.

Blocker:

- x86 has a prepared edge-publication move/home consumer, but no direct or
  indirect x86 consumer that joins the same-edge prepared
  `PreparedEdgePublication` answer to a Route 5/BIR
  `BirCfgEdgePublicationSourceIdentity` / `Route5CfgEdgePublicationRecord`
  source-producer identity and rejects disagreement. Until such an x86 query
  facade or bridge exists, prepared move/home consumption and emitted `mov`
  rows cannot count as semantic source-producer proof.

## Suggested Next

Execute Step 4, "Recheck Riscv Evidence Against the Same Relation", as an
analysis-only packet. Recheck riscv evidence against the same selected
same-edge CFG publication source-producer identity relation, separating
Route 5/BIR semantic agreement rows from prepared lookup/status, fallback, and
target emission policy.

## Watchouts

- Keep source idea 252 unchanged unless durable intent truly changes.
- Do not implement a BIR producer index or adapter in this blocker-map plan.
- Do not delete, privatize, wrap, rename, or bypass public prepared
  `edge_publication_source_producers` helpers.
- Step 3 leaves x86 blocked, not non-applicable: x86 consumes prepared
  edge-publication move/home status but has no Route 5/BIR source-producer
  agreement consumer.
- Do not infer x86 readiness from prepared edge-publication move consumption,
  `mov` output, operand spelling, module diagnostics, helper names, prepared
  publication statuses, or value-home checks.
- Do not infer readiness from Route 5 edge/source evidence alone; it is only
  the selected semantic fact to prove, block, or exclude per target.
- Treat RISC-V Route 5/Route 3 agreement rows as diagnostic evidence until
  Step 4 rechecks them against this exact selected relation; prepared output
  and fallback remain authoritative.
- Preserve helper/oracle names, compatibility strings, fallback names,
  publication/output rows, and target-policy-sensitive behavior.
- Treat testcase-shaped shortcuts, expectation weakening, helper/status/output
  renames, and classification-only maps claimed as implementation progress as
  route drift.

## Proof

Analysis-only packet. No build or test proof required by supervisor.
Local validation: `git diff --check -- todo.md`.
