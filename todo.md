Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md
Source Plan: plan.md

# Current Packet

## Just Finished

Step 1 landed: `src/frontend/string_id_table.hpp` now defines a distinct
`LinkNameId` / `LinkNameTable` contract backed by the shared `TextTable`,
`src/frontend/lexer/token.hpp` now consumes the shared `TextId` / `TextTable`
definitions from that helper layer, and `tests/frontend/frontend_parser_tests.cpp`
adds focused coverage for interning, lookup, invalid handling, and text-table
reuse.

## Suggested Next

Start Step 2 from `plan.md`: thread `LinkNameTable` ownership into the HIR
materialization boundary and add parallel `link_name_id` fields on the first
HIR carriers for emitted symbols, keeping this slice focused on HIR-side
interning rather than LIR or backend consumer migration.

## Watchouts

- keep `LinkNameTable` backed by the existing TU `TextTable`; do not add a
  second owning string store
- do not merge `LinkNameId` with `TextId` or parser/source-atom `SymbolId`
- HIR, not parser ingestion or backend emission, owns the first interning of
  final logical symbol names
- the first packet should stay on the table/id-space boundary; do not sprawl
  into broad HIR/LIR/backend rewrites before the contract exists
- avoid testcase-overfit proof or brittle emitted-text substring matching as a
  substitute for a real id path

## Proof

Build: `cmake --build --preset default -j4`
Narrow proof: `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
