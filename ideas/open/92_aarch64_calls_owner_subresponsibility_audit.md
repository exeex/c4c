# AArch64 Calls Owner Subresponsibility Audit

## Goal

Audit `src/backend/mir/aarch64/codegen/calls.cpp` after the call/publication,
aggregate transport, prepared-wrapper, call-boundary/printer, and
aggregate-lane schema cleanup routes, then produce scoped follow-up ideas only
for calls subresponsibilities whose owner boundary is clear.

## Why This Exists

`calls.cpp` is now the largest AArch64 codegen owner. The earlier cleanup routes
established important boundaries:

- idea 69 made AArch64 call lowering consume prepared call-boundary, move
  bundle, after-call result, and publication source facts before target-local
  emission
- idea 75 established shared aggregate transport authority while accepting
  large indirect byval selected-source materialization as target-local residue
- idea 84 removed redundant prepared consumer wrappers while retaining helpers
  that own AArch64 ABI, diagnostics, scratch choice, addressing, or
  machine-record construction
- idea 87 rejected broad calls/printer rewrites without traced record paths
- ideas 90 and 91 preserved the boundary where `calls.cpp` owns ABI/prepared
  source construction and `machine_printer.cpp` owns final validation/spelling

The remaining question is not whether call lowering should move wholesale to
BIR/prealloc. It is whether `calls.cpp` contains separable local owners for
call-boundary moves, stack argument materialization, indirect callee
materialization, f128/aggregate carriers, preservation/republication, result
retargeting, or ABI binding records.

## Owned Files

- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- Read-only comparison against:
  - `ideas/closed/69_aarch64_call_publication_prepared_authority_cleanup.md`
  - `ideas/closed/75_shared_aggregate_transport_plan_probe.md`
  - `ideas/closed/84_aarch64_prepared_consumer_wrapper_contraction.md`
  - `ideas/closed/87_aarch64_call_boundary_record_printer_surface_audit.md`
  - `ideas/closed/90_aarch64_aggregate_lane_helper_table_contraction.md`
  - `ideas/closed/91_aarch64_call_boundary_aggregate_lane_record_schema_cleanup.md`

## In Scope

- Build a function-level inventory of the calls owner and classify clusters as:
  - `prepared-call-plan-consumption`
  - `direct-indirect-call-emission`
  - `before-after-call-move-bundles`
  - `call-boundary-record-construction`
  - `call-boundary-abi-binding-records`
  - `stack-argument-marshalling`
  - `indirect-callee-materialization`
  - `value-and-preservation-republication`
  - `immediate-argument-publication`
  - `f128-carrier-handling`
  - `aggregate-byval-lane-transport`
  - `sret-and-result-retargeting`
  - `callee-saved-preservation`
  - `diagnostics-and-selection-status`
  - `candidate-local-subowner`
  - `needs-shared-authority-evidence`
- Produce follow-up implementation ideas only when a cluster has a stable owner
  boundary, narrow public/private API, and proof route.
- Explicitly mark clusters that should remain in `calls.cpp` because they own
  AArch64 ABI construction, selected-node facts, prepared-source decisions,
  scratch/preservation material, call spelling, or machine-record construction.
- Identify whether any cluster needs a separate evidence idea before
  implementation.

## Out Of Scope

- Direct implementation edits in `calls.cpp` or `calls.hpp`.
- Moving AArch64 ABI register conversion, concrete call spelling, scratch
  choice, source/destination selection, or call-boundary machine-record
  construction into shared BIR/prealloc.
- Reopening dispatch publication, edge-copy, memory, instruction/printer, or
  aggregate-lane cleanup routes.
- Splitting every call-boundary record family without a traced no-semantics
  cleanup boundary.
- Treating line-count reduction as success without preserving call lowering
  behavior and diagnostics.

## Proof Expectations

- This is audit-only; no backend tests are required unless implementation files
  are accidentally touched.
- The close note must name any generated follow-up ideas and explain which
  calls clusters remain intentionally target-local.
- Any generated implementation idea must name a focused proof set covering its
  affected call forms, such as ordinary argument/return moves, before/after
  move bundles, indirect calls, stack arguments, f128 carriers, aggregate byval
  lanes, preservation/republication, sret/result retargeting, and rejection
  diagnostics.

## Reviewer Reject Signals

- The audit proposes one monolithic "shrink calls.cpp" implementation route.
- It says "move to BIR" without naming a missing target-neutral call or ABI
  fact.
- It ignores ideas 84, 87, 90, or 91 and reopens broad calls/printer cleanup.
- It treats aggregate transport, call-boundary record schema, or prepared call
  authority as incomplete without new evidence.
- It proposes helper movement that would re-derive prepared call, move-bundle,
  after-call result, preservation, aggregate transport, or publication facts
  locally under new names.

