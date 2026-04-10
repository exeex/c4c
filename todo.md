# Parser Symbol Tables And Speculative State Compaction Todo

Status: Active
Source Idea: ideas/open/03_parser_symbol_tables_and_speculative_state_compaction.md
Source Plan: plan.md

## Active Item

- Step 3: migrate the next narrow speculative/state-management call sites onto
  grouped parser symbol-table accessors instead of direct top-level member
  storage assumptions, continuing into the next parser-local typedef/template
  helper surface after the dependent-typename owner cleanup
- Current slice: remove the remaining raw typedef-map threading from the
  template specialization-pattern helpers by routing
  `select_template_struct_pattern_for_args()` and the shared
  `types_helpers.hpp` pattern matcher through parser-owned typedef resolution
  and compatibility helpers
- Iteration target: keep reducing parser call sites that still need raw
  typedef-map parameters, without widening into namespace-state grouping,
  non-parser storage redesign, or backend work

## Completed Items

- Activated `plan.md` from
  `ideas/open/03_parser_symbol_tables_and_speculative_state_compaction.md`
- Inventoried the first speculative parsing slice as the heavy snapshot symbol
  table surface: `typedefs_`, `user_typedefs_`, `typedef_types_`, and
  `var_types_`
- Grouped that surface under `Parser::ParserSymbolTables` in
  `src/frontend/parser/parser.hpp`
- Updated heavy snapshot save/restore in
  `src/frontend/parser/parser_support.cpp` to move the grouped symbol-table
  state as one unit
- Updated builtin typedef seeding in `src/frontend/parser/parser_core.cpp` to
  initialize through the grouped symbol-table state
- Added parser-local typedef/value accessor helpers in
  `src/frontend/parser/parser.hpp` and `src/frontend/parser/parser_core.cpp`
- Migrated typedef registration and namespace `using` import paths in
  `src/frontend/parser/parser_declarations.cpp` onto those accessors
- Routed typedef/value namespace lookup helpers in
  `src/frontend/parser/parser_core.cpp` through the new accessors
- Added frontend regression
  `tests/cpp/internal/postive_case/namespace_using_decl_typedef_and_value_frontend.cpp`
  to cover namespace `using` imports of typedef-backed types and values
- Migrated `src/frontend/parser/parser_statements.cpp` local alias
  registration, `if` declaration probing, and block-scope
  declaration-vs-expression typedef/value checks onto parser-local helper
  accessors
- Added parse-only regression
  `tests/cpp/internal/parse_only_case/local_using_alias_statement_probe_parse.cpp`
  plus `cpp_parse_local_using_alias_statement_probe_dump` in
  `tests/cpp/internal/InternalTests.cmake` to cover local alias registration,
  shadowed value bindings, and qualified static member call disambiguation
- Validated with targeted parser tests plus full `ctest --test-dir build -j4
  --output-on-failure`:
  3342/3342 tests passed
- Revalidated with targeted frontend tests plus full
  `ctest --test-dir build -j8 --output-on-failure` and regression guard:
  3343/3343 tests passed, no new failures
- Revalidated this slice with targeted parser tests plus full
  `ctest --test-dir build -j8 --output-on-failure` and regression guard:
  3344/3344 tests passed, no new failures
- Added visible typedef/value lookup helpers in
  `src/frontend/parser/parser.hpp` and `src/frontend/parser/parser_core.cpp`
  so read-only type-parser probes can resolve through parser-local accessors
- Migrated the read-only typedef/value lookup probes in
  `src/frontend/parser/parser_types_base.cpp` and
  `src/frontend/parser/parser_types_declarator.cpp` onto those helpers for
  `is_type_start()`, template type-argument fast paths, `typeof`-style
  identifier lookup, and typedef resolution reads
- Added parse-only regression
  `tests/cpp/internal/postive_case/namespace_using_decl_template_typedef_type_arg_parse.cpp`
  and registered it in `tests/cpp/internal/InternalTests.cmake` to cover
  namespace-imported typedef use in declaration parsing and template type
  arguments
- Revalidated this slice with targeted type-parser tests plus full
  `ctest --test-dir build -j8 --output-on-failure` and regression guard:
  3345/3345 tests passed, no new failures
- Added parser-local typedef cache and scoped struct-member typedef helpers in
  `src/frontend/parser/parser.hpp` and `src/frontend/parser/parser_core.cpp`
  for type-parser mutation sites that should not directly reach into the
  grouped symbol tables
- Migrated dependent member typedef caching in
  `src/frontend/parser/parser_types_declarator.cpp` onto those helpers,
  including the preserved `Owner::type` cache path and dependent member typedef
  resolution cache fill
- Migrated template-instantiation scoped member typedef registration in
  `src/frontend/parser/parser_types_base.cpp` onto the new scoped-member helper
  and routed nearby typedef reads through existing parser-local lookup helpers
- Added parse-only regression
  `tests/cpp/internal/postive_case/template_member_typedef_cache_roundtrip_parse.cpp`
  and registered it in `tests/cpp/internal/InternalTests.cmake` to cover both
  dependent member typedef caching and concrete `Template<Args>::type`
  registration
- Revalidated this slice with targeted template/member typedef parser tests,
  `cpp_hir_template_member_owner_resolution`, full
  `ctest --test-dir build -j8 --output-on-failure`, and regression guard:
  3346/3346 tests passed, no new failures
- Added parse-only dump regression
  `tests/cpp/internal/parse_only_case/record_member_typedef_using_dump_parse.cpp`
  plus `cpp_parse_record_member_typedef_using_dump` in
  `tests/cpp/internal/InternalTests.cmake` to assert record-body `using` and
  multi-declarator `typedef` members stay visible to the following field
  declarations
- Migrated record-body `using` alias registration and `typedef` member
  registration in `src/frontend/parser/parser_types_struct.cpp` onto
  `register_typedef_binding()` and
  `register_struct_member_typedef_binding()`, preserving member typedef vectors
  and function-pointer typedef metadata without direct grouped-storage writes
- Revalidated this slice with targeted record-member parser tests plus full
  `ctest --test-dir build -j8 --output-on-failure` and regression guard:
  3347/3347 tests passed, no new failures
- Added parser-local typedef/tag helper operations in
  `src/frontend/parser/parser.hpp` and `src/frontend/parser/parser_core.cpp`
  for plain typedef-name visibility, synthesized template-parameter typedef
  seeding, injected record/enum tag registration, and teardown cleanup
- Migrated `src/frontend/parser/parser_types_struct.cpp` template-member
  prelude seeding, template-scope cleanup, record-body self-type exposure,
  forward-declaration tag injection, record finalization registration, and
  enum tag registration onto those parser-local helpers
- Added parse-only regression
  `tests/cpp/internal/postive_case/record_template_prelude_and_tag_registration_parse.cpp`
  to cover templated record-member prelude seeding together with record and
  enum tag registration paths
- Revalidated this slice with targeted record/type parser tests plus full
  `ctest --test-dir build -j8 --output-on-failure` and regression guard:
  3348/3348 tests passed, no new failures
- Added parser-local typedef-chain resolution in
  `src/frontend/parser/parser.hpp` and `src/frontend/parser/parser_core.cpp`
  so record/enum parsing can resolve typedef-backed type metadata without
  direct grouped typedef-map access
- Migrated the remaining direct `typedef_types_` reads in
  `src/frontend/parser/parser_types_struct.cpp` onto parser-local helpers for
  record enum member underlying-base propagation, template-origin setup, and
  enum fixed underlying-type resolution
- Added parse-only regression
  `tests/cpp/internal/postive_case/record_template_enum_underlying_typedef_parse.cpp`
  plus `cpp_parse_record_template_enum_underlying_typedef_dump` in
  `tests/cpp/internal/InternalTests.cmake` to cover a template-specialized
  record body that uses a local typedef as an enum fixed underlying type
- Revalidated this slice with targeted record/enum parser tests, clean rebuild,
  full `ctest --test-dir build -j8 --output-on-failure`, and regression guard:
  3350/3350 tests passed, no new failures
- Added parser-owned `RecordTemplatePreludeGuard` in
  `src/frontend/parser/parser.hpp` so record-member template prelude cleanup no
  longer depends on raw compatibility references at the call site
- Migrated the remaining `TemplateParamGuard` teardown path in
  `src/frontend/parser/parser_types_struct.cpp` onto that parser-owned guard,
  removing the last direct `typedefs_` and `template_scope_stack_`
  compatibility-reference use from the record parser slice
- Added parse-only dump regression
  `tests/cpp/internal/parse_only_case/record_member_template_friend_cleanup_parse.cpp`
  plus `cpp_parse_record_member_template_friend_cleanup_dump` in
  `tests/cpp/internal/InternalTests.cmake` to cover template-member prelude
  cleanup when a friend-member path returns early
- Revalidated this slice with targeted record-member parser tests plus full
  `ctest --test-dir build -j8 --output-on-failure` and regression guard:
  3351/3351 tests passed, no new failures
- Added parser-owned `TemplateDeclarationPreludeGuard` in
  `src/frontend/parser/parser.hpp` so template declaration prelude cleanup can
  stay local to the declaration parser instead of manually erasing grouped
  typedef storage at the call site
- Migrated `src/frontend/parser/parser_declarations.cpp` template type-parameter
  seeding/teardown onto `register_synthesized_typedef_binding()` plus the new
  declaration prelude guard, and routed template primary struct name/type
  registration through `register_tag_type_binding()`
- Added parse-only regression
  `tests/cpp/internal/postive_case/template_declaration_prelude_cleanup_parse.cpp`
  and registered it in `tests/cpp/internal/InternalTests.cmake` to cover
  template-parameter default visibility together with cleanup before a
  following ordinary `struct T` declaration
- Revalidated this slice with
  `ctest --test-dir build -R template_declaration_prelude_cleanup_parse
  --output-on-failure`, full `ctest --test-dir build -j8
  --output-on-failure`, and regression guard:
  3352/3352 tests passed, no new failures
- Migrated the remaining declaration-parser read-only `typedef_types_` probes
  in `src/frontend/parser/parser_declarations.cpp` onto parser-local typedef
  helpers for constructor-init ambiguity checks, alias-template metadata
  capture, and top-level qualified typedef resolution
- Added parse-only regression
  `tests/cpp/internal/postive_case/template_alias_qualified_typedef_resolution_parse.cpp`
  and registered it in `tests/cpp/internal/InternalTests.cmake` to cover
  alias-template reconstruction together with qualified typedef base
  resolution during declaration parsing
- Revalidated this slice with targeted declaration-parser tests,
  `ctest --test-dir build -R template_alias_qualified_typedef_resolution_parse
  --output-on-failure`,
  `ctest --test-dir build -R 'template_declaration_prelude_cleanup_parse|eastl_slice7b_template_using_alias_parse|qualified_type_spelling_shared_parse|qualified_type_start_probe_parse' --output-on-failure`,
  full `ctest --test-dir build -j8 --output-on-failure`, and regression
  guard:
  3353/3353 tests passed, no new failures
- Added parser-local user-typedef compatibility helpers in
  `src/frontend/parser/parser.hpp` and `src/frontend/parser/parser_core.cpp`
  so declaration-parser typedef redefinition checks no longer read raw
  `user_typedefs_` or pass `typedef_types_` directly at the call site
- Migrated the remaining top-level and local typedef redefinition compatibility
  checks in `src/frontend/parser/parser_declarations.cpp` onto
  `has_conflicting_user_typedef_binding()`
- Added negative C regression
  `tests/c/internal/negative_case/bad_typedef_local_redefine_conflict.c` to
  cover local-scope and multi-declarator conflicting typedef redefinitions
- Revalidated this slice with
  `ctest --test-dir build -R 'negative_tests_bad_typedef_(redefine_conflict|local_redefine_conflict)' --output-on-failure`,
  `ctest --test-dir build -R 'template_alias_qualified_typedef_resolution_parse|negative_tests_bad_typedef_(redefine_conflict|local_redefine_conflict)' --output-on-failure`,
  full `ctest --test-dir build -j8 --output-on-failure`, and regression
  guard:
  3354/3354 tests passed, no new failures
- Migrated the remaining declaration-parser `typeof(var)` bookkeeping writes in
  `src/frontend/parser/parser_declarations.cpp` onto
  `register_var_type_binding()`, removing the last direct grouped symbol-table
  writes from the declaration parser slice
- Added positive C regression
  `tests/c/internal/positive_case/ok_typeof_decl_var_binding.c` to cover both
  global and local `typeof(var)` resolution after ordinary declarations
- Revalidated this slice with
  `ctest --test-dir build -R 'positive_sema_ok_typeof_decl_var_binding' --output-on-failure`,
  clean rebuild, full `ctest --test-dir build -j8 --output-on-failure`, and
  regression guard:
  3355/3355 tests passed, no new failures
- Added parser-local struct-like typedef resolution in
  `src/frontend/parser/parser.hpp` and `src/frontend/parser/parser_core.cpp`
  so dependent-typename owner discovery can collapse typedef chains without
  directly reaching into grouped typedef-map storage
- Migrated the remaining direct
  `resolve_typedef_chain(..., typedef_types_)` use in
  `src/frontend/parser/parser_types_declarator.cpp` onto
  `resolve_struct_like_typedef_type()` for qualified dependent member-typedef
  owner resolution
- Added HIR regression
  `tests/cpp/internal/hir_case/template_alias_member_owner_hir.cpp` plus
  `cpp_hir_template_alias_member_owner` in
  `tests/cpp/internal/InternalTests.cmake` to cover alias-template owners used
  in dependent `typename ...::value_type` spellings
- Revalidated this slice with
  `ctest --test-dir build -R 'cpp_hir_template_alias_member_owner|cpp_hir_template_member_owner_(chain|decl_and_cast|field_and_local|signature_local)|template_alias_qualified_typedef_resolution_parse|qualified_type_spelling_shared_parse' --output-on-failure`,
  full `ctest --test-dir build -j8 --output-on-failure`, and regression
  guard:
  3358/3358 tests passed, no new failures; previously failing
  `cpp_hir_template_visible_typedef_deferred_nttp` now passes
- Added parser-owned template helper wrappers in
  `src/frontend/parser/parser.hpp` and `src/frontend/parser/parser_core.cpp`
  for typedef compatibility checks and template specialization-pattern
  selection so the template helper slice no longer needs raw grouped-storage
  reads at the migrated call sites
- Migrated `src/frontend/parser/parser_types_template.cpp` typedef decoding,
  deferred NTTP type-token resolution, and template-pattern selection onto
  `find_typedef_type()`, `find_visible_typedef_type()`,
  `resolve_typedef_type_chain()`, `are_types_compatible()`, and
  `select_template_struct_pattern_for_args()`
- Migrated the remaining direct typedef probes in
  `src/frontend/parser/types_helpers.hpp` onto parser-local lookup helpers for
  known-type heads and qualified typedef probes
- Added positive C++ regression
  `tests/cpp/internal/postive_case/template_visible_typedef_type_arg_parse.cpp`
  and registered it in `tests/cpp/internal/InternalTests.cmake` to cover a
  default template type argument using `lib::wrap<app::value_type>::type`
  together with an explicit `holder<value_type>` instantiation
- Revalidated this slice with
  `ctest --test-dir build -R 'cpp_positive_sema_(template_visible_typedef_type_arg_parse_cpp|template_alias_qualified_typedef_resolution_parse_cpp|namespace_using_decl_template_typedef_type_arg_parse_cpp)' --output-on-failure`,
  full `ctest --test-dir build -j8 --output-on-failure`, and regression
  guard:
  3356/3356 tests passed, no new failures
- Migrated the remaining shared template specialization-pattern helper
  signatures in `src/frontend/parser/types_helpers.hpp` and
  `src/frontend/parser/parser_types_template.cpp` to take `Parser` and use
  parser-owned typedef-chain resolution plus type-compatibility helpers
  instead of threading raw typedef maps through the helper boundary
- Added positive C++ regression
  `tests/cpp/internal/postive_case/template_specialization_typedef_chain_parse.cpp`
  and registered it in `tests/cpp/internal/InternalTests.cmake` to cover a
  partial specialization selected through a typedef-chain template argument
  used in both `__is_same(...)` and a functional-cast expression
- Revalidated this slice with
  `ctest --test-dir build -R 'cpp_positive_sema_(template_specialization_typedef_chain_parse|template_specialization_member_typedef_trait_parse|template_visible_typedef_type_arg_parse)_cpp' --output-on-failure`,
  `ctest --test-dir build -R 'cpp_hir_template_alias_member_owner|cpp_hir_template_member_owner_(chain|decl_and_cast|field_and_local|signature_local)' --output-on-failure`,
  full `ctest --test-dir build -j8 --output-on-failure`, and regression
  guard:
  3359/3359 tests passed, no new failures
- Added parser-owned record-constructor and typedef-compatibility helpers in
  `src/frontend/parser/parser.hpp` and `src/frontend/parser/parser_core.cpp`
  so parser expression and type-base call sites can stop threading raw
  `typedef_types_` access for record-like functional casts and builtin
  type-trait comparisons
- Migrated the remaining direct `select_template_struct_pattern(...)` call
  sites in `src/frontend/parser/parser_types_base.cpp` onto
  `select_template_struct_pattern_for_args()`, and replaced nearby direct
  typedef-chain reads with parser-local lookup helpers
- Migrated `src/frontend/parser/parser_expressions.cpp` builtin type-trait
  checks, typedef-owned functional-cast detection, and qualified owner-type
  probes onto parser-local helpers instead of direct grouped typedef-map reads
- Added positive C++ regression
  `tests/cpp/internal/postive_case/template_specialization_member_typedef_trait_parse.cpp`
  and registered it in `tests/cpp/internal/InternalTests.cmake` to cover a
  specialization-selected member typedef used in both `__is_same(...)` and a
  functional-cast expression
- Revalidated this slice with
  `ctest --test-dir build -R 'cpp_positive_sema_(builtin_trait_is_same_frontend|template_specialization_member_typedef_trait_parse|template_visible_typedef_type_arg_parse|typedef_owned_functional_cast_parse)_cpp' --output-on-failure`,
  full `ctest --test-dir build -j8 --output-on-failure`, and regression
  guard:
  3357/3357 tests passed, no new failures

## Next Intended Slice

- Audit whether the parser-side typedef/value grouping work is ready to retire
  some `parser.hpp` compatibility-reference members now that parser
  implementation files no longer thread raw typedef maps through template
  helper signatures
- If that cleanup is still too wide for the next slice, record the remaining
  compatibility-only surfaces explicitly and shift Step 4 toward closing the
  typedef/value-table wrapper work as complete
- Keep namespace-state grouping deferred to a later slice unless it becomes
  necessary for one of those parser-local wrapper migrations

## Blockers

- None

## Resume Notes

- Repo lifecycle uses `todo.md`; ignore stale `plan_todo.md` references in the
  skill docs
- Default priority is the lowest-numbered open idea, which is now active
- The current refactor preserves all existing direct call sites via compatibility
  reference members while moving ownership and heavy snapshots under
  `ParserSymbolTables`
- This slice centralizes typedef/value registration and namespace-aware lookup
  behind parser helpers and now covers the statement parser's local alias and
  declaration-disambiguation probes, but intentionally leaves many type-parser
  mutation and cache-population paths for later Step 3 slices
- Read-only type-parser probes and the targeted dependent-member cache writes
  now use parser-local typedef helpers, and `parser_types_struct.cpp` no
  longer carries raw compatibility-reference cleanup for the member-template
  prelude path
- `parser_declarations.cpp` no longer carries direct read-only
  `typedef_types_.count/find` probes for alias-template reconstruction,
  constructor-init ambiguity checks, or qualified typedef resolution; the
  typedef redefinition compatibility checks now go through
  `has_conflicting_user_typedef_binding()`, and the declaration-parser
  `typeof(var)` bookkeeping writes now go through
  `register_var_type_binding()` as well
- `types_helpers.hpp` and `parser_types_template.cpp` now route their typedef
  decoding through parser-local helpers, and the shared template
  specialization matcher now resolves typedef chains via `Parser` instead of
  receiving raw typedef maps directly
- `parser_types_base.cpp`, `parser_types_template.cpp`, and
  `parser_expressions.cpp` now use parser-owned template-selection,
  typedef-compatibility, and record-constructor helpers; the likely next
  cleanup is narrowing or removing compatibility-reference members in
  `parser.hpp`, not more raw typedef-map threading in parser implementation
  files
