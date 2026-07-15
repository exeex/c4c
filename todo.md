# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.26
Current Step Title: Receive selected hoisted alloca authority

## Just Finished

- Step 7.26 completed: the new Raw-BIR importer receives exactly one selected
  hoisted `LirAllocaOp` from its typed result and live local-object authority,
  with a typed container, builder, view, and reachable verifier. Nearby
  interface coverage proves transactional rejection for missing pointer
  definition, invalid object, foreign owner, pointee mismatch, dead, and
  repeated authority rows.

## Suggested Next

- Send the exhausted Step 7.26 runbook to plan-owner for an explicit closure,
  repair, replacement, or conclusion decision; do not infer source-idea
  completion from runbook exhaustion.

## Watchouts

- The receiver remains deliberately limited to one static selected alloca row.
  It does not receive local load/store/GEP, VLA lifetime, memory/va,
  aggregate/vector, body parameters, PHI/CFG, or later families, and never
  derives identity from local names or `%t` spelling.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_lir_to_bir_interface$' > test_after.log`.
  The focused receiver proof is preserved at `test_after.log`; broader
  acceptance validation remains supervisor-owned.
