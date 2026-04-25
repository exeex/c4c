Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Demote Parser-Owned Legacy String Paths

# Current Packet

## Just Finished

Step 8 - Demote Parser-Owned Legacy String Paths began with one conservative
demotion in `Parser::find_visible_typedef_type(TextId, string_view)`: after the
TextId/local and TextId typedef lookup paths miss, the raw spelling retry now
runs only when the caller has no valid TextId. This demotes the parser-owned
visible typedef spelling fallback from a primary lookup key to a
compatibility-only path for TextId-less callers.

The demotion is safe because existing Step 2-7 parser coverage already proves
ID-first typedef registration and visible typedef lookup prefer the TextId
spelling over mismatched fallback spelling. This packet added a focused stale
TextId case proving a valid but unbound TextId no longer promotes the fallback
spelling `"typedefLookupBridge"` into a visible typedef hit.

## Suggested Next

Next coherent packet: continue Step 8 by auditing the sibling visible value
lookup bridge (`Parser::find_visible_var_type(TextId, string_view)`) for the
same TextId-valid/string-fallback demotion, but only if equivalent mismatch
coverage is already stable or can be added without weakening compatibility
tests.

## Watchouts

- Public string overloads remain preserved: `find_visible_typedef_type(string)`
  still enters with the parser text id for that spelling when available, and
  spelling-only callers with no TextId still use the compatibility path.
- Remaining parser string bridges include visible value lookup, direct
  qualified-key typedef/value fallback lookup, structured value legacy mirrors,
  template primary/specialization rendered-name maps, NTTP default cache mirrors,
  template instantiation de-dup mirrors, known-function string lookup, and
  using-alias compatibility spellings. Do not demote any of these without
  matching structured/TextId proof.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
