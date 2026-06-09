Status: Active
Source Idea Path: ideas/open/134_shared_select_chain_same_block_dependency_queries.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Run Backend Regression Guard

# Current Packet

## Just Finished

Step 4 ran the backend regression guard after the same-block and select-chain
shared query facades landed.

The finalized shared surfaces are `PreparedSameBlockValueMaterializationQuery`
for same-block materialization inputs and
`PreparedSelectChainDependencyQuery` for select-chain dependency inputs. AArch64
emission, hazard, scratch, and materialization-order policy stayed local to the
target call sites; the shared facades only package dependency/materialization
query inputs.

## Suggested Next

Supervisor should compare the canonical regression logs and decide whether this
completed runbook is ready for lifecycle closure or follow-up planning.

## Watchouts

- Do not add named-case select-chain or direct-global shortcuts.
- Do not move AArch64 emission, hazard, scratch, or materialization-order
  policy into shared code.
- Do not weaken test expectations or mark supported paths unsupported.
- The query overloads intentionally package dependency inputs only; target
  policy remains in the AArch64 call sites.
- Lower-level parameterized and shared-owner-specific call sites remain
  deferred where the current shared facades are not yet the right ownership
  shape, including `alu.cpp`, `calls.cpp`, `call_plans.cpp`, and prepared
  printer code.

## Proof

Ran exactly `(cmake --build --preset default && ctest --test-dir build -R
'^backend_' --output-on-failure) > test_after.log 2>&1`.

Result: passed. `test_after.log` reports `100% tests passed, 0 tests failed out
of 179`.
