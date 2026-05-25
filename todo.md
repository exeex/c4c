Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Move Non-Emission Spelling To Its Owner

# Current Packet

## Just Finished

Completed Step 3 of `plan.md`: moved the generic operand-to-machine-effect
spelling used by call records out of the AArch64 calls printing surface and
into the shared AArch64 machine instruction record layer.

Concrete cleanup:

- Deleted the call-local `call_effect_from_operand`,
  `call_effects_from_operands`, and `call_prepared_value_def` helpers from
  `calls_printing.cpp`.
- Exposed shared `machine_effect_from_operand`,
  `machine_effects_from_operands`, and `machine_prepared_value_def` helpers
  from `instruction.hpp` with implementations in `instruction.cpp`.
- Updated call-boundary move and call instruction record construction to use
  the shared machine-effect spelling while preserving printed and structured
  output behavior.

## Suggested Next

Continue Step 3 with a narrow printer-owned spelling scan only if the
supervisor wants another non-emission ownership move; otherwise proceed toward
Step 4 translation-unit retirement assessment.

## Watchouts

- The shared `machine_*` helpers intentionally use distinct names instead of
  exposing `effect_from_operand`, avoiding ambiguity with existing
  target-local helpers in other translation units.
- `effect_from_prepared_call_preserved_value` remains call-preservation
  specific because it still interprets prepared call preservation routes before
  building a machine effect resource.
- No tests were weakened or rewritten for this slice.

## Proof

Ran delegated proof successfully:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_machine_printer|backend_aarch64_call_boundary_owner|backend_call_boundary_effect_plan|backend_)') > test_after.log 2>&1`

Result: build passed; delegated CTest subset passed, 162/162 tests.

Proof log: `test_after.log`.
