# Current Packet

Status: Active
Source Idea Path: ideas/open/416_prepared_helper_operand_home_contracts.md
Source Plan Path: plan.md
Current Step ID: Step 7
Current Step Title: Retire the Optional-Bag Consumer API

## Just Finished

Step 7 started the optional-bag consumer API retirement by quarantining the
remaining prepared-printer report path. Variadic helper operand-home rows now
print helper-specific homes from the typed per-helper payload accessors for
`va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy`; incoherent or
incomplete rows print typed `<none>` payload fields instead of leaking unrelated
legacy optional-bag values. The focused printer test now covers a mismatched
legacy optional-bag row and rejects reporting it as helper-home content.

## Suggested Next

Continue Step 7 by auditing whether the remaining public legacy optional fields
and generic completeness helper names can be narrowed, renamed, or documented as
producer/test fixture compatibility storage without breaking existing setup
paths.

## Watchouts

- Do not edit the source idea for routine execution notes.
- Do not infer helper operand homes in target lowering.
- Do not weaken tests or expectations to claim progress.
- Migrated target, verifier, and printer consumers use typed helper payload
  accessors; do not reintroduce direct optional-bag consumption there.
- Existing producer behavior still writes legacy optional fields while typed
  payloads derive from complete coherent rows for compatibility.
- Aggregate `va_arg` has an RV64-only `payload_write_address` requirement that
  remains object-route support data on the typed aggregate access plan, not
  generic typed payload completeness.
- Stale typed payloads must not mask missing or incomplete optional facts.
  Complete rewritten legacy rows intentionally refresh the derived typed cache
  for compatibility with existing mutable target fixtures.
- Do not infer helper operand homes from stack layout, BIR shape, or source
  spelling in target lowering.
- Remaining optional-bag reads are producer construction, compatibility
  derivation, or test fixture setup/mutation scaffolding.

## Proof

Ran the supervisor-selected Step 7 proof:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_prealloc_prepared_contract_verifier|backend_prepared_printer|backend_prepare_frame_stack_call_contract|backend_prepared_object_consumer_contract|backend_riscv_object_emission|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_cli_riscv64_variadic_entry_missing_contract_obj|backend_dump_riscv64_variadic_aggregate_overflow_helper_contract|backend_cli_riscv64_variadic_aggregate_overflow_helper_contract_obj)$' > test_after.log 2>&1`

Result: passed; `test_after.log` contains 11/11 passing tests.
