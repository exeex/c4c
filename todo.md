Status: Active
Source Idea Path: ideas/open/99_hir_module_symbol_structured_lookup_mirror.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add declaration-side name text metadata

# Current Packet

## Just Finished

Step 2: Add declaration-side name text metadata. Added passive
declaration-side `name_text_id` fields to HIR `Function` and `GlobalVar`,
defaulting to `kInvalidText`. Existing rendered `name` fields, link-name ids,
lookup maps, and lowering behavior are unchanged.

## Suggested Next

Execute Step 3 from `plan.md`: populate the passive HIR declaration
`name_text_id` metadata at semantic declaration-lowering sites without changing
rendered names or lookup behavior.

## Watchouts

- Preserve rendered-name maps and link-name behavior.
- Step 2 intentionally did not populate the new fields; they remain invalid
  unless a later packet explicitly assigns declaration text ids.
- Treat `ModuleDeclLookupKey` and `name_text_id` as passive metadata until the
  mirror-map wiring packet explicitly uses them.
- Do not broaden into struct/type, member/method, parser, or unrelated sema
  rewrites.
- Do not downgrade expectations or add testcase-shaped shortcuts.

## Proof

Passed. Proof output is captured in `test_after.log`.

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|cpp_hir_template_global_specialization|cpp_hir_if_constexpr_branch_unlocks_later|cpp_hir_multistage_shape_chain)$"`
