Status: Active
Source Idea Path: ideas/open/99_hir_module_symbol_structured_lookup_mirror.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Add targeted lookup proof/instrumentation

# Current Packet

## Just Finished

Step 7: Strengthen focused HIR proof if needed. Added a focused HIR dump
fixture for namespace-qualified module function/global references through the
structured mirror proof surface. The CTest entry asserts the expected
namespace-qualified HIR shape and fails if the dump prints a
`module decl lookup parity mismatches` section.

Route review in `review/99_step8_pre_demotion_route_review.md` found the
implementation on track through Step 7, but not enough evidence to demote
rendered fallback yet.

## Suggested Next

Execute Step 8 by adding targeted proof or instrumentation that distinguishes
structured hits, legacy rendered hits, concrete-ID hits, and `LinkNameId` hits
for module function/global references. Keep rendered fallback parked until
that evidence is green and supervisor approves any demotion.

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
- Do not broaden into struct/type, member/method, parser, or unrelated sema
  rewrites.
- Do not downgrade expectations or add testcase-shaped shortcuts.

## Proof

Passed. Proof output is captured in `test_after.log`.

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L "^hir$"`

No Step 8 proof has been run yet after the route review.
