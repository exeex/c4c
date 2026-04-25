Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Demote Parser-Owned Legacy String Paths

# Current Packet

## Just Finished

Step 8 - Demote Parser-Owned Legacy String Paths continued by auditing the
namespace-visible `lookup_type_in_context`/`lookup_value_in_context` fallback
paths. `lookup_value_in_context` no longer promotes the raw global fallback
spelling when the caller supplies a valid but unbound `TextId`; that raw
fallback is now reserved for TextId-less callers. `lookup_type_in_context`
keeps its bridge-required fallback behavior, but TextId-backed global typedef
hits now report the TextId spelling instead of echoing a mismatched raw
fallback.

Focused parser coverage now proves namespace-visible type/value lookups prefer
TextId spellings over mismatched fallbacks, valid but unbound TextIds do not
promote global or namespace-scoped fallback spellings, and TextId-less callers
still retain the compatibility fallback for both global and namespace-scoped
type/value lookups.

## Suggested Next

Next coherent packet: continue Step 8 by auditing the structured value legacy
mirrors around `register_var_type_binding`, `register_structured_var_type_binding`,
and `find_visible_var_type` to decide whether any legacy cache probes can be
demoted while preserving explicit string-overload/TextId-less compatibility.

## Watchouts

- Public string overloads remain preserved, direct qualified-key callers with
  an invalid key still use the raw fallback compatibility path, and
  namespace-visible type/value lookups with `kInvalidText` still use the
  compatibility bridge.
- Remaining parser string bridges include structured value legacy mirrors,
  template primary/specialization rendered-name maps, NTTP default cache
  mirrors, template instantiation de-dup mirrors, known-function string lookup,
  and using-alias compatibility spellings. Do not demote any of these without
  matching structured/TextId proof.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
