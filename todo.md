# Current Packet

Status: Active
Source Idea Path: ideas/open/416_prepared_helper_operand_home_contracts.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Migrate RV64 Helper Lowering

## Just Finished

Step 6 started the RV64 target-consumer migration in the object route. RV64
`va_start` and aggregate `va_arg` helper diagnostics/fragments now obtain
operand homes/access plans through typed helper payload accessors instead of
directly consuming legacy optional-bag fields. The object-route helper
coherence check now dispatches by helper kind to typed accessors, and
`rv64_va_start_destination_load_offset(...)` selects the prior `va_start`
payload through the typed accessor before reading destination homes. The
aggregate `payload_write_address` requirement remains an RV64 object-route
support check on the typed aggregate access plan. RV64 object-emission tests now
assert typed payload exposure for the supported `va_start` and aggregate
`va_arg` fixtures.

## Suggested Next

Continue Step 6 by auditing remaining RV64 variadic-helper object-route
consumer/report paths for direct optional-bag reads, including unsupported
helper diagnostics for scalar `va_arg`/`va_copy`, then decide whether RV64
migration is complete or needs one final cleanup packet.

## Watchouts

- Do not edit the source idea for routine execution notes.
- Do not infer helper operand homes in target lowering.
- Do not weaken tests or expectations to claim progress.
- Keep the old optional-bag fields until RV64 consumer migration is fully
  audited and the later cleanup step can remove or quarantine compatibility
  storage; typed accessors intentionally preserve compatibility derivation and
  refresh from complete legacy rows.
- Existing RV64 producer behavior still only pushes complete `va_start` homes,
  but pushes scalar `va_arg`, aggregate `va_arg`, and `va_copy` rows in more
  incomplete states so diagnostics can name helper-specific missing facts.
- Aggregate `va_arg` has an RV64-only `payload_write_address` requirement that
  is not part of the typed aggregate payload added in this slice because the
  existing generic completeness predicate does not require it.
- Stale typed payloads must not mask missing or incomplete optional facts.
  Complete rewritten legacy rows intentionally refresh the derived typed cache
  for compatibility with existing mutable target fixtures.
- AArch64 target lowering already uses typed helper payload accessors for
  helper consumption; do not reintroduce optional-bag consumption there.
- RV64 aggregate `payload_write_address` is still object-route-specific
  support data on the typed aggregate access plan, not generic typed payload
  completeness.
- Do not infer helper operand homes from stack layout, BIR shape, or source
  spelling in RV64 lowering.

## Proof

Ran the supervisor-selected Step 6 proof:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_riscv64_variadic_entry_missing_contract_obj|backend_dump_riscv64_variadic_aggregate_overflow_helper_contract|backend_cli_riscv64_variadic_aggregate_overflow_helper_contract_obj)$' > test_after.log 2>&1`

Result: passed; `test_after.log` contains 6/6 passing tests.
