# Current Packet

Status: Active
Source Idea Path: ideas/open/416_prepared_helper_operand_home_contracts.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Migrate RV64 Helper Lowering

## Just Finished

Step 6 RV64 object-route migration is complete. RV64 `va_start` and aggregate
`va_arg` helper diagnostics/fragments obtain operand homes/access plans through
typed helper payload accessors instead of directly consuming legacy optional-bag
fields. The object-route helper coherence check dispatches by helper kind to
typed accessors, and `rv64_va_start_destination_load_offset(...)` selects the
prior `va_start` payload through the typed accessor before reading destination
homes. The final audit found no remaining direct optional-bag consumer/report
reads in `object_emission.cpp`; remaining optional-bag references in the owned
RV64 test are fixture setup/mutation scaffolding. The aggregate
`payload_write_address` requirement remains an RV64 object-route support check
on the typed aggregate access plan.

## Suggested Next

Proceed to the next plan step: remove or quarantine legacy optional-bag
compatibility storage once all remaining prepared/target consumers are ready,
while preserving typed accessor compatibility until the cleanup is explicit.

## Watchouts

- Do not edit the source idea for routine execution notes.
- Do not infer helper operand homes in target lowering.
- Do not weaken tests or expectations to claim progress.
- Step 6 is complete for RV64 object-route consumers; keep the old optional-bag
  fields until the explicit compatibility-storage cleanup step.
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
- Remaining RV64 optional-bag reads are in test fixture setup/mutation code
  that deliberately constructs complete or incomplete helper rows.

## Proof

Ran the supervisor-selected Step 6 proof:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_riscv64_variadic_entry_missing_contract_obj|backend_dump_riscv64_variadic_aggregate_overflow_helper_contract|backend_cli_riscv64_variadic_aggregate_overflow_helper_contract_obj)$' > test_after.log 2>&1`

Result: passed; `test_after.log` contains 6/6 passing tests.
