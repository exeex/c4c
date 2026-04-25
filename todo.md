Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Demote Parser-Owned Legacy String Paths

# Current Packet

## Just Finished

Step 8 - Demote Parser-Owned Legacy String Paths continued with the sibling
demotion in `Parser::find_visible_var_type(TextId, string_view)`: after the
TextId/local and TextId value lookup paths miss, the raw spelling retry now
runs only when the caller has no valid TextId. This demotes the parser-owned
visible value spelling fallback from a primary lookup key to a
compatibility-only path for TextId-less callers.

The demotion is safe because existing Step 2-7 parser coverage already proves
ID-first value registration and visible value lookup prefer the TextId spelling
over mismatched fallback spelling. This packet added a focused stale TextId
case proving a valid but unbound TextId no longer promotes the fallback
spelling `"valueLookupBridge"` into a visible value hit, while preserving the
public string visible value overload for compatibility.

## Suggested Next

Next coherent packet: continue Step 8 by auditing direct qualified-key
typedef/value fallback lookup bridges for any remaining valid-TextId raw
spelling promotion, but only with matching structured/TextId proof and without
weakening public string overload compatibility.

## Watchouts

- Public string overloads remain preserved: `find_visible_typedef_type(string)`
  and `find_visible_var_type(string)` still enter with the parser text id for
  that spelling when available, and spelling-only callers with no TextId still
  use the compatibility path.
- Remaining parser string bridges include direct qualified-key typedef/value
  fallback lookup, structured value legacy mirrors, template
  primary/specialization rendered-name maps, NTTP default cache mirrors,
  template instantiation de-dup mirrors, known-function string lookup, and
  using-alias compatibility spellings. Do not demote any of these without
  matching structured/TextId proof.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
