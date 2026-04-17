Status: Active
Source Idea: ideas/open/15_parser_and_hir_text_id_convergence.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 2 / Step 4 for the bounded parser
`last_using_alias_name_` route in template using-alias bookkeeping and
`frontend_parser_tests.cpp`.

- Added parser-side `last_using_alias_name_text_id_` storage and helper accessors
  so the alias-template wrapper no longer depends on raw string ownership alone
  for that persistent parser state.
- Switched template using-alias metadata capture to resolve the alias name
  through the new `TextId` carrier before typedef lookup and alias-template map
  insertion.
- Added focused parser proof that corrupting the legacy raw alias string does
  not break the visible alias spelling while the `TextId` carrier is present.

## Suggested Next

Use the same bounded pattern on one remaining parser or HIR persistent text
carrier with live consumers, such as `current_namespace_` or a struct-field
name path, rather than widening into registry churn.

## Watchouts

- Do not collapse `TextId`, `SymbolId`, and `LinkNameId`.
- HIR must intern its own spellings into `module.link_name_texts`; parser-owned
  `TextId`s cannot be copied directly into HIR because they belong to a
  different table.
- `last_using_alias_name_text_id_` must stay aligned anywhere template using
  aliases clear, overwrite, or read the parser-side bookkeeping slot.
- Parser-side `TextId` tests should keep proving the route still works after the
  legacy string sidecar is intentionally corrupted, so the migration is not
  passive storage only.
- Keep diagnostic/debug/serialization strings out of scope.
- Do not absorb unrelated EASTL lifecycle churn into this plan.

## Proof

Baseline:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`

Packet proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`

Result:
Passed on 2026-04-17 via `test_after.log`.
