# Current Packet

Status: Active
Source Idea Path: ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Convert Or Demote Legacy HIR Lookup Routes

## Just Finished

Step 4 - Convert Or Demote Legacy HIR Lookup Routes now has a local function
prototype demotion. `Lowerer::lower_local_decl_stmt` no longer checks rendered
`module_->fn_index.count(n->name)` for local function prototypes; it builds a
metadata-backed function `DeclRef` from the declaration node and resolves
through `Module::resolve_function_decl`, preserving rendered fallback through
the central resolver when structured metadata is absent or incomplete.

Focused coverage proves a stale rendered local prototype name resolves through
structured function metadata, skips local allocation, and records a legacy
rendered parity mismatch instead of trusting the stale rendered function entry.

## Suggested Next

Continue Step 4 by auditing the remaining direct `lookup_function_id` /
`lookup_global_id` callers. Template/global materialization by mangled names
should likely stay compatibility payload; ordinary AST-backed variable/callee
references should route through `resolve_*_decl` when they still bypass it.

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
- Step 4 should preserve `LegacyRendered` tests for missing or incomplete
  metadata; the goal is to move metadata-backed callers to the central
  structured-first resolver APIs, not to remove compatibility fallback.

## Proof

Step 3 focused pre-proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$'`.

Step 3 delegated proof passed and wrote `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_.*|frontend_parser_lookup_authority_tests|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

Step 4 focused pre-proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$'`.

Delegated proof passed and wrote `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_.*|frontend_parser_lookup_authority_tests|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.
