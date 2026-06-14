Status: Active
Source Idea Path: ideas/open/252_phase_f3_edge_publication_source_producer_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Recheck Riscv Evidence Against the Same Relation

# Current Packet

## Just Finished

Completed Step 4, "Recheck Riscv Evidence Against the Same Relation", as an
analysis-only RISC-V classification for idea 252.

Classification: RISC-V is proven for the selected same-edge CFG publication
source-producer identity relation as diagnostic agreement evidence, not as
authority transfer. RISC-V has an explicit prepared-to-Route 5/Route 3
agreement bridge and tests for agreeing and non-agreeing rows, while prepared
lookup/status/fallback/output remains authoritative.

Semantic Route 5/BIR agreement evidence:

- `src/backend/mir/riscv/codegen/emit.hpp:59` exposes optional diagnostic
  fields on `EdgePublicationMoveIntent`: `route5_edge_status`,
  `route5_edge_source_agrees`, and `route3_source_memory_agrees`.
- `src/backend/mir/riscv/codegen/emit.hpp:84` declares the overload of
  `consume_edge_publication_move_intent(...)` that accepts a
  `const bir::Route5CfgEdgePublicationRecord*` for the same predecessor,
  successor, and destination value relation.
- `src/backend/mir/riscv/codegen/emit.cpp:305` maps prepared
  `PreparedEdgePublicationSourceProducerKind` rows to
  `bir::Route5PublicationSourceKind`, covering `Immediate`, `LoadLocal`,
  `LoadGlobal`, `Cast`, `Binary`, `SelectMaterialization`, and `Unknown`.
- `src/backend/mir/riscv/codegen/emit.cpp:366` defines
  `route3_source_memory_agrees_with_prepared_publication(...)`; for a
  `LoadLocal` source it checks prepared source memory access status, Route 3
  node kind, result value, address space, volatility, byte offset, size,
  alignment, and base identity.
- `src/backend/mir/riscv/codegen/emit.cpp:394` defines
  `route5_edge_source_agrees_with_prepared_publication(...)`; it requires
  Route 5 status `Available` or `MemorySource`, same predecessor/successor
  labels, same destination value name/type, same source value kind/type, and
  matching prepared-vs-Route 5 source producer kind. Named sources also require
  a Route 5 source producer instruction and instruction index. `LoadLocal`
  sources additionally require `MemorySource`, source-memory identity, and
  Route 3 agreement.
- `src/backend/mir/riscv/codegen/emit.cpp:485` attaches those Route 5/Route 3
  results to the intent when a Route 5 record is supplied; `emit.cpp:717`
  threads the optional Route 5 record through the public helper overload.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1594`
  starts `check_route5_route3_oracle_rows_preserve_prepared_riscv_fallback()`.
  The scalar case proves an agreeing Route 5 edge with status `available`,
  same edge labels, `%src` -> `%dst`, `Binary` producer kind, and the
  predecessor instruction pointer; the helper records
  `route5_edge_source_agrees`.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1652`
  checks non-agreeing Route 5 rows: destination-type mismatch reports
  `no_match`, absent predecessor reports `no_source`, and the helper records
  `route5_edge_source_agrees == false`.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1705`
  checks the dynamic memory-source relation: Route 3 exposes `load_local`,
  pointer-value base `%base`, byte offset 12, size 4, and Route 5 reports
  `memory_source`; the helper records both `route5_edge_source_agrees` and
  `route3_source_memory_agrees`.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1749`
  and `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1768`
  prove mismatched or incomplete Route 3 memory identity rows clear both
  agreement booleans.

Prepared lookup/status, fallback, output, and target emission policy that
remain non-semantic:

- `src/backend/mir/riscv/codegen/emit.cpp:468` still finds the concrete
  publication through `prepare::find_unique_indexed_prepared_edge_publication`
  on `lookups->edge_publications`; this is prepared publication lookup
  authority, not Route 5/BIR authority.
- `src/backend/mir/riscv/codegen/emit.cpp:501` returns prepared/status-driven
  intent rows: `MissingSharedLookups`, `MissingPublication`,
  `UnsupportedPublication`, `UnsupportedSourceHome`,
  `UnsupportedDestinationHome`, or `Available`.
- `src/backend/mir/riscv/codegen/emit.cpp:528` through `emit.cpp:699` keeps
  publication availability, source/destination homes, stack-slot width/offset
  policy, pointer-base materialization policy, and instruction text as
  prepared-backed target policy.
- `src/backend/mir/riscv/codegen/emit.cpp:733` appends output only when the
  prepared-backed intent status is `Available`; it calls the overload without
  a Route 5 record, so normal emission does not require Route 5 agreement.
- `src/backend/mir/riscv/codegen/emit.cpp:747` emits prepared modules by
  iterating `lookups.edge_publications.publications` and appending
  prepared-backed edge-publication moves.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1641`
  proves missing prepared publication still returns `MissingPublication` even
  when Route 5 facts exist.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1674`,
  `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1750`,
  and `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1769`
  prove non-agreeing Route 5/Route 3 rows preserve prepared fallback output
  (`mv a1, a0` or `lw a1, 12(s2)`) instead of rejecting emission.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1797`
  keeps missing lookup, missing publication, malformed homes, unsupported
  sources/destinations, and non-move publication rows fail-closed through the
  prepared status surface.

Boundary:

- RISC-V evidence proves the same selected relation can be observed and
  classified against Route 5/Route 3 facts. It does not prove adapter
  readiness or fail-closed source-producer authority transfer, because
  disagreement currently clears diagnostic agreement booleans while preserving
  prepared-backed fallback/output.

## Suggested Next

Execute Step 5, "Build the Producer/Source Fail-Closed Matrix", as an
analysis-only packet. Build the duplicate, conflict, mismatch, absent,
unsupported, prepared-only, fallback, and target-policy matrix needed before a
later adapter can turn RISC-V diagnostic agreement evidence, x86 blocker
evidence, and prepared compatibility rows into any source-producer authority
claim.

## Watchouts

- Keep source idea 252 unchanged unless durable intent truly changes.
- Do not implement a BIR producer index or adapter in this blocker-map plan.
- Do not delete, privatize, wrap, rename, or bypass public prepared
  `edge_publication_source_producers` helpers.
- Step 3 left x86 blocked, not non-applicable: x86 consumes prepared
  edge-publication move/home status but has no Route 5/BIR source-producer
  agreement consumer.
- Step 4 leaves RISC-V proven for diagnostic same-edge Route 5/Route 3
  agreement evidence, but not proven for fail-closed authority transfer:
  non-agreeing Route rows preserve prepared fallback/output.
- Keep RISC-V semantic rows separate from prepared lookup/status rows:
  `route5_edge_status`, `route5_edge_source_agrees`, and
  `route3_source_memory_agrees` are diagnostic agreement rows; instruction
  text, fallback behavior, source/destination homes, and status rows remain
  compatibility or target policy.
- Do not infer x86 readiness from prepared edge-publication move consumption,
  `mov` output, operand spelling, module diagnostics, helper names, prepared
  publication statuses, or value-home checks.
- Do not infer adapter readiness from RISC-V Route 5/Route 3 agreement alone;
  Step 5 still needs fail-closed duplicate, conflict, mismatch, absent,
  prepared-only, fallback, unsupported, and policy-sensitive rows.
- Preserve helper/oracle names, compatibility strings, fallback names,
  publication/output rows, and target-policy-sensitive behavior.
- Treat testcase-shaped shortcuts, expectation weakening, helper/status/output
  renames, and classification-only maps claimed as implementation progress as
  route drift.

## Proof

Analysis-only packet. No build or test proof required by supervisor.
Local validation: `git diff --check -- todo.md` passed.
