Status: Active
Source Idea Path: ideas/open/160_sema_canonical_symbol_template_key_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Structured Identity Carriers

# Current Packet

## Just Finished

Step 2 added structured identity carriers beside existing display spellings in
`CanonicalScopeSegment`, `CanonicalTemplateParam`, `CanonicalType`,
`CanonicalSymbol`, and `CanonicalIdentity`.

`canonical_symbol.cpp` now populates those carriers from parser/sema metadata on
covered paths:
- declaration names from `Node::unqualified_text_id`, qualifiers, global
  qualification, and namespace context into `CanonicalSymbol::name_identity`
  and then `CanonicalIdentity::name_identity`
- template params from `Node::template_param_name_text_ids`, owner namespace,
  owner text id, index, domain, pack, and default metadata into
  `CanonicalTemplateParam::identity`
- nominal type and template-param type leaves from `TypeSpec` tag, qualifier,
  namespace, record-def, template-param, template-struct-origin, and deferred
  member typedef metadata into `CanonicalType::identity`
- top-level record and enum symbols from `Node` name metadata into their
  canonical type identity carriers

Existing display strings remain available and retain current semantic authority
for equality, hashing, substitution, ABI mangling, debug formatting, and
no-metadata compatibility in this slice.

## Suggested Next

Start the next packet by moving one bounded authority site to prefer the new
structured carriers when both sides have metadata, while preserving explicit
rendered-spelling fallback for no-metadata paths.

## Watchouts

- This packet intentionally did not change `types_equal`,
  `substitute_template_args`, `CanonicalIdentity::operator==`, or
  `CanonicalIdentityHash`; those still use existing strings as authority.
- `CanonicalScopeSegment::identity` is present beside `name`, but current
  `canonical_symbol.cpp` only constructs translation-unit scope segments, so
  there was no namespace/record scope metadata path to populate in this slice.
- Do not collapse owner-aware carriers into a single raw `TextId`; template
  params and qualified nominal names require owner/scope context.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_sema_canonical_symbol_structured_metadata|cpp_positive_sema_lookup_value_structured_metadata)$'`

Result: passed. Full combined output is preserved in `test_after.log`.
