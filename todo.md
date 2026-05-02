# Current Packet

Status: Active
Source Idea Path: ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate HIR NTTP And Consteval Handoff

## Just Finished

Step 2 - Migrate HIR NTTP And Consteval Handoff audited the residual
`NttpBindings` consumers and migrated one remaining metadata-backed HIR route:
static constexpr struct/member NTTP const evaluation. `eval_const_int_with_nttp_bindings`
now accepts an optional `NttpTextBindings` mirror and prefers an AST
`unqualified_text_id` lookup before falling back to the rendered name map.
Template struct instantiation and ordinary struct lowering build that mirror
from template parameter `TextId` metadata before evaluating static constexpr
member initializers.

Focused coverage in `frontend_hir_lookup_tests` now checks a stale rendered
NTTP name with a valid TextId mirror and verifies static-member NTTP const eval
uses the TextId value instead of the rendered-map value.

Residual Step 2 `NttpBindings` classification:
- Compatibility payloads: `TemplateCallInfo::nttp_args`,
  `PendingConstevalExpr::nttp_bindings`, `HirTemplateInstantiation::nttp_bindings`,
  and `FunctionCtx::nttp_bindings` remain rendered mirrors beside TextId maps
  for older callers and no-metadata fallback.
- Mangling/spec-key payloads: `mangle_template_name`,
  `make_specialization_key`, instance registry keys, and template-global/struct
  emitted names intentionally use rendered parameter names and values as stable
  ABI/cache/display payloads, not lookup authority.
- Pack naming/counting compatibility: `pack_binding_name`,
  `parse_pack_binding_name`, `count_pack_bindings_for_name`, and pack loops in
  template materialization use rendered synthetic keys such as `Ts#0`; there is
  no current per-pack-element TextId carrier, so treating those keys as
  semantic TextIds would be a parser/Sema carrier boundary.
- Template struct materialization and member-typedef helpers still contain
  rendered `nttp_bindings.find(...)` probes for encoded debug strings,
  deferred `$expr:` text, `TemplateArgRef::debug_text`, and inherited/selected
  pattern maps. These are compatibility/no-metadata or generated-name surfaces
  unless a future carrier gives each encoded template argument a structured
  identity.
- Deferred NTTP expression helpers consume vectorized rendered env pairs for
  `$expr:` evaluation. That route is a compatibility boundary shared with the
  deferred expression representation, not a HIR TextId-backed lookup path.

## Suggested Next

Step 2 appears ready for supervisor/plan-owner review before moving to Step 3.
If more NTTP cleanup is requested, split a new follow-up for structured pack
element/template-argument carriers instead of expanding this HIR resweep packet.

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
- Residual rendered `NttpBindings` uses should not be removed mechanically:
  several are ABI/display/cache payloads, pack synthetic-key compatibility, or
  deferred expression/debug-text boundaries without a complete structured
  carrier.

## Proof

Delegated proof passed and wrote `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_.*|frontend_parser_lookup_authority_tests|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.
