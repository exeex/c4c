Status: Active
Source Idea Path: ideas/open/289_structured_opaque_pointer_byte_range_provenance.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Publish Provenance Verdicts to Prepared Consumers

# Current Packet

## Just Finished

Step 5: Publish Provenance Verdicts to Prepared Consumers audited current
prepared/prealloc `can_use_base_plus_offset` consumers for object-range-safety
inference risk. No narrowly safe code change was made in this packet: the safe
copy/reporting surfaces already carry explicit provenance verdict metadata, and
the remaining gates are behavior-changing optimization or target-lowering
decisions.

- `PreparedAddress::can_use_base_plus_offset` in
  `src/backend/prealloc/addressing.hpp` is the source syntactic address form
  bit; it is not an object-range verdict.
- `prepared_printer/addressing.cpp` is copy-surface reporting only: it prints
  `base_plus_offset` alongside `range_verdict` and `dynamic_array_verdict`.
- `prepared_lookups.cpp::copy_source_memory_access_fact` copies the syntactic
  bit into `PreparedEdgePublication` and derives
  `source_memory_requires_address_materialization`; verdicts are copied next to
  it and availability still does not gate on them.
- `prepared_lookups.cpp::find_prepared_global_load_access` uses the bit as a
  same-block global-load addressing-form gate. Treating out-of-bounds verdicts
  as rejection here would change prepared lookup behavior, so it is deferred.
- `publication_plans.cpp::copy_prepared_edge_copy_source_facts` and
  `prepared_edge_publication_source_memory_matches_access` are copy-surface and
  stale-row/identity checks; they compare explicit verdict metadata without
  weakening existing fail-closed behavior.
- `publication_plans.cpp::make_prepared_store_source_publication_plan` copies
  the destination syntactic bit into store-source plans. Downstream users in
  target lowering still own acceptance policy, so verdict gating is deferred.
- `publication_plans.cpp::prepared_store_source_load_local_is_byval_formal_pointer_source`
  uses the bit for a byval formal pointer-source optimization gate. Adding
  range-verdict rejection here would be a behavior change and belongs with the
  Step 6 bridge/acceptance decision.
- Downstream prepared consumers of copied bits were identified in RISC-V edge
  publication, AArch64 dispatch edge copies/store-source lowering, and x86
  module lowering. These are target-specific gates and were not changed.

## Suggested Next

Move to Step 6 under supervisor direction: decide how target-specific gates and
the opaque pointer compatibility bridge should treat explicit provenance
verdicts before any consumer starts rejecting currently accepted prepared rows.

## Watchouts

- `range_verdict` and `dynamic_array.verdict` remain passive metadata; no
  prepared/prealloc consumer currently uses them as an admission gate.
- Changing `find_prepared_global_load_access`,
  `prepared_store_source_load_local_is_byval_formal_pointer_source`, or target
  lowering to reject `ProvenOutOfBounds` would alter current behavior and needs
  explicit Step 6 policy.
- Existing stale-row/fail-closed behavior remains unchanged.
- The audit used `rg` plus `c4c-clang-tool-ccdb` callers for the key prepared
  helpers.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepared_lookup_helper_test backend_riscv_prepared_edge_publication_test backend_prepared_printer_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepared_lookup_helper|backend_riscv_prepared_edge_publication|backend_prepared_printer|backend_aarch64_instruction_dispatch)$' > test_after.log`

CTest result: 5/5 passed.
