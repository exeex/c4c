Status: Active
Source Idea Path: ideas/open/198_parser_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Closure Ledger And Broader Parser Proof

# Current Packet

## Just Finished

Step 6 - Closure Ledger And Broader Parser Proof completed the parser legacy
compatibility retirement ledger for the active runbook.

Closure ledger:

- Deleted: the unconditional rendered `owner_mangled` retry in the
  static-member/owner base route was removed. Metadata-rich owner resolution
  no longer retries `definition_state_.struct_tag_def_map` by rendered owner
  spelling after the structured route misses.
- Converted: parser record lookup through
  `resolve_record_type_spec_with_parser_tag_map_compatibility` now accepts
  direct `record_def`, scoped `TextId`, namespace/global qualification, and
  qualifier metadata before any compatibility tail. Metadata-rich static-member
  base and owner callers use structured lookup or fail closed.
- Converted: parser layout and constant-layout helpers route complete
  `record_def` and structured record carriers through structured lookup. The
  parser record-layout tag map is no longer semantic authority after a
  complete structured miss.
- Converted: qualified template member typedef owner recovery now constructs a
  structured template owner key and uses the resolved owner `record_def`;
  stale rendered owner-map entries no longer recover a missing structured
  owner.
- Converted: expression construction for `Template<Args>::member` treats a
  qualified owner `TextId` or structured template argument carrier as complete
  owner metadata. Rendered owner segment splitting is skipped after a complete
  structured owner miss.
- Converted: parser const-int named constants use structured `TextId` tables
  on the primary surface. The rendered named-constant bridge now checks
  structured metadata first and fails closed after a complete structured miss.
- Converted: pending base materialization now starts from structured
  `TemplateInstantiationKey` plus parsed argument metadata instead of rendered
  `base_mangled` tag-map lookup.
- Converted: direct template-emission reuse now distinguishes a complete
  structured instantiation key from a merely marked key. A complete structured
  key miss does not recover through stale
  `definition_state_.struct_tag_def_map[mangled]`.
- Fenced: parser-local `struct_tag_def_map` and `defined_struct_tags` remain
  parser scratch/mirror state for registration, diagnostics, tests, and
  explicit no-metadata compatibility. They are not accepted as semantic
  authority for covered metadata-rich routes.
- Fenced: `template_origin_name`, rendered candidate keys, source spelling,
  parser diagnostics, AST display strings, and final output text remain
  observation/display payloads. Covered semantic lookup must use structured
  record, qualifier, owner, template-key, or named-constant metadata.

Intentionally retained parser compatibility:

- Owner: parser support record/layout helpers. Limitation: callers with no
  `record_def`, no scoped `TextId`, and no qualifier/context carrier may still
  consult rendered parser tag maps as legacy/final-spelling compatibility.
  Removal condition: remove this tail when the remaining parser/HIR template
  callers provide structured record carriers.
- Owner: expression template-owner construction. Limitation: rendered owner
  segment splitting remains only when neither the qualified owner nor template
  arguments carry structured owner metadata. Removal condition: remove after
  all expression template-owner routes populate qualified-owner `TextId` and
  template-argument identity carriers.
- Owner: rendered named-constant compatibility bridge. Limitation: rendered
  NTTP/named-constant lookup is retained for legacy/HIR template probes that
  still pass only rendered names; parser structured named-constant metadata is
  authoritative when present. Removal condition: remove after those callers
  carry named-constant `TextId` metadata.
- Owner: template base materialization and direct-emission reuse. Limitation:
  rendered map reuse remains only in explicit no-carrier branches where there
  is no complete structured template instantiation key. Removal condition:
  remove after all template instantiated record callers pass structured
  template origin and argument metadata.
- Owner: member typedef owner resolution. Limitation: the retained
  `parse_base_type_legacy_tag_if_no_metadata(ts)` fallback may consult rendered
  owner tags only for instantiated owners that lack `TypeSpec::record_def` and
  structured owner metadata. Removal condition: remove after instantiated
  owner `TypeSpec` construction always carries `record_def` or complete
  structured owner keys.

Follow-up candidate outside this parser-owned runbook: retire the remaining
legacy/no-metadata HIR or Sema template callers that still require rendered
record, owner, template-origin, or named-constant compatibility bridges. That
work should become a separate open idea if pursued; it is not hidden expansion
of this parser compatibility plan.

## Suggested Next

Hand Step 6 back to the supervisor for review, broader acceptance decision,
and lifecycle close/deactivate routing.

## Watchouts

- Keep the work parser-owned; do not expand into Sema, HIR, LIR, BIR, or
  backend cleanup.
- Do not downgrade supported tests or replace semantic fixes with expectation
  rewrites.
- Do not treat parser diagnostics, AST display strings, source spelling, or
  final output text as semantic identity.
- Do not treat a raw `TextId` alone as complete semantic identity across
  scopes.
- Newly retained parser bridges need `legacy` or `deprecated` comments with
  owner, limitation, and removal condition.
- Step 6 is a closure-ledger and proof step. Do not perform implementation
  cleanup unless the supervisor delegates a separate executor packet for a
  specific blocker found while building the ledger.
- The ledger must not hide retained parser bridges. Each retained compatibility
  path needs owner, limitation, and removal condition.
- Existing HIR/Sema template callers may still lack parser structured carriers.
  Preserve those as explicit legacy/no-metadata boundaries unless parser-owned
  metadata is proven complete.
- Do not treat `template_origin_name`, rendered candidate keys, parser
  diagnostics, AST display strings, source spelling, or final output text as
  semantic authority.
- Do not create backend, HIR, LIR, BIR, or Sema cleanup inside this plan. If the
  closure ledger finds non-parser residual work, record it as a separate
  follow-up idea candidate.

## Proof

Step 6 broader parser/frontend proof passed:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_support_residual_structured_metadata|cpp_hir_parser_type_base_pending_base_substitution_structured_metadata|cpp_hir_parser_type_base_instantiated_deferred_member_structured_metadata|cpp_hir_template_canonical_primary_origin_structured_metadata|cpp_parser_debug_qualified_type_spelling_stack)$"' > test_after.log 2>&1`

Result: 100% tests passed, 0 failed out of 7.

Proof log: `test_after.log`.
