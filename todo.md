# Current Packet

Status: Active
Source Idea Path: ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Structured HIR Record-Layout Handoff

## Just Finished

Step 3 - Add Structured HIR Record-Layout Handoff now has a narrow structured
consteval record-layout route. `ConstEvalEnv` carries an optional
`struct_def_owner_index` alongside rendered `struct_defs`, and
`lookup_record_layout` now derives a `HirRecordOwnerKey` from `TypeSpec`
`record_def` / TextId metadata before falling back to rendered `TypeSpec::tag`.

HIR-created consteval environments now thread `Module::struct_def_owner_index`
through lowerer consteval and compile-time-engine consteval/static-assert
routes. Rendered `ConstEvalEnv::struct_defs` remains the explicit
compatibility payload for no-metadata paths and final layout storage.

Focused coverage proves `sizeof` and `alignof` choose the structured HIR owner
layout when `TypeSpec::tag` names a stale rendered record.

## Suggested Next

Continue Step 3 by auditing remaining HIR record-layout consumers that still
query `Module::struct_defs` directly from rendered tags and migrate one bounded
metadata-backed route to resolve through `HirRecordOwnerKey` first.

## Watchouts

- Do not edit parser/Sema, LIR, BIR, or backend code except to record a
  boundary or bridge a HIR-owned blocker required by the active plan.
- Do not weaken tests, mark supported routes unsupported, or add named-case
  shortcuts.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads while removing rendered strings as semantic authority.
- Do not count mangled/link-visible strings or HIR dump strings as cleanup
  targets unless they are used to recover semantic identity.
- Record-layout cleanup is a separate boundary from NTTP cleanup. Sema
  consteval now receives HIR owner metadata, but many HIR lowering/layout
  consumers still query rendered `Module::struct_defs` directly.
- Residual rendered `NttpBindings` uses should not be removed mechanically:
  several are ABI/display/cache payloads, pack synthetic-key compatibility, or
  deferred expression/debug-text boundaries without a complete structured
  carrier.
- `ConstEvalEnv::struct_defs` remains rendered-keyed by design in this packet;
  `struct_def_owner_index` only selects the authoritative rendered layout entry
  when `TypeSpec` metadata can form a complete `HirRecordOwnerKey`.
- Template-instantiation record-owner keys need specialization identity; this
  packet covers declaration-owner layout lookup only. Do not infer that all
  template-record layout paths are structured yet.

## Proof

Focused pre-proof passed:
`cmake --build --preset default --target frontend_hir_lookup_tests && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$'`.

Delegated proof passed and wrote `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_.*|frontend_parser_lookup_authority_tests|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.
