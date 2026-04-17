Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md
Source Plan: plan.md

# Current Packet

## Just Finished

Extended Step 3 from the first emitted-function path into the direct emitted
global path: `codegen::lir::LirGlobal` now carries a parallel
`link_name_id`, HIR-to-LIR lowering forwards `hir::GlobalVar.link_name_id`
through both standard and flexible-array global lowering, and
`tests/frontend/frontend_hir_tests.cpp` proves the forwarded id survives on
both the function and global LIR carriers in one real HIR fixture.

## Suggested Next

Decide whether Step 3 needs one more explicit LIR carrier for
template-specialization metadata, or whether the route is now strong enough to
start Step 4 by threading shared link-name lookup into a late text-emitting
consumer boundary.

## Watchouts

- `hir::Module` currently owns the link-name text table for this slice; keep
  the source-idea goal in view and avoid hardening that into a permanent
  second string store once the shared TU text-table boundary is available
- keep `LinkNameId` distinct from both `TextId` and parser/source-atom
  `SymbolId`; the new HIR fields are parallel carriers, not replacements
- late consumer lookup is still string-based today even though both direct
  emitted function/global LIR carriers now preserve `LinkNameId`
- keep forwarding explicit ids through LIR carriers rather than treating
  `name` strings as the semantic source of truth
- avoid testcase-overfit proof or brittle emitted-text substring matching as a
  substitute for a real id path

## Proof

Build: `cmake --build --preset default -j4`
Narrow proof: `ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`
Log: `test_after.log`
