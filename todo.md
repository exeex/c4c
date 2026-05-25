Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consolidate the Affected Helper Boundary

# Current Packet

## Just Finished

Step 3 checked whether the Step 2 small byval stack-lane fallback removal made
any `calls_moves.cpp` helper declaration, file boundary, or build entry obsolete.
No consolidation was justified: `calls_moves.cpp` still owns the exported
before-call and after-call move entrypoints plus ordering/reload helpers used by
dispatch, and its local helpers still cover outgoing stack setup, explicit
prepared boundary effects, immediate binding moves, preservation publication,
return moves, and non-call value moves.

## Suggested Next

Supervisor should select the next packet from the remaining Step 1 audit
families. A coherent next slice is the local aggregate address publication
predicate duplicate-authority family in `calls_argument_sources.cpp` and
`calls_dispatch_bridge.cpp`, if the supervisor wants to continue removing
retained call-argument ABI planning reads.

## Watchouts

- Keeping `calls_moves.cpp` is still justified by the mixed public boundary:
  call-boundary move lowering, return/value move lowering, and dispatch-facing
  source-preservation helpers share local `CallBoundaryMoveInstructionRecord`
  construction and prepared-move diagnostics.
- The outgoing stack-byte computation still appears to need a new prepared slot
  width fact before retained `arg_abi` can be removed there.
- The local aggregate address publication predicates in
  `calls_argument_sources.cpp` and `calls_dispatch_bridge.cpp` are a separate
  coherent duplicate-authority family; keep them out of this helper-boundary
  packet.

## Proof

Supervisor-selected proof passed and is preserved in `test_after.log`:

`git diff --check > test_after.log 2>&1`
