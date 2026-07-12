# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and migrate AArch64 dispatch authority

## Just Finished

- Plan Step 1 migrated the AArch64 indirect-callee stored-value family in
  `calls.cpp` to direct prepared authority. The prepared lookup now validates
  current-block identity, store/load instruction bounds and ordering, prepared
  access indices, instruction kinds, and exact loaded/stored values itself;
  the Route3 agreement helper and `Route3Identity` discriminator were retired,
  and missing or inconsistent authority fails closed.

## Suggested Next

- Continue Plan Step 1 with the next supervisor-selected AArch64 dispatch
  authority family, keeping the separate call-boundary Route4 family out of
  this packet.

## Watchouts

- The separate call-boundary Route4 family was not changed by this packet.
- The focused baseline retains the known scalar-FP literal-add failure (test
  354, missing `bl printf`) and has no additional failures; no expectations
  changed.

## Proof

- Ran `set -o pipefail; { cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R '^backend_(codegen_route|cli)_aarch64_'; } 2>&1
  | tee test_after.log`. Build succeeded; 33/34 tests passed, with only the
  baseline test 354 failure (`bl printf` missing). The delegated proof is
  sufficient relative to that baseline. Proof log: `test_after.log`.
