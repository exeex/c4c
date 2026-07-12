# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate sibling AArch64 materializers

## Just Finished

- Plan Step 2 migrated the AArch64 scalar call-argument producer materializer
  in `calls.cpp` off its target-local prepared producer lookup reconstruction.
  It now consumes only traversal-attached common prepared producer lookups after
  validating owner/pointer identity; focused missing-authority and detached-
  pointer coverage verifies that absent or inconsistent authority fails closed.

## Suggested Next

- Continue Plan Step 2 with the next remaining AArch64 target-local prepared
  producer lookup consumer selected by the supervisor, keeping the packet to
  one materializer and its focused proof.

## Watchouts

- The scalar call-argument entry point returns no producer instructions and no
  diagnostic when traversal lookup authority is missing or detached, matching
  its existing fail-closed result contract.
- Other target-local lookup rebuilding remains in AArch64 dispatch, memory, and
  call materializers outside this packet.

## Proof

- Ran `set -o pipefail; { cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R
  '^(backend_aarch64_(call_boundary_owner|prepared_handoff_gate|prepared_register_conversion))$';
  } 2>&1 | tee test_after.log`. Build succeeded and all 3 delegated tests
  passed. The delegated proof is sufficient. Proof log: `test_after.log`.
