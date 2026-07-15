# Current Packet

Status: Active
Source Idea Path: ideas/open/784_lir_native_vaarg_operand_carrier_foundation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Bind the minimal generic carrier contract

## Just Finished

- Closed 785 accepted its direct factory carrier at `3b53451c0`. The AArch64
  FP128 ptrmask direct call now retains its native result through the immediate
  typed GEP consumer; focused structural proof passed. 784 Step 1 remains
  accepted at `b96751b03`.

## Suggested Next

- Plan Step 2: retry the minimal three-chain carrier contract with the direct
  FP128 ptrmask result available, then retain the separate decision on whether
  value-only PHI incoming transport is indispensable.

## Watchouts

- 785 does not authorize HFA ptrmask conversion, unrelated generic call/argument
  changes, PHI verification, predecessor/edge identity, Raw-BIR/importer,
  backend, target lowering, MIR, emission, or broad generic-expression work.
- Do not recover identity from compatibility spelling or other text.

## Proof

- Accepted 785 proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^frontend_lir_call_type_ref$'` passed 1/1. The
  monotonic regression guard passed with before/after 1 passed, 0 failed, and
  no new failures/timeouts; commit `3b53451c0`.
