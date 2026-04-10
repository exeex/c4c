# Parser Symbol Tables And Speculative State Compaction Todo

Status: Active
Source Idea: ideas/open/03_parser_symbol_tables_and_speculative_state_compaction.md
Source Plan: plan.md

## Active Item

- Step 3: migrate the next narrow speculative/state-management call sites onto
  grouped parser symbol-table accessors instead of direct top-level member
  storage assumptions

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
- Validated with targeted parser tests plus full `ctest --test-dir build -j4
  --output-on-failure`:
  3342/3342 tests passed

## Next Intended Slice

- Add narrow parser-local accessor helpers for typedef and variable-type lookup
  mutations that still reach through the compatibility reference members
- Migrate one coherent parser call-site family, likely declaration/typedef
  registration or tentative type-probe helpers, onto those accessors
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
