# Current Packet

Status: Active
Source Idea Path: ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Structured HIR Record-Layout Handoff

## Just Finished

Step 3 - Add Structured HIR Record-Layout Handoff now has a second bounded HIR
record-layout migration. The HIR builtin layout query helper used by lowered
`sizeof(type)` / `alignof(type)` now accepts the full `TypeSpec`, derives a
`HirRecordOwnerKey` from `record_def` / TextId metadata when present, and uses
`Module::find_struct_def_by_owner_structured` before falling back to rendered
`TypeSpec::tag`.

Focused coverage proves both builtin `sizeof` and builtin `alignof` choose the
structured HIR owner layout when the rendered tag points at a stale record.

Step 3 direct `Module::struct_defs` consumer classification from this audit:
layout storage/final payload (`Module::struct_defs`, insertion/order,
`struct_def_owner_index` values), display/dump/codegen payloads
(`inspect/printer`, `hir_build` emission checks, generated template struct
names), metadata-backed semantic layout routes already migrated in this step
(Sema consteval layout lookup and HIR builtin layout queries), metadata-backed
semantic routes still worth future migration (global aggregate init
normalization, flat scalar counting, member/object owner recovery, inherited
field lookup), and no-metadata/generated-name compatibility routes (template
materialization by mangled specialization name, pack/deferred template helpers,
fallback method/constructor lookups when only a rendered tag is available).

## Suggested Next

Continue Step 3 with one more metadata-backed HIR semantic layout route if the
supervisor wants a broader Step 3 slice: global aggregate-init normalization
is the next likely candidate because it receives `TypeSpec` and currently
recurses through rendered `struct_defs` lookups.

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
  consteval and HIR builtin layout queries now receive/use owner metadata, but
  several HIR lowering/layout consumers still query rendered `Module::struct_defs`
  directly.
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
- Do not mechanically replace `struct_defs` lookups in dump/codegen/template
  materialization paths: those often consume rendered storage names or generated
  specialization names as compatibility payloads, not semantic lookup authority.

## Proof

Focused pre-proof passed:
`cmake --build --preset default --target frontend_hir_lookup_tests && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$'`.

Delegated proof passed and wrote `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_.*|frontend_parser_lookup_authority_tests|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.
