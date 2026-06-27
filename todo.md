# Current Packet

Status: Active
Source Idea Path: ideas/open/416_prepared_helper_operand_home_contracts.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Introduce Typed Per-Helper Payloads

## Just Finished

Step 3 first prepared-side slice introduced typed `va_start` helper payloads.
`PreparedVariadicVaStartOperandHomes` now carries destination storage and address
homes, `find_prepared_variadic_va_start_operand_homes(...)` rejects wrong-helper
or incomplete rows, and
`publish_prepared_variadic_va_start_operand_homes(...)` keeps the typed payload
in sync with the existing optional-bag fields. AAPCS64 and RV64 producers now
publish typed `va_start` payloads while preserving the old optional fields for
existing target consumers. Focused prepared/printer/verifier tests cover coherent
typed `va_start` rows plus wrong-helper and incomplete optional-bag rejection.
Compatibility repair: complete legacy `VaStart` optional-bag rows now lazily
derive the typed payload through the accessor, so existing handcrafted target
fixtures remain coherent without weakening wrong-helper or incomplete-row
rejection.

## Suggested Next

Continue Step 3 with the next typed helper payload packet, preferably scalar
`va_arg`: introduce a typed scalar payload/accessor around `source_va_list`,
`scalar_result`, and complete `scalar_access_plan` while leaving existing
optional-bag consumers compiling.

## Watchouts

- Do not edit the source idea for routine execution notes.
- Do not infer helper operand homes in target lowering.
- Do not weaken tests or expectations to claim progress.
- Keep the old optional-bag fields until the AArch64/RV64 consumer migration
  steps; this slice added typed `va_start` publication/access plus compatibility
  derivation for complete legacy `VaStart` rows.
- Existing RV64 producer behavior still only pushes complete `va_start` homes,
  but pushes scalar `va_arg`, aggregate `va_arg`, and `va_copy` rows in more
  incomplete states so diagnostics can name helper-specific missing facts.
- Aggregate `va_arg` has an RV64-only `payload_write_address` requirement that
  is not part of the current generic completeness predicate.

## Proof

Ran the supervisor-selected Step 3 proof:
`cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_prealloc_prepared_contract_verifier_test && ctest --test-dir build --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_prealloc_prepared_contract_verifier)$' > test_after.log 2>&1`

Result: passed; `test_after.log` contains 3/3 passing tests.

Compatibility regression proof also run:
`cmake --build build --target backend_riscv_object_emission_test backend_aarch64_machine_printer_test backend_aarch64_target_instruction_records_test backend_aarch64_instruction_dispatch_test`
then
`ctest --test-dir build --output-on-failure -R '^(backend_riscv_object_emission|backend_aarch64_machine_printer|backend_aarch64_target_instruction_records|backend_aarch64_instruction_dispatch)$'`

Result: passed; 4/4 previously failing backend tests passed after rebuilding
those test executables.
