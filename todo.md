# Current Packet

Status: Active
Source Idea Path: ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate HIR NTTP And Consteval Handoff

## Just Finished

Step 2 - Migrate HIR NTTP And Consteval Handoff continued through the
template-call route. `TemplateCallInfo` now carries `nttp_args_by_text`
beside rendered `nttp_args`, `build_call_nttp_bindings` can resolve
forwarded NTTP values from enclosing TextId bindings before rendered names,
and the deferred template instantiation callback threads that carrier into
`FunctionCtx`, `InstantiationRegistry`, and `HirTemplateInstantiation`
metadata. Lowered template bodies now seed `ConstEvalEnv::nttp_bindings_by_text`
from the template-call handoff while preserving rendered `NttpBindings` as the
compatibility path.

Focused coverage in `frontend_hir_lookup_tests` stales the rendered NTTP
parameter spelling for a nested `outer<39> -> inner<V> -> lift<V>` route after
Sema validation. The test verifies the HIR `TemplateCallInfo` still records the
rendered compatibility binding, the TextId mirror carries value `39`, the
compile-time engine records the deferred `inner` instantiation with TextId NTTP
metadata, and the deferred consteval work fully reduces through the TextId env.

## Suggested Next

Continue Step 2 by migrating the next remaining rendered-primary NTTP surface,
likely `HirTemplateInstantiation::nttp_bindings` consumers or
`FunctionCtx::nttp_bindings` call sites outside the template-call/consteval
handoff, so metadata-backed routes prefer TextId carriers and rendered names
remain compatibility only.

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
- This slice intentionally migrated the template-call handoff after the prior
  pending-consteval handoff. The broader `FunctionCtx::nttp_bindings` and
  `HirTemplateInstantiation::nttp_bindings` rendered maps remain compatibility
  surfaces, though the migrated handoffs now have parallel TextId mirrors.

## Proof

Delegated proof passed and wrote `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_.*|frontend_parser_lookup_authority_tests|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.
