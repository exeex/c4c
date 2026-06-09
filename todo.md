Status: Active
Source Idea Path: ideas/open/134_shared_select_chain_same_block_dependency_queries.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Cover The Companion Same-Block Or Select-Chain Relationship

# Current Packet

## Just Finished

Step 3 added `PreparedSelectChainDependencyQuery` in
`src/backend/prealloc/prepared_lookups.hpp` with shared-query overloads for
direct-global select-chain dependency and scalar select-chain materialization
in `src/backend/prealloc/select_chain_lookups.cpp`.

`src/backend/mir/aarch64/codegen/dispatch_producers.cpp` now routes
`select_chain_contains_direct_global_load` through the shared query object for
both prepared lookup storage and its existing local fallback lookup build.
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` now routes
scalar select-chain materialization through the same query shape. AArch64
emission, hazard, scratch, and materialization-order policy were left local and
unchanged.

## Suggested Next

Step 4 should run the backend regression guard with matching before/after logs
and compare them before closure.

## Watchouts

- Do not add named-case select-chain or direct-global shortcuts.
- Do not move AArch64 emission, hazard, scratch, or materialization-order
  policy into shared code.
- Do not weaken test expectations or mark supported paths unsupported.
- The query overloads intentionally package dependency inputs only; target
  policy remains in the AArch64 call sites.
- Other call sites in `alu.cpp`, `calls.cpp`, `call_plans.cpp`, and prepared
  printer code still use the lower-level parameterized APIs because they were
  outside this packet or already target/shared-owner specific.

## Proof

Ran exactly `(cmake --build --preset default && ctest --test-dir build -R
'^backend_' --output-on-failure) > test_after.log 2>&1`.

Result: passed. `test_after.log` reports `100% tests passed, 0 tests failed out
of 179`.
