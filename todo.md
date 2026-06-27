# Current Packet

Status: Active
Source Idea Path: ideas/open/416_prepared_helper_operand_home_contracts.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Introduce Typed Per-Helper Payloads

## Just Finished

Step 3 completed the typed helper payload rollout for `va_copy`.
`PreparedVariadicVaCopyOperandHomes` now carries `destination_va_list` and
`source_va_list`. `find_prepared_variadic_va_copy_operand_homes(...)` rejects
wrong-helper or incomplete optional-bag rows, while complete legacy `VaCopy`
rows lazily derive the typed payload for compatibility. AAPCS64 and RV64
producers now publish typed `va_copy` payloads when both operand homes are
complete, and focused prepared/printer/verifier tests cover coherent typed
`va_copy` rows plus mismatched/incomplete rejection.
Repair: cached typed `va_copy` payloads no longer mask later missing or
contradictory optional-bag homes; the accessor requires the current
`destination_va_list` and `source_va_list` optional fields to still be present
and match the cached typed payload before reporting coherence.

## Suggested Next

Continue with Step 4: move completeness verification to the typed payload
accessors for `va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy`
while preserving producer-missing versus producer-incoherent owner
classification.

## Watchouts

- Do not edit the source idea for routine execution notes.
- Do not infer helper operand homes in target lowering.
- Do not weaken tests or expectations to claim progress.
- Keep the old optional-bag fields until the AArch64/RV64 consumer migration
  steps; Step 3 now has typed `va_start`, scalar `va_arg`, aggregate `va_arg`,
  and `va_copy` publication/access plus compatibility derivation for complete
  legacy rows.
- Existing RV64 producer behavior still only pushes complete `va_start` homes,
  but pushes scalar `va_arg`, aggregate `va_arg`, and `va_copy` rows in more
  incomplete states so diagnostics can name helper-specific missing facts.
- Aggregate `va_arg` has an RV64-only `payload_write_address` requirement that
  is not part of the typed aggregate payload added in this slice because the
  existing generic completeness predicate does not require it.

## Proof

Ran the supervisor-selected Step 3 `va_copy` proof:
`cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_prealloc_prepared_contract_verifier_test && ctest --test-dir build --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_prealloc_prepared_contract_verifier)$' > test_after.log 2>&1`

Result: passed; `test_after.log` contains 3/3 passing tests.

Repair reproduction/proof:
`cmake --build build --target backend_aarch64_instruction_dispatch_test && ./build/tests/backend/mir/backend_aarch64_instruction_dispatch_test`

Result: passed; stale typed `va_copy` payloads no longer hide missing prepared
source homes in AArch64 dispatch.
