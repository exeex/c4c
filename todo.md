# Current Packet

Status: Active
Source Idea Path: ideas/open/779_lir_cast_result_authority_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add malformed-result verifier coverage

## Just Finished

- Plan Step 1 is complete: selected standalone casts retain the explicit
  missing-result rejection from `5a9888938`, and `verify_module` now records
  native instruction-result `LirValueId` owners across the module. A selected
  standalone cast rejects an ID owned by another `LirFunction`; invalid IDs and
  same-function duplicate definition rejection retain their existing paths.

## Suggested Next

- Plan Step 2: add nearby malformed-result verifier coverage for missing,
  invalid, same-function duplicate, and cross-function standalone cast result
  IDs, then prove the semantic capability without touching producer lowering.

## Watchouts

- The cross-function ownership index contains instruction results only and is
  consulted only by the explicit standalone cast flag; it has no rendered
  spelling, result-name-map, PHI, logical, or generic-producer behavior.
- Keep PHI, logical producer lowering, generic expression APIs, and the
  unaccepted 778 `binary.cpp` diff out of the next packet.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`.
- Proof output: `test_after.log`.
