# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and migrate AArch64 dispatch authority

## Just Finished

- Plan Step 1 migrated the AArch64 dispatch publication-source producer family
  in `dispatch_producers.cpp` to the common named/prepared same-block producer
  handoff. The consumer no longer builds or queries a Route1 producer index;
  the Route1-specific public view was retired, coherent owned prepared lookups
  drive recursive dependency checks directly, and missing or inconsistent
  lookup authority fails closed.

## Suggested Next

- Continue Plan Step 1 with the next supervisor-selected AArch64 dispatch
  authority family, keeping comparison, call-boundary, and ALU families out of
  this packet.

## Watchouts

- The separate comparison, call-boundary, and ALU route families were not
  changed by this packet.
- The focused dispatch test now verifies that missing prepared producer records
  do not fall back to Route1 facts.
- The focused baseline retains the known scalar-FP literal-add failure (test
  354, missing `bl printf`) and has no additional failures; no expectations
  changed.

## Proof

- Ran `set -o pipefail; { cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R '^backend_(codegen_route|cli)_aarch64_'; } 2>&1
  | tee test_after.log`. Build succeeded; 33/34 tests passed, with only the
  baseline test 354 failure (`bl printf` missing). The delegated proof is
  sufficient relative to that baseline. Proof log: `test_after.log`.
