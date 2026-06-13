Status: Active
Source Idea Path: ideas/open/241_phase_e5_route45_edge_publication_identity_adapter_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Select The Route 4/5 Publication Identity Reader

# Current Packet

## Just Finished

Step 1 selected one semantic Route 5 reader for the Route 4/5 adapter plan:
`current_block_join_prepared_query_source(...)` in
`src/backend/mir/aarch64/codegen/dispatch_producers.cpp`.

Selected boundary:

- Reader: the `routing.sources[instruction_index]` read served by
  `current_block_join_prepared_query_source(...)` after
  `build_current_block_join_prepared_query_routing(...)` computes current-block
  join-source routing.
- Route fact to consume: `mir::find_bir_current_block_join_source_identity(...)`
  over a `bir::Route5EdgeJoinSourceIndex`, specifically the
  `BirCurrentBlockJoinSourceIdentity::source_values` / `facts` row whose source
  producer, instruction index, successor label, destination value, and source
  value identify the current-block Route 5 join source.
- Current prepared answer:
  `prepare::prepare_current_block_join_parallel_copy_source_facts(...)`, reached
  through `prepare_current_block_join_parallel_copy_source_facts(context)`, with
  the current source answer stored in `source_value_ids`, `source_value_names`,
  and per-row `PreparedCurrentBlockJoinParallelCopySourceFact` data from
  `edge_publications`, `value_homes`, and block-entry parallel-copy moves.
- Fallback path: keep `prepared_source` as the answer unless
  `current_block_join_route5_source_agrees_with_prepared(...)` proves a Route 5
  source fact agrees with the prepared fact. Missing prepared inputs, missing
  Route 5 facts, mismatched source or destination, wrong predecessor/successor,
  missing source producer, duplicate/conflicting Route 5 agreement, memory-source
  or stack-source policy cases, and unsupported move/source policy all stay on
  the prepared path.
- Semantic-only reason: this reader answers only whether one current-block
  instruction result is a join source identity. It does not construct edge
  publications, choose move order, choose homes/storage, schedule parallel
  copies, spell branches/output, format wrappers, select instructions, or emit
  final target moves.
- Nearby same-feature cases: current-block join routing normal predecessor,
  missing predecessor, no-source, memory-source, absent Route 5, mismatched
  source, duplicate source; prepared lookup helper Route 5 CFG edge
  load/cast/binary/select source records, missing-source, wrong
  predecessor/successor, wrong destination type, missing destination, and
  multi-predecessor lookup.

## Suggested Next

Execute Step 2 from `plan.md`: implement one agreement-gated adapter around the
selected `current_block_join_prepared_query_source(...)` reader without moving
publication construction, parallel-copy scheduling, home/storage policy, block
order, wrapper formatting, instruction selection, or emission ownership.

## Watchouts

- Keep the adapter boundary to one semantic identity read.
- Preserve `routing.incoming_expressions` behavior; the selected reader is only
  the source classification returned by `current_block_join_prepared_query_source`.
- `prepare::attach_route5_current_block_join_source_if_agrees(...)` already
  records prepared/Route 5 agreement metadata for prepared facts; any Step 2
  code should reuse or mirror that semantic agreement instead of trusting a
  naked Route 5 row.
- Do not delete, privatize, rename, or hide `edge_publications`,
  `edge_publication_source_producers`, `PreparedFunctionLookups`, or
  `PreparedBirModule`.
- Do not move publication construction, move/home/storage policy, block order,
  wrapper output, formatting, instruction selection, or emission policy into
  Route 4/5 ownership.
- Do not rewrite expectations, helper names, supported-path status, wrapper
  output, or baselines as proof.

## Proof

No build or CTest was run because this was a docs/lifecycle selection packet and
no implementation files were changed.

Source inspection commands used:

- `git status --short`
- `rg -n "edge_publications|edge_publication_source_producers|publication_source|join-source|join source|edge publication" src tests docs ideas/open/241_phase_e5_route45_edge_publication_identity_adapter_follow_up.md`
- `rg -n "PreparedFunctionLookups|PreparedBirModule|prepared.*publication|publication.*prepared|edge.*producer|source_producer" src tests`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/dispatch_producers.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/query.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/prealloc/prepared_lookups.cpp build/compile_commands.json`

Recommended Step 2/3 proof subset after implementation:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_current_block_join_routing|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure > test_after.log
```
