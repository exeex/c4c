Status: Active
Source Idea Path: ideas/open/131_aarch64_dispatch_edge_copy_helper_surface_privatization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fold Dispatch-Orchestration Helpers If Clear

# Current Packet

## Just Finished

Completed plan Step 3: Fold Dispatch-Orchestration Helpers If Clear.

No code move was made. The two retained hooks should stay in
`dispatch_edge_copies.cpp` for this slice:

- `should_emit_block_entry_edge_copy_move` is called only from `dispatch.cpp`
  and tests, but its decision depends on edge-copy internals that are already
  private in `dispatch_edge_copies.cpp`: current join-publication clobber
  checks, prepared edge-publication source facts, producer-context recovery,
  and recursive register-read analysis through
  `edge_value_publication_may_read_register_index`.
- `lower_predecessor_select_parallel_copy_sources` is also called only from
  `dispatch.cpp` and tests, but its ownership is the edge-copy materialization
  path: it validates the block-entry parallel-copy bundle, derives prepared
  edge source facts, and delegates into
  `lower_predecessor_join_source_publication`, which consumes the private
  producer lookup/materialization helpers.
- Moving either hook into `dispatch.cpp` would not make the dispatch loop
  clearer unless the private producer/materialization helpers moved with it.
  Moving that helper cluster would undo the Step 2 surface contraction or split
  edge-copy policy across files, while wrapper-only relocation would leave a
  public/private bridge with no ownership gain.
- The current public seam remains useful for focused backend tests that verify
  predecessor select publication and block-entry edge-copy suppression without
  re-exposing the four already-private helpers.

## Suggested Next

Proceed to Step 4 to clean stale `dispatch_edge_copies.hpp` includes after the
public surface contraction, if the supervisor wants the follow-up packet.

## Watchouts

- The four helper declarations should stay out of `dispatch_edge_copies.hpp`.
- Do not move the retained hooks unless a later design moves the whole
  edge-copy producer/materialization ownership boundary, not just the wrapper
  functions.
- Several files include `dispatch_edge_copies.hpp` without using the retained
  declarations directly; Step 4 can remove stale includes after this no-move
  decision.

## Proof

Ran the exact delegated proof:

`bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_'" > test_after.log 2>&1`

Result: passed. `cmake --build --preset default` completed with no rebuild
needed, and the `^backend_aarch64_` CTest subset reported 28/28 tests passed.

Supervisor regression guard: maintenance non-decreasing mode passed with
before=28/28 and after=28/28, no new failures.

Proof log path: `test_after.log`.
