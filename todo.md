Status: Active
Source Idea Path: ideas/open/99_hir_module_symbol_structured_lookup_mirror.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Add targeted lookup proof/instrumentation

# Current Packet

## Just Finished

Step 8: Add targeted lookup proof/instrumentation. Added module declaration
lookup hit instrumentation that records resolver hits by authority without
changing precedence: structured, legacy rendered, concrete global-ID, and
`LinkNameId`. The HIR dump now prints focused lookup-hit evidence and annotates
qualified module refs with concrete/link authority, and a frontend-only HIR
lookup unit test now directly exercises structured, legacy-rendered,
concrete global-ID, and `LinkNameId` resolver hits while preserving the
existing parity mismatch section and rendered fallback behavior.

## Suggested Next

Supervisor review/route decision for the next coherent packet after Step 8.
Do not start fallback demotion unless the supervisor explicitly delegates the
Step 9 demotion slice.

## Watchouts

- Preserve rendered-name maps and link-name behavior.
- Hoisted compound-literal globals still have no declaration metadata and keep
  the default invalid `name_text_id`, so they update only the rendered global
  map through `Module::index_global_decl`.
- Structured lookup currently requires complete text-id metadata for all
  qualifier segments; refs with incomplete metadata intentionally fall back to
  legacy rendered-name lookup.
- Parity mismatch visibility is HIR-dump scoped and only emits when a resolver
  call has observed a structured/rendered disagreement.
- The new focused case proves no parity-mismatch report is emitted for the
  namespace-qualified fixture; it does not alter implementation behavior.
- Do not demote rendered fallback during Step 8; this step is evidence
  gathering only.
- The reviewer specifically noted that resolver parity currently runs after
  concrete-ID and `LinkNameId` bridge paths, so targeted proof must make those
  paths distinguishable from structured and legacy rendered hits.
- Step 8 now explicitly proves legacy-rendered lookup hits with a direct
  frontend-only HIR `Module` resolver test, alongside structured,
  concrete global-ID, and `LinkNameId` hit coverage.
- Do not broaden into struct/type, member/method, parser, or unrelated sema
  rewrites.
- Do not downgrade expectations or add testcase-shaped shortcuts.

## Proof

Passed. Proof output is captured in `test_after.log`.

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L "^hir$"`
