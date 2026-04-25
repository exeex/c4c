Status: Active
Source Idea Path: ideas/open/99_hir_module_symbol_structured_lookup_mirror.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add structured DeclRef lookup helpers

# Current Packet

## Just Finished

Step 5: Add structured `DeclRef` lookup helpers. Added module helpers to form
structured function/global lookup keys from `DeclRef::name_text_id` and
namespace qualifier metadata when the text-id metadata is complete. Updated
`resolve_function_decl` and `resolve_global_decl` to preserve local/param and
direct global handling, keep `LinkNameId` lookup before structured lookup, and
fall back to the legacy rendered-name maps after structured lookup misses.

## Suggested Next

Execute Step 6 from `plan.md`: run parity checks around structured
function/global lookup behavior without removing rendered maps or link-name
paths.

## Watchouts

- Preserve rendered-name maps and link-name behavior.
- Hoisted compound-literal globals still have no declaration metadata and keep
  the default invalid `name_text_id`, so they update only the rendered global
  map through `Module::index_global_decl`.
- Structured lookup currently requires complete text-id metadata for all
  qualifier segments; refs with incomplete metadata intentionally fall back to
  legacy rendered-name lookup.
- Do not broaden into struct/type, member/method, parser, or unrelated sema
  rewrites.
- Do not downgrade expectations or add testcase-shaped shortcuts.

## Proof

Passed. Proof output is captured in `test_after.log`.

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L "^hir$"`
