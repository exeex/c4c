# Current Packet

Status: Active
Source Idea Path: ideas/open/149_template_instantiation_structured_argument_key.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Migrate HIR Late Instantiation Consumers

## Just Finished

Completed the Step 5 HIR specialization-key packet. The latest accepted slice
made `SpecializationOwnerIdentity::display_name` display/compatibility data for
structured owners, with equality, ordering, and hashing consulting it only for
ownerless fallback keys.

## Suggested Next

Next coherent packet under Step 5: migrate `make_pending_template_type_key` and
its pending-template-type dedup users to a structured key over pending type
kind, owner identity, structured `TypeSpec`, ordered type/NTTP bindings,
context, and span.

Acceptance focus for the packet:

- Rendered/canonical type text must remain diagnostics/display data only.
- Pending-template-type dedup must compare structured pending-type payloads, not
  formatted `TypeSpec` text.
- HIR late instantiation should consume the structured key without reparsing a
  display string.

## Watchouts

- `HirRecordOwnerTemplateIdentity::specialization_key` still stores
  `spec_key.canonical` as a metadata bridge; later packets should replace that
  bridge only when record-owner template identity can carry the structured key
  without widening this slice.
- `canonical_type_str` remains in use for display/compatibility and non-template
  type-trait evaluation. This packet did not attempt to redefine general HIR
  type equality outside specialization-key identity.
- Avoid widening this packet into Step 6 cleanup. Remove or relabel only the
  string mirrors that become directly non-semantic while migrating
  `make_pending_template_type_key`.

## Proof

Previous completed-packet proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_lookup_tests|cpp_hir_)'`

Result: 109/109 tests passed. Proof log: `test_after.log`.

No proof has been run for the next packet yet.
