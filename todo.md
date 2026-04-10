# Parser Symbol Tables And Speculative State Compaction Todo

Status: Active
Source Idea: ideas/open/03_parser_symbol_tables_and_speculative_state_compaction.md
Source Plan: plan.md

## Active Item

- Step 3: migrate the next narrow speculative/state-management call sites onto
  grouped parser symbol-table accessors instead of direct top-level member
  storage assumptions, starting with typedef/value registration and
  namespace-aware lookup helpers
- Current slice: continue migrating the remaining direct typedef lookup and
  mutation sites in the type parser, especially dependent member typedef
  caching and struct-member typedef registration paths in
  `parser_types_declarator.cpp` and `parser_types_base.cpp`

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

## Next Intended Slice

- Continue migrating the remaining direct typedef mutation and cache-population
  sites in `parser_types_declarator.cpp` and `parser_types_base.cpp`,
  especially dependent member typedef preservation and struct-member typedef
  registration
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
- Read-only type-parser probes now use parser-local visible typedef/value
  helpers; the remaining direct accesses are the write paths and a few
  typedef-map plumbing helpers that still intentionally operate on raw storage
