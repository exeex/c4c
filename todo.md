Status: Active
Source Idea Path: ideas/open/117_aarch64_comparison_fused_compare_publication_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Or Tighten Prepared Fact Visibility

# Current Packet

## Just Finished

Step 2 - Add Or Tighten Prepared Fact Visibility has a reviewable
current-block publication visibility surface.

Chosen surface:

- `--dump-prepared-bir` now prints `--- prepared-block-entry-publications ---`
  with resolved shared `prepare::collect_prepared_block_entry_publications`
  facts: successor label, availability status, destination value id/name,
  resolved destination home kind, destination role/storage, resolved register
  spelling, and source bundle position.
- `backend_cli_dump_prepared_bir_exposes_contract_sections` now requires the
  new section and a representative available `logic.end.4` publication line
  from `smoke_expr_branch_lifecycle.c`.

Why this is not target-local:

- The emitted fact is the target-neutral prepared publication contract before
  any AArch64 register parsing, scalar view selection, branch opcode choice, or
  emission fallback. It exposes the shared destination identity/home/status
  that comparison and dispatch consumers were previously forced to re-match
  locally.
- The dump visibility is shared prepared-printer output, not a
  `comparison.cpp` behavior move and not a named-case shortcut.

## Suggested Next

Delegate Step 3 to expose fused-compare operand producer facts as a reusable
branch-condition-level prepared query before moving ownership. Step 4 can then
replace `comparison.cpp`'s current-block publication iteration/duplicate
register helper with a shared prepared publication query or small shared
AArch64 dispatch-publication adapter that consumes the now-visible
`PreparedBlockEntryPublication` fact.

## Watchouts

- Do not migrate ownership by rescanning same-block BIR, matching raw
  terminators, matching BIR names, or adding a `comparison.cpp` named-case path.
- The next Step 3 packet should not move AArch64 condition suffix selection,
  scalar view selection, branch spelling, or branch emission into prepared
  code.
- Fused-compare producer pair facts are still only indirectly visible through
  existing branch-condition dump coverage; Step 3 should add or expose a
  branch-condition-level lhs/rhs producer contract.

## Proof

Passed. Exact delegated proof command:

`(cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_prepare_authoritative_join_ownership|backend_prealloc_block_entry_publications|backend_prepared_lookup_helper|backend_cli_dump_prepared_bir_exposes_contract_sections)$' --output-on-failure) > test_after.log 2>&1`

Result: `100% tests passed, 0 tests failed out of 5`; proof log is
`test_after.log`.
