# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and migrate AArch64 dispatch authority

## Just Finished

- Plan Step 1 migrated the remaining AArch64 scalar call-argument
  source-producer family to common prepared authority. Call lowering no longer
  reconstructs or consults Route6 call-source/producer records; same-block
  producer identity, uniqueness, instruction agreement, and binary
  materializability now come exclusively from the prepared queries and their
  fail-closed contract verification.

## Suggested Next

- Inventory the next AArch64 dispatch family in Plan Step 1 and migrate the
  next coherent Route3/Route4/Route6 authority slice that already has common
  prepared coverage.

## Watchouts

- The direct-global select-chain call-argument API still accepts a legacy
  Route6 index parameter in `select_materialization.*`; `calls.cpp` now passes
  null and does not build the index. Removing that dead interface parameter is
  outside this packet's owned files.
- The focused baseline retains the known scalar-FP literal-add failure (test
  354, missing `bl printf`) and has no additional failures; no expectations
  changed.

## Proof

- Ran `set -o pipefail; { cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R '^backend_(codegen_route|cli)_aarch64_'; } 2>&1
  | tee test_after.log`. Build succeeded; 33/34 tests passed, with only the
  baseline test 354 failure (`bl printf` missing). The delegated proof is
  sufficient relative to that baseline. Proof log: `test_after.log`.
