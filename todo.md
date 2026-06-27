# Current Packet

Status: Active
Source Idea Path: ideas/open/416_prepared_helper_operand_home_contracts.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Introduce Typed Per-Helper Payloads

## Just Finished

Step 3 continued typed helper payload rollout with scalar `va_arg`.
`PreparedVariadicScalarVaArgOperandHomes` now carries `source_va_list`,
`scalar_result`, and a complete `scalar_access_plan`.
`find_prepared_variadic_scalar_va_arg_operand_homes(...)` rejects wrong-helper
or incomplete optional-bag rows, while complete legacy `VaArg` rows lazily
derive the typed payload for compatibility. AAPCS64 and RV64 producers now
publish typed scalar `va_arg` payloads when the scalar access plan is complete,
and focused prepared/printer/verifier tests cover coherent typed scalar rows
plus mismatched/incomplete rejection.

## Suggested Next

Continue Step 3 with the next typed helper payload packet, preferably aggregate
`va_arg`: introduce a typed aggregate payload/accessor around `source_va_list`,
`aggregate_destination_payload`, and complete `aggregate_access_plan` while
leaving existing optional-bag consumers compiling.

## Watchouts

- Do not edit the source idea for routine execution notes.
- Do not infer helper operand homes in target lowering.
- Do not weaken tests or expectations to claim progress.
- Keep the old optional-bag fields until the AArch64/RV64 consumer migration
  steps; Step 3 currently has typed `va_start` and scalar `va_arg`
  publication/access plus compatibility derivation for complete legacy rows.
- Existing RV64 producer behavior still only pushes complete `va_start` homes,
  but pushes scalar `va_arg`, aggregate `va_arg`, and `va_copy` rows in more
  incomplete states so diagnostics can name helper-specific missing facts.
- Aggregate `va_arg` has an RV64-only `payload_write_address` requirement that
  is not part of the current generic completeness predicate.

## Proof

Ran the supervisor-selected Step 3 scalar `va_arg` proof:
`cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_prealloc_prepared_contract_verifier_test && ctest --test-dir build --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_prealloc_prepared_contract_verifier)$' > test_after.log 2>&1`

Result: passed; `test_after.log` contains 3/3 passing tests.
