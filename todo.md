Status: Active
Source Idea Path: ideas/open/154_bir_memory_access_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove BIR-Representable Memory Equivalence

# Current Packet

## Just Finished

Step 4 BIR-representable memory equivalence proof is complete.

Tightened the existing Step 4 prepared/BIR comparison coverage in
`tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`.
The direct memory identity helper already proves local, global, pointer,
string, volatile, result-name, stored-name, base-kind, address-space, and
semantic source identity for BIR-representable positives. This packet added
explicit prepared/BIR comparison rejection for global symbol, address-space,
volatile flag, pointer value, and string identity mismatches in the unavailable
path test, alongside the existing missing access, result-name mismatch, and
stored-name mismatch checks.

Existing committed tests continue to cover same-block global-load access
identity, load-local stored-value/source identity, store-source publication
facts, and the prepared-only same-slot non-overlap boundary where prepared can
prove range non-overlap but BIR intentionally fails closed. No consumers were
switched and no implementation files were touched.

## Suggested Next

Next executable packet candidate: Step 5 consumer migration for the first
BIR-representable memory path only.

Start with one narrow consumer path whose correctness is fully covered by the
Step 4 semantic proof, such as direct global-load access identity or direct
memory access identity. Keep prepared lookup fallback/oracle behavior for
layout-derived local-slot range overlap and non-overlap cases.

## Watchouts

- Keep `review/phase_a_steps_1_4_route_review.md` untouched.
- Do not switch `dispatch_value_materialization.cpp`,
  `fp_value_materialization.cpp`, `alu.cpp`, `calls.cpp`, `globals.cpp`, or
  memory-retargeting consumers unless the supervisor delegates that exact
  consumer.
- Step 5 should not chase exact prepared parity for layout-derived positives.
  BIR still does not own frame offsets, concrete sizes, alignment, register
  homes, storage encoding, target addressing, or stack-layout overlap math.
- The local-slot direct-memory comparison intentionally proves BIR semantic
  local-slot presence rather than equality to prepared frame-slot ids; those are
  different identity namespaces in existing fixtures.
- Keep prepared lookup fallback available for same-slot non-overlap positives
  and prior-store source recovery that require prepared stack-layout range
  facts.

## Proof

`cmake --build build --target backend_aarch64_prepared_memory_operand_records_test backend_store_source_publication_plan_test backend_prepared_lookup_helper_test && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_store_source_publication_plan|backend_prepared_lookup_helper)$' > test_after.log 2>&1`

Passed. `test_after.log` contains the delegated proof log with all three
selected tests passing.
