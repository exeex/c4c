# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 5.3
Current Step Title: Take subsequent checked ordinary rows one at a time

## Just Finished

- Plan Step 5.3: received native integer intrinsic `LirCallOp` rows for
  `Cttz`, `Ctlz`, and `Ctpop` into a separate tagged Raw-BIR intrinsic-call
  payload with module `LinkNameId`, exact integer operands/signature, native
  zero-count behavior/`i1` flag where applicable, source-backed owning result,
  verifier coverage, and Raw/Canonical transactional rejection coverage.

## Suggested Next

- Supervisor to select the next bounded Step 5 ordinary-instruction receipt
  packet; the intrinsic-call row is complete.

## Watchouts

- The next packet must keep the i64 intrinsic follow-on `LirCastOp Trunc`,
  scalar binary/compare/select/abs rows, direct floating calls, and opaque
  inline-assembly rows separately scoped. Other intrinsic kinds and unsupported
  direct/indirect/variadic/ABI/aggregate/object forms remain fail-closed.

## Proof

- `cmake --build --preset default` followed by
  `ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$' > test_after.log`
  passed for Step 5.3; `test_after.log` is the preserved proof log.
