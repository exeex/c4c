# Current Packet

Status: Active
Source Idea Path: ideas/open/783_lir_native_vaarg_operand_result_seam_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the focused frontend-LIR baseline

## Just Finished

- Plan Step 1 baseline: identified
  `tests/frontend/frontend_lir_call_type_ref_test.cpp:4144`
  (`test_vaarg_helper_result_authority_boundary`) as the structural
  frontend-LIR probe. It verifies the semantic `LirVaArgOp`, its typed i32
  result, and the exact native SSA result ID consumed by the immediate `Add`.
  No implementation or PHI change occurred.

## Suggested Next

- Plan Step 2: extend this frontend-LIR structural fixture with the three
  native vaarg operand/result-chain probes specified by the active runbook.

## Watchouts

- `tests/backend/case/` is not the probe location for this frontend-LIR
  authority work; do not substitute backend or rendered-output assertions.
- Do not modify PHI carrier/verification or absorb Raw-BIR/importer, backend,
  target lowering, MIR, emission, generic migration, or text recovery.
- The selected fixture currently establishes only the existing vaarg result
  identity boundary; the Step 2 probes must remain structural and native-ID
  based rather than rendered-text or testcase-shaped checks.

## Proof

- Passed fresh: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'` (1/1 tests passed).
  The supervisor-selected focused structural proof is sufficient for this
  baseline packet. Full command output: `test_after.log`.
