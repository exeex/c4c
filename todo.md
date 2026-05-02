# Current Packet

Status: Active
Source Idea Path: ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Convert Or Demote Legacy HIR Lookup Routes

## Just Finished

Plan-owner review accepted Step 3 - Add Structured HIR Record-Layout Handoff as
complete enough for this runbook. Step 3 now has the Sema consteval structured
owner-index bridge, HIR builtin layout query migration, aggregate-init
normalization migration, implicit-this field recovery migration,
stale-rendered-tag tests, and residual direct `struct_defs` classification.

Step 4 - Convert Or Demote Legacy HIR Lookup Routes is now the active
execution step.

## Suggested Next

Start Step 4 with one narrow legacy HIR lookup route. Prefer a route with an
existing structured carrier and focused stale-rendered-name coverage, such as a
caller of `find_function_by_name_legacy`, `find_global_by_name_legacy`,
`ModuleDeclLookupAuthority::LegacyRendered`, `has_legacy_mangled_entry`, or a
rendered-name compatibility index. Convert that caller to structured authority
where metadata exists, or demote the rendered route to explicitly named
no-metadata compatibility.

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
  consteval, HIR builtin layout queries, global aggregate-init normalization,
  and implicit-this member recovery now receive/use owner metadata, but several
  HIR lowering/layout consumers still query rendered `Module::struct_defs`
  directly.
- Step 3 residual direct `struct_defs` consumers are classified boundaries, not
  Step 4 targets by default. Step 4 should focus on legacy declaration,
  function/global/template/member lookup authority unless a rendered
  `struct_defs` route is also acting as normal semantic lookup with an existing
  structured carrier.
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

Step 3 focused pre-proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$'`.

Step 3 delegated proof passed and wrote `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_.*|frontend_parser_lookup_authority_tests|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

No new validation was run for this lifecycle-only Step 3 review and Step 4
pointer update.
