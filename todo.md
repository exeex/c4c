Status: Active
Source Idea Path: ideas/open/131_aarch64_dispatch_edge_copy_helper_surface_privatization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Privatize File-Local Helpers

# Current Packet

## Just Finished

Completed plan Step 2: Privatize File-Local Helpers.

`dispatch_edge_copies.hpp` no longer declares `EdgeProducerContext`,
`edge_value_publication_may_read_register_index`,
`emit_edge_load_local_to_register`, or
`emit_edge_value_publication_to_register`. The two dispatch-orchestration hooks,
`should_emit_block_entry_edge_copy_move` and
`lower_predecessor_select_parallel_copy_sources`, remain public and unchanged.

`dispatch_edge_copies.cpp` now keeps `EdgeProducerContext` and the helper
definitions in file-local anonymous-namespace scope. No behavior logic was
rewritten.

`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` no longer
directly references the four privatized helpers. The affected edge-publication
tests now validate the same emitted behavior through the retained public
`lower_predecessor_select_parallel_copy_sources` hook, with prepared producer
facts attached to the public lookup data where the old helper tests previously
passed private publication context by hand.

## Suggested Next

Proceed to Step 3 to evaluate whether the two retained dispatch-orchestration
hooks can be folded into `dispatch.cpp` or should remain public.

## Watchouts

- The four helper declarations should stay out of
  `dispatch_edge_copies.hpp`; tests now cover their behavior through retained
  public dispatch-edge-copy behavior.
- Preserve edge-copy instruction order, predecessor select parallel-copy
  behavior, and block-entry publication behavior.
- Do not replace prepared edge-copy or publication facts with local
  rediscovery.
- Do not move AArch64 register hazard policy or final instruction spelling into
  shared code.
- The two dispatch-only hooks are real cross-file calls today because
  `dispatch_prepared_block` uses them. Do not remove them from the header until
  either their implementations move to `dispatch.cpp` or replacement ownership
  is proven clean.
- Several files include `dispatch_edge_copies.hpp` without using the audited
  declarations directly; Step 4 should clean stale includes after the public
  surface contraction is known.

## Proof

Ran the exact delegated proof:

`bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_'" > test_after.log 2>&1`

Result: passed. `cmake --build --preset default` completed and the
`^backend_aarch64_` CTest subset reported 28/28 tests passed.

Supervisor regression guard: strict mode rejected equal pass count, then
maintenance non-decreasing mode passed with before=28/28 and after=28/28, no
new failures.

Proof log path: `test_after.log`.
