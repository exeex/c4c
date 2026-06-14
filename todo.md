Status: Active
Source Idea Path: ideas/open/252_phase_f3_edge_publication_source_producer_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Source-Producer Surfaces and Consumers

# Current Packet

## Just Finished

Completed Step 1, "Inventory Source-Producer Surfaces and Consumers", as an
analysis-only inventory for idea 252.

Prepared source-producer authority:

- Public prepared aggregate field:
  `PreparedFunctionLookups::edge_publication_source_producers` in
  `src/backend/prealloc/prepared_lookups.hpp`, populated by
  `make_prepared_function_lookups(...)` in
  `src/backend/prealloc/prepared_lookups.cpp`.
- Source-producer record shape:
  `PreparedEdgePublicationSourceProducerLookups::producers_by_value_name`
  maps `ValueNameId` to `PreparedEdgePublicationSourceProducer` in
  `src/backend/prealloc/publication_plans.hpp`.
- Stable producer kinds/string rows from
  `prepared_edge_publication_source_producer_kind_name(...)`:
  `unknown`, `immediate`, `load_local`, `load_global`, `cast`, `binary`,
  and `select_materialization`.
- Construction helper:
  `make_prepared_edge_publication_source_producer_lookups(...)` in
  `src/backend/prealloc/select_chain_lookups.cpp` publishes same-block
  `LoadLocal`, `LoadGlobal`, `Cast`, `Binary`, and `Select` results by
  prepared value name, block label, and instruction index.
- Lookup helpers:
  `find_indexed_prepared_edge_publication_source_producer(...)`,
  `find_prepared_select_chain_source_producer(...)`,
  `find_prepared_same_block_load_local_source_producer(...)`,
  `find_prepared_direct_global_select_chain_dependency(...)`,
  `find_prepared_scalar_select_chain_materialization(...)`,
  `find_prepared_call_argument_source_producer_materialization(...)`, and
  `find_prepared_same_block_scalar_producer(...)` / related public prepared
  scalar-publication readers.
- Observable prepared/publication rows:
  `PreparedEdgePublication::source_producer_kind`,
  `source_producer_block_label`, `source_producer_instruction_index`,
  `source_load_local`, `source_load_global`, `source_cast`, `source_binary`,
  `source_select`, plus `source_memory_access_status` rows
  `unavailable`, `available`, `missing_prepared_memory_access`, and
  `incomplete_prepared_memory_access`.
- Helper/oracle rows that keep the surface public include
  `prepare_edge_copy_source_facts(...)`,
  `prepared_edge_copy_source_facts_have_materializable_producer(...)`,
  `prepared_edge_publication_source_home_matches_source(...)`,
  `prepared_edge_publication_source_memory_matches_access(...)`,
  store-source publication planning via
  `plan_pending_prepared_store_global_publications(...)`, and same-block
  load-local source lookup through
  `find_prepared_same_block_load_local_source_producer(...)`.

Prepared helper and AArch64 consumers:

- Prepared helper/oracle tests in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` assert that edge
  publications expose `LoadLocal`, `Cast`, `Binary`, and
  `SelectMaterialization` source-producer facts, source-memory details,
  source-home matching, mismatch rejection, and BIR/Route 5 edge-source
  identity agreement. These rows keep helper/oracle names and strings stable.
- AArch64 direct prepared source-producer readers include
  `prepared_same_block_publication_source_producer(...)`,
  `prepared_publication_source_producer_for_value(...)`, and
  `prepared_source_producer_instruction(...)` in
  `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`.
- AArch64 source-producer consumers also appear in scalar call argument
  materialization in `src/backend/mir/aarch64/codegen/calls.cpp`, memory and
  ALU source materialization paths, comparison helpers, edge-copy dispatch,
  and value materialization. These use prepared lookup authority as fallback or
  compatibility authority even when Route 1/3/4/6 evidence is consulted.
- AArch64 tests keep observable rows stable, including
  `branch_condition_publication_uses_prepared_source_producer_authority`,
  `scalar_call_argument_source_producer_reads_bir_materialization`, and
  `call_boundary_stack_source_uses_route4_identity_with_prepared_fallback` in
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`.

Route/BIR candidate authority:

- Route 4 and Route 5 records carry candidate producer/source identity fields:
  `source_producer_kind`, `source_producer_instruction`,
  `source_producer_instruction_index`, and source block labels in
  `src/backend/bir/bir.hpp` / `src/backend/bir/bir.cpp`.
- MIR query exposes the candidate semantic surface through
  `BirCfgEdgePublicationSourceIdentity`,
  `find_bir_cfg_edge_publication_source_identity(...)`,
  `route5_edge_source_producer_to_mir(...)`, and
  `find_bir_select_chain_source_producer(...)` in
  `src/backend/mir/query.hpp` / `src/backend/mir/query.cpp`.
- Route 5 edge-source evidence can match prepared source producer identity for
  CFG edge publication, including `LoadLocal` plus Route 3 source-memory
  identity and non-memory `Cast` / `Binary` / `SelectMaterialization` rows.
  This is candidate semantic evidence only; it is not readiness by itself.
- Route 6 call-argument source-producer records exist for call arguments, but
  those are adjacent call-boundary evidence, not edge-publication
  source-producer authority for this plan.

x86 consumer buckets:

- Direct source-producer consumers: none observed. Targeted search of
  `src/backend/mir/x86` found no use of
  `edge_publication_source_producers`,
  `PreparedEdgePublicationSourceProducer`, `source_producer_kind`, or Route/BIR
  producer/source records.
- Indirect prepared edge-publication consumers: present. x86
  `ConsumedPlans::shared_function_lookups()` exposes prepared function lookups,
  and `x86::prepared::consume_edge_publication_move_intent(...)` in
  `src/backend/mir/x86/prepared/dispatch.cpp` reads
  `lookups->edge_publications`, publication status, prepared move, source
  home, destination home, prepared value ids, and rendered operand strings.
- No-consumer bucket for Step 1: x86 currently preserves prepared
  edge-publication/move/home compatibility but does not consume the prepared
  source-producer sublookup or a Route 4/5 BIR source-producer identity surface.
  Existing x86 rows such as `MissingSharedLookups`, `MissingPublication`,
  `UnsupportedPublication`, `UnsupportedSourceHome`,
  `UnsupportedDestinationHome`, and emitted `mov` output remain
  compatibility-owned.

riscv consumer buckets:

- Direct prepared publication consumers: present in
  `src/backend/mir/riscv/codegen/emit.cpp`; RISC-V consumes prepared edge
  publications and source/destination homes to form edge-publication move
  intent and output.
- Indirect Route/BIR source-producer evidence: present but diagnostic. RISC-V
  `route5_edge_source_agrees_with_prepared_publication(...)` compares prepared
  publication source producer kind against Route 5 and, for `LoadLocal`, also
  requires Route 3 source-memory agreement through
  `route3_source_memory_agrees_with_prepared_publication(...)`.
- No independent source-producer adapter observed: RISC-V output remains
  prepared-backed through `consume_edge_publication_move_intent(...)`; Route
  5/Route 3 facts annotate agreement/fallback but do not replace prepared
  source-producer lookup authority.
- RISC-V tests in
  `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` keep
  agreement/fallback rows stable, including prepared-backed `mv` / `lw` output,
  `available`, `no_match`, `no_source`, duplicate/mismatch/absence diagnostic
  rows, and prepared fallback for non-agreeing or incomplete Route 5/Route 3
  facts.

Stable compatibility and policy-sensitive rows:

- Helper/oracle names and public helpers listed above must remain stable; do
  not rename, privatize, wrap, or bypass them in this blocker-map plan.
- Stable strings include source-producer kind names, source-memory status
  names, Route 5 publication status names such as `available`, `no_match`,
  `no_source`, and `missing_source_producer`, and target helper statuses such
  as x86/RISC-V missing/unsupported publication-home statuses.
- Prepared printer/output rows with `source_producer=...`,
  `source_producer_block=...`, and `source_producer_inst=...` remain
  compatibility-visible, including rows in
  `tests/backend/bir/backend_prepared_printer_test.cpp`.
- Target-policy-sensitive rows remain out of semantic migration scope:
  x86 source/destination operand spelling and `mov` output, RISC-V register and
  stack-source instruction text such as `mv` / `lw`, fallback names, carrier
  selection, helper status names, publication-output rows, storage/home policy,
  addressing legality, scratch policy, branch/parallel-copy placement, and
  final emitted assembly.

## Suggested Next

Execute Step 2, "Select One Candidate Producer/Source Relation", as an
analysis-only selection packet. Pick one narrow producer/source relation tied
to prepared `edge_publication_source_producers` lookup agreement, define the
Route/BIR-to-prepared agreement rule, and explicitly exclude target output,
publication availability, move/home/storage policy, helper names, and fallback
strings.

## Watchouts

- Keep source idea 252 unchanged unless durable intent truly changes.
- Do not implement a BIR producer index or adapter in this blocker-map plan.
- Do not delete, privatize, wrap, rename, or bypass public prepared
  `edge_publication_source_producers` helpers.
- Do not infer readiness from Route 5 edge/source evidence alone.
- Treat x86 as having prepared edge-publication consumption but no observed
  source-producer consumer until Step 3 proves otherwise.
- Treat RISC-V Route 5/Route 3 agreement rows as diagnostic evidence; prepared
  output and fallback remain authoritative until a later step selects and
  proves one relation.
- Preserve helper/oracle names, compatibility strings, fallback names,
  publication/output rows, and target-policy-sensitive behavior.
- Treat testcase-shaped shortcuts, expectation weakening, helper/status/output
  renames, and classification-only maps claimed as implementation progress as
  route drift.

## Proof

Analysis-only packet. No build or test proof required by supervisor.
Local validation: `git diff --check -- todo.md`.
