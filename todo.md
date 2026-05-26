Status: Active
Source Idea Path: ideas/open/16_bir_edge_value_flow_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Current Edge Value Flow

# Current Packet

## Just Finished

Completed Step 1 audit of the current AArch64 edge value-flow paths.

AArch64 rediscovery paths to retire or narrow in later implementation steps:

- `dispatch_edge_copies.cpp::find_edge_named_producer` searches the successor
  block, current edge/predecessor block, and up to three unique branch
  predecessors to rediscover the BIR producer for a named edge value.
- `dispatch_edge_copies.cpp::emit_edge_value_publication_to_register` and
  `edge_value_publication_may_read_register_index` recursively rediscover
  `LoadLocal`, `Cast`, `Binary`, and `Select` producer trees to rebuild edge
  publication materialization and scratch-read hazards.
- `dispatch_edge_copies.cpp::lower_predecessor_select_parallel_copy_sources`
  scans successor BIR instructions for a `BinaryInst` result matching the
  prepared source home, then lowers that expression as an edge publication.
- `dispatch_producers.cpp::is_current_block_join_parallel_copy_source`,
  `is_current_block_join_parallel_copy_incoming_expression`,
  `same_block_result_depends_on_value`, and
  `build_current_block_join_parallel_copy_cache` walk current-block BIR
  expression dependencies and prepared move bundles to infer which
  instructions are join incoming expressions or parallel-copy sources.
- `dispatch_publication.cpp::lower_missing_conditional_branch_condition_publication`
  and `lower_missing_fused_compare_operand_publication` still use same-block
  producer lookup to repair missing branch/compare operand publications.
- `dispatch_publication.cpp::value_publication_may_read_register_index`
  repeats same-block producer recursion for publication hazard checks, with a
  special stop when a current block-entry publication already owns the value.

Shared semantic/prepared facts needed to replace those rediscoveries:

- A prepared edge-publication record keyed by function, predecessor label,
  successor label, destination value id/name, source value, and publication
  phase, with a fail-closed status when any required identity is missing.
- Direct links from prepared edge publications to the existing
  `PreparedJoinTransfer::edge_transfers` and `PreparedParallelCopyBundle`
  records, so AArch64 does not recompute branch-owned join predecessor
  ownership or bundle ownership from CFG shape.
- Source/destination home facts for each edge publication: source value id/name
  where available, destination `PreparedValueHome`, destination storage kind,
  register/stack-slot/immediate/pointer-base home information, and whether the
  copy is redundant because source and destination are the same prepared home.
- Prepared expression/source classification for edge publications: whether the
  source is a plain home read, rematerializable immediate, load-local
  publication, cast, binary compare/arithmetic, or select-chain materialization,
  plus the producer instruction index when the source is intentionally
  materialized from BIR.
- Target-neutral dependency/hazard facts for reusable planning only: source
  value dependency set, required source evaluation order for parallel-copy
  edges, cycle/temp-save steps already published by prepared parallel-copy
  bundles, and a register-read dependency expressed in prepared register
  identities or value homes rather than AArch64 register spelling.
- Current block-entry publication availability should continue to come from
  `collect_prepared_block_entry_publications`, but later steps need a helper
  that returns the publication for a destination value/home without AArch64
  re-scanning all moves at each use site.
- Existing shared coverage to preserve while extending the contract:
  `PreparedEdgeValueTransfer`, `PreparedJoinTransfer`,
  `PreparedParallelCopyBundle`, `find_authoritative_branch_owned_join_transfer`,
  `find_published_parallel_copy_bundle_for_edge_transfer`, and
  `collect_prepared_block_entry_publications`.

Target-specific AArch64 pieces that should remain in AArch64:

- Physical register spelling/parsing, alias checks, scalar view choice,
  scratch register choice, and target register bank filtering.
- Instruction spelling and encoding decisions for `mov`, `ldr`, `cmp`, `cset`,
  `add`, `sub`, sign/zero extension, stack/frame-slot addresses, and pointer
  base-plus-offset addressing.
- AArch64-specific scratch hazards, including whether RHS must be evaluated
  before LHS to avoid clobbering the target register and whether a block-entry
  move clobbers a just-published join register.
- Select-chain label generation, inline assembly materialization records,
  emitted-register bookkeeping, diagnostics, and MachineInstruction payload
  construction.
- Fallback emission from already-prepared homes to AArch64 registers or memory
  should remain target-local, but the decision that an edge publication exists
  and which semantic value it publishes should come from shared prepared facts.

## Suggested Next

Delegate Step 2 to define a target-neutral prepared edge-publication lookup
record/API that covers predecessor, successor, source value, destination home,
publication phase, and links to published join/parallel-copy ownership.

## Watchouts

- The next slice should fail closed on missing prepared edge-publication facts
  instead of preserving `find_edge_named_producer` under a new helper name.
- Keep AArch64 register names, scratch-register policy, addressing modes,
  immediate constraints, instruction spelling, and MachineInstruction payloads
  out of the shared BIR/prealloc record.
- Some expression classification can be shared, but actual rematerialization
  and hazard avoidance must stay target-local unless expressed only in prepared
  value/home dependencies.

## Proof

`git diff --check` passed. Proof log: `test_after.log`.
