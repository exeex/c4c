# Current Packet

Status: Active
Source Idea Path: ideas/open/416_prepared_helper_operand_home_contracts.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Migrate AArch64 Helper Lowering

## Just Finished

Step 5 AArch64 cleanup is complete. AArch64 variadic helper record
construction for `va_start`, scalar `va_arg`, aggregate `va_arg`, and
`va_copy` obtains operand homes/access plans through the typed helper payload
accessors instead of dereferencing legacy optional-bag fields directly. The
AArch64 va_list field-address lookup and helper-completeness check also route
through typed accessors. The final audit migrated remaining consumer-like test
assertions to typed accessors; remaining optional-bag reads in owned AArch64
tests are fixture setup/mutation scaffolding for constructing complete or
incomplete helper rows, not target consumer/report paths.

## Suggested Next

Proceed to Step 6: migrate RV64 target consumers to use typed variadic helper
payload accessors while preserving existing RV64 diagnostics and without
inferring operand homes in target lowering.

## Watchouts

- Do not edit the source idea for routine execution notes.
- Do not infer helper operand homes in target lowering.
- Do not weaken tests or expectations to claim progress.
- Keep the old optional-bag fields until RV64 consumer migration is complete
  and the later cleanup step can remove or quarantine compatibility storage;
  typed accessors intentionally preserve compatibility derivation and refresh
  from complete legacy rows.
- Existing RV64 producer behavior still only pushes complete `va_start` homes,
  but pushes scalar `va_arg`, aggregate `va_arg`, and `va_copy` rows in more
  incomplete states so diagnostics can name helper-specific missing facts.
- Aggregate `va_arg` has an RV64-only `payload_write_address` requirement that
  is not part of the typed aggregate payload added in this slice because the
  existing generic completeness predicate does not require it.
- Stale typed payloads must not mask missing or incomplete optional facts.
  Complete rewritten legacy rows intentionally refresh the derived typed cache
  for compatibility with existing mutable target fixtures.
- AArch64 target lowering now uses typed helper payload accessors for helper
  consumption; do not reintroduce optional-bag consumption there.
- RV64 target consumers are not migrated yet and should be handled in a later
  owned packet.
- Remaining AArch64 optional-bag reads are in test fixture setup/mutation code
  that deliberately constructs complete or incomplete helper rows.

## Proof

Ran the supervisor-selected Step 5 proof:
`cmake --build build --target backend_aarch64_instruction_dispatch_test backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer)$' > test_after.log 2>&1`

Result: passed; `test_after.log` contains 3/3 passing tests.
