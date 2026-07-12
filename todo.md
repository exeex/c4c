# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and migrate AArch64 dispatch authority

## Just Finished

- Plan Step 1 removed the remaining optional Route6 call-use evidence from the
  AArch64 call-result source-register publication API and implementation.
  Publication now consumes only the prepared late-publication fact and indexed
  prepared value home, and the focused owner test asserts that prepared
  publication directly without constructing or varying Route6 evidence.

## Suggested Next

- Review the remaining AArch64 named-handoff materializers and select the next
  coherent executable-route family for migration to common prepared queries.

## Watchouts

- Route6 call-argument/select-chain dependency handling remains in
  `calls.cpp` and `select_materialization.cpp`; it is a separate family and was
  not changed by this call-result source-publication packet.
- The focused baseline retains the known scalar-FP literal-add failure (test
  354, missing `bl printf`) and has no additional failures.

## Proof

- Ran `set -o pipefail; { cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R '^backend_(codegen_route|cli)_aarch64_'; } 2>&1
  | tee test_after.log`. Build succeeded; 33/34 tests passed, with only the
  baseline test 354 failure. Proof log: `test_after.log`.
