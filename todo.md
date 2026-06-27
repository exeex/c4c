# Current Packet

Status: Active
Source Idea Path: ideas/open/416_prepared_helper_operand_home_contracts.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Migrate AArch64 Helper Lowering

## Just Finished

Step 5 started the AArch64 target-consumer migration. AArch64 variadic helper
record construction for `va_start`, scalar `va_arg`, aggregate `va_arg`, and
`va_copy` now obtains operand homes/access plans through the typed helper
payload accessors instead of dereferencing legacy optional-bag fields directly.
The AArch64 va_list field-address lookup and helper-completeness check also
route through typed accessors. Target instruction record tests now assert that
selected helper call records expose typed payloads for all four helper
families while preserving existing supported lowering behavior and diagnostics.

## Suggested Next

Continue Step 5 by auditing any remaining AArch64 variadic-helper target
consumer/report paths for legacy optional-bag reads, then decide whether the
AArch64 migration is complete or needs a final cleanup packet before RV64.

## Watchouts

- Do not edit the source idea for routine execution notes.
- Do not infer helper operand homes in target lowering.
- Do not weaken tests or expectations to claim progress.
- Keep the old optional-bag fields until both AArch64 and RV64 consumer
  migrations are complete; typed accessors intentionally preserve
  compatibility derivation and refresh from complete legacy rows.
- Existing RV64 producer behavior still only pushes complete `va_start` homes,
  but pushes scalar `va_arg`, aggregate `va_arg`, and `va_copy` rows in more
  incomplete states so diagnostics can name helper-specific missing facts.
- Aggregate `va_arg` has an RV64-only `payload_write_address` requirement that
  is not part of the typed aggregate payload added in this slice because the
  existing generic completeness predicate does not require it.
- Stale typed payloads must not mask missing or incomplete optional facts.
  Complete rewritten legacy rows intentionally refresh the derived typed cache
  for compatibility with existing mutable target fixtures.
- AArch64 target lowering should not infer or synthesize helper homes; missing
  or incoherent helper facts should continue to fail closed through the
  existing diagnostics.
- RV64 target consumers are not migrated yet and should be handled in a later
  owned packet.

## Proof

Ran the supervisor-selected Step 5 proof:
`cmake --build build --target backend_aarch64_instruction_dispatch_test backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer)$' > test_after.log 2>&1`

Result: passed; `test_after.log` contains 3/3 passing tests.
