Status: Active
Source Idea Path: ideas/open/99_hir_module_symbol_structured_lookup_mirror.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Add parity checks during migration

# Current Packet

## Just Finished

Step 6: Add parity checks during migration. Added structured-vs-rendered
function/global lookup parity checks in the module declaration resolution paths.
Mismatches are recorded in HIR state, printed by the focused HIR dump surface,
and resolve to the legacy rendered declaration during the proof window.

## Suggested Next

Execute Step 7 from `plan.md`: strengthen focused HIR proof only if the
supervisor wants additional coverage beyond the green `hir` label proof.

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
- Do not broaden into struct/type, member/method, parser, or unrelated sema
  rewrites.
- Do not downgrade expectations or add testcase-shaped shortcuts.

## Proof

Passed. Proof output is captured in `test_after.log`.

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L "^hir$"`
