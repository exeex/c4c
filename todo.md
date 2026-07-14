# Current Packet

Status: Active
Source Idea Path: ideas/open/767_lir_computed_goto_table_element_pointer_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Hand off the resolved capability to 764

## Just Finished

- Completed 767 Step 5: `IndexExpr` now has an operand-carrying structured
  lvalue route. Supported static-global and current-function-local table bases
  retain authority into a typed i64 indexed GEP with a valid result ID, and the
  rvalue load consumes that exact GEP operand and gets its own valid result ID.
  The legacy fallback remains for unsupported bases/indices, and accepted 766
  SSA behavior was not otherwise changed.
- Activated static-local and local computed-goto table-element frontend-LIR
  contracts, including raw/invalid/foreign base, raw index, missing GEP result,
  and foreign/nonmatching load-pointer rejection. The fixtures strip the
  unpublished `IndirBr` only before isolated verifier checks; no `IndirBr`
  publication is claimed.

## Suggested Next

- Step 6: hand the completed producer/result contract and direct proof to the
  supervisor for lifecycle recording and return to 764 Step 1 only after
  acceptance; do not publish or rerun the parent carrier here.

## Watchouts

- The statement seam remains out of scope: no `IndirBrStmt`/`addr_value`,
  Raw-BIR/importer, backend, verifier, or testcase-specific changes were made.
- Computed-goto fixtures are lowered without the global verifier only to
  observe the table producer; their isolated GEP/load modules remove the
  unpublished indirect branch before verifier mutation checks.

## Proof

- Fresh `cmake --build --preset default` succeeded. The direct focused
  `./build/tests/frontend/frontend_lir_call_type_ref_test` passed. The exact
  delegated command `ctest --test-dir build -j --output-on-failure -R
  '^frontend_cxx_' > test_after.log` passed (1/1); `test_after.log` is the
  proof log. `git diff --check` passed.
