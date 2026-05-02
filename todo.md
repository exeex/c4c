# Current Packet

Status: Active
Source Idea Path: ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate HIR NTTP And Consteval Handoff

## Just Finished

Step 2 - Migrate HIR NTTP And Consteval Handoff continued through the remaining
FunctionCtx/template-call NTTP consumer surfaces in the Step 2 area. HIR now
uses a `lookup_nttp_binding` helper that prefers `FunctionCtx::nttp_bindings_by_text`
from AST `TextId` metadata before falling back to rendered `NttpBindings`.
`infer_call_result_type` passes enclosing TextId NTTP bindings into
`build_call_nttp_bindings`, template value-argument evaluation seeds
`ConstEvalEnv::nttp_bindings_by_text`, and direct NTTP expression/var lowering
uses the TextId-first helper.

Template-global and template-struct method handoff paths now construct and pass
TextId NTTP mirrors into `lower_global`, pending struct methods, and
`lower_struct_method`, so metadata-backed initializer and method bodies can
populate their compile-time env before using rendered names as compatibility.
`build_call_nttp_text_bindings` no longer requires a call AST node, allowing
defaulted or materialized NTTP maps to be mirrored from template parameter
metadata.

Focused coverage in `frontend_hir_lookup_tests` now includes a template-global
initializer route that stales the rendered NTTP parameter spelling for
`template<int V> int value = lift<V>();` after Sema validation and verifies the
instantiated global still reduces to `42` through the TextId-backed
`FunctionCtx` binding.

## Suggested Next

Continue Step 2 by auditing residual rendered `NttpBindings` uses that are
outside the migrated compile-time/template-call handoff, especially template
struct pattern selection/materialization and pack NTTP surfaces. Split any
missing parser/Sema TextId carrier boundary into a follow-up instead of
widening this HIR packet.

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
- This slice migrated the remaining obvious Step 2 FunctionCtx/template-call
  consumers that already had AST TextId metadata. Rendered `NttpBindings`
  remain compatibility payloads and are still used for pack naming,
  specialization-key/mangling payloads, and unresolved surfaces without a
  complete TextId carrier.

## Proof

Delegated proof passed and wrote `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_.*|frontend_parser_lookup_authority_tests|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.
