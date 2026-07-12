# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and migrate AArch64 dispatch authority

## Just Finished

- Plan Step 1 migrated the AArch64 same-block global-load materialization
  family to prepared addressing authority. Direct global lowering, scalar
  dispatch materialization, and FP materialization no longer reconstruct or
  consult Route3 global-load identities; missing or inconsistent prepared
  memory-access authority now fails closed, including removal of the FP
  no-prepared semantic fallback.

## Suggested Next

- Migrate the AArch64 indirect-callee source-producer family in `calls.cpp`
  from Route4 publication records to its existing prepared producer query.

## Watchouts

- Route3 global-load authority remains absent from the three migrated
  materialization paths; other Route3 families in `alu.cpp` and `calls.cpp`
  are separate packets.
- The focused baseline retains the known scalar-FP literal-add failure (test
  354, missing `bl printf`) and has no additional failures; no expectations
  changed.

## Proof

- Ran `set -o pipefail; { cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R '^backend_(codegen_route|cli)_aarch64_'; } 2>&1
  | tee test_after.log`. Build succeeded; 33/34 tests passed, with only the
  baseline test 354 failure (`bl printf` missing). The delegated proof is
  sufficient relative to that baseline. Proof log: `test_after.log`.
