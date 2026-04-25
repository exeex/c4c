Status: Active
Source Idea Path: ideas/open/99_hir_module_symbol_structured_lookup_mirror.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Populate function/global name_text_id

# Current Packet

## Just Finished

Step 3: Populate function/global `name_text_id`. Populated passive
declaration-side `name_text_id` metadata for normal lowered functions,
bodyless callable registration inputs, consteval-only callable registration,
normal globals, static-local globals, and template function/global instances
using existing declaration spelling metadata. Rendered names, link-name ids,
and legacy lookup maps remain unchanged.

## Suggested Next

Execute Step 4 from `plan.md`: add the structured module lookup mirror maps
using the populated passive declaration metadata without changing legacy lookup
behavior.

## Watchouts

- Preserve rendered-name maps and link-name behavior.
- Hoisted compound-literal globals still have no declaration metadata and keep
  the default invalid `name_text_id`.
- Treat `ModuleDeclLookupKey` and `name_text_id` as passive metadata until the
  mirror-map wiring packet explicitly uses them.
- Do not broaden into struct/type, member/method, parser, or unrelated sema
  rewrites.
- Do not downgrade expectations or add testcase-shaped shortcuts.

## Proof

Passed. Proof output is captured in `test_after.log`.

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|cpp_hir_template_global_specialization|cpp_hir_if_constexpr_branch_unlocks_later|cpp_hir_multistage_shape_chain)$"`
