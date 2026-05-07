# Current Packet

Status: Complete
Source Idea Path: ideas/open/149_template_instantiation_structured_argument_key.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Migrate HIR Late Instantiation Consumers

## Just Finished

Completed the Step 5 review fix for structured HIR specialization-key owner
identity.

Implemented:

- `SpecializationOwnerIdentity::display_name` is now explicitly display/
  compatibility data for structured owners.
- `SpecializationOwnerIdentity` equality and ordering use `display_name` only
  when both sides are ownerless fallback keys.
- `specialization_owner_identity_hash` hashes `display_name` only for
  ownerless fallback keys, keeping hash behavior aligned with equality.

## Suggested Next

Next coherent packet: migrate `make_pending_template_type_key` and its
pending-template-type dedup users to a structured key over pending type kind,
owner identity, structured `TypeSpec`, ordered type/NTTP bindings, context, and
span, leaving rendered text as diagnostics/display data only.

## Watchouts

- `HirRecordOwnerTemplateIdentity::specialization_key` still stores
  `spec_key.canonical` as a metadata bridge; later packets should replace that
  bridge only when record-owner template identity can carry the structured key
  without widening this slice.
- `canonical_type_str` remains in use for display/compatibility and non-template
  type-trait evaluation. This packet did not attempt to redefine general HIR
  type equality outside specialization-key identity.

## Proof

Delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_lookup_tests|cpp_hir_)'`

Result: 109/109 tests passed. Proof log: `test_after.log`.
