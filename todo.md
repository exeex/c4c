# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Parser-Owned Semantic Producers

## Just Finished

Step 2 - Migrate Parser-Owned Semantic Producers migrated
`src/frontend/parser/impl/support.cpp::types_compatible_p`.

The support-level type compatibility route now mirrors the structured nominal
identity policy used by `Parser::are_types_compatible`: shared `record_def`
identity first, then namespace context plus `tag_text_id`, global
qualification, and qualifier TextIds when complete metadata is available.
Rendered tag comparison remains explicit no-metadata compatibility only when
neither side carries structured nominal identity.

Added focused stale-rendered-spelling coverage in
`frontend_parser_lookup_authority_tests`: `types_compatible_p` accepts shared
`record_def` or matching enum `tag_text_id`/namespace metadata despite stale
rendered tags, rejects mismatched structured record/TextId identity even when
rendered tags match, rejects one-sided structured metadata fallback, and keeps
rendered-only compatibility working.

Residual parser `TypeSpec::tag` audit classification:

- `support.cpp::resolve_typedef_chain` remains rendered-key compatibility
  because its only input binding map is `std::unordered_map<std::string,
  TypeSpec>`; migrating it needs a structured typedef binding map at that API
  boundary.
- `support.cpp` layout helpers (`struct_sizeof`, `field_align`,
  `compute_offsetof`, `eval_const_int`) already flow through
  `resolve_record_type_spec`, which prefers `record_def`; rendered
  `struct_tag_def_map` is layout compatibility storage for tag-only callers.
- `declarations.cpp` duplicated `is_incomplete_object_type` lambdas already
  call `resolve_record_type_spec`, so `record_def` wins for complete record
  metadata. The remaining `ts.tag` self-reference check is a display/current
  record spelling compatibility route; proving stale rendered drift there would
  require a broader parser fixture or extracting a helper, so leave it for a
  later producer-metadata/helper packet.
- Parser generated names, anonymous record tags, enum diagnostic text,
  mangled template instantiation tags, and template argument debug rendering
  remain display/final-spelling/debug payloads, not Step 2 semantic lookup
  targets.

## Suggested Next

Continue Step 2 with a parser producer-metadata packet if the supervisor wants
to extract the duplicated incomplete-object helper or add a structured typedef
map to `support.cpp::resolve_typedef_chain`; otherwise consider Step 2 ready
for plan-owner review before moving to HIR consumer migration.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- Keep downstream LIR/BIR/backend carrier gaps as separate follow-up ideas
  instead of broadening this runbook.
- Do not attempt `TypeSpec::tag` field deletion in Step 2; removal belongs to
  the later deletion/probe and removal steps.
- Treat a `TypeSpec::tag` deletion build as a temporary probe only until the
  runbook reaches the removal step.

## Proof

Step 2 delegated proof passed for the parser support `types_compatible_p`
packet and wrote `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_hir_tests|cpp_hir_.*template.*|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

`git diff --check` passed.
