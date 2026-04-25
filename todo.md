Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Demote Parser-Owned Legacy String Paths

# Current Packet

## Just Finished

Step 8 - Demote Parser-Owned Legacy String Paths continued by auditing
structured value legacy mirrors around `register_var_type_binding`,
`register_structured_var_type_binding`, `find_var_type`, and
`find_visible_var_type`.

`register_structured_var_type_binding` no longer writes a legacy value-cache
mirror, and direct qualified-key `find_var_type` no longer promotes rendered
legacy-only cache entries when the caller supplies a valid structured key.
The invalid-key fallback remains as the explicit TextId-less compatibility
bridge. String-facing `has_var_type`/`find_var_type` now bridge to structured
value storage directly, so downstream string overloads still recover direct
structured registrations without requiring a legacy mirror.

`register_var_type_binding` keeps its legacy cache write because the public
string overload and parser-owned TextId registration path are still the
compatibility bridge for legacy cache consumers. `find_visible_var_type` keeps
its TextId-less string fallback because focused proof still requires
`kInvalidText` visible lookup compatibility for legacy-only cache entries.

## Suggested Next

Next coherent packet: continue Step 8 by auditing using-value alias and
namespace value lookup bridge probes around `lookup_using_value_alias`,
`lookup_value_in_context`, and `resolve_qualified_value`; demote
`has_var_type(spelling)`/`has_var_type(candidate)` probes only where focused
using-alias and namespace-import proof preserves explicit compatibility-name
and TextId-less behavior.

## Watchouts

- Public string overloads remain preserved through string-facing structured
  lookup plus the existing `register_var_type_binding` legacy cache write.
- Direct qualified-key value lookup now intentionally rejects legacy-only
  rendered cache hits for valid keys; invalid-key fallback still preserves
  TextId-less compatibility.
- `find_visible_var_type(kInvalidText, name)` still accepts legacy-only cache
  entries and should not be demoted without focused visible-lookup proof.
- Remaining parser string bridges include structured value legacy mirrors,
  template primary/specialization rendered-name maps, NTTP default cache
  mirrors, template instantiation de-dup mirrors, known-function string lookup,
  and using-alias compatibility spellings. Do not demote any of these without
  matching structured/TextId proof.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
