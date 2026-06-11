Status: Active
Source Idea Path: ideas/open/192_residual_route_view_consumer_migration_map.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Write The Durable Migration Map

# Current Packet

## Just Finished

Step 4 durable migration map complete for `plan.md` Step 4. Created
`docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md` from
the Step 2 residual-consumer inventory and Step 3 candidate table.

The artifact contains a Route 1-7 table covering selected-reader evidence,
residual prepared consumers, consumer classification, candidate migrations,
required route/facade facts, fallback/oracle surfaces, and proof expectations.
It also includes separate sections for printer/debug/oracle consumers and for
target-wrapper/retained-policy consumers, preserving these as blockers to
prepared API contraction.

## Suggested Next

Execute Step 5 from `plan.md`: final consistency check against source idea 192
acceptance criteria and reviewer reject signals. Confirm no implementation
files changed, the durable map does not claim route-wide completion or prepared
API retirement, and `todo.md` records a closure recommendation for supervisor
review.

## Watchouts

- The artifact is descriptive and proposal-granular only. It does not
  implement migrations, delete/privatize prepared APIs, or open draft 155.
- Each route names one next selected-consumer candidate, but printer/debug,
  oracle/test, target-wrapper, and retained prepared/target policy consumers
  remain visible as blockers to broad retirement.
- Ideas 182-188 remain selected-reader evidence only; idea 189 remains one x86
  Route 6 reuse point only, with no riscv imported reuse proof.
- Route 3 target-addressing policy, Route 6 ABI/call policy, Route 7 branch
  policy, materialized-compare render policy, final emission policy, and
  return-chain facts stay outside Route 1-7 semantic migration scope.

## Proof

Docs/lifecycle-only packet; no build required. Verification checked that
`docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
exists and contains the required Route 1-7 table plus separate
printer/debug/oracle and target-wrapper/retained-policy sections. Existing
`test_after.log` remains the proof log path; no implementation files changed.
