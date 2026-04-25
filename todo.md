Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Demote Parser-Owned Legacy String Paths

# Current Packet

## Just Finished

Step 8 - Demote Parser-Owned Legacy String Paths continued by demoting the
direct qualified-key typedef/value bridges in
`Parser::find_typedef_type(QualifiedNameKey, string_view)` and
`Parser::find_var_type(QualifiedNameKey, string_view)`: after structured and
structured-rendered legacy lookup miss, the raw fallback spelling is now used
only when the key carries no valid `TextId`.

Focused parser coverage now proves valid direct qualified-key typedef/value
lookups prefer the TextId spelling over a mismatched fallback, valid but
unbound TextIds no longer promote `"typedefLookupBridge"` or
`"valueLookupBridge"` into a direct bridge hit, and invalid-key callers still
retain the compatibility fallback. No remaining valid-structured-identity raw
spelling promotion was found in the direct qualified-key typedef/value bridge
overloads.

## Suggested Next

Next coherent packet: continue Step 8 by auditing the remaining parser string
bridges called out below, starting with the namespace visible
`lookup_type_in_context`/`lookup_value_in_context` fallback paths only if a
focused structured/TextId proof can distinguish compatibility fallback from
valid-identity raw spelling promotion.

## Watchouts

- Public string overloads remain preserved, and direct qualified-key callers
  with an invalid key still use the raw fallback compatibility path.
- Remaining parser string bridges include namespace visible type/value fallback
  lookup, structured value legacy mirrors, template primary/specialization
  rendered-name maps, NTTP default cache mirrors, template instantiation de-dup
  mirrors, known-function string lookup, and using-alias compatibility
  spellings. Do not demote any of these without matching structured/TextId
  proof.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
