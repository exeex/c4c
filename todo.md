# Current Packet

Status: Active
Source Idea Path: ideas/open/81_aarch64_dispatch_edge_copy_owner_contraction.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Relocate narrow target-local emission helpers

## Just Finished

Completed `plan.md` Step 3 by moving only the same-width I32 typed
stack-source publication carrier and `emit_same_width_i32_stack_source_publication`
from the AArch64 edge-copy implementation into the memory owner.

Concrete changes:

- Removed the private `PreparedTypedStackSourceEdgeEmission` carrier and helper
  definition from `dispatch_edge_copies.cpp`.
- Added owner-neutral `PreparedTypedStackSourcePublicationEmission` and the
  same helper declaration to `memory.hpp`.
- Added the same helper definition to `memory.cpp`, preserving the logic.
- Kept `lower_predecessor_join_source_publication` calling
  `emit_same_width_i32_stack_source_publication` through the existing
  `memory.hpp` include.

## Suggested Next

Supervisor should choose the next Step 3 contraction packet or ask the plan
owner whether Step 3 is exhausted if no narrow target-local helper remains.

## Watchouts

- The relocated memory helper still depends on scalar register view conversion,
  scalar load width selection, and frame-slot address formatting that were
  already exposed by memory/codegen helpers.
- No edge publication selection, fallback producer materialization, or test
  expectation logic changed.

## Proof

Passed:

`bash -o pipefail -c "(cmake --build build --target c4c_backend backend_aarch64_instruction_dispatch_test c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_instruction_dispatch' --output-on-failure) 2>&1 | tee test_after.log"`

Proof log: `test_after.log`.
