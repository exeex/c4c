# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and migrate AArch64 dispatch authority

## Just Finished

- Plan Step 1 migrated the AArch64 direct-global select-chain call-argument
  family to prepared call-plan authority. The materializer now fails closed
  unless the prepared publication routing names the source and supplies its
  direct-global dependency, and call lowering no longer reconstructs Route6
  direct-global records for this family.

## Suggested Next

- Migrate the remaining scalar call-argument source-producer family away from
  its Route6 source and producer record reconstruction, if existing common
  prepared queries cover its uniqueness and agreement gates.

## Watchouts

- Route6 scalar call-argument source/producer validation remains in
  `calls.cpp`; this packet intentionally preserved it as a separate dependency
  family.
- The selected-indirect-call route test exercises nearby direct-global select
  behavior and remained green without expectation changes.
- The focused baseline retains the known scalar-FP literal-add failure (test
  354, missing `bl printf`) and has no additional failures.

## Proof

- Ran `set -o pipefail; { cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R '^backend_(codegen_route|cli)_aarch64_'; } 2>&1
  | tee test_after.log`. Build succeeded; 33/34 tests passed, including the
  selected-indirect-call route, with only the baseline test 354 failure.
  Proof log: `test_after.log`.
