# Current Packet

Status: Active
Source Idea Path: ideas/open/175_hir_typespec_ref_structured_equivalence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select The HIR Type-Ref Family

## Just Finished

Step 1 selected the HIR ordinary type-ref family: local aggregate direct
assignment during initializer-list lowering.  The first implementation packet
should inspect `src/frontend/hir/impl/expr/object.cpp`, especially
`same_aggregate_direct_assign_type`, `Lowerer::lower_local_decl_stmt`, the
`can_direct_assign_agg` lambda, and the local `resolve_aggregate_owner` /
`has_aggregate_structured_owner_identity` helpers.  Adjacent context to read
first is `src/frontend/hir/hir_lowering_core.cpp::generic_type_compatible`,
`src/frontend/hir/hir_types.cpp::make_record_owner_key_for_type`, and the
direct-aggregate fixtures in
`tests/frontend/frontend_hir_lookup_tests.cpp`.

The old decision to replace or guard is
`same_aggregate_direct_assign_type`: after broad `generic_type_compatible`
accepts struct/union/enum shape, the helper currently treats the presence of
either `TypeSpec` nominal carrier as sufficient and returns true, while the
legacy no-metadata path falls back to rendered tag equality.  That decision
controls whether initializer-list lowering emits a direct assignment for an
aggregate field/array element instead of descending into the aggregate owner.

Structured identity facts already available at this surface include
`TypeSpec::record_def`, `TypeSpec::tag_text_id`, namespace/global qualifier
metadata, template origin/argument carriers, `HirRecordOwnerKey` from
`make_record_owner_key_for_type` / `make_struct_def_node_owner_key`, and
`Module::find_struct_def_tag_by_owner`.  The next packet should make the
metadata-rich path require matching structured aggregate owner identity, with
any rendered-tag compatibility kept explicit for true no-metadata inputs only.

## Suggested Next

Implement the local aggregate direct-assignment guard in
`src/frontend/hir/impl/expr/object.cpp` and add focused coverage in
`tests/frontend/frontend_hir_lookup_tests.cpp` proving equal rendered spelling
or stale rendered tags cannot authorize aggregate direct assignment when
structured owner identity is present and mismatched or unresolved.

## Watchouts

- Do not expand into LIR `LirTypeRef` equality; that belongs to idea 176.
- Do not expand into template record owner identity; that belongs to idea 177.
- Do not claim progress from helper renames, printed-text changes, or
  testcase-shaped matching while the old semantic comparison remains.
- Preserve syntax/display uses of `TypeSpec` unless the selected HIR path
  explicitly requires a tested behavior change.
- Keep the slice to ordinary HIR aggregate direct assignment in initializer-list
  lowering; do not migrate operator object storage, parser typedef chains,
  member typedef readiness, or backend aggregate layout in the same packet.
- Do not weaken existing direct-aggregate tests or recategorize them as
  unsupported; the next proof should show semantic owner matching, not a
  rendered-name expectation rewrite.

## Proof

Inspection and todo-only update. No build or test proof was required by this
packet, and no `test_after.log` was produced. Suggested first implementation
proof: `cmake --build build --target frontend_hir_lookup_tests && ctest --test-dir build -R '^frontend_hir_lookup_tests$' --output-on-failure`.
