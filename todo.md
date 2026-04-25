Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Demote Parser-Owned Legacy String Paths

# Current Packet

## Just Finished

Step 8 - Demote Parser-Owned Legacy String Paths continued by auditing
template instantiation de-dup mirrors in
`src/frontend/parser/impl/types/template.cpp` and
`src/frontend/parser/impl/types/base.cpp`.

Template instantiation de-dup sync now treats structured
`TemplateInstantiationKey` state as authoritative when the key carries a valid
TextId: legacy-only rendered keys still increment mismatch counters, but no
longer promote/heal the structured de-dup set. Structured-present paths still
repopulate the rendered mirror for compatibility.

Direct template emission now preserves the rendered de-dup bridge only when no
structured key is available; when a structured key is available, rendered-only
state no longer suppresses concrete emission. Focused parser proof covers both
the pre-mark sync path and the direct-emission fallback path.

## Suggested Next

Next coherent packet: continue Step 8 by auditing parser-owned known-function
string lookup bridges in `src/frontend/parser/impl/core.cpp` and focused parser
tests; demote rendered fallback only where `QualifiedNameKey`/TextId lookup is
available, and preserve public string-facing lookup compatibility explicitly.

## Watchouts

- Template primary/specialization rendered maps are still populated as mirrors
  during registration; this packet demoted read fallback, not mirror writes.
- NTTP default rendered cache mirrors are still populated and mismatch-checked;
  only valid-TextId legacy-only fallback was demoted.
- Template instantiation de-dup rendered mirrors are still populated by mark
  paths and healed from structured-present state; legacy-only rendered state is
  now mismatch-observed but not promoted when a structured
  `TemplateInstantiationKey` exists.
- `lookup_value_in_context`, `lookup_using_value_alias`,
  `register_var_type_binding`, string-facing `find_var_type`, and
  `find_visible_var_type(kInvalidText, name)` remain compatibility bridges and
  should not be demoted without focused public-string and visible-lookup proof.
- Remaining parser string bridges include known-function string lookup.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
