Status: Active
Source Idea Path: ideas/open/198_parser_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Audit Template Instantiated Record Lookup

# Current Packet

## Just Finished

Step 5 fixed the direct template-emission existing-instantiation regression
from commit `12589ce3e` in `src/frontend/parser/impl/types/base.cpp`.

The parser now distinguishes a complete structured direct-emission
instantiation key from a key that has already been marked. Rendered
`definition_state_.struct_tag_def_map[mangled]` reuse is permitted only when
there is no complete structured instantiation key. If a complete key exists
but has not yet been marked, the route scans structured records and then emits
a fresh concrete record instead of recovering through rendered tag spelling.

The direct-emission parser test now clears the mark, removes the structured
record, poisons the rendered tag map, and verifies that an unmarked complete
structured key does not reuse the stale rendered record.

## Suggested Next

Continue Step 5 with the remaining parser-owned template instantiated record
lookup surfaces in `src/frontend/parser/impl/types/base.cpp`. Classify each
retained rendered map lookup as structured-key authority, legacy/no-carrier
compatibility, or closure-ledger debt, then consider a Step 5 route review once
the remaining rendered `struct_tag_def_map` reads are accounted for.

## Watchouts

- Keep the work parser-owned; do not expand into Sema, HIR, LIR, BIR, or
  backend cleanup.
- Do not downgrade supported tests or replace semantic fixes with expectation
  rewrites.
- Do not treat parser diagnostics, AST display strings, source spelling, or
  final output text as semantic identity.
- Do not treat a raw `TextId` alone as complete semantic identity across
  scopes.
- Newly retained parser bridges need `legacy` or `deprecated` comments with
  owner, limitation, and removal condition.
- Step 5 owns template instantiated record lookup only. Do not drift back into
  record layout, owner recovery, named-const identity, or backend consteval
  work unless a separate lifecycle packet says so.
- Existing HIR/Sema template callers may still lack parser structured carriers.
  Preserve those as explicit legacy/no-metadata boundaries unless the packet
  proves parser-owned metadata is complete.
- Do not treat `template_origin_name` or rendered candidate keys as semantic
  authority when structured instantiated-record metadata is available.
- The retained rendered `base_mangled` lookups around base materialization and
  direct-emission reuse are intended only for no-carrier compatibility. Do not
  move them back into metadata-rich paths.
- Step 3 reviewer watch item: retained no-metadata/template-instantiation
  fallbacks in `base.cpp` should be accounted for in Step 5 and the Step 6
  closure ledger.

## Proof

Step 5 focused proof passed:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_parser_tests|frontend_parser_lookup_authority_tests)$"' > test_after.log 2>&1`

Proof log: `test_after.log`.
