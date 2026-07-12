# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Audit ownership and run integration proof — final three-target acceptance packet

## Just Finished

- Step 3.3 dead Route 1/Route 3 conversion-helper cleanup removed the
  unreachable `route1_value_identity_to_same_block` and closed
  `route3_memory_access_to_mir` conversion family after AST-backed caller
  checks confirmed that neither root helper had callers.
- Ratcheted the exact common-query Route 1 and Route 3 inventories from 2/18
  to zero and retained explicit regex canaries while requiring zero remaining
  route-owned records, indexes, and analysis entry points.

## Suggested Next

- Execute one bounded Step 6 acceptance packet: audit every remaining common
  query declaration/definition for named BIR or prepared-view ownership and
  fail-closed status handling; confirm the affected x86, AArch64, and RV64
  callers contain no BIR analysis, prepared reconstruction, or route fallback;
  then build and run the route-authority guard, directly affected three-target
  handoff contracts, and the supervisor-selected broader backend regression
  guard with matching canonical before/after scope.
- Treat Steps 4 and 5 as acceptance claims only after that audit and proof are
  recorded. If the audit finds an executable ownership or target-caller gap,
  return the smallest concrete implementation packet instead of closing.

## Watchouts

- All exact Route 1-Route 8 inventories are now zero. The guard separately
  requires zero route record/index payloads and route analysis entry points in
  the common boundary, with synthetic canaries proving both regexes remain
  effective.
- Zero vocabulary is necessary but not sufficient for closure: it does not
  alone prove ownership-correct fail-closed behavior, all three target-facing
  callers, or the broader regression requirement in Step 6.
- The frame/call contract still retains its known baseline failure:
  `call_plans no longer publish the direct call`.

## Proof

- Exact delegated proof built both targets. The guard passes; the frame/call
  contract retains the known baseline `call_plans no longer publish the direct
  call` failure, with no new failure. Command: `cmake --build --preset default --target
  backend_common_mir_query_route_authority_guard_test
  backend_prepare_frame_stack_call_contract_test && ctest --test-dir build
  --output-on-failure -R
  '^(backend_common_mir_query_route_authority_guard|backend_prepare_frame_stack_call_contract)$'
  > test_after.log 2>&1`; canonical log: `test_after.log`.
