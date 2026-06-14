Status: Active
Source Idea Path: ideas/open/250_phase_f3_route3_memory_source_parity_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select One Candidate Memory-Source Identity Fact

# Current Packet

## Just Finished

Completed `plan.md` Step 2, "Select One Candidate Memory-Source Identity
Fact", as an analysis-only selection.

Selected fact:

- Route 3 `LoadLocal` result/source-memory identity agreement for a prepared
  edge-publication dynamic source-memory row.
- The semantic fact is that a route/BIR `Route3MemoryAccessRecord` for a
  same-function, same-block `LoadLocal` source identifies the same memory
  access as the prepared publication's `PreparedMemoryAccess` source when both
  sides describe the same loaded result value and memory-source identity.

Agreement rule:

- Prepared side: `PreparedEdgePublicationSourceMemoryAccessStatus::Available`
  and a non-null `PreparedEdgePublication::source_memory_access` produced by
  `apply_source_memory_access_fact(...)` from a prepared `LoadLocal` memory
  access. Missing or incomplete prepared facts remain observable as
  `MissingPreparedMemoryAccess` or `IncompletePreparedMemoryAccess`.
- Route/BIR side: an available `Route3MemoryAccessRecord` whose node kind is
  `LoadLocal`, whose result identity is available and has the same kind, type,
  and value name as the prepared source-memory result, and whose memory-source
  fields agree with the prepared source memory on address space, volatility,
  byte offset, size, align, and base kind/identity.
- Agreement is true only when both sides are complete and all selected
  identity fields match. Missing prepared lookup, missing Route 3 identity,
  incomplete Route 3 identity, duplicate/ambiguous prepared lookup, mismatched
  result identity, mismatched base identity, mismatched layout fields, or
  unsupported source kind must fail closed and preserve existing status rows.

Target policy and compatibility rows excluded from semantic identity:

- x86 stack/global operand spelling, `BYTE`/`WORD`/`DWORD` size spelling,
  frame-slot offsets, same-module global span policy, base-plus-offset
  legality, local/global storage placement, register homes, value-home
  requirements, handoff diagnostics, and final instruction text remain
  target/prepared compatibility.
- RISC-V `lw` text, signed-12-bit offset legality, base register selection,
  destination register placement, source-home policy, move authority, fallback
  instruction text, and route/status strings such as `"memory_source"`,
  `"missing_source_memory_access"`, `"incomplete_source_memory_access"`,
  `"no_match"`, and `"no_source"` remain compatibility or target policy.
- Prepared helper/oracle names and status strings remain compatibility:
  `"missing_prepared_memory_access"`,
  `"incomplete_prepared_memory_access"`, duplicate prepared lookup ambiguity,
  invalid value names, prepared-only rows, and unsupported/fallback rows are
  not semantic wins.

Alternatives considered and not selected:

- x86 local/global prepared memory operand identity was not selected because
  Step 1 found prepared-memory consumers but no direct Route 3 production
  consumer under `src/backend/mir/x86`; that makes it a Step 3 blocker
  candidate, not the strongest shared fact.
- Same-block load-local stored-value source agreement was not selected because
  it is a useful helper/query oracle but has weaker emitted-output evidence
  than the RISC-V dynamic source-memory publication path.
- Generic prepared `memory_accesses` parity was not selected because it mixes
  semantic identity with addressing mode, frame/global layout, storage policy,
  register materialization, duplicate lookup behavior, status strings, and
  target output formatting.
- Route 6 call-argument memory-source identity was not selected because the
  current plan is Route 3 memory/source parity and Step 1 found Route 5/RISC-V
  publication as the active consumer family.

Why broader memory parity is outside this plan:

- Broad memory parity would require proving all prepared memory lookup helpers,
  x86 stack/global memory rendering, RISC-V fallback behavior, Route 5/Route 6
  source uses, duplicate/invalid lookup behavior, storage layout, addressing
  legality, and emitted instruction text as one authority migration. Step 2
  selects only one semantic `LoadLocal` source-memory identity fact so later
  steps can either prove a bounded adapter or block it precisely without
  weakening prepared compatibility rows.

Targeted Step 2 confirmation commands used:

- `rg -n "route3_source_memory_agrees|memory_source|source_memory_access_status|PreparedEdgePublicationSourceMemoryAccessStatus|route3_source_memory_agrees_with_prepared_publication" src/backend/mir/riscv tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- `rg -n "route3_find_memory_access_record|find_bir_memory_access_identity|source_memory_identity_available|Route3MemoryAccessRecord|route3_source_memory_agrees" src/backend/mir/x86 src/backend/mir/query.cpp src/backend/bir/bir.hpp`
- `rg -n "prepared_edge_publication_source_memory_matches_access|apply_source_memory_access_fact|PreparedEdgePublicationSourceMemoryAccessStatus|MissingPreparedMemoryAccess|IncompletePreparedMemoryAccess|source_memory_access" src/backend/prealloc src/backend/bir/bir.hpp`

Targeted Step 2 evidence:

- `src/backend/mir/riscv/codegen/emit.cpp:366` defines
  `route3_source_memory_agrees_with_prepared_publication(...)`; `:438` and
  `:495` feed the agreement result into RISC-V publication/move intent.
- `src/backend/mir/riscv/codegen/emit.hpp:71` carries
  `route3_source_memory_agrees`.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1725`,
  `:1743`, `:1761`, and `:1780` cover the `"memory_source"` row and
  agreeing, mismatched, and incomplete Route 3 source-memory agreement flags.
- The x86-targeted Route 3 query returned no `src/backend/mir/x86` hits, while
  `src/backend/bir/bir.hpp:1756` exposes
  `Route5CfgEdgePublicationRecord::source_memory_identity_available` and
  `:1757` exposes its `Route3MemoryAccessRecord source_memory_access`.
- `src/backend/mir/query.cpp:1419` exposes
  `find_bir_memory_access_identity(...)`, but Step 1 found no x86 production
  consumer for that query facade.
- `src/backend/prealloc/publication_plans.hpp:65`, `:274`, and `:376` define
  prepared source-memory status and retained source-memory fields;
  `src/backend/prealloc/prepared_lookups.cpp:461` applies the prepared
  source-memory fact; `src/backend/prealloc/publication_plans.cpp:877`
  remains the prepared comparison oracle.

Prior Step 1 inventory evidence retained below for continuity.

Evidence commands used:

- `rg -n "memory_accesses|PreparedFunctionLookups|prepared.*memory|memory.*prepared" src tests docs ideas/open/250_phase_f3_route3_memory_source_parity_blocker_map.md`
- `rg -n "Route 3|Route3|route 3|route3|memory_source|source_value_id|memory.*source|source.*memory|BirMemory|MemoryAccess|MemorySource" src tests docs ideas/open/250_phase_f3_route3_memory_source_parity_blocker_map.md`
- `rg -n "struct PreparedMemoryAccess|struct PreparedMemoryAccessLookups|make_prepared_memory_access_lookups|find_.*prepared_memory_access|prepared_edge_publication_source_memory|source_memory_access_status|PreparedEdgePublicationSourceMemoryAccessStatus" src/backend tests/backend/bir/backend_prepared_lookup_helper_test.cpp tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp tests/backend/bir/backend_x86_handoff_boundary_multi_defined_call_test.cpp`
- `rg -n "Route3MemoryAccessRecord|Route3MemoryAccessValueRecord|Route3MemoryAccessIndex|Route3SameBlockGlobalLoadAccessRecord|Route3SameBlockLoadLocalSourceRecord|Route3SameBlockLoadLocalStoredValueSourceRecord|route3_build_memory_access_index|route3_find_memory_access_record|route3_find_same_block_global_load_access|route3_find_same_block_load_local" src/backend tests/backend/bir/backend_prepared_lookup_helper_test.cpp tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp tests/backend/bir/backend_x86_handoff_boundary_multi_defined_call_test.cpp`
- `rg -n "prepared_edge_publication_source_memory_access_status_name|MissingPreparedMemoryAccess|IncompletePreparedMemoryAccess|Route3|route3|source_memory_access_status|source_memory_" src/backend/mir/riscv src/backend/mir/x86 src/backend/bir tests/backend/bir/backend_prepared_lookup_helper_test.cpp tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp tests/backend/bir/backend_x86_handoff_boundary_multi_defined_call_test.cpp`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/prealloc/prepared_lookups.cpp build-x86/compile_commands.json`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/bir.cpp build-x86/compile_commands.json`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp build-x86/compile_commands.json`

Prepared memory lookup/status surfaces:

- `PreparedFunctionLookups::memory_accesses` is the aggregate field carrying
  `PreparedMemoryAccessLookups`; `make_prepared_function_lookups(...)` builds
  it from `make_prepared_memory_access_lookups(addressing, &value_home_lookups)`
  in `src/backend/prealloc/prepared_lookups.cpp`.
- `PreparedMemoryAccess` in `src/backend/prealloc/addressing.hpp:190` carries
  `function_name`, `block_label`, `inst_index`, optional
  `result_value_name`, optional `stored_value_name`, `address_space`,
  `is_volatile`, and prepared `address`.
- Direct prepared lookup helpers in `src/backend/prealloc/addressing.hpp`
  are `find_prepared_memory_access(...)`,
  `find_prepared_memory_access_by_result_name(...)`,
  `find_prepared_memory_access_by_result_value_name(...)`, and
  `find_prepared_memory_access_before_by_result_value_name(...)`.
- Indexed lookup helpers in `src/backend/prealloc/addressing.hpp` and
  `src/backend/prealloc/prepared_lookups.cpp` are
  `make_prepared_memory_access_lookups(...)`,
  `find_indexed_prepared_memory_accesses_by_result_value_name(...)`,
  `find_indexed_prepared_memory_access(...)`,
  `find_unique_indexed_prepared_memory_access_by_result_value_name(...)`,
  `find_indexed_prepared_memory_accesses_by_result_value_id(...)`, and
  `find_unique_indexed_prepared_memory_access_by_result_value_id(...)`.
  Existing helper coverage at
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:5343` preserves
  unique lookup, duplicate-name ambiguity, duplicate-id ambiguity, all duplicate
  entries, and invalid result-name rejection.
- Prepared source-memory publication status is
  `PreparedEdgePublicationSourceMemoryAccessStatus` in
  `src/backend/prealloc/publication_plans.hpp:65`: `Unavailable`,
  `Available`, `MissingPreparedMemoryAccess`, and
  `IncompletePreparedMemoryAccess`. Stable public strings are
  `"unavailable"`, `"available"`, `"missing_prepared_memory_access"`, and
  `"incomplete_prepared_memory_access"` via
  `prepared_edge_publication_source_memory_access_status_name(...)`.
- `PreparedEdgePublication` and `PreparedEdgeCopySourceFacts` retain
  source-memory fields: `source_memory_access_status`,
  `source_memory_access`, `source_memory_base_kind`,
  `source_memory_frame_slot_id`, `source_memory_symbol_name`,
  `source_memory_pointer_value_name`, `source_memory_byte_offset`,
  `source_memory_size_bytes`, `source_memory_align_bytes`,
  `source_memory_address_space`, `source_memory_is_volatile`,
  `source_memory_can_use_base_plus_offset`, and
  `source_memory_requires_address_materialization`
  (`src/backend/prealloc/publication_plans.hpp:274` and `:376`).
- `apply_source_memory_access_fact(...)` in
  `src/backend/prealloc/prepared_lookups.cpp:461` is prepared-owned:
  non-`LoadLocal` producers stay `Unavailable`; missing producer/block/access
  becomes `MissingPreparedMemoryAccess`; mismatched result, incomplete base,
  zero size, or zero align becomes `IncompletePreparedMemoryAccess`; otherwise
  `copy_source_memory_access_fact(...)` publishes `Available`.
- `prepared_edge_copy_source_facts_status(...)` maps prepared source-memory
  status to public helper/oracle rows:
  `MissingPreparedMemoryAccess` -> `MissingSourceMemoryAccess`, and
  `IncompletePreparedMemoryAccess` -> `IncompleteSourceMemoryAccess`
  (`src/backend/prealloc/publication_plans.cpp:844`).
- `prepared_edge_publication_source_memory_matches_access(...)` is the prepared
  comparison oracle for source memory; it requires available publication,
  named result value, matching base/frame/global/pointer/offset/size/align,
  address-space, volatility, and base-plus-offset policy
  (`src/backend/prealloc/publication_plans.cpp:877`).

Route 3 / BIR memory-source records and accessors:

- Route 3 names and records live in `src/backend/bir/bir.hpp:1294`:
  node kinds `unknown`, `load_local`, `load_global`, `store_local`,
  `store_global`; base kinds `none`, `local_slot`, `global_symbol`,
  `pointer_value`, `string_constant`; value roles `none`, `result`, `stored`.
- `Route3MemoryAccessRecord` carries availability, instruction pointer/index,
  node kind, block label/id, result/stored `Route1SourceValueIdentity`,
  address-space, volatile bit, base kind, local/global/pointer/string identity,
  byte offset, size, and align.
- Route 3 helper records are `Route3MemoryAccessValueRecord`,
  `Route3MemoryAccessIndex`, `Route3MemoryAccessQuery`,
  `Route3SameBlockGlobalLoadAccessRecord`,
  `Route3SameBlockLoadLocalSourceRecord`, and
  `Route3SameBlockLoadLocalStoredValueSourceRecord`.
- Route 3 accessors/publication helpers are
  `route3_memory_access_node_kind(...)`,
  `route3_memory_access_base_kind(...)`,
  `route3_memory_access_record(...)`,
  `route3_memory_access_result_value_record(...)`,
  `route3_memory_access_stored_value_record(...)`,
  `route3_build_memory_access_index(...)`,
  `route3_find_memory_access_record(...)`,
  `route3_same_block_global_load_access_record(...)`,
  `route3_same_block_load_local_source_record(...)`,
  `route3_find_same_block_global_load_access(...)`,
  `route3_find_same_block_load_local_source(...)`, and
  `route3_find_same_block_load_local_stored_value_source(...)`
  (`src/backend/bir/bir.hpp:1445`).
- MIR query facade surfaces in `src/backend/mir/query.cpp` convert Route 3
  records to public query identities:
  `find_bir_memory_access_identity(...)`,
  `find_bir_same_block_global_load_access_identity(...)`,
  `find_bir_same_block_load_local_source_identity(...)`, and
  `find_bir_same_block_load_local_stored_value_source_identity(...)`.
  The direct-memory identity test at
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:5510` checks
  Route 3 load/store identity against prepared semantic fields and rejects
  node-kind, block-label, and missing-index mismatches.
- Route 5 carries optional Route 3 source-memory identity through
  `Route5CfgEdgePublicationRecord::source_memory_identity_available` and
  `source_memory_access` (`src/backend/bir/bir.hpp:1756`). Its status names
  include `memory_source`, `missing_source_memory_access`, and
  `incomplete_source_memory_access`.
- Route 6 can also carry a `Route3MemoryAccessRecord memory_source` in
  `Route6CallArgumentPublicationSourceRecord` (`src/backend/bir/bir.hpp:2005`),
  but Step 1 evidence found the current active consumer family for this plan
  in Route 5/RISC-V publication, not x86 Route 6.

x86 consumer or absence buckets:

- x86 has concrete prepared-memory consumers in
  `src/backend/mir/x86/module/module.cpp`, but they consume prepared addressing
  and frame/global policy directly rather than Route 3 query identities.
- `render_prepared_frame_slot_memory_operand(...)` calls
  `find_prepared_memory_access(...)` and requires prepared `FrameSlot`,
  frame-slot id, nonnegative byte offset, `can_use_base_plus_offset`, and
  target-specific size-to-operand spelling (`BYTE`, `WORD`, `DWORD`) before
  rendering stack memory (`src/backend/mir/x86/module/module.cpp:3817`).
- `render_prepared_local_slot_statement_memory_operand(...)` reuses
  `render_prepared_frame_slot_memory_operand(...)` and throws if prepared
  `result_value_name` / `stored_value_name` drift from the expected BIR
  statement (`src/backend/mir/x86/module/module.cpp:3990`).
- x86 local-slot store/load and compare/guard paths call the prepared local
  memory render helper at `src/backend/mir/x86/module/module.cpp:4129`,
  `:4158`, `:4346`, `:4379`, `:4528`, `:4575`, `:4918`, and `:5281`.
- `render_prepared_same_module_global_memory_operand(...)` consumes
  `find_prepared_memory_access(...)`, requires prepared `GlobalSymbol`,
  expected result/stored names, nonnegative offset, base-plus-offset policy,
  target type/size, and same-module span support before rendering global memory
  (`src/backend/mir/x86/module/module.cpp:5489`; callers at `:5585` and
  `:5617`).
- Absence bucket: targeted `rg` found no x86 production consumer of
  `route3_build_memory_access_index`, `route3_find_memory_access_record`,
  `route3_source_memory_agrees_with_prepared_publication`, or the MIR Route 3
  query facade in `src/backend/mir/x86`; current x86 memory-source output is
  prepared-addressing/target-policy backed.
- Compatibility-owned x86 output includes stack/global operand text, size
  spelling, frame-slot offsets, global symbol span policy, base-plus-offset
  legality, value-home/register requirements, and handoff error text.

riscv consumer/status bucket:

- RISC-V source-memory output is prepared-backed in
  `render_edge_publication_source_operand(...)`
  (`src/backend/mir/riscv/codegen/emit.cpp:134`): the dynamic source-memory
  case requires prepared `LoadLocal`, `Available` source memory, pointer-value
  base, i32 source/destination, size 4, align >= 4, default address space,
  non-volatile, base-plus-offset allowed, no address materialization, signed
  12-bit offset, register destination, move authority, and base register home.
- RISC-V agreement checks are
  `route3_base_kind_agrees_with_prepared_source_memory(...)` and
  `route3_source_memory_agrees_with_prepared_publication(...)`
  (`src/backend/mir/riscv/codegen/emit.cpp:328` and `:366`). Agreement requires
  prepared `LoadLocal` + `Available`, non-null prepared memory, available Route
  3 `LoadLocal`, matching result kind/type/name, address-space, volatile bit,
  byte offset, size, align, and matching base kind/identity.
- RISC-V carries the status rows in `EdgePublicationMoveIntent`:
  `route5_edge_status`, `route5_edge_source_agrees`, and
  `route3_source_memory_agrees` (`src/backend/mir/riscv/codegen/emit.hpp:71`).
  `attach_route5_edge_agreement(...)` fills Route 5 and Route 3 agreement bits
  when a route edge is supplied (`src/backend/mir/riscv/codegen/emit.cpp:494`).
- Existing RISC-V coverage in
  `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1593`
  keeps scalar Route 5 output prepared-backed, verifies duplicate/mismatch/no
  source diagnostic rows, and checks dynamic memory-source output:
  agreeing Route 3 facts produce `lw a1, 12(s2)` with
  `route3_source_memory_agrees`; mismatched byte offset and incomplete Route 3
  identity keep the same prepared fallback output but clear agreement flags.
- Helper/oracle names and unsupported/status rows that must stay stable include
  prepared `"missing_prepared_memory_access"`,
  `"incomplete_prepared_memory_access"`, Route 5/RISC-V `"memory_source"`,
  `"missing_source_memory_access"`, `"incomplete_source_memory_access"`,
  `"no_match"`, and `"no_source"` rows observed in
  `src/backend/prealloc/publication_plans.hpp`,
  `src/backend/bir/bir.hpp`, and the RISC-V test above.

Addressing/storage-sensitive output that stays compatibility-owned:

- Prepared target-addressing fields remain compatibility authority:
  prepared address base kind, frame-slot id, symbol name, pointer value name,
  byte offset, size, align, address-space, volatility, base-plus-offset
  legality, and address-materialization requirement.
- Target output remains target/prepared-owned: x86 stack/global operand
  spelling and size class; RISC-V `lw` text, base register selection,
  signed-12-bit offset legality, destination register placement, value-home
  policy, stack-slot/source-home policy, and fallback instruction text.
- Unsupported/prepared-only rows remain compatibility-owned: missing prepared
  memory access, incomplete prepared memory access, duplicate prepared lookup
  ambiguity, invalid value names, invalid block/inst/node references,
  prepared-only range authority, prepared/Route 3 mismatch, incomplete Route 3
  memory identity, and target-policy cases such as
  `can_use_base_plus_offset = false`.

## Suggested Next

Execute Step 3 by proving or blocking x86 evidence for the selected Route 3
`LoadLocal` result/source-memory identity fact. The expected question is
whether x86 has any direct or indirect consumer for this Route 3 identity, or
whether current x86 memory-source behavior is blocked as prepared-addressing
and target-policy backed only.

## Watchouts

- The selected fact is deliberately not final memory operand text. A later
  adapter must not claim parity through x86 operand rendering or RISC-V `lw`
  output alone.
- x86 evidence is likely blocked unless a real Route 3 consumer exists outside
  the Step 1/Step 2 targeted searches.
- RISC-V agreement flags are useful identity evidence, but emitted output is
  still prepared-backed and remains unchanged when Route 3 is mismatched or
  incomplete.
- Preserve fail-closed rows for missing prepared memory access, incomplete
  prepared memory access, missing Route 3 source identity, incomplete Route 3
  identity, mismatches, duplicates, invalid names, and target-policy cases.

## Proof

No build/test proof required by the delegated packet. Analysis-only validation:
targeted `rg` evidence recorded above, plus prior Step 1 inventory retained in
this packet. No `test_after.log` produced because proof was explicitly not
required.
