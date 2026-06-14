Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate the Byval Pointer-Source Helper Surface

# Current Packet

## Just Finished

Step 1 located the byval pointer-source classification surface for the active
idea 260 runbook. The selected helper is
`prepared_store_source_load_local_is_byval_formal_pointer_source(...)` in
`src/backend/prealloc/publication_plans.cpp`, with
`is_byval_formal_value_name(...)` as the local formal-name classifier.

The helper currently depends on prepared authority from
`PreparedAddressingFunction`, the selected
`PreparedEdgePublicationSourceProducer`, prepared memory-access rows, and
`PreparedNameTables` value-name lookup against the BIR function's byval
parameters. It accepts only a load-local producer with a valid block label,
finds the prepared memory access at the producer block/instruction index,
requires pointer-value addressing with a pointer value name, requires
`can_use_base_plus_offset`, and then requires that prepared pointer value name
to spell a byval BIR formal.

The primary prepared consumer boundary is
`populate_store_source_publication_plans(...)`, which computes
`byval_load_local_source` for store-local publication planning and passes it
through `plan_prepared_store_source_publication(...)` into
`PreparedStoreSourcePublicationPlan::byval_load_local_source`. The nearby fixed
formal surface, `plan_prepared_fixed_formal_store_source_publication(...)`,
stays a store-source/formal-publication consumer boundary and does not own the
byval classifier. A target-side consumer also exists in
`src/backend/mir/aarch64/codegen/memory.cpp` through
`plan_store_local_source_publication(...)`, but Step 2 should not edit target
lowering unless a local prepared-boundary change exposes a real integration
gap.

Current fail-closed behavior to preserve: missing prepared addressing, missing
source producer, non-`LoadLocal` producer, missing `load_local`, invalid block
label, missing prepared memory-access row, non-pointer-value addressing,
missing pointer value name, `can_use_base_plus_offset == false`, missing raw
prepared value name, missing BIR function, and non-byval formal names all
return `false`.

## Suggested Next

Execute Step 2 by adding the smallest prepared agreement boundary in
`src/backend/prealloc/publication_plans.cpp`, with focused coverage in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`. Add
`src/backend/prealloc/publication_plans.hpp` only if Step 2 needs a small
test-visible seam; otherwise keep the existing public helper surface.

## Watchouts

- Keep this packet limited to the byval pointer-source classification
  candidate.
- Do not absorb the direct-global select-chain dependency or
  source-value/source-producer metadata candidates.
- Do not reactivate completed module, names, control-flow, or recovered-source
  packets.
- Preserve prepared addressing authority, formal-name authority, public
  prepared aggregate compatibility, and current fail-closed behavior.
- Do not rewrite output expectations, diagnostics, helper statuses, or target
  output to claim progress.
- Leave `formal_publications.cpp`, `formal_publications.hpp`,
  `tests/backend/bir/backend_prepare_liveness_test.cpp`, target lowering,
  direct-global select-chain logic, recovered-source logic, source-producer
  metadata, printers, diagnostics, and output baselines out of Step 2 unless
  the supervisor delegates a wider packet.
- Existing MIR coverage in
  `tests/backend/mir/backend_store_source_publication_plan_test.cpp` exercises
  the current public positive path, but the next focused packet should add the
  delegated helper-test rows instead of moving this packet into MIR coverage.

## Proof

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`

Result: passed. Build was up to date, and
`backend_prepared_lookup_helper` passed 1/1. Output is preserved in
`test_after.log`.
