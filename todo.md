Status: Active
Source Idea Path: ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Consume Prepared Edge Publication Sources

# Current Packet

## Just Finished

Completed Step 3's first narrow `dispatch_publication.cpp` migration:
fused-compare operand publication now gates source-producer use through
`find_prepared_fused_compare_operand_producer`, preserving same-block and
before-instruction constraints from the prepared lookup helper. The
select-materialization scratch preference also consumes that prepared fused
operand helper instead of the local producer-kind probe.

## Suggested Next

Next narrow Step 3 packet: evaluate `value_publication_may_read_register_index`
and replace its raw `mir::find_same_block_named_producer_record` fallback with
a prepared same-block scalar producer lookup only if the existing helper
preserves the current recursive register-read behavior when prepared lookups
are available.

## Watchouts

- `find_prepared_materialized_condition_producer` was inspected but was not the
  right helper for this fused-compare operand slice; the select scratch
  preference needs the operand's `SelectMaterialization` producer, which is
  exposed by `find_prepared_fused_compare_operand_producer`.
- `value_publication_may_read_register_index` still has raw same-block producer
  discovery after prepared producer lookup misses. That is the clearest
  remaining local source-selection residue in this file.
- The existing local fallback that builds source-producer lookups when
  `prepared_lookups` is absent was preserved.

## Proof

Command: `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(prealloc_block_entry_publications|riscv_prepared_edge_publication|publication_plan_record|x86_publication_plan_reuse|codegen_route_aarch64_store_global_stack_publication|cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$'; } > test_after.log 2>&1`

Result: passed; `test_after.log` contains the proof output.
