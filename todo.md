Status: Active
Source Idea Path: ideas/open/413_prepared_contract_verifier_and_owner_taxonomy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Three Fact-Family Classifications

# Current Packet

## Just Finished

Step 3 - Implement Three Fact-Family Classifications: added prepared contract
verifier classifications for the remaining selected families, call boundary
argument/result plans and variadic entry helper operand-home/access-plan facts,
without adding target fallback inference. Tightened the call-boundary
argument/result plan family so failed AArch64 call-boundary move consumers
attach the shared prepared contract report on the normal module-lowering
diagnostic surface instead of leaving the verifier helper as a standalone API.

Files changed:

- `src/backend/prealloc/prepared_contract_verifier.hpp`
- `src/backend/prealloc/prepared_contract_verifier.cpp`
- `src/backend/mir/aarch64/codegen/variadic.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `tests/backend/bir/backend_prealloc_call_boundary_classification_test.cpp`
- `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp`
- `tests/backend/mir/backend_aarch64_operand_resolution_test.cpp`

Implemented classifications:

- `verify_prepared_call_boundary_move_contract` wraps the existing
  `classify_prepared_call_boundary_move` statuses into the shared owner
  taxonomy. Missing call argument/result plans and ABI bindings classify as
  `producer_missing`, mismatched result plans and missing ABI indices classify
  as `producer_incoherent`, and unsupported move op kinds classify as
  `target_unsupported_but_coherent`.
- `verify_prepared_variadic_entry_plan_contract` classifies missing or
  incomplete variadic entry-plan authority as `producer_missing` before
  AArch64 variadic consumers lower helper calls.
- `verify_prepared_variadic_entry_helper_operand_homes_contract` classifies
  missing helper operand-home/access-plan facts as `producer_missing` and
  present-but-incomplete facts as `producer_incoherent`.

Target boundary wiring:

- AArch64 variadic entry-plan diagnostics from
  `require_prepared_variadic_entry_plan` now attach a target-neutral prepared
  contract report.
- AArch64 variadic helper operand-home/access-plan rejection in call lowering
  now attaches the shared report before returning `std::nullopt`.
- AArch64 call-boundary argument/result move rejection now attaches the shared
  call-boundary report when the prepared classifier exposes missing or
  incoherent argument/result plan authority. Focused tests assert the exposed
  report for a missing call argument ABI binding and a mismatched call result
  plan.
- The existing value-home/typed-storage Step 2 classification remains in place,
  so the three selected families now all have report coverage.

## Suggested Next

Proceed to Step 4 by publishing
`docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md` with
rows for the three selected verifier families and placeholders for remaining
prepared contract follow-up work.

## Watchouts

Call-boundary classification deliberately reuses existing prealloc status
authority; do not duplicate or replace it in target code. The target consumer
only attaches the report at existing fail-closed rejection points so specialized
prepared call-boundary lowerings that are already coherent remain intact.
Variadic helper reports classify missing and incoherent helper/access-plan
facts, but broader publication facts such as edge/block publication remain
outside this packet.

## Proof

Delegated proof:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '\''^backend_'\''' |& tee test_after.log'`
passed. `test_after.log` records 326/326 backend tests passed.
