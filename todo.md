Status: Active
Source Idea Path: ideas/open/267_phase_f4_edge_publication_source_producers_x86_consumer_boundary_fail_closed_blocker_map.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Build Fail-Closed Row Map

# Current Packet

## Just Finished

Completed Step 3, `Build Fail-Closed Row Map`, as an analysis-only blocker map
for idea 267. The Step 2 decision is preserved: x86 still has no complete
concrete consumer boundary for the public
`PreparedFunctionLookups::edge_publication_source_producers` relation. The x86
Route 5 edge-publication move gate is only a partial candidate plus
compatibility behavior; non-`LoadLocal` compatibility fallback is not semantic
replacement proof.

Common evidence:

- Prepared source-producer authority is keyed by value name in
  `PreparedEdgePublicationSourceProducerLookups::producers_by_value_name` at
  `src/backend/prealloc/publication_plans.hpp:248`. Producer facts are published
  by `make_prepared_edge_publication_source_producer_lookups(...)` for
  `LoadLocal`, `LoadGlobal`, `Cast`, `Binary`, and `SelectMaterialization` at
  `src/backend/prealloc/select_chain_lookups.cpp:172`; duplicate publication of
  the same value name is collapsed to an empty `Unknown` producer by
  `publish_source_producer(...)` at
  `src/backend/prealloc/select_chain_lookups.cpp:38`.
- Prepared edge-publication rows copy source-producer kind/block/instruction
  and producer pointers into `PreparedEdgePublication` through
  `apply_source_producer_fact(...)` at
  `src/backend/prealloc/prepared_lookups.cpp:401`, copy `LoadLocal`
  source-memory evidence through `apply_source_memory_access_fact(...)` at
  `src/backend/prealloc/prepared_lookups.cpp:467`, and publish the public
  `edge_publication_source_producers` field separately in
  `make_prepared_function_lookups(...)` at
  `src/backend/prealloc/prepared_lookups.cpp:1517`.
- x86 dispatch reads `lookups->edge_publications` through
  `consume_edge_publication_move_intent(...)` at
  `src/backend/mir/x86/prepared/dispatch.cpp:55`; it does not directly consume
  the public `edge_publication_source_producers` lookup.
- x86's partial agreement gate requires exactly one matching Route 5 edge
  record, matching labels/destination/source identity, mapped producer kind,
  prepared producer block/instruction identity, and matching Route 5 producer
  instruction index in `route5_edge_source_agrees_with_prepared_publication(...)`
  at `src/backend/mir/x86/module/module.cpp:4110`. `LoadLocal` additionally
  requires Route 5 `MemorySource` and Route 3 memory agreement at
  `src/backend/mir/x86/module/module.cpp:4187`.
- The x86 outer gate returns `true` for exact agreement, but if agreement fails
  it still returns `true` for every copied publication whose producer kind is
  not `LoadLocal` at
  `src/backend/mir/x86/module/module.cpp:4221`. That fallback preserves
  compatibility behavior and blocks any claim that the gate is a complete
  semantic consumer.
- Route 5/BIR comparison evidence exists in `route5_cfg_edge_publication_record(...)`
  at `src/backend/bir/bir.cpp:1719` and MIR conversion/query helpers at
  `src/backend/mir/query.cpp:689` and `src/backend/mir/query.cpp:1590`. `rg`
  found no x86 calls to `BirCfgEdgePublicationSourceIdentity`,
  `find_bir_cfg_edge_publication_source_identity(...)`,
  `BirCurrentBlockJoinSourceIdentity`, or
  `find_bir_current_block_join_source_identity(...)`.
- RISC-V comparison evidence mirrors the agreement shape in
  `route5_edge_source_agrees_with_prepared_publication(...)` and attaches
  agreement flags to move intents at `src/backend/mir/riscv/codegen/emit.cpp:394`
  and `src/backend/mir/riscv/codegen/emit.cpp:485`, but RISC-V is not the x86
  consumer boundary required by this idea.

Fail-closed row map:

| Row/family | x86 evidence | RISC-V evidence or non-applicability | Preserved compatibility surfaces | Expected fail-closed behavior / blocker |
| --- | --- | --- | --- | --- |
| Duplicate prepared source-producer rows for one value name | Prepared duplicates collapse to `Unknown` in `publish_source_producer(...)` at `src/backend/prealloc/select_chain_lookups.cpp:38`; `apply_source_producer_fact(...)` ignores `Unknown` producers at `src/backend/prealloc/prepared_lookups.cpp:418`, so copied publication identity is not promoted. | Same prepared lookup feeds all targets; RISC-V can observe only the copied publication identity, with agreement comparison at `src/backend/mir/riscv/codegen/emit.cpp:394`. | Keep public `edge_publication_source_producers` and copied `PreparedEdgePublication` fields visible for compatibility and diagnostics. | Duplicate prepared authority must fail closed to no semantic replacement: do not demote or hide the public lookup; a later consumer must reject duplicate authority instead of treating `Unknown` as agreement. |
| Duplicate Route 5 edge records for one prepared publication key | x86 returns `false` when more than one Route 5 candidate matches the prepared publication key at `src/backend/mir/x86/module/module.cpp:4127`. | RISC-V receives at most the selected Route 5 record passed to its adapter and does not provide x86 boundary proof; comparison helper still returns only per-record agreement at `src/backend/mir/riscv/codegen/emit.cpp:394`. | Preserve current x86 compatibility fallback for non-`LoadLocal`; preserve Route 5 debug/query surfaces. | Duplicate Route 5 identity must block exact agreement. For `LoadLocal`, the move gate fails closed; for non-`LoadLocal`, current fallback may still emit and must be recorded as compatibility-only, not proof. |
| Conflict / mismatched producer kind | x86 compares Route 5 source-producer kind to the prepared kind through `route5_source_kind_from_prepared(...)` and rejects mismatches at `src/backend/mir/x86/module/module.cpp:4077` and `src/backend/mir/x86/module/module.cpp:4145`. | RISC-V has the same kind mapping and mismatch rejection at `src/backend/mir/riscv/codegen/emit.cpp:305` and `src/backend/mir/riscv/codegen/emit.cpp:411`. | Keep existing prepared move rendering and target-specific output; do not relabel helper/oracle statuses or fallback names. | Kind conflicts must reject exact agreement. Current x86 non-`LoadLocal` fallback remains compatibility-only and cannot authorize source-producer lookup replacement. |
| Mismatched labels, destination, source value, or producer instruction | x86 rejects predecessor/successor, destination, source kind/type/name, missing source value id, missing prepared producer block/instruction, null Route 5 producer instruction, and instruction-index mismatch at `src/backend/mir/x86/module/module.cpp:4118` and `src/backend/mir/x86/module/module.cpp:4162`. | RISC-V rejects status/key/source mismatches and missing producer instruction identity at `src/backend/mir/riscv/codegen/emit.cpp:399` and `src/backend/mir/riscv/codegen/emit.cpp:427`. | Preserve prepared-printer, route-debug, wrapper, and exact target output behavior. | Mismatches must fail exact agreement. x86 currently enforces this only inside the partial move gate; non-`LoadLocal` fallback remains a blocker for semantic replacement. |
| Missing prepared publication or missing prepared fields | x86 dispatch returns `MissingSharedLookups`, `MissingPublication`, `UnsupportedPublication`, `UnsupportedSourceHome`, or `UnsupportedDestinationHome` from `consume_edge_publication_move_intent(...)` at `src/backend/mir/x86/prepared/dispatch.cpp:62`; the compare-join emitter emits no move unless status is `Available` at `src/backend/mir/x86/module/module.cpp:2548`. | RISC-V returns analogous missing/unsupported statuses in its adapter at `src/backend/mir/riscv/codegen/emit.cpp:501`. | Keep old status names and unsupported behavior. | Missing prepared authority must produce no semantic agreement and no replacement authorization; do not weaken statuses or force availability. |
| Missing Route 5/BIR source identity | x86 `agreed_route5_edge_publication_source(...)` returns `false` when predecessor/successor blocks are missing or the Route 5 match is absent/unavailable at `src/backend/mir/x86/module/module.cpp:4196` and `src/backend/mir/x86/module/module.cpp:4138`. | MIR query records missing predecessor/successor/destination statuses at `src/backend/mir/query.cpp:1590`; RISC-V per-record comparison is non-applicable when no Route 5 record is supplied, aside from leaving agreement unattached at `src/backend/mir/riscv/codegen/emit.cpp:485`. | Preserve compatibility fallback and query/status surfaces. | Missing Route 5 authority blocks exact agreement. For non-`LoadLocal`, x86 still allows compatibility fallback, so missing Route 5 identity is an explicit blocker to any implementation packet. |
| Prepared-only source-producer rows | x86 can consume copied prepared publication fields but not the public source-producer map directly; `apply_source_producer_fact(...)` copies rows at `src/backend/prealloc/prepared_lookups.cpp:401`, while x86 dispatch reads `edge_publications` at `src/backend/mir/x86/prepared/dispatch.cpp:69`. | RISC-V also consumes prepared-backed publication moves and attaches Route 5 agreement only when a Route 5 record is provided at `src/backend/mir/riscv/codegen/emit.cpp:717`. | Preserve public lookup, copied publication fields, prepared output, and wrappers. | Prepared-only rows are compatibility authority only. They must not be counted as semantic replacement because no x86 consumer joins them to same Route 5/BIR identity. |
| Fallback rows, non-`LoadLocal` (`LoadGlobal`, `Cast`, `Binary`, `SelectMaterialization`, `Immediate`, `Unknown`) | x86 returns true after failed agreement for all non-`LoadLocal` publication kinds at `src/backend/mir/x86/module/module.cpp:4221`. Non-`LoadLocal` exact agreement, when present, requires Route 5 status `Available` at `src/backend/mir/x86/module/module.cpp:4182`. | RISC-V records agreement booleans but still has prepared-backed move behavior; its evidence does not close the x86 boundary at `src/backend/mir/riscv/codegen/emit.cpp:485`. | Preserve non-`LoadLocal` fallback output and status/oracle names. | Fallback rows must be labeled compatibility-only. A later implementation must either introduce an x86 fail-closed consumer for these rows or remain blocked. |
| `LoadLocal` memory-source rows | x86 exact agreement requires Route 5 `MemorySource`, available memory identity, predecessor block label id, and Route 3 memory agreement at `src/backend/mir/x86/module/module.cpp:4187`; Route 3/prepared memory comparison checks frame slot, address space, volatility, offset, size, alignment, and base-plus-offset at `src/backend/mir/x86/module/module.cpp:3985`. If a prepared `LoadLocal` source-publication candidate exists and agreement is missing, local-slot rendering returns `std::nullopt` at `src/backend/mir/x86/module/module.cpp:4258` and `src/backend/mir/x86/module/module.cpp:4295`. | RISC-V has comparable Route 3 memory agreement at `src/backend/mir/riscv/codegen/emit.cpp:366` and requires Route 5 `MemorySource` for `LoadLocal` at `src/backend/mir/riscv/codegen/emit.cpp:434`. | Preserve existing prepared local-slot compatibility when no source-publication candidate exists at `src/backend/mir/x86/module/module.cpp:4295`; preserve frame-slot handoff errors. | This is the only row family with a partial x86 fail-closed behavior today. It is still insufficient for lookup replacement because the boundary is copied-publication based and does not cover non-`LoadLocal` rows. |
| Immediate-producer rows | Prepared construction marks immediate source values as `Immediate` without a producer instruction at `src/backend/prealloc/prepared_lookups.cpp:404`; x86 compares Route 5 immediate value identity directly at `src/backend/mir/x86/module/module.cpp:4154`, but the outer gate also treats failed immediate agreement as non-`LoadLocal` fallback at `src/backend/mir/x86/module/module.cpp:4221`. | RISC-V performs the same immediate value comparison at `src/backend/mir/riscv/codegen/emit.cpp:420`. | Preserve immediate encoding and target-owned materialization/output policy. | Immediate mismatch must reject exact agreement, but x86 fallback means immediate rows are compatibility-only unless a later fail-closed consumer removes or narrows that fallback. |
| Policy-sensitive rows: storage, addressing, carrier/helper, formatting, wrapper, ABI, register, stack, instruction spelling, emission policy | x86 agreement checks source identity only at the move gate; dispatch/rendering still uses prepared homes, operands, storage, and emitted instruction text from `consume_edge_publication_move_intent(...)` at `src/backend/mir/x86/prepared/dispatch.cpp:87` and `src/backend/mir/x86/prepared/dispatch.cpp:103`. `LoadLocal` memory rendering verifies the rendered access remains the agreed access and throws on drift at `src/backend/mir/x86/module/module.cpp:4319`. | RISC-V has target-specific operand rendering and agreement metadata, but it is not an x86 policy boundary at `src/backend/mir/riscv/codegen/emit.cpp:459`. | Preserve all target-owned policy surfaces: unsupported expectations, helper/oracle statuses, fallback names, route-debug output, prepared-printer output, wrapper output, exact target output, baselines, ABI/register/stack/instruction spelling, and emission behavior. | Policy-sensitive rows are blockers for migration into BIR or broad prepared retirement. Any later implementation must keep policy target-owned and fail closed on identity disagreement without changing output contracts. |

Step 3 blocker conclusion:

- The row map covers duplicate, conflict, mismatch, missing, prepared-only,
  fallback, `LoadLocal` memory-source, immediate-producer, and
  policy-sensitive rows.
- Exact x86 fail-closed behavior exists only as a partial copied-publication
  check for `LoadLocal` edge-publication moves and local-slot memory rendering.
- No current row proves a complete x86 semantic consumer for the public
  `edge_publication_source_producers` lookup. Implementation remains blocked
  unless a later lifecycle idea names a real x86 consumer boundary that joins
  prepared rows to Route 5/BIR identity and rejects disagreement across all
  required row families.

## Suggested Next

Execute Step 4, `Decide Implementation Eligibility`, using the Step 3 row map:
record that implementation is currently blocked unless the supervisor chooses
to create a separate lifecycle idea for a complete x86 fail-closed consumer
boundary.

## Watchouts

- Do not treat non-`LoadLocal` compatibility fallback as semantic replacement
  proof; `edge_publication_move_allowed_by_route5_agreement_or_compatibility(...)`
  deliberately allows it after failed agreement.
- Do not demote, hide, wrap, or migrate
  `edge_publication_source_producers`; x86 reads copied
  `PreparedEdgePublication` fields, not the public lookup itself.
- RISC-V evidence is useful comparison only. It is not the required x86
  consumer boundary.
- No implementation files, expectations, route-debug output, printer output,
  wrappers, or baselines were changed.

## Proof

Analysis-only proof delegated by supervisor for Step 3:

- `sed -n '1,260p' todo.md` after the update
- `git status --short`
- `git diff --check`

No build or CTest was required for this todo-only boundary packet. Per the
delegated proof command, no `test_after.log` update was made.
