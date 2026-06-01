# Current Packet

Status: Active
Source Idea Path: ideas/open/81_aarch64_dispatch_edge_copy_owner_contraction.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Contract the remaining edge-copy owner surface

## Just Finished

Step 4 completed the remaining edge-copy owner surface contraction:

- Removed the unused
  `prepared_edge_select_source_is_destination_register` declaration and
  definition after confirming it has no call sites.
- Removed `lower_predecessor_join_source_publication` from
  `dispatch_edge_copies.hpp` and kept its implementation internal to
  `dispatch_edge_copies.cpp`.
- Removed stale local includes made unnecessary by the contracted header and
  edge-copy implementation surface.

## Suggested Next

Supervisor should review whether the active runbook is now exhausted and route
the next lifecycle decision through the plan owner if closure, retirement, or a
replacement runbook is needed.

## Watchouts

- `should_emit_block_entry_edge_copy_move` and
  `lower_predecessor_select_parallel_copy_sources` remain public edge-copy
  hooks with external users; this packet did not fold them into `dispatch.cpp`.
- No behavior or test expectations were changed.
- Do not pull dispatch-publication, printer, instruction-record, x86, or
  RISC-V cleanup into this route.

## Proof

Latest Step 4 surface-contraction proof passed:

`bash -o pipefail -c "(cmake --build build --target c4c_backend backend_aarch64_instruction_dispatch_test c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_instruction_dispatch' --output-on-failure) 2>&1 | tee test_after.log"`

Proof log: `test_after.log`.
