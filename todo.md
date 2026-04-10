# Parser Symbol Tables And Speculative State Compaction Todo

Status: Active
Source Idea: ideas/open/03_parser_symbol_tables_and_speculative_state_compaction.md
Source Plan: plan.md

## Active Item

- Step 3: migrate the next narrow speculative/state-management call sites onto
  grouped parser symbol-table accessors instead of direct top-level member
  storage assumptions, starting with typedef/value registration and
  namespace-aware lookup helpers
- Current slice: continue Step 3 by shrinking the remaining parser-local
  compatibility-reference use in `src/frontend/parser/parser_types_struct.cpp`
  and then reassess whether the next bounded typedef-access cleanup should stay
  in record parsing or move to another parser type-parsing surface
- Iteration target: migrate the remaining `TemplateParamGuard` dependency on
  the compatibility `typedefs_` reference in
  `src/frontend/parser/parser_types_struct.cpp` onto a parser-local helper or
  grouped-state-aware guard without widening scope

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

## Next Intended Slice

- Finish the remaining `parser_types_struct.cpp` compatibility-reference use by
  replacing the `TemplateParamGuard` dependency on raw `typedefs_` storage with
  a parser-local/state-aware helper
- After that, reassess whether the next Step 3 typedef-access cleanup should
  stay in record parsing or move to another localized parser type-parsing file
- Leave namespace-state grouping for a separate slice unless it becomes
  necessary for the chosen accessor migration

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
  now use parser-local typedef helpers; the remaining raw accesses are mostly
  compatibility-reference uses, with `parser_types_struct.cpp` now reduced to
  the `TemplateParamGuard` `typedefs_` reference and the broader repo still
  carrying non-Step-3 direct typedef map consumers outside this slice
