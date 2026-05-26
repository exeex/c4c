Status: Active
Source Idea Path: ideas/open/30_riscv_prepared_edge_publication_stack_destination_policy_broadening.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement or Preserve Fail-Closed Source-to-Stack Policy

# Current Packet

## Just Finished

Step 2 preserved the fail-closed RISC-V source-to-stack policy without changing
implementation behavior. Focused coverage in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` now asserts
that `RematerializableImmediate -> StackSlot`, `StackSlot -> StackSlot`, and
`PointerBasePlusOffset -> StackSlot` return
`UnsupportedDestinationHome` and render no instruction text unless an explicit
scratch-register policy exists. Existing `Register -> StackSlot` coverage
remains intact.

## Suggested Next

Proceed to the next supervisor-selected packet only after deciding whether the
active plan should introduce an explicit scratch-register policy or keep
source-to-stack broadening deferred.

## Watchouts

This packet intentionally did not add scratch semantics or broaden stack-source
register-destination or pointer-base register-destination behavior. The helper
still supports only `Register -> StackSlot` for concrete 4-byte stack
destinations.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$' > test_after.log 2>&1`.
Focused CTest passed 1/1 in `test_after.log`. The delegated proof is sufficient
for this Step 2 fail-closed policy coverage packet.
