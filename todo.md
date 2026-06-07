Status: Active
Source Idea Path: ideas/open/116_aarch64_dispatch_prepared_producer_contract_surface.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Boundary Audit For Remaining Producer Logic

# Current Packet

## Just Finished

Step 4 - Boundary Audit For Remaining Producer Logic completed a todo-only
audit of the remaining producer and hazard boundaries.

- AST-backed symbol/call queries show the remaining producer-facing helpers in
  `dispatch_producers.cpp` are prepared lookup wrappers plus target-local
  hazard checks:
  `prepared_publication_source_producer_for_value`,
  `prepared_source_producer_instruction`,
  `select_chain_contains_direct_global_load`,
  `build_current_block_join_prepared_query_routing`,
  `block_entry_move_clobbers_current_join_publication`,
  `prepared_value_home_reads_register_index`, and
  `value_publication_may_read_register_index`.
- `value_publication_may_read_register_index` consumes prepared value homes,
  current-block entry publication facts, and prepared same-block source
  producers, then recursively follows only same-block cast, binary, and select
  operands to answer an AArch64 machine-register read hazard. It remains local
  because the final decision parses AArch64 register names and compares
  general-purpose register indexes, not because it owns a missing prepared
  producer fact.
- `prepared_value_home_reads_register_index` is keep-local hazard glue: it
  checks `PreparedValueHomeKind::Register`, parses `home.register_name` with
  `abi::parse_aarch64_register_name`, requires
  `abi::RegisterBank::GeneralPurpose`, and compares the parsed physical
  register index against the queried scratch/target register index.
- `block_entry_move_clobbers_current_join_publication` is keep-local clobber
  policy: it collects prepared block-entry publications from value locations,
  parses each prepared destination register name as an AArch64 register, builds
  a `RegisterOperand` with storage-plan role/value identity, and calls
  `registers_alias` against the call-boundary move destination register.
- `dispatch_edge_copies.cpp` still owns edge-publication emission and edge
  register hazard ordering. Its remaining recursive
  `edge_value_publication_may_read_register_index` is target-local for the same
  concrete reason: callers pass a physical destination register index from a
  `CallBoundaryMoveInstructionRecord`, and the helper combines prepared
  producer/source facts with AArch64 register-index read checks before deciding
  whether to suppress or reorder block-entry edge-copy moves.
- `dispatch.cpp` did not gain producer-discovery logic. It wires existing
  producer helpers into branch-fusion hooks, skips before-instruction moves
  that clobber current join publications, and otherwise delegates publication,
  edge-copy, call, memory, scalar, and branch lowering to the existing owners.
- `dispatch_value_materialization.cpp` did not gain edge/source producer
  discovery. It consumes prepared same-block scalar producers, prepared integer
  constants, prepared current-block join query facts, and prepared value homes;
  its calls to `value_publication_may_read_register_index` are scratch/target
  ordering hazards for binary materialization.
- `dispatch_edge_copies.cpp` did not gain new unprepared producer discovery in
  this audit. The remaining edge source/producers are resolved through prepared
  edge-publication source producers, `prepare_edge_copy_source_facts`, and
  prepared move bundles, while AArch64 emission and physical-register hazard
  ordering stay local.

## Suggested Next

Delegate Step 5 final proof and closure package. Suggested package: run a fresh
build plus the focused prepared/AArch64 dispatch subset from prior packets, then
have the supervisor decide whether a broader regression guard or plan-owner
closure review is needed for
`ideas/open/116_aarch64_dispatch_prepared_producer_contract_surface.md`.

## Watchouts

- Do not move `value_publication_may_read_register_index`,
  `prepared_value_home_reads_register_index`,
  `block_entry_move_clobbers_current_join_publication`, or
  `edge_value_publication_may_read_register_index` to prepare without a new
  design for target register banks, register-name parsing, and alias policy.
- The remaining same-block recursion is acceptable only as an AArch64
  register-hazard query layered over prepared producer facts; it should not
  grow BIR-name matching, predecessor rescans, or testcase-shaped producer
  discovery.
- Step 5 should verify no implementation files changed during this audit before
  treating this as closure-ready.

## Proof

No build or test proof was required by this todo-only audit packet, and no
root proof logs were modified. Tooling proof consisted of AST-backed
`c4c-clang-tool-ccdb` symbol/caller/callee queries for
`dispatch_producers.cpp`, `dispatch_edge_copies.cpp`, `dispatch.cpp`, and
`dispatch_value_materialization.cpp`, plus targeted text checks for prepared
producer-discovery terms in the same files.
