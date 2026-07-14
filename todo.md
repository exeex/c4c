# Current Packet

Status: Active
Source Idea Path: ideas/open/773_lir_gep_direct_label_address_constant_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Carry the validated form through printer and backend/lowering

## Just Finished

- Lifecycle return: 774 is capability-complete and archived. Its accepted
  typed Raw-BIR GEP-base representation (`c64b78c48`, `97121c359`) is now the
  bounded Step 2 input; it does not complete any 773 printer or dispatch work.
- Step 1 remains accepted in `a4415f99c`: only the current-function typed
  direct-label-address `DirectConstant(LirValueId)` passes the GEP verifier
  gate, with nearby malformed-boundary coverage.

## Suggested Next

- Execute Step 2 only: add printer receipt and typed-direct-label LIR-to-BIR
  dispatch using the structured Raw-BIR authority, with focused positive and
  malformed coverage. Do not edit 772 `emit_indexed_gep` forwarding or pr70460.

## Watchouts

- Direct GEP identity is table-backed only. Do not recover labels from text,
  fabricate SSA/globals, coerce to a global base, or widen generic GEP
  authority. The rejected 3037/1 pr70460 baseline remains out of scope.

## Proof

- 774 accepted fresh `cmake --build --preset default` and
  `ctest --test-dir build -j --output-on-failure -R
  '^backend_lir_to_bir_interface$'`; matched `^backend_` before/after guards
  passed 5/5. This proof establishes the Raw-BIR input only.
- Step 2 requires its own fresh build and focused verifier/printer/lowering
  proof. Do not refresh or accept `test_baseline.new.log`; 772 owns the later
  pr70460 forwarding repair and baseline route.
