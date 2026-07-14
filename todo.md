# Current Packet

Status: Active
Source Idea Path: ideas/open/756_lir_switch_selector_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish switch-selector authority
你該做code review了

## Just Finished

- Plan Step 1, Publish switch-selector authority: `LirSwitch.selector` now
  carries a current-function `LirValueId`, emitted from the structured selector
  operand (or a typed materialization for a legacy raw operand). The verifier
  rejects missing, foreign, non-integer, and display-mismatched selectors before
  printing/downstream use; default and case successor carriers are unchanged.
- Changed: `src/codegen/lir/ir.hpp`, `src/codegen/lir/hir_to_lir/{lowering.hpp,core.cpp,stmt.cpp}`,
  `src/codegen/lir/verify.cpp`, and `tests/frontend/frontend_lir_call_type_ref_test.cpp`.

## Suggested Next

- Supervisor-select the next Step 1 packet boundary; this packet deliberately
  stops before any Raw-BIR receipt/downstream switch work.

## Watchouts

- Selector names/types remain verifier-checked display mirrors. Do not recover
  authority from `selector_name`, `selector_type`, labels, printer output, or
  rendered text; do not change Raw-BIR receipt or switch-successor authority.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_' > test_after.log`
  (4/4 focused frontend LIR tests). Proof log: `test_after.log`.
- Supervisor owns any broader/full acceptance separately.
