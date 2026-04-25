Status: Active
Source Idea Path: ideas/open/99_hir_module_symbol_structured_lookup_mirror.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Dual-write module structured mirrors

# Current Packet

## Just Finished

Step 4: Dual-write module structured mirrors. Added passive structured module
function/global mirror maps keyed by `ModuleDeclLookupKey`, dual-wrote the
owned function/global rendered-map insertion sites through shared module
indexing helpers when `name_text_id` is valid, and kept all lookup reads on the
existing rendered maps/link-name paths. Filtered `c4cll` modules now clear and
rebuild both structured mirrors for retained functions/globals. Hoisted
compound-literal globals now route through the same module global indexing
helper while keeping their `name_text_id` invalid and preserving direct
`DeclRef` global ids.

## Suggested Next

Execute Step 5 from `plan.md`: add structured `DeclRef` lookup helpers while
preserving concrete-id, link-name, and legacy rendered-map lookup precedence.

## Watchouts

- Preserve rendered-name maps and link-name behavior.
- Hoisted compound-literal globals still have no declaration metadata and keep
  the default invalid `name_text_id`, so they update only the rendered global
  map through `Module::index_global_decl`.
- Treat `fn_structured_index` and `global_structured_index` as passive metadata
  until Step 5 explicitly reads them.
- Do not broaden into struct/type, member/method, parser, or unrelated sema
  rewrites.
- Do not downgrade expectations or add testcase-shaped shortcuts.

## Proof

Passed. Proof output is captured in `test_after.log`.

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|cpp_hir_template_global_specialization|cpp_hir_if_constexpr_branch_unlocks_later|cpp_hir_multistage_shape_chain)$"`
