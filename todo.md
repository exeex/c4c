# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- The Plan Step 3 accumulated audit found and remediated six unclassified
  verified Raw-BIR lookup/uniqueness indexes: link and struct spelling,
  typed struct declaration, string data, external declaration, and global
  object indexes. Each now has exact guard classification and code-adjacent
  evidence of its primary typed/order/identity authority and bidirectional
  `FoundationVerifier` checks.
- This is string-authority guard hygiene only, not container capability
  progress and not completion of a semantic family.

## Suggested Next

- Resume the producer-valid Plan Step 3 global-family audit and checkpoint
  decision. Treat any uncovered producer family as a bounded Step 3 packet
  rather than declaring the checkpoint complete from this guard-only cleanup.

## Watchouts

- The spelling/name maps remain verified secondary indexes. The typed IDs,
  ordered tables, and explicit `LinkNameId`/fallback identity variants remain
  primary; no runtime behavior or semantic acceptance changed.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^string_authority_guard$' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records the guard passing
  1/1; JSON validation and `git diff --check` also passed.
