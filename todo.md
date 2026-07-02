Status: Active
Source Idea Path: ideas/open/518_bir_core_model_cleanup_umbrella.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Inventory Implementation Families

# Current Packet

## Just Finished

Step 3 - Inventory Implementation Families completed the implementation-family
map in `docs/bir_core_cleanup/implementation_inventory.md`.

The inventory records exact clang-tool commands and the one non-blocking
`memory/coordinator.cpp` function-signature limitation, documents raw-text
fallback commands used only for line and private-surface anchoring, and maps
pure helpers in `bir.cpp`, printer-only helpers, validator-only helpers,
LIR-to-BIR-only surfaces, route-specific memory/publication/comparison
implementations, caller/callee dependency observations, and low/medium/late
movement-risk regions.

## Suggested Next

Delegate Step 4: draft the destination map for the inventoried families,
preferring existing focused files where ownership is already clear and keeping
broad model types plus route6/facade surfaces late.

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
- Route6 call publication is the highest-risk route implementation family
  because it composes route1, route2, route3, route4, route5, call ABI, and
  tail query helpers.
- `bir_printer.cpp` and `bir_validate.cpp` are already clear implementation
  owners for their private helpers; Step 4 should distinguish those existing
  owners from public API declarations.
- Treat `lir_to_bir/` as a private lowering subsystem; do not propose moving
  public BIR model records into that tree unless they are proven lowering-only.

## Proof

Proof command:

```sh
git diff --check && test -s docs/bir_core_cleanup/implementation_inventory.md
```

Result: passed.

No implementation files were changed. No `test_after.log` was produced because
the delegated proof command was a docs/lifecycle sanity check and did not write
one.
