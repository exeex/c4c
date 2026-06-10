Status: Active
Source Idea Path: ideas/open/167_route1_producer_constant_oracle_thinning.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate And Handoff

# Current Packet

## Just Finished

Completed Step 4 scan portion: re-scanned direct consumers after the selected
AArch64 GP scalar value-publication materialization migration and found no
prepared producer, constant, cache, or API surface that is safe to contract yet.

Consumer evidence:
- The selected migrated file no longer calls
  `prepare::find_prepared_same_block_scalar_producer(...)` or
  `prepare::evaluate_prepared_same_block_integer_constant(...)` for its primary
  GP scalar value-publication producer/constant facts.
- The selected file still intentionally reads
  `prepared_lookups->edge_publication_source_producers` through
  `find_prepared_scalar_select_chain_materialization(...)` for select-chain
  materialization compatibility.
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp` still calls
  `prepare::find_prepared_same_block_scalar_producer(...)` and reads
  `edge_publication_source_producers` for publication-source producer queries.
- `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp` still calls
  `prepare::find_prepared_same_block_scalar_producer(...)` for FP
  materialization.
- `src/backend/mir/aarch64/codegen/alu.cpp` still calls
  `prepare::find_prepared_same_block_load_local_source_producer(...)`,
  `prepare::find_prepared_same_block_scalar_producer(...)`, and
  `prepare::find_prepared_scalar_select_chain_materialization(...)`.
- `src/backend/mir/aarch64/codegen/calls.cpp` still uses prepared
  source-producer lookups for scalar call-argument materialization and indirect
  callee source discovery.
- `src/backend/prealloc/comparison.cpp`,
  `src/backend/prealloc/call_plans.cpp`,
  `src/backend/prealloc/publication_plans.cpp`, and
  `src/backend/prealloc/select_chain_lookups.cpp` still expose prepared
  producer/constant/materialization helper APIs used by non-selected paths.

Decision: no contraction is safe yet. `PreparedFunctionLookups::
edge_publication_source_producers`,
`PreparedEdgePublicationSourceProducerLookups`,
`PreparedSameBlockScalarProducer`,
`find_prepared_same_block_scalar_producer(...)`, and
`evaluate_prepared_same_block_integer_constant(...)` still have live public or
cross-module consumers outside the migrated GP scalar value-publication path.

## Suggested Next

Execute Step 5: validate and hand off this Route 1 thinning slice as a
no-contraction result. Recommended proof command:
`bash -lc "set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$'" |& tee test_after.log`

No implementation contraction packet is recommended from this Step 4 scan.

## Watchouts

- `dispatch_value_materialization.cpp` no longer calls the prepared same-block
  producer or prepared integer-constant helpers directly for the migrated
  semantic facts, but it still depends on prepared select-chain/publication
  surfaces.
- Do not move homes, registers, storage, emitted-register state, operand views,
  frame slots, final instruction order, spill/reload behavior, or publication
  hooks into BIR.
- Reject testcase-shaped shortcuts, expectation rewrites, or narrow named-case
  matching as progress.
- Residual prepared consumers are blockers for contraction, not cleanup targets
  for this plan step.
- The local prepared-shaped adapter in the selected file reshapes a MIR
  producer answer for existing AArch64 target-policy APIs; it does not by
  itself justify hiding prepared lookup APIs.

## Proof

No build or tests run; this was a scan-only Step 4 packet.

Tooling used:
- `c4c-clang-tool-ccdb function-callers` on
  `src/backend/prealloc/prepared_lookups.cpp` for prepared helper callers.
- `rg` over `src/backend` and `tests` for direct prepared lookup/helper and
  Route 1 helper consumers.

No new proof log was produced by this scan-only packet. The accepted prior
proof has been rolled forward to `test_before.log`; there is currently no
`test_after.log`.
