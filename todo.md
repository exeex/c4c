# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate HIR Type And Record Consumers

## Just Finished

Plan-owner review accepted Step 2 - Migrate Parser-Owned Semantic Producers as
complete enough to advance after five parser/Sema migration slices through
HEAD `54adc0651`:

- `src/frontend/sema/type_utils.cpp::type_binding_values_equivalent` compares
  TypeSpec bindings through template parameter owner/index/TextId metadata,
  `record_def`, complete namespace plus `tag_text_id` plus qualifier TextIds,
  `tpl_struct_origin_key`, and `deferred_member_type_text_id` before rendered
  compatibility.
- Parser typedef-chain resolution uses TypeSpec `tag_text_id` and structured
  typedef keys before tag spelling, and blocks rendered fallback once a TextId
  carrier exists but misses.
- Parser nominal type compatibility compares shared `record_def` or complete
  structured text identity before tag spelling.
- Parser record-constructor classification uses `record_def`, structured
  typedef metadata, template primary keys, and record-node metadata keyed by
  `unqualified_text_id` before rendered compatibility.
- `src/frontend/parser/impl/support.cpp::types_compatible_p` mirrors the
  structured nominal identity policy used by `Parser::are_types_compatible`.

Step 2 tests in `frontend_parser_lookup_authority_tests` cover stale rendered
type spelling for the migrated parser/Sema routes, including positive
structured-carrier wins, structured mismatch rejection despite matching
rendered tags, one-sided metadata rejection, and rendered-only compatibility.

Residual parser `TypeSpec::tag` classifications accepted for this transition:

- `support.cpp::resolve_typedef_chain` remains rendered-key compatibility
  because its only input binding map is `std::unordered_map<std::string,
  TypeSpec>`; migrating it needs a structured typedef binding map at the API
  boundary and is not required before HIR consumer migration.
- `support.cpp` layout helpers (`struct_sizeof`, `field_align`,
  `compute_offsetof`, `eval_const_int`) already flow through
  `resolve_record_type_spec`, which prefers `record_def`; rendered
  `struct_tag_def_map` is layout compatibility storage for tag-only callers.
- `declarations.cpp` duplicated `is_incomplete_object_type` lambdas already
  call `resolve_record_type_spec`, so `record_def` wins for complete record
  metadata. The remaining `ts.tag` self-reference check is a display/current
  record spelling compatibility route; extracting a helper or proving stale
  rendered drift there can wait for a later parser helper packet if deletion
  probing shows it still blocks.
- Parser generated names, anonymous record tags, enum diagnostic text,
  mangled template instantiation tags, and template argument debug rendering
  remain display/final-spelling/debug payloads, not Step 2 semantic lookup
  targets.

## Suggested Next

Start Step 3 - Migrate HIR Type And Record Consumers with one narrow
metadata-backed HIR consumer migration. Prefer a route where `record_def`,
`tag_text_id` plus namespace/qualifier TextIds, `HirRecordOwnerKey`, module
declaration lookup keys, member keys, template metadata, or module type refs
are already present, and leave rendered `TypeSpec::tag`/`struct_defs` lookup
only as explicit no-metadata compatibility or output spelling. Add focused HIR
coverage where valid structured metadata wins over stale rendered type
spelling.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- Keep downstream LIR/BIR/backend carrier gaps as separate follow-up ideas
  instead of broadening this runbook.
- Do not attempt `TypeSpec::tag` field deletion in Step 3; removal belongs to
  the later deletion/probe and removal steps.
- Treat a `TypeSpec::tag` deletion build as a temporary probe only until the
  runbook reaches the removal step.

## Proof

Step 2 delegated packets recorded focused proof with:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_hir_tests|cpp_hir_.*template.*|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

Code review in `review/step2_typespec_tag_parser_route_review.md` reported no
blocking findings, no overfit, and the route on track. The current workspace
does not contain `test_after.log`; this lifecycle-only Step 2 review did not
rerun build/test proof.

`git diff --check -- todo.md` passed after the plan-owner transition.
