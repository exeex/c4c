Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Tighten `last_resolved_typedef` Around `TextId`

# Current Packet

## Just Finished

Step 2 code packet completed for the qualified-type tentative parse rollback in
`parser_expressions.cpp`.

The local rollback now treats `last_resolved_typedef_text_id` as the
authoritative saved identity: it clears typedef state, rehydrates spelling
through `parser_text(saved_typedef_text_id, saved_typedef_fallback)`, and
restores through `set_last_resolved_typedef(...)`. The saved string mirror is
kept only as fallback text recovery when no valid `TextId` is available.

## Suggested Next

Continue Step 2 with a narrow audit packet for the remaining
`last_resolved_typedef` call sites in `parser_types_base.cpp` and
`parser_declarations.cpp`, confirming that semantic lookup and metadata handoff
stay on `last_resolved_typedef_text_id` and that any spelling use is fallback
only.

## Watchouts

- This packet intentionally did not change alias-template storage,
  template-struct lookup, or typedef visibility table semantics.
- `set_last_resolved_typedef(...)` recomputes the stored `TextId` from the
  recovered spelling, matching the existing tentative snapshot restore model in
  `parser_support.cpp`.
- `current_struct_tag` remains the higher-risk family because several paths
  still use spelling for owner registration and injected/member contexts.

## Proof

Delegated proof passed:
`bash -lc 'set -o pipefail; cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '\''^frontend_parser_tests$'\'' | tee test_after.log'`

Additional local validation passed:
`git diff --check`

Proof log path: `test_after.log`.
