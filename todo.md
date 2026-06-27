# Current Packet

Status: Active
Source Idea Path: ideas/open/416_prepared_helper_operand_home_contracts.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Move Completeness Verification to Typed Payloads

## Just Finished

Step 4 cleanup is complete for verifier/report paths. The verifier already
classifies `va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy`
coherence through typed helper payload accessors. The prepared variadic printer
now also prints scalar and aggregate access-plan detail through the typed
payload accessors, so incoherent or incomplete optional-bag rows report
`<none>` instead of treating partial legacy access-plan fields as coherent
report data. Remaining generic completeness references are limited to producer
compatibility, typed-accessor wrapper helpers, and tests that assert the
transition behavior.

## Suggested Next

Proceed to Step 5: migrate AArch64 target consumers to use the typed helper
payload accessors for variadic helper operand homes while preserving existing
diagnostics and without inferring operand homes in target lowering.

## Watchouts

- Do not edit the source idea for routine execution notes.
- Do not infer helper operand homes in target lowering.
- Do not weaken tests or expectations to claim progress.
- Keep the old optional-bag fields until the AArch64/RV64 consumer migration
  steps; typed accessors intentionally preserve compatibility derivation and
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
- Remaining generic helper completeness references after this cleanup are
  intentional: the RV64/AAPCS64 producer path still uses them for compatibility
  gating, the wrappers in `variadic.hpp` delegate to typed accessors, and
  focused tests assert complete/incomplete transitional behavior.

## Proof

Ran the supervisor-selected Step 4 proof:
`cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_prealloc_prepared_contract_verifier_test && ctest --test-dir build --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_prealloc_prepared_contract_verifier)$' > test_after.log 2>&1`

Result: passed; `test_after.log` contains 3/3 passing tests.
