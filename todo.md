# Current Packet

Status: Active
Source Idea Path: ideas/open/752_lir_local_object_pointer_authority_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Verify and prove the producer boundary
你該做code review了

## Just Finished

- Completed Step 2: the verifier now binds every populated selected local-object
  authority record to its instruction pointer/result and a current-function
  pointer definition; repeated records for the same pointer must agree on
  object, owner, pointer/pointee type, and liveness. Nearby frontend coverage
  keeps alloca, local load/store/GEP, and VLA save/restore positives and rejects
  missing/invalid, foreign, pointer/object/type-mismatched, and dead records.

## Suggested Next

- Supervisor should review the completed Step 2 slice, then choose the bounded
  Step 3 Raw-BIR handoff packet. Do not absorb Raw-BIR/importer work here.

## Watchouts

- Stack restore is emitted only on a backward goto with a VLA lifetime route;
  ordinary VLA fixtures do not cover it. The VLA pointer-slot store is selected
  through its dynamic-allocation value ID, never its rendered spelling. Keep
  Raw-BIR/importer, memory/va, aggregate/vector, PHI/CFG, and target-lowering
  outside this packet. Local-object authority remains opt-in for compatibility
  rows; this packet fail-closes malformed populated selected records.

## Proof

- Focused `^frontend_lir_call_type_ref$` coverage passed. The required
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
  proof passed (5/5); `test_after.log` is the required proof log.
