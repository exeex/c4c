# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and migrate AArch64 dispatch authority

## Just Finished

- Plan Step 1 migrated the AArch64 current-block entry publication consumer in
  `dispatch_publication.cpp` to the common named/prepared publication identity.
  The consumer no longer creates or classifies Route4-labelled executable
  claims, checks the common identity against the prepared destination home,
  and fails closed when proof authority is missing or inconsistent.

## Suggested Next

- Continue Plan Step 1 with the next supervisor-selected AArch64 dispatch
  authority family, keeping comparison, call-boundary, and ALU families out of
  this packet.

## Watchouts

- The separate comparison, call-boundary, and ALU route families were not
  changed by this packet.
- The focused dispatch test now verifies that missing proof authority and
  mismatched proof name/type do not fall back to prepared publication payloads.
- The focused baseline retains the known scalar-FP literal-add failure (test
  354, missing `bl printf`) and has no additional failures; no route
  expectations were weakened.

## Proof

- Ran `set -o pipefail; { cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R '^backend_(codegen_route|cli)_aarch64_'; } 2>&1
  | tee test_after.log`. Build succeeded; 33/34 tests passed, with only the
  baseline test 354 failure (`bl printf` missing). The delegated proof is
  sufficient relative to that baseline. Proof log: `test_after.log`.
