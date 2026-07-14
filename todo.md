# Current Packet

Status: Active
Source Idea Path: ideas/open/772_lir_gep_pointer_authority_pr70460.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace and repair the production GEP pointer authority loss

## Just Finished

- Plan Step 1 complete: `emit_indexed_lval_operand` now preserves the
  `DirectConstant(LirValueId)` returned by the label-address producer, and the
  typed `emit_indexed_gep` overload accepts that verified base into
  `LirGepOp.ptr` rather than falling through to text. Nearby coverage proves a
  direct label address reaches the structured GEP with its current-function ID
  and that a foreign direct ID is rejected. Fresh build, focused `pr70460`, and
  both label-address probes pass.

## Suggested Next

- Supervisor: evaluate the Step 1 handoff and select the next lifecycle or
  baseline-validation action; do not widen this completed forwarding packet.

## Watchouts

- The repair uses only the accepted table-backed direct-label-address value ID;
  it does not recover labels from text, fabricate SSA/globals, or modify 773
  verifier/printer/backend contracts. `test_after.log` contains the focused
  `pr70460` proof only; any full-suite baseline decision remains supervisor-owned.

## Proof

- Passed: fresh `cmake --build --preset default`; `ctest --test-dir build
  --output-on-failure -R '^llvm_gcc_c_torture_src_pr70460_c$'` (captured in
  `test_after.log`); and `ctest --test-dir build --output-on-failure -R
  '^frontend_lir_label_address_(rvalue_probe|table_decay_probe)$'` (2/2).
  The selected focused proof is sufficient for this packet; baseline evaluation
  remains supervisor-owned.
