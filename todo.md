Status: Active
Source Idea Path: ideas/open/413_prepared_contract_verifier_and_owner_taxonomy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Publish The Taxonomy Matrix

# Current Packet

## Just Finished

Step 4 - Publish The Taxonomy Matrix: published
`docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md` as
the durable idea 413 handoff artifact for idea 418 and ideas 414-417.

Files changed:

- `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`
- `todo.md`

Documented selected row IDs:

- `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001`
- `TAX-FAM-CALL-BOUNDARY-ARG-RESULT-001`
- `TAX-FAM-VARIADIC-HELPER-OPERAND-HOMES-001`

Documented placeholder/follow-up row IDs:

- `TAX-FAM-CALL-ARG-TYPED-ROUTES-PLACEHOLDER-001`
- `TAX-FAM-VALUE-MATERIALIZATION-PLACEHOLDER-001`
- `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001`
- `TAX-FAM-HELPER-OPERAND-TYPED-PLACEHOLDER-001`
- `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001`
- `TAX-FAM-GLOBAL-INITIALIZER-STORAGE-PLACEHOLDER-001`

Evidence recorded in the document:

- Owner classes and report fields from
  `src/backend/prealloc/prepared_contract_verifier.hpp`.
- Selected family classification behavior from
  `src/backend/prealloc/prepared_contract_verifier.cpp`.
- AArch64 diagnostic attachment points from
  `src/backend/mir/aarch64/module/module.hpp`,
  `src/backend/mir/aarch64/codegen/operands.cpp`,
  `src/backend/mir/aarch64/codegen/calls.cpp`, and
  `src/backend/mir/aarch64/codegen/variadic.cpp`.
- Focused backend tests from
  `tests/backend/bir/backend_prealloc_decoded_home_storage_test.cpp`,
  `tests/backend/bir/backend_prealloc_call_boundary_classification_test.cpp`,
  `tests/backend/mir/backend_aarch64_operand_resolution_test.cpp`, and
  `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp`.

## Suggested Next

Proceed to Step 5 acceptance validation and close readiness. The next packet
should run the supervisor-selected final validation, review the diff for
testcase overfit or expectation weakening, and decide whether idea 413 is ready
for lifecycle close handling.

## Watchouts

The matrix reserves row IDs for memory-access, publication, value
materialization, typed call-argument, helper-operand, and global initializer
follow-up work, but those rows are placeholders. Downstream ideas should refine
them after idea 418 audits concrete target consumer sites. No implementation,
test, build output, README, plan, or source idea files were touched in this
docs-only packet.

## Proof

Docs-only packet; no build or test command was delegated or run. Proof is the
published matrix artifact plus source/file-reference evidence recorded there and
in this `todo.md` packet. This docs-only proof relies on the accepted Step 3
backend validation, which recorded 326/326 backend tests passed before
supervisor regression-log roll-forward.
