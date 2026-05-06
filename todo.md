Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Repair Sema TypeSpec Record Key Width

# Current Packet

## Just Finished

Step 7 repaired the Sema TypeSpec record key-width blocker.
Metadata-backed record completion now routes TypeSpec and direct `record_def`
record-domain keys through structured metadata that preserves
`is_global_qualified` and qualifier `TextId` sequences, and record completion
tables are populated with the same full-width record key.

This repair also fixed the full-suite regressions from that key-width change:
Sema now derives method-owner and static-member owner keys from structured
qualifier metadata, resolves width-compatible owner keys through the Sema
record table when parser carrier shapes differ, and enters method scope with
the same full-width record key used by field/static-member tables.

Focused coverage in `frontend_parser_lookup_authority_tests` now proves that
same namespace/base-`TextId` record completion accepts matching global and
qualifier metadata, rejects mismatched qualifier `TextId` metadata, and rejects
mismatched global qualification metadata.

## Suggested Next

Supervisor should route Step 8 final boundary review or regression-guard
closure checks for `ideas/open/145_move_record_tag_authority_from_parser_to_sema.md`.

## Watchouts

This slice intentionally did not add a Sema-owned canonicalization rule for
global/qualifier metadata. It preserves those fields instead, so future record
completion paths should not reintroduce namespace-context/base-`TextId`-only
keys as a shortcut.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_positive_sema_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_structured_static_member_lookup_runtime_cpp|cpp_positive_sema_template_variable_member_typedef_normalization_runtime_cpp|frontend_parser_lookup_authority_tests|frontend_parser_tests|frontend_hir_lookup_tests|frontend_hir_tests|cpp_hir_.*structured_metadata)$' > test_after.log`

Result: green. `test_after.log` records 43/43 focused tests passing after a
successful `cmake --build --preset default`.

Supervisor acceptance proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure > test_after.log`

Result: green. `test_after.log` records 3023/3023 full-suite tests passing
after a successful `cmake --build --preset default`.
