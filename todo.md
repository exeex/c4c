# Current Packet

Status: Active
Source Idea Path: ideas/open/779_lir_cast_result_authority_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Repair native standalone cast result ownership

## Just Finished

- Plan Step 1 is partially complete at commit `5a9888938`: selected standalone
  casts now reject a missing native result ID. Existing local invalid-ID and
  duplicate detection remains intact.
- Plan Step 2 made no code or test changes. Its attempted foreign-ID case
  exposed that `verify_function_value_ownership` creates a fresh definitions
  set per function and has no result-definition provenance, so a result ID
  defined in another function is accepted as a fresh local definition.

## Suggested Next

- Repair Plan Step 1: add native function provenance or equivalent module-level
  result-definition ownership so a selected standalone cast rejects reuse of a
  result `LirValueId` owned by another `LirFunction`, without regressing local
  invalid-ID or duplicate checks. Return to Step 2 only after this path exists.

## Watchouts

- The explicit flag is required because source-ID presence alone also occurs
  in the excluded logical RHS cast path, whose result authority remains raw.
- Do not claim all four malformed cases are supported yet: foreign-result
  rejection is the unresolved in-scope Step 1 gap.
- Keep PHI, logical producer lowering, generic expression APIs, and the
  unaccepted 778 `binary.cpp` diff out of the next packet.

## Proof

- Baseline passed before this runbook repair: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`.
- This Step 2 investigation did not produce test edits or new acceptance proof.
