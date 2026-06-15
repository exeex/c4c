Status: Active
Source Idea Path: ideas/open/267_phase_f4_edge_publication_source_producers_x86_consumer_boundary_fail_closed_blocker_map.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Locate x86 Consumer Boundary

# Current Packet

## Just Finished

Completed Step 2, `Locate x86 Consumer Boundary`, as an analysis-only boundary
classification for idea 267.

Boundary decision:

- x86 does not currently have a complete concrete consumer boundary for the
  `edge_publication_source_producers` relation.
- The x86 Route 5 edge-publication move gate is a partial candidate plus
  compatibility behavior, not a full source-producer relation consumer. It can
  reject disagreement for `LoadLocal` edge-publication moves, but it still
  permits non-`LoadLocal` moves when exact Route 5 agreement is absent.
- Therefore Step 2 records no exact x86 identity consumer that can authorize an
  implementation packet for demoting, replacing, or hiding the prepared lookup.

x86 prepared lookup and copied-field readers:

- x86 stores `PreparedFunctionLookups` as an optional aggregate in
  `ConsumedPlans::prepared_lookups`, exposes it with `shared_function_lookups()`,
  and constructs it through `consume_prepared_function_lookups(...)` at
  `src/backend/mir/x86/x86.hpp:14` and `src/backend/mir/x86/x86.hpp:70`.
- x86 prepared dispatch does not read
  `edge_publication_source_producers` directly. `consume_edge_publication_move_intent(...)`
  reads `lookups->edge_publications` and consumes the source/destination homes,
  move status, and copied publication fields on the selected
  `PreparedEdgePublication` at
  `src/backend/mir/x86/prepared/dispatch.cpp:55`.
- Prepared construction copies source-producer facts from the local
  source-producer lookup into each `PreparedEdgePublication` in
  `apply_source_producer_fact(...)`, and copies `LoadLocal` source-memory
  evidence through `apply_source_memory_access_fact(...)`, at
  `src/backend/prealloc/prepared_lookups.cpp:401` and
  `src/backend/prealloc/prepared_lookups.cpp:472`. The x86 gate reads these
  copied publication fields, not the public
  `PreparedFunctionLookups::edge_publication_source_producers` map itself.

Route 5 / BIR identity evidence:

- Route 5 edge records carry source value kind/type/name,
  source-producer kind, source-producer block/instruction identity, and
  optional Route 3 memory identity for `LoadLocal` edge publications through
  `route5_cfg_edge_publication_record(...)` at
  `src/backend/bir/bir.cpp:1719`.
- MIR query conversion preserves Route 5 edge identity in
  `route5_edge_record_to_mir(...)` and rebuilds/query-converts edge records in
  `find_bir_cfg_edge_publication_source_identity(...)` at
  `src/backend/mir/query.cpp:689` and `src/backend/mir/query.cpp:1590`.
- `find_bir_current_block_join_source_identity(...)` publishes Route 5 join
  source facts at `src/backend/mir/query.cpp:1704`, but `rg` found no x86 calls
  to `BirCfgEdgePublicationSourceIdentity`,
  `find_bir_cfg_edge_publication_source_identity(...)`,
  `BirCurrentBlockJoinSourceIdentity`, or
  `find_bir_current_block_join_source_identity(...)`. Current x86 Route 5 use is
  the local edge-publication move gate in `module.cpp`, not the MIR identity API.

Route 5 edge-publication move gate classification:

- `append_prepared_compare_join_parallel_copy(...)` obtains an
  `EdgePublicationMoveIntent` from prepared dispatch, then emits a move only
  when `edge_publication_move_allowed_by_route5_agreement_or_compatibility(...)`
  accepts and the operands are narrow i32 register operands at
  `src/backend/mir/x86/module/module.cpp:2503`.
- `route5_edge_source_agrees_with_prepared_publication(...)` requires an
  available prepared publication, exactly one matching Route 5 edge record,
  matching predecessor/successor labels, matching destination value identity,
  matching source value kind/type/name, matching mapped source-producer kind,
  prepared source-producer block/instruction fields, and matching Route 5
  source-producer instruction index at
  `src/backend/mir/x86/module/module.cpp:4110`.
- For immediate source values, the agreement function checks the Route 5
  immediate value against the prepared publication immediate and can accept
  without a source-producer instruction at
  `src/backend/mir/x86/module/module.cpp:4154`.
- For non-`LoadLocal` named producers, the agreement function accepts only when
  the matched Route 5 record status is `Available` at
  `src/backend/mir/x86/module/module.cpp:4182`.
- For `LoadLocal`, the agreement function requires Route 5 `MemorySource`,
  available source-memory identity, the predecessor block label id, and Route 3
  memory identity agreement through
  `route3_load_local_source_memory_matches_publication(...)` at
  `src/backend/mir/x86/module/module.cpp:4187`.
- The outer gate then returns `true` for exact agreement, but otherwise returns
  `true` for every publication whose copied source-producer kind is not
  `LoadLocal` at `src/backend/mir/x86/module/module.cpp:4221`. That makes
  `LoadGlobal`, `Cast`, `Binary`, `SelectMaterialization`, `Immediate`, and
  `Unknown` non-agreeing rows compatibility-allowed at this x86 move boundary.

Agreement and fallback map:

- Prepared and Route 5/BIR identity must agree for `LoadLocal`
  edge-publication moves before the x86 compare-join parallel-copy move may be
  emitted. If agreement fails and there is a prepared `LoadLocal` publication
  candidate with Route 3 evidence, local-slot compare-load rendering returns
  `std::nullopt` rather than silently using prepared memory at
  `src/backend/mir/x86/module/module.cpp:4258`.
- Prepared and Route 5/BIR identity may agree for non-`LoadLocal` edge
  publications, but x86 does not require that agreement before allowing the
  move. The current fallback is explicit compatibility behavior in
  `edge_publication_move_allowed_by_route5_agreement_or_compatibility(...)`.
- No exact x86 identity consumer exists for the public
  `edge_publication_source_producers` lookup itself. Existing x86 behavior
  consumes prepared edge-publication rows with copied source-producer fields and
  has a partial fail-closed check only at the Route 5 edge-publication move
  gate.

## Suggested Next

Execute Step 3 by building the fail-closed row map for duplicate, conflict,
mismatch, missing, prepared-only, fallback, `LoadLocal` memory-source,
immediate-producer, and policy-sensitive source-producer rows.

## Watchouts

- Step 2 found no complete x86 consumer boundary. Treat the Route 5
  edge-publication move gate as a partial candidate plus compatibility behavior
  until Step 3 maps each fail-closed row.
- `edge_publication_move_allowed_by_route5_agreement_or_compatibility(...)`
  deliberately allows compatibility fallback for non-`LoadLocal` producers when
  Route 5 agreement fails; Step 3 should not treat those rows as proven exact
  identity consumers.
- Prepared edge-publication rows copy source-producer facts into
  `PreparedEdgePublication`; x86 reads those copied fields and the prepared
  move/home data, not the public source-producer lookup directly.
- No implementation files, expectations, route-debug output, printer output,
  wrappers, or baselines were changed.

## Proof

Analysis-only proof delegated by supervisor for Step 2:

- `sed -n '1,260p' todo.md` after the update
- `git status --short`
- `git diff --check`

No build or CTest was required for this todo-only boundary packet. Per the
delegated proof command, no `test_after.log` update was made.
