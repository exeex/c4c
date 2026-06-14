Status: Active
Source Idea Path: ideas/open/265_phase_f4_memory_accesses_unsupported_stale_fail_closed_proof_map.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Gather Memory-Access Evidence

# Current Packet

## Just Finished

Step 1: `Gather Memory-Access Evidence` completed as an analysis-only
inventory for `PreparedFunctionLookups::memory_accesses`.

Evidence inventory:

- Source idea 265 requires a proof map, not implementation, for prepared-only,
  stale-publication, byte-offset drift, and cross-publication mismatch rows.
  It explicitly forbids demotion, deletion, privatization, accessor wrapping,
  adapter migration, helper/status/output weakening, or broad
  `PreparedFunctionLookups` retirement.
- Closed gate 264 classifies `PreparedFunctionLookups::memory_accesses` as
  still blocked. It records that x86 now has selected Route 3 `LoadLocal`
  source-memory agreement and riscv has diagnostic evidence, but public lookup
  demotion is blocked by missing fail-closed proof for prepared-only,
  stale-publication, byte-offset drift, and cross-publication mismatch rows.
- Idea 250 selected the narrow fact family: Route 3 `LoadLocal`
  result/source-memory identity agreement for a prepared dynamic source-memory
  row. Its closure says riscv had diagnostic semantic evidence for agreement,
  mismatch, and cleared-identity rows, while x86 was then blocked by the lack
  of a consumer/query facade joining a same-function, same-block
  `Route3MemoryAccessRecord` to `PreparedEdgePublication::source_memory_access`.
- Idea 258 closed that x86 gap for the selected reader path. The x86 bridge now
  requires matching Route 3 `LoadLocal` memory authority and prepared
  `source_memory_access` before treating prepared data as a semantic mirror.
  Its own closure leaves prepared-only, stale-publication, byte-offset drift,
  and cross-publication mismatch out of scope because those rows need
  synthetic or stale prepared state.
- Idea 261 supplied the supported x86 fixture surface: a joined-branch
  compare-join `EdgeStoreSlot` selected-`LoadLocal` route that naturally
  carries both the Route 3 `LoadLocal` memory record and the prepared
  `source_memory_access` publication into the x86 agreement facade. It proves
  positive rendering plus reachable fail-closed rows for missing source
  address, join-carrier-only drift, and incomplete prepared source-memory
  publication. It also records that prepared-only, stale-publication,
  byte-offset drift, and cross-publication mismatch remain outside that
  supported fixture.
- Idea 262 added the x86 compare-join selected-`LoadLocal` production support
  needed by idea 261, with tests for positive selected-arm rendering and
  missing, incomplete, drifted, or unsupported authoritative metadata failing
  closed instead of falling back past the handoff.
- Idea 254 is the reusable compatibility-retention checklist: missing,
  invalid, duplicate/conflict, mismatch, unsupported, prepared-only, fallback,
  and policy-sensitive behavior must preserve helper/oracle statuses, fallback
  names, route-debug names, prepared printer/debug text, wrapper output,
  `ConsumedPlans`, source-memory statuses, publication statuses, and exact
  target output. It does not authorize implementation, demotion, baseline
  rewrites, or target-policy migration.

Current Route 3 semantic authority candidates:

- Shared/BIR authority is the Route 3 memory/source identity produced from BIR
  memory records, especially `route3_build_memory_access_index(...)`,
  `route3_find_memory_access_record(...)`,
  `find_bir_same_block_load_local_source_identity(...)`, and Route 5 memory
  source records carrying `Route3MemoryAccessRecord` for `LoadLocal`.
- The prepared mirror is `PreparedEdgePublication::source_memory_access` plus
  copied fields such as source-memory base kind, frame slot or pointer value,
  byte offset, size, alignment, address space, volatility, and
  base-plus-offset/materialization flags.
- The public lookup cache remains
  `PreparedFunctionLookups::memory_accesses`, built by
  `make_prepared_memory_access_lookups(...)` from
  `PreparedAddressingFunction::accesses` and exposed through
  position/result-name/result-id helpers. It is still a retained public
  compatibility surface, not a demotion-ready semantic source.

Preserved public compatibility surfaces:

- `PreparedFunctionLookups::memory_accesses`,
  `PreparedMemoryAccessLookups`, `find_prepared_memory_access(...)`,
  `find_indexed_prepared_memory_access(...)`,
  `find_unique_indexed_prepared_memory_access_by_result_value_name(...)`, and
  `find_unique_indexed_prepared_memory_access_by_result_value_id(...)`.
- `PreparedEdgePublication::source_memory_access_status` and stable status
  names, especially `available`, `missing_prepared_memory_access`,
  `incomplete_prepared_memory_access`, and `unavailable`.
- Prepared publication/source helper behavior:
  `apply_source_memory_access_fact(...)`,
  `prepared_edge_publication_source_memory_matches_access(...)`,
  `prepared_and_bir_cfg_edge_publication_source_identity_match(...)`, and the
  route/BIR publication records that expose memory-source identity.
- x86 compatibility and output surfaces:
  `ConsumedPlans::shared_function_lookups()`,
  `render_agreed_route3_load_local_statement_memory_operand(...)`,
  `find_agreed_route3_load_local_source_memory_access(...)`,
  local-slot/compare-join handoff errors, route-debug/handoff-boundary checks,
  wrapper output, exact assembly output, and fallback behavior when there is no
  Route 3 publication candidate.
- riscv compatibility and output surfaces:
  `consume_edge_publication_move_intent(...)`,
  `route3_source_memory_agrees_with_prepared_publication(...)`,
  `route5_edge_source_agrees_with_prepared_publication(...)`,
  `EdgePublicationMoveIntent` status fields, dynamic stack-source instruction
  text, and exact prepared-module output.
- Prepared lookup/printer/helper-oracle tests that assert duplicate ambiguity,
  invalid lookup rejection, source-memory status spelling, and source-memory
  field preservation must remain stable.

Current proof coverage:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` proves
  `memory_accesses` lookup construction, unique result lookup, duplicate-name
  and duplicate-id ambiguity, invalid-name rejection, direct BIR memory access
  identity lookup, prepared source-memory field copying, source-memory offset
  mismatch rejection, unnamed source-memory rejection, and Route 5/Route 3
  load-local memory-source identity exposure.
- `tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp`
  proves the selected x86 compare-join `LoadLocal` surface: positive selected
  arm rendering through Route 3 agreement; rejection of missing source-memory
  authority; join-carrier-only drift rejection; incomplete prepared
  source-memory rejection; missing Route 5 source-memory evidence rejection;
  Route 5/Route 3 source-memory mismatch rejection; and source-producer index
  mismatch rejection.
- `tests/backend/bir/backend_x86_handoff_boundary_local_i16_guard_test.cpp`
  keeps older direct prepared frame-slot memory-access fixture coverage for
  local-slot statement rows, but that is compatibility evidence rather than
  Route 3 demotion proof.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` proves
  riscv dynamic stack-source consumption from shared source-memory authority,
  plus fail-closed behavior for missing shared lookups, missing publication,
  missing prepared memory access, incomplete prepared source memory, and
  non-`LoadLocal` source-memory unavailability. Its output remains riscv
  target policy, not BIR ownership.
- `src/backend/mir/x86/module/module.cpp` compares Route 3 `LoadLocal`
  records against prepared publication fields before rendering selected
  memory operands, including result identity, source memory status, base/frame
  slot, effective byte offset, size, alignment, address space, volatility, and
  candidate uniqueness.
- `src/backend/mir/riscv/codegen/emit.cpp` compares Route 3 and Route 5 memory
  source records against prepared publication fields before recording route
  agreement diagnostics and dynamic stack-source intents.

Missing evidence for later row-map steps:

- No current supported row proves prepared-only memory-access fail-closed
  behavior for `memory_accesses` without mapping the consumer boundary first.
- No current supported row proves stale-publication behavior where old prepared
  source-memory facts disagree with current Route 3 authority.
- No current supported row proves byte-offset drift as a standalone
  `memory_accesses` row separate from the already covered selected x86 Route
  5/Route 3 mismatch and prepared source-memory field mismatch checks.
- No current supported row proves cross-publication mismatch across all public
  prepared/Route 3/Route 5/riscv/x86 publication surfaces.
- Existing x86 positive proof is selected `LoadLocal` only; older direct
  local-slot fallback surfaces and target addressing/output policy remain
  compatibility and target-owned policy, not semantic transfer.

x86/riscv applicability notes:

- x86 is applicable for selected `LoadLocal` source-memory agreement through
  the compare-join selected-arm path and local-slot render bridge. The current
  proof is agreement-gated and intentionally narrow.
- riscv is applicable through dynamic stack-source publication consumption and
  Route 3/Route 5 agreement diagnostics. It proves diagnostic/consumer
  behavior for the same source-memory fact but does not transfer x86 output,
  fallback/status strings, helper/oracle names, registers, addressing
  legality, source-home policy, storage placement, or instruction text.
- Neither target has enough evidence to demote `memory_accesses`; both still
  require explicit fail-closed maps for the prepared-only, stale-publication,
  byte-offset drift, and cross-publication mismatch families.

## Suggested Next

Execute Step 2 by mapping prepared-only memory-access rows at inventory-to-row
granularity: name the consumer boundary, Route 3 authority candidate, retained
prepared compatibility surface, expected fail-closed result, and existing
proof gap for each prepared-only row. Do not claim demotion readiness.

## Watchouts

- This active plan is analysis/proof work only.
- Do not demote, delete, privatize, wrap, or migrate
  `PreparedFunctionLookups::memory_accesses`.
- Do not weaken unsupported expectations, helper/oracle statuses, fallback
  names, route-debug output, prepared-printer output, wrapper output, exact
  target output, or baseline behavior.
- Do not map stale-publication, byte-offset drift, or cross-publication rows
  yet beyond the inventory facts above.
- A supported-path x86/riscv agreement proof is not a public lookup demotion
  proof; the missing synthetic/stale mismatch rows still block demotion.

## Proof

No build/test proof was required for this analysis-only packet. No
`test_after.log` was generated or modified.
