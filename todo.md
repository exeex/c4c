# Current Packet

Status: Active
Source Idea Path: ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate HIR NTTP And Consteval Handoff

## Just Finished

Step 2 - Migrate HIR NTTP And Consteval Handoff started with the pending
consteval route. `PendingConstevalExpr` now carries `NttpTextBindings`
beside the rendered `NttpBindings` compatibility map. The HIR
`try_lower_consteval_call_expr` path populates the TextId mirror from
`template_param_name_text_ids` for explicit expression NTTP args, forwarded
NTTP args resolved from the enclosing rendered map, and NTTP defaults. The
compile-time engine copies that carrier into
`ConstEvalEnv::nttp_bindings_by_text` when evaluating pending consteval
calls, while still setting `env.nttp_bindings` for compatibility.

Focused coverage was added to the always-built `frontend_hir_lookup_tests`
target to inspect initial HIR before compile-time reduction and verify
`plus_one<41>` has both the rendered `NttpBindings["N"]` entry and the
TextId-keyed mirror for the same value.

## Suggested Next

Continue Step 2 by threading a TextId/structured NTTP carrier through the
template-call handoff path (`TemplateCallInfo::nttp_args` or
`build_call_nttp_bindings`) so instantiated template calls can populate
metadata-backed NTTP env data before relying on rendered names.

## Watchouts

- Do not edit parser/Sema, LIR, BIR, or backend code except to record a
  boundary or bridge a HIR-owned blocker required by the active plan.
- Do not weaken tests, mark supported routes unsupported, or add named-case
  shortcuts.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads while removing rendered strings as semantic authority.
- Do not count mangled/link-visible strings or HIR dump strings as cleanup
  targets unless they are used to recover semantic identity.
- Record-layout cleanup is a separate boundary from NTTP cleanup: HIR has
  `HirRecordOwnerKey`/`struct_def_owner_index`, but Sema consteval does not yet
  receive a structured layout map.
- This slice intentionally migrated only the pending-consteval handoff. The
  broader `FunctionCtx::nttp_bindings`, `TemplateCallInfo::nttp_args`, and
  `HirTemplateInstantiation::nttp_bindings` routes remain rendered-primary
  compatibility surfaces.

## Proof

Delegated proof passed and wrote `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_.*|frontend_parser_lookup_authority_tests|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

Additional focused local check passed:
`ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$'`.
