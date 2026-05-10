Status: Active
Source Idea Path: ideas/open/160_sema_canonical_symbol_template_key_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Repair Canonical Symbol Identity And Hashing

# Current Packet

## Just Finished

Step 5 repaired canonical symbol identity equality and hashing in
`canonical_symbol.cpp`.

`CanonicalIdentity::operator==` and `CanonicalIdentityHash` now prefer complete
`name_identity` metadata over rendered `name`. A complete structured identity
miss does not fall back through matching rendered spelling, while the rendered
name path remains the intentional no-metadata compatibility bridge. C++ function
overload discrimination still delegates discriminator type comparison through
`types_equal`.

The focused metadata test now proves same-spelled symbols in distinct
structured domains differ in equality and hash, stay separate in
`CanonicalSymbolTable` lookup, matching complete identities ignore stale
rendered spelling, and no-metadata spelling fallback remains intentional.

## Suggested Next

Move the next supervisor-selected bounded authority site to prefer structured
identity when complete metadata exists, preserving explicit spelling fallback
only for intentional no-metadata compatibility.

## Watchouts

- This packet intentionally did not change ABI mangling or debug formatting;
  those paths still use their existing rendered names.
- `CanonicalScopeSegment::identity` is present beside `name`, but current
  `canonical_symbol.cpp` only constructs translation-unit scope segments, so
  there was no namespace/record scope metadata path to populate in this slice.
- If a future producer creates one complete structured identity and one
  no-metadata spelling-only symbol for the same entity,
  `CanonicalIdentity::operator==` now treats that as a structured/incomplete
  mismatch rather than reopening spelling fallback.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_sema_canonical_symbol_structured_metadata|cpp_positive_sema_lookup_value_structured_metadata)$'`

Result: passed. Full combined output is preserved in `test_after.log`.
