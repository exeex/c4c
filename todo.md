# Current Packet

Status: Active
Source Idea Path: ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Structured HIR Record-Layout Handoff

## Just Finished

Step 3 - Add Structured HIR Record-Layout Handoff now has a bounded
member/object owner recovery migration. Implicit `this` field recovery in
`Lowerer::lower_var_expr` now builds a `TypeSpec` for the current method owner,
uses `record_def` / TextId metadata through `find_struct_def_for_layout_type`
and `find_struct_instance_field_including_bases`, and only falls back to
rendered `ctx.method_struct_tag` through the layout helper compatibility path.

The recovered member expression now carries the structured owner tag when owner
metadata is available, and member-symbol lookup uses the `TypeSpec` owner route
before rendered member-symbol names.

Focused coverage proves stale rendered `ctx.method_struct_tag` loses to the
structured owner layout for implicit `this->field` recovery.

## Suggested Next

Continue Step 3 by auditing inherited-field and defaulted-method/destructor
member traversal routes that still start from rendered `struct_defs` tags, or
ask supervisor/plan-owner whether Step 3 has enough representative structured
record-layout handoffs to move toward Step 4.

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
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$'`.

Delegated proof passed and wrote `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_.*|frontend_parser_lookup_authority_tests|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.
