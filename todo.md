Status: Active
Source Idea Path: ideas/open/64_aarch64_join_parallel_copy_prepared_query.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Current-Block Join-Copy Authority

# Current Packet

## Just Finished

Step 1 inventory completed for
`src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
`build_current_block_join_parallel_copy_cache`; no implementation files were
edited.

Cache-derived facts:
- Prepared inputs: current `bir_block` instruction results, prepared value-name
  to `PreparedValueId` lookup, `PreparedValueHome` records for move sources and
  destinations, and every `PreparedMoveBundle` with phase `BlockEntry`,
  authority `OutOfSsaParallelCopy`, and
  `source_parallel_copy_successor_label == current block label`.
- Move facts: only `Move` operations with destination kind `Value` participate.
  Non-immediate moves contribute `move.from_value_id` source-home identity and
  prepared source value names to the incoming-expression walk. Register
  destination moves whose destination matches `move.to_value_id` contribute
  `move.to_value_id` to the local source set; if the move is non-immediate and
  not same-value, the source value also joins the source set when its source
  home shares the destination register with the destination home, or when the
  source home is stack-homed.
- Incoming-expression facts: the cache marks an instruction result as incoming
  when its prepared home id is one of the non-immediate move source ids, or when
  its textual BIR result name is reached by recursively walking named operands
  from those source names through same-block `Binary`, `Cast`, and `Select`
  producer instructions.
- Source facts: the cache marks an instruction result as source when its
  prepared home id is in the source-value set above. Immediate sources only
  make the destination a source; they do not seed incoming-expression facts.
- Raw fallbacks: `cached_current_block_join_parallel_copy_*` falls back to the
  older non-cache helpers when the cache context or instruction index does not
  match, so those helpers still encode the same local authority.

Consumers:
- `dispatch.cpp` builds the cache once per block before the retained BIR loop.
- `cached_current_block_join_parallel_copy_incoming_expression` is target-local
  emission routing only: it suppresses non-stack-home incoming expression
  lowering before normal scalar/address-materialization lowering. It needs the
  incoming-expression identity for each instruction result, not raw bundle
  iteration.
- `cached_current_block_join_parallel_copy_source` is also target-local
  emission routing: it suppresses non-stack-home source-result lowering after
  cast/fusion publication routes. It needs source-value identity for each
  instruction result.
- `dispatch_value_materialization.cpp` still calls
  `is_current_block_join_parallel_copy_source` directly when materializing a
  same-block producer to a register. That consumer needs source identity plus
  the prepared home for the requested value so it can publish from the prepared
  home.
- Nearby but not direct cache consumers: branch-fusion hooks use current block
  entry publication register/home facts, before-instruction move filtering uses
  `block_entry_move_clobbers_current_join_publication`, and before-return
  publication uses prepared before-return move/source-home facts. These can
  regress if join source facts stop matching block-entry publication and
  prepared-home identity, even though their sequencing is out of scope.

Shared-owner candidates and gaps:
- Existing shared prepared authority for block-entry destination publication is
  `collect_prepared_block_entry_publications` in
  `src/backend/prealloc/value_locations.hpp`; it owns destination value id,
  destination home, destination storage/register, and block-entry
  out-of-SSA bundle filtering by successor label.
- Existing shared source authority is
  `prepare_edge_copy_source_facts` /
  `prepare_block_entry_parallel_copy_edge_source_facts` in
  `src/backend/prealloc/prepared_lookups.cpp`; it owns predecessor/successor
  edge identity, destination/source value ids, source/destination homes,
  source producer kind, publication/move matching, and missing/ambiguous status.
- Existing shared move helpers already own register-destination and
  source-shares-destination-register predicates:
  `prepared_out_of_ssa_parallel_copy_register_destination_matches_value` and
  `prepared_out_of_ssa_parallel_copy_source_shares_destination_register`.
- Migration gap: there is no shared per-current-block query that answers the
  AArch64 cache shape directly: destination/source ids for all block-entry
  join moves into the current block plus the incoming same-block expression
  closure over BIR `Binary`/`Cast`/`Select` producers. Step 2 should add that
  query beside prepared move/publication authority, using structured prepared
  value ids and edge/publication facts instead of exposing raw AArch64 cache
  reconstruction.

## Suggested Next

Step 2 should define the shared prepared join-copy query in the prepared
move/publication authority. It should return structured per-block-entry
join-copy facts for incoming expression ids/names, source value ids, source and
destination homes, move/publication provenance, and explicit missing or
ambiguous statuses.

## Watchouts

Keep the query semantic owner in shared prepared code, not under a renamed
AArch64 cache helper. Do not change branch-fusion sequencing,
before-instruction filtering, before-return publication ordering, same-block
scalar recursion, edge fallback, or select-chain dependency discovery while
migrating these facts. The expression closure currently depends on textual BIR
names plus prepared value-home lookup; replacing it must preserve the
same-block `Binary`/`Cast`/`Select` dependency behavior or fail closed with a
clear missing shared-query status.

## Proof

Read-only inspection only; no build/test proof required for this inventory
packet. Commands run: targeted `rg` over join-copy/cache/prepared-move symbols,
targeted `sed`/`nl` reads of dispatch producer, dispatch, publication,
before-return, and prepared lookup owners, plus `git diff --check`.
