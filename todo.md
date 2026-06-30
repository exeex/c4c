Status: Active
Source Idea Path: ideas/open/458_select_edge_source_producer_move_bundle_placement_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Placement Metadata Packet

# Current Packet

## Just Finished

Completed Step 3 for idea 458. Implemented producer/prepared placement metadata
for `predecessor_edge_consumed_suppression` on select-edge binary source
producers. Supporting artifact:
`build/agent_state/458_step3_select_edge_placement_metadata/summary.md`.

New metadata surface:

- `PreparedSelectEdgeSourceProducerPlacementKind`
- `PreparedSelectEdgeSourceProducerPlacementStatus`
- `PreparedSelectEdgeSourceProducerPlacementInputs`
- `PreparedSelectEdgeSourceProducerPlacement`
- `PreparedSelectEdgeSourceProducerPlacementRecord`
- `PreparedSelectEdgeSourceProducerPlacementRecords`
- `plan_prepared_select_edge_source_producer_placement`
- `prepared_select_edge_source_producer_placement_available`
- `collect_prepared_select_edge_source_producer_placements`

The accepted route is false by default and available only when an available
block-entry predecessor-terminator edge publication has
`carrier_kind=select_materialization`, the source producer is a binary producer
whose result is the edge source, the target before-instruction bundle has
`authority=none`, the bundle block label/instruction match the prepared
source-producer site, and all register-destination moves target the producer
result.

Focused coverage rejects missing placement kind, non-binary source producers,
non-select publication carriers, missing or wrong producer-site linkage,
out-of-SSA before-instruction bundles, non-producer move destinations, and
non-register move destinations.

The exported collector is covered too:
`collect_prepared_select_edge_source_producer_placements` is exercised on a
minimal prepared module with semantic BIR binary source-producer discovery,
select-materialization edge publication, block-entry out-of-SSA edge move, and
the target before-instruction bundle at the producer site. The same test proves
the collector emits no record when the target bundle is moved to the wrong
semantic block.

## Suggested Next

Execute Step 4: `Residual Disposition And Close Readiness`.

Re-check the target `20010329-1` before-instruction bundle against the new
placement metadata. Decide whether idea 458 is complete as producer/prepared
placement authority, whether an exact printing/population packet remains, or
whether lifecycle should route back to a later RV64 consumer only after
plan-owner decision.

Suggested Step 4 owned files:

- `todo.md`
- `build/agent_state/458_step4_residual_disposition/**`

Suggested Step 4 proof command:

```sh
git diff --check
```

## Watchouts

- Do not route to RV64 lowering in Step 4; this slice only publishes
  producer/prepared metadata.
- Do not infer predecessor/successor identity or suppression authority from
  value ids, block indexes, instruction indexes, raw BIR shape, filenames,
  function names, or one prepared dump.
- Do not reopen idea 456 cast-dependency consumption.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Keep generic stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- The new collector uses semantic edge-publication/source-producer facts plus
  control-flow block labels to validate the bundle site. It still does not
  mutate the existing move bundle or add RV64 consumption.
- Printer exposure was not added because the focused metadata API/test surface
  covers this packet without touching non-owned printer submodule files.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, baseline logs, or `review/`.

## Proof

Step 3 code/test validation:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. `test_after.log` reports `100% tests passed, 0 tests failed out of 327`.
