Status: Active
Source Idea Path: ideas/open/458_select_edge_source_producer_move_bundle_placement_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Placement Authority Contract

# Current Packet

## Just Finished

Completed Step 2 for idea 458. The placement authority contract is recorded
under `build/agent_state/458_step2_placement_authority_contract/contract.md`.

Required authority is false by default: absence of a placement record means no
RV64 consumer may emit or suppress the target bundle. Required facts include
function identity, target bundle phase/block/instruction, source-producer kind
and result, select carrier/join-transfer identity, edge transfer identity when
edge-related, selected placement kind, source/destination value ids and homes,
dependency-operand authority linkage, and status/rejection reason.

Accepted placement kinds:

- `same_block_compare_operand_setup`: emit immediately before the producer only
  when same-block emission cannot overwrite another edge's selected value and
  scratch/clobber proof exists.
- `edge_owned_source_materialization`: consume on a specific edge with explicit
  predecessor/successor labels, matching edge transfer incoming value, and
  matching select-carrier destination.
- `predecessor_edge_consumed_suppression`: suppress at the join instruction
  only when predecessor-edge publication already materializes the same source
  producer for the same incoming/destination pair.

Rejected shapes:

- missing placement authority record;
- `authority=none` bundle without a separate source-producer placement record;
- inferred predecessor/successor identity from value ids, block indexes,
  instruction indexes, raw BIR, filenames, function names, or one dump;
- ambiguous or mismatched source producer, select carrier, join transfer, edge
  transfer, incoming value, destination value, or value ids;
- `load_from_stack_slot status=missing_stack_freshness`;
- generic stack/register move bundles unrelated to select-edge source
  producers;
- RV64 object-route consumption before producer metadata exists.

## Suggested Next

Execute Step 3: `Implement Or Route First Placement Metadata Packet`.

Selected first packet: add producer/prepared metadata and focused tests for
`predecessor_edge_consumed_suppression` on select-edge binary source producers.
The representative already has the edge transfer and predecessor-edge
publication for `%t18 -> %t22`; the missing fact is whether the target
before-instruction producer bundle is already consumed by that edge publication
and should be suppressible at the join instruction.

Re-read:

- `ideas/open/458_select_edge_source_producer_move_bundle_placement_authority.md`
- `build/agent_state/458_step1_select_edge_placement_audit/audit.md`
- `build/agent_state/458_step2_placement_authority_contract/contract.md`

Suggested Step 3 owned files:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- prepared printer/dump file only if exposing the new fact requires it
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp` only if the new fact
  is printed
- `todo.md`
- `test_after.log`
- `build/agent_state/458_step3_select_edge_placement_metadata/**`

Step 3 proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not route to RV64 lowering before placement authority exists.
- Do not infer predecessor/successor identity or suppression authority from
  value ids, block indexes, instruction indexes, raw BIR shape, filenames,
  function names, or one prepared dump.
- Do not reopen idea 456 cast-dependency consumption.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Keep generic stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Step 3 must stop if producer code cannot link the target before-instruction
  move bundle to the select-edge source producer and edge publication without
  raw-shape inference.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, baseline logs, or
  `review/`.

## Proof

Step 2 contract-only validation:

```sh
git diff --check
```

Result: passed.
