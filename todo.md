# Current Packet

Status: Active
Source Idea Path: ideas/open/416_prepared_helper_operand_home_contracts.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Move Completeness Verification to Typed Payloads

## Just Finished

Step 4 repaired the typed accessor/verifier migration for mutable complete
legacy variadic helper rows. Typed helper accessors still reject wrong-helper
or incomplete optional-bag rows, but if a derived cache exists and the current
legacy optional facts are complete and have been rewritten, the cache is
refreshed from the current facts. This restores valid AArch64 scalar FP
`va_arg` prepared access-plan selection after GP scalar `va_arg` selection has
already populated the typed cache. Focused verifier coverage now checks that a
complete rewritten scalar `va_arg` row refreshes to the current FP result while
missing access-plan rows still fail closed.

## Suggested Next

Continue Step 4 by reviewing any remaining verifier/report call sites that
still depend on generic optional-bag completeness for variadic helper operand
homes, then hand off to target-consumer migration once typed-accessor
compatibility is accepted.

## Watchouts

- Do not edit the source idea for routine execution notes.
- Do not infer helper operand homes in target lowering.
- Do not weaken tests or expectations to claim progress.
- Keep the old optional-bag fields until the AArch64/RV64 consumer migration
  steps; typed accessors intentionally preserve compatibility derivation for
  complete legacy rows.
- Existing RV64 producer behavior still only pushes complete `va_start` homes,
  but pushes scalar `va_arg`, aggregate `va_arg`, and `va_copy` rows in more
  incomplete states so diagnostics can name helper-specific missing facts.
- Aggregate `va_arg` has an RV64-only `payload_write_address` requirement that
  is not part of the typed aggregate payload added in this slice because the
  existing generic completeness predicate does not require it.
- Stale typed payloads must not mask missing or incomplete optional facts.
  Complete rewritten legacy rows intentionally refresh the derived typed cache
  for compatibility with existing mutable target fixtures.

## Proof

Ran the supervisor-selected Step 4 proof:
`cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_prealloc_prepared_contract_verifier_test && ctest --test-dir build --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_prealloc_prepared_contract_verifier)$' > test_after.log 2>&1`

Result: passed; `test_after.log` contains 3/3 passing tests.

Repair reproduction/proof:
`cmake --build build --target backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test backend_aarch64_instruction_dispatch_test && ./build/tests/backend/mir/backend_aarch64_target_instruction_records_test && ./build/tests/backend/mir/backend_aarch64_machine_printer_test && ./build/tests/backend/mir/backend_aarch64_instruction_dispatch_test`

Result: passed; all three direct AArch64 binaries accept the scalar FP
`va_arg` prepared access plan.
