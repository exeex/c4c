Status: Active
Source Idea Path: ideas/open/137_select_chain_public_owner_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Update consumers to include the owner boundary

# Current Packet

## Just Finished

Completed plan Step 3 consumer include-boundary cleanup for the select-chain
lookup API. Direct users of `PreparedSelectChainDependencyQuery`,
`PreparedStoreSourceDirectGlobalSelectChainDependency`,
`PreparedScalarSelectChainMaterialization`, and the `find_prepared_*select_chain*`
helpers now include `src/backend/prealloc/select_chain_lookups.hpp`
explicitly. Removed the temporary select-chain owner exposure from
`prepared_lookups.hpp` and `publication_plans.hpp`; kept `module.hpp` as the
single justified aggregate include because backend module-surface consumers
compile against the prepared module umbrella and may call the public select-chain
lookup API through that umbrella.

## Suggested Next

Supervisor should choose the next plan packet; this slice leaves no known
include-boundary blocker in the touched select-chain consumer set.

## Watchouts

`module.hpp` is intentionally the remaining aggregate exposure for the public
prepared module surface. `prepared_lookups.hpp` and `publication_plans.hpp` no
longer expose `select_chain_lookups.hpp` transitively.

## Proof

Ran the delegated proof command:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -R "^backend_" --output-on-failure; } 2>&1 | tee test_after.log'`

Result: passed. Build completed and `ctest -R "^backend_"` reported 179/179 tests passed. Proof log: `test_after.log`.
