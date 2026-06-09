Status: Active
Source Idea Path: ideas/open/137_select_chain_public_owner_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move declarations to the selected owner

# Current Packet

## Just Finished

Completed plan Step 2 declaration-owner move for the select-chain lookup API.
Added `src/backend/prealloc/select_chain_lookups.hpp` as the paired owner for `PreparedSelectChainDependencyQuery`, `PreparedStoreSourceDirectGlobalSelectChainDependency`, `PreparedScalarSelectChainMaterialization`, and the select-chain lookup declarations. Removed the duplicate/misplaced declarations and result types from `prepared_lookups.hpp` and `publication_plans.hpp`, then updated `select_chain_lookups.cpp` to include its paired header. Existing facade headers include the new owner so current consumers continue to compile without semantic or AArch64 materialization behavior changes.

## Suggested Next

Proceed to the next cleanup packet by making direct select-chain API consumers include `select_chain_lookups.hpp` explicitly, then consider whether the temporary transitive includes from the old facades can be reduced safely.

## Watchouts

`select_chain_lookups.hpp` intentionally does not include `publication_plans.hpp`. This packet preserved old facade reachability by including the new owner from `prepared_lookups.hpp` and `publication_plans.hpp`; follow-up include cleanup should remove only the unnecessary transitive exposure after all direct consumers are explicit.

## Proof

Ran the delegated proof command:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -R "^backend_" --output-on-failure; } 2>&1 | tee test_after.log'`

Result: passed. Build completed and `ctest -R "^backend_"` reported 179/179 tests passed. Proof log: `test_after.log`.
