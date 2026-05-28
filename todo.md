Status: Active
Source Idea Path: ideas/open/66_aarch64_local_dispatch_block_route.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Preserve Hook Wiring And Sequencing Boundaries

# Current Packet

## Just Finished

Step 4 completed: preserved and documented AArch64 dispatch hook wiring and
sequencing boundaries without implementation edits.

Sequencing boundary decision:
- `make_dispatch_branch_fusion_hooks` remains route-local hook wiring. It
  binds precise owner callbacks into comparison lowering while keeping the
  branch-fusion sequence visible in `dispatch.cpp`.
- Before-instruction move ordering remains local: before-instruction moves are
  considered only for binary instructions, after current-block join clobber
  filtering, prepared-stack-source filtering, and stack-home fused-compare
  suppression.
- Prepared-memory retry routing remains local: retained and prepared memory
  indices are compared in the main dispatch loop, `memory_lowering_index` is
  chosen locally, and retry is attempted only when the prepared index matches.
- Before-return publication sequencing remains local: before-return moves are
  emitted after atomic lowering and before return terminator lowering, with
  duplicate ABI publications filtered by scalar state.
- Terminator emission remains local: branch edge source publications precede
  branch terminator emission, conditional branch fusion attempts run before
  fallback condition publication, and final successors are assigned in the same
  route block.
- Final instruction append order remains local and explicit through direct
  `block.instructions.push_back` sequencing and final
  `result.emitted_instructions = block.instructions.size()`.
- No behavior-preserving narrowing was clearer than leaving this ordering
  visible in `dispatch.cpp`; no hook sequencing, retry behavior,
  before-return ordering, terminator ordering, or final append order changed.

## Suggested Next

Step 5 should remove residual semantic drift from block dispatch. Suggested
packet: scan `dispatch.cpp` for producer, publication, value-materialization,
prepared-query, or target-emission authority that should live in precise
owners; keep route-level orchestration local, but reject any helper that
rebuilds semantic facts or target spelling locally.

## Watchouts

This plan preserves the local AArch64 block route. Do not move block traversal
or target diagnostics to shared code, do not reintroduce producer/publication
or value-materialization authority into `dispatch.cpp`, and do not bundle idea
67's local-slot offset probe into this route.

Proof coverage needed for Step 5: scans for semantic producer/publication and
value-materialization authority in `dispatch.cpp`, representative direct
`dispatch_prepared_block` tests, production traversal through `traversal.cpp`,
branch-fusion hooks, before-return publication, prepared-memory retry routing,
and current-block join suppression.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_|backend_cli_aarch64_)') > test_after.log 2>&1`

`test_after.log` reports 32/32 tests passed. `git diff --check` passed.
Targeted `rg` scan recorded hook wiring, before-instruction move ordering,
prepared-memory retry routing, before-return publication sequencing, terminator
emission, and final `result.emitted_instructions` assignment in `dispatch.cpp`.
