# Current Packet

Status: Active
Source Idea Path: ideas/open/752_lir_local_object_pointer_authority_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Verify and prove the producer boundary
你該做code review了

## Just Finished

- Accepted Step 1: `ca26a8242` established selected local authority across
  alloca, direct local load/store/GEP, and VLA save/restore routes; `b200ac033`
  repaired the reviewed VLA pointer-slot store mismatch. Fresh builds passed,
  and matching `^backend_` baseline/after runs were 5/5 with the regression
  guard accepted using `--allow-non-decreasing-passed`.

## Suggested Next

- Execute Step 2 malformed producer-boundary checks: verify
  current-function ownership, pointer/object and type relations, and live
  lifetime; add nearby positive and missing/invalid/foreign/mismatched/dead
  negative coverage. Do not begin the Step 3 Raw-BIR handoff.

## Watchouts

- Stack restore is emitted only on a backward goto with a VLA lifetime route;
  ordinary VLA fixtures do not cover it. The VLA pointer-slot store is selected
  through its dynamic-allocation value ID, never its rendered spelling. Keep
  Raw-BIR/importer, memory/va, aggregate/vector, PHI/CFG, and target-lowering
  outside this packet.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log` passed (5/5). `test_after.log` is the required proof log.
