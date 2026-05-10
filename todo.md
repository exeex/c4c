Status: Active
Source Idea Path: ideas/open/160_sema_canonical_symbol_template_key_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Replace Template Substitution Binding Authority

# Current Packet

## Just Finished

Step 3 replaced template-substitution binding authority in
`canonical_symbol.cpp`.

`substitute_template_args` now builds owner-aware template-parameter identity
bindings when complete metadata exists, while keeping a clearly named fallback
map for rendered-name compatibility on incomplete/no-metadata paths.
`substitute_template_args_impl` now resolves `CanonicalType` template-parameter
leaves by `CanonicalType::identity.template_param` when complete metadata
exists, and only falls back to `user_spelling` when the use-site identity is
incomplete/no-metadata.

The focused metadata test now proves two same-spelled `T` parameters with
different owner metadata substitute independently, a complete owner-aware miss
does not reopen spelling fallback, and the legacy no-metadata spelling fallback
remains intentional.

## Suggested Next

Move the next supervisor-selected bounded authority site to prefer structured
identity when both sides have metadata, preserving explicit spelling fallback
only for incomplete/no-metadata compatibility.

## Watchouts

- This packet intentionally did not change `types_equal`,
  `CanonicalIdentity::operator==`, `CanonicalIdentityHash`, ABI mangling, or
  debug formatting; those still use existing strings as authority.
- `CanonicalScopeSegment::identity` is present beside `name`, but current
  `canonical_symbol.cpp` only constructs translation-unit scope segments, so
  there was no namespace/record scope metadata path to populate in this slice.
- The spelling fallback in substitution remains available only for
  old/incomplete metadata leaves; complete template-param leaves use the
  owner-aware path and remain unsubstituted on identity miss.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_sema_canonical_symbol_structured_metadata|cpp_positive_sema_lookup_value_structured_metadata)$'`

Result: passed. Full combined output is preserved in `test_after.log`.
