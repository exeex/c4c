Status: Active
Source Idea Path: ideas/open/160_sema_canonical_symbol_template_key_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Nominal Type Equality

# Current Packet

## Just Finished

Step 4 repaired nominal/template-parameter type equality in
`canonical_symbol.cpp`.

`types_equal` now compares complete owner-aware template-parameter metadata
before rendered spelling, compares complete nominal metadata through
`record_def` or qualified-name identity, and only uses `user_spelling` for the
legacy no-metadata fallback path. Template-parameter semantic identity excludes
declaration-only carrier attributes such as pack/default flags.

The focused metadata test now proves same-spelled records, typedef leaves, and
template-parameter leaves from different structured domains do not compare
equal, while matching structured identities do not require matching rendered
spelling and no-metadata fallback remains intentional.

## Suggested Next

Move the next supervisor-selected bounded authority site to prefer structured
identity when complete metadata exists, preserving explicit spelling fallback
only for intentional no-metadata compatibility.

## Watchouts

- This packet intentionally did not change `CanonicalIdentity::operator==`,
  `CanonicalIdentityHash`, ABI mangling, or debug formatting outside the
  existing `types_equal` call path.
- `CanonicalScopeSegment::identity` is present beside `name`, but current
  `canonical_symbol.cpp` only constructs translation-unit scope segments, so
  there was no namespace/record scope metadata path to populate in this slice.
- If a future producer creates one complete structured identity and one
  no-metadata spelling-only leaf for the same entity, `types_equal` now treats
  that as a structured/incomplete mismatch rather than reopening spelling
  fallback.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_sema_canonical_symbol_structured_metadata|cpp_positive_sema_lookup_value_structured_metadata)$'`

Result: passed. Full combined output is preserved in `test_after.log`.
