Status: Active
Source Idea Path: ideas/open/413_prepared_contract_verifier_and_owner_taxonomy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add The Initial Verifier Report Surface

# Current Packet

## Just Finished

Step 2 - Add The Initial Verifier Report Surface: added the initial
prealloc-level prepared contract verifier/report API and wired a selected
malformed value-home/typed-storage fact through it before AArch64 operand
consumer lowering.

Files changed:

- `src/backend/prealloc/prepared_contract_verifier.hpp`
- `src/backend/prealloc/prepared_contract_verifier.cpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/codegen/operands.cpp`
- `tests/backend/bir/backend_prealloc_decoded_home_storage_test.cpp`
- `tests/backend/mir/backend_aarch64_operand_resolution_test.cpp`

Implemented surface:

- `PreparedContractOwnerClass` covers `coherent`, `producer_missing`,
  `producer_incoherent`, `target_unsupported_but_coherent`, and
  `pre_prepared_semantic_failure`.
- `PreparedContractFactFamily` names the selected initial families:
  `value_home_typed_storage`, `call_boundary_argument_result_plan`, and
  `variadic_entry_helper_operand_homes`.
- `PreparedContractVerificationReport` carries fact family, owner class,
  prepared value identity, `fail_closed`, and precise detail text.
- `verify_prepared_decoded_home_storage_contract` classifies the first concrete
  family. Missing authority reports `producer_missing`, present-but-incomplete
  payload/placement reports `producer_incoherent`, and unsupported coherent
  storage/home forms report `target_unsupported_but_coherent`.

Target boundary wiring:

- AArch64 `resolve_value_operand` now verifies the decoded prepared
  home/storage fact immediately after the prepared decode and before
  `make_decoded_operand` can materialize a target MIR operand.
- Fail-closed decoded facts still use the existing precise producer diagnostic
  messages, but now attach the target-neutral prepared contract report to the
  `ModuleLoweringDiagnostic`.
- The existing register-spelling-only value-home malformed fact remains
  stopped before operand lowering and now asserts a `producer_incoherent`
  `value_home_typed_storage` report.

## Suggested Next

Proceed to Step 3 by adding verifier classification functions and target
boundary assertions for the call boundary argument/result plan family and the
variadic entry helper operand-home/access-plan family.

## Watchouts

The current verifier only enforces the value-home/typed-storage family. The
call-boundary and variadic family identifiers exist in the shared report API
for Step 3, but their concrete classification functions are not implemented
yet. Keep edge/block publication facts out of the next packet unless the
supervisor broadens scope.

## Proof

Focused pre-proof:
`ctest --test-dir build -j --output-on-failure -R
'^backend_(prealloc_decoded_home_storage|aarch64_operand_resolution)$'`
passed 2/2.

Delegated proof:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '\''^backend_'\''' |& tee test_after.log'`
passed. `test_after.log` records 326/326 backend tests passed.
