Status: Active
Source Idea Path: ideas/open/99_hir_module_symbol_structured_lookup_mirror.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Strengthen focused HIR proof if needed

# Current Packet

## Just Finished

Step 7: Strengthen focused HIR proof if needed. Added a focused HIR dump
fixture for namespace-qualified module function/global references through the
structured mirror proof surface. The CTest entry asserts the expected
namespace-qualified HIR shape and fails if the dump prints a
`module decl lookup parity mismatches` section.

## Suggested Next

Supervisor should review Step 7 and decide whether the active runbook is ready
for the Step 8 legacy-fallback demotion route or needs independent review first.

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
- Do not broaden into struct/type, member/method, parser, or unrelated sema
  rewrites.
- Do not downgrade expectations or add testcase-shaped shortcuts.

## Proof

Passed. Proof output is captured in `test_after.log`.

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L "^hir$"`
