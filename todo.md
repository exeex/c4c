Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md
Source Plan: plan.md

# Current Packet

## Just Finished

Plan activation only. No executor packet has landed yet.

## Suggested Next

Start with Step 1 from `plan.md`: inspect the current `TextTable` and adjacent
id-table helpers, add the `LinkNameId` / `LinkNameTable` data model, and keep
the first slice narrowly focused on the semantic id space rather than HIR/LIR
consumer migration. Choose proof that builds the repo and exercises the
narrowest path covering the new table API.

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

Not run yet.
