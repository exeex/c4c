# Current Packet

Status: Active
Source Idea Path: ideas/open/175_hir_typespec_ref_structured_equivalence.md
Source Plan Path: plan.md
Current Step ID: 2/3
Current Step Title: Add Structured Identity Probe Coverage And Thread Structured Identity

## Just Finished

Step 2/3 added focused compound-literal initializer-list coverage for aggregate
direct assignment in `tests/frontend/frontend_hir_lookup_tests.cpp` and updated
`src/frontend/hir/impl/expr/object.cpp::same_aggregate_direct_assign_type` so
structured aggregate metadata requires matching `HirRecordOwnerKey` identity
that resolves to a known HIR aggregate owner.  The rendered-tag fallback now
remains explicit for true no-structured-metadata inputs only; unresolved
structured identity and mismatched structured identity cannot authorize direct
assignment.

## Suggested Next

Supervisor should review the Step 2/3 slice for acceptance and commit
readiness.  A coherent next packet, if continuing this plan, is to select the
next ordinary HIR type-ref family from the source idea rather than widening this
direct-aggregate path.

## Watchouts

- Do not expand into LIR `LirTypeRef` equality; that belongs to idea 176.
- Do not expand into template record owner identity; that belongs to idea 177.
- The object-path guard now requires a structured key to resolve through
  `Module::find_struct_def_tag_by_owner`; if a future legitimate direct
  aggregate assignment has structured metadata but no owner index, the missing
  owner registration should be fixed instead of restoring rendered fallback.
- Existing local-declaration direct aggregate coverage in `decl.cpp` was left
  untouched; this packet only changed the object compound-literal path and
  frontend HIR lookup tests.

## Proof

`cmake --build build --target frontend_hir_lookup_tests && ctest --test-dir build -R '^frontend_hir_lookup_tests$' --output-on-failure > test_after.log`
passed.  Proof log: `test_after.log`.
