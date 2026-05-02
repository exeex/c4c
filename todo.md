# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 probed the requested namespace-qualified and nested-owner
static-member value-argument paths. Namespace-qualified explicit NTTP value
arguments already evaluate through the current structured carrier route, but
the requested nested-owner case with an enclosing type binding and NTTP default
is blocked before the delegated parser/HIR consumer files can repair it.

Concrete probe: `Wrap<int>` with field
`Buffer<Outer<T>::Count::value + 1>` is already cloned by parser-side template
struct instantiation as `Buffer_V_0`; at that point the field `TypeSpec` no
longer carries a usable `TemplateArgRef::type.array_size_expr` for HIR
materialization. The missing producer/consumer carrier is in the parser
template-instantiation field-cloning/substitution path that owns concrete
field `TypeSpec` creation, not in the delegated `expressions.cpp` or
`value_args.cpp` handoff consumer. No rendered owner parsing, `$expr:`
broadening, expectation downgrade, or named-test shortcut was left behind.

## Suggested Next

Continue Step 4 with a parser template-instantiation packet that owns
`src/frontend/parser/impl/types/base.cpp`: when cloning/substituting template
struct fields, preserve or re-materialize structured `TemplateArgRef` value
argument carriers for nested-owner expressions such as
`Outer<T>::Nested::value`, including enclosing type bindings and NTTP defaults.

## Watchouts

- The source idea remains active; Step 4 progress is not source-idea closure.
- Parser static-member references now canonicalize nested owner paths only when
  the path resolves through structured record/type metadata or nested
  `TypeSpec::record_def` carriers. Namespace-qualified ordinary values should
  continue through the normal qualified value path.
- `SourceLanguage` is declaration provenance, not linkage. Keep
  `linkage_spec` authoritative for explicit `extern "C"` conflict behavior.
- Manually constructed AST tests default zero-initialized nodes to C source
  language unless they are created through `Parser::make_node`.
- Static-member lookup still depends on parser/Sema owner and member metadata.
  A generated `NK_VAR` for `Owner::member` must carry `qualifier_text_ids`,
  `unqualified_text_id`, and the correct owner namespace context when the
  parser has the owner identity; do not restore rendered owner lookup to cover
  missing carriers.
- The remaining `$expr:` re-lex branch is compatibility-only for value args
  with no `ParsedTemplateArg::expr`, no value-arg `TypeSpec::array_size_expr`,
  no `ParsedTemplateArg::nttp_text_id`, and no `captured_expr_tokens`.
- `lookup_rendered_nttp_compatibility()` is still needed for older consteval
  NTTP parameter references that have no NTTP structured/TextId binding
  carrier. Do not broaden the gate to all local/named const metadata misses
  without first promoting those references to structured NTTP metadata.
- Some existing consteval producers still call `bind_consteval_call_env()` or
  `resolve_type()` with flat `type_bindings` only. This packet intentionally
  preserves that flat-only compatibility path; removing it requires a separate
  handoff packet that supplies structured/TextId type-binding channels at those
  producers.
- `captured_expr_tokens` is parser-local structured metadata. Do not promote
  stale `nttp_name`/`$expr:` text back to semantic authority when the token
  carrier exists.
- `tpl_struct_origin` remains compatibility/display spelling. Do not use it to
  rediscover template primaries or decide Sema `TypeSpec` equivalence when
  `tpl_struct_origin_key` or structured arg carriers exist.
- Simple type-only pending template args now use `TemplateArgRef` structure.
  The broad direct nested template-origin path must still stay on the legacy
  display route for now: promoting `Outer<Inner<T>>`-style pending args to
  typed nested `TemplateArgRef` carriers in the direct parser branch triggers
  existing HIR `owner struct still pending` failures in
  `template_struct_advanced.cpp` and `template_struct_nested.cpp`.
- Do not reintroduce rendered qualified-text parsing, `$expr:` debug-text
  semantic authority, string/string_view semantic lookup parameters, fallback
  spelling, expectation downgrades, or named-test shortcuts.
- If a handoff requires a cross-module carrier outside parser/Sema ownership,
  record a separate metadata idea instead of expanding idea 139.
- Nested static-member owner metadata still uses Sema's existing owner key:
  owner namespace context plus final owner `TextId`. This packet repairs the
  parser carrier for available nested record identity; it does not add an
  enclosing-record path to the Sema key.
- HIR template value-argument evaluation now has a lowerer-owned recursive
  fallback for structured expression nodes after generic consteval misses. Keep
  future extensions semantic and carrier-driven; do not broaden `$expr:` debug
  text compatibility to cover structured-carrier misses.
- The `cpp_positive_sema_template_nttp_default_runtime_cpp` proof case covers
  the default-NTTP side of this path: static-member value expressions must carry
  enclosing type bindings when evaluating defaults such as
  `is_void<T>::value || is_void<T>::value`.
- The current blocker is specifically the parser-side concrete field clone for
  nested-owner value arguments with type bindings/defaults: HIR receives
  `Wrap_T_int::data` as `Buffer_V_0`, so `value_args.cpp` never sees a
  structured `TemplateArgRef::type.array_size_expr` carrier for that field.

## Proof

Ran the delegated proof command after the blocked probe:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp|cpp_hir_template_struct_arg_materialization|cpp_hir_template_value_arg_static_member_trait)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `888/888` matched tests.
Final proof log path: `test_after.log`.
