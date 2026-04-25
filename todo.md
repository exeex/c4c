Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Demote Parser-Owned Legacy String Paths

# Current Packet

## Just Finished

Step 8 - Demote Parser-Owned Legacy String Paths continued by auditing
using-value alias and namespace value lookup bridge probes around
`lookup_using_value_alias`, `lookup_value_in_context`, and
`resolve_qualified_value`.

`lookup_using_value_alias` no longer probes `has_var_type(spelling)` as a
success gate after structured target checks; alias resolution still reports the
rendered target spelling or explicit compatibility name, and downstream
string-facing visible value lookup remains the compatibility bridge.

`lookup_value_in_context` no longer promotes rendered legacy-only value cache
entries for valid TextId namespace lookups. The early
`has_var_type(structured_candidate)` bridge was removed, and the remaining
rendered `has_var_type(candidate)` probe is now guarded to `kInvalidText`.
Focused tests preserve explicit TextId-less compatibility for direct namespace,
qualified value, and using-namespace import lookup while rejecting valid-TextId
legacy-only rendered-name promotion.

## Suggested Next

Next coherent packet: continue Step 8 by auditing remaining parser-owned
legacy string bridges in template value/type lookup, starting with template
primary/specialization rendered-name maps and NTTP default cache mirrors in
`src/frontend/parser/impl/types/template.cpp`; demote only where TextId-backed
structured template keys and explicit compatibility fallback proof both exist.

## Watchouts

- `lookup_value_in_context` still intentionally accepts rendered legacy value
  names when `name_text_id == kInvalidText`; this is the explicit
  TextId-less namespace compatibility bridge.
- `lookup_using_value_alias` still returns aliases whose target is not
  currently structured-resolvable when it can render a target spelling or has
  an explicit compatibility name; visible value lookup owns the final
  string-facing compatibility recovery.
- `register_var_type_binding`, string-facing `find_var_type`, and
  `find_visible_var_type(kInvalidText, name)` remain compatibility bridges and
  should not be demoted without focused public-string and visible-lookup proof.
- Remaining parser string bridges include template primary/specialization
  rendered-name maps, NTTP default cache mirrors, template instantiation de-dup
  mirrors, and known-function string lookup.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
