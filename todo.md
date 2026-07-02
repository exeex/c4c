Status: Active
Source Idea Path: ideas/open/518_bir_core_model_cleanup_umbrella.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Inventory Declaration Families

# Current Packet

## Just Finished

Step 2 - Inventory Declaration Families completed the `bir.hpp` declaration
family inventory in `docs/bir_core_cleanup/declaration_inventory.md`.

The inventory records exact clang-tool commands, notes the one non-blocking
`RouteIndexContext` type-reference miss, documents narrow raw-text fallback
commands for line anchoring, and adds a declaration-family table with current
location, likely owner, movement risk, and do-not-move-yet notes for the core
model, value/type/module/function/block/instruction definitions,
memory-address/object-storage authority, comparison/scalar producers,
select-chain/direct-global dependency, compatibility helpers, route-specific
memory analysis, validation facade, and printer/render surfaces.

## Suggested Next

Delegate Step 3: inventory implementation families in `src/backend/bir/bir.cpp`,
`src/backend/bir/bir_printer.cpp`, `src/backend/bir/bir_validate.cpp`, and
`src/backend/bir/lir_to_bir/`, then extend the durable cleanup analysis with an
implementation-family map and low/medium/late movement risk notes.

## Watchouts

- This idea is analysis-only; do not move declarations or definitions.
- Continue using AST-backed structure queries before raw long-file reading.
- Keep idea 422 producer implementation work separate from cleanup planning.
- Do not change tests, expectations, unsupported diagnostics, or pass/fail
  accounting.
- Route3 memory access is directly used by route6 call publication source
  analysis; do not propose route3 extraction without recording that coupling.
- `Value`, `Inst`, `Block`, `Function`, and `Module` have broad fanout and
  should remain late/no-move candidates until lower-risk route families are
  classified.
- `RouteIndexContext` is not a current `bir.hpp` type; the relevant facade
  declaration is `RouteIndexReferenceFacade`.
- No exact `ObjectStorage` declaration family was found in `bir.hpp`; storage
  authority is represented through memory extent/range/provenance records,
  local slots, globals, and module/function storage vectors.

## Proof

Proof command:

```sh
git diff --check && test -s docs/bir_core_cleanup/declaration_inventory.md
```

Result: passed.

No implementation files were changed. No `test_after.log` was produced because
the delegated proof command was a docs/lifecycle sanity check and did not write
one.
