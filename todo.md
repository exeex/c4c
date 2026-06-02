Status: Active
Source Idea Path: ideas/open/95_aarch64_calls_immediate_scalar_argument_publication_owner.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Route Immediate Publication Uses Through The Owner

# Current Packet

## Just Finished

Completed plan Step 3: `Route Immediate Publication Uses Through The Owner`.

Audited the remaining immediate-looking calls-side paths in
`src/backend/mir/aarch64/codegen/calls.cpp`. No additional code routing was
needed: the in-scope same-block immediate cast call-argument publication path
already consumes `ImmediateScalarCallArgumentPublicationOwner::instruction`,
which owns the immediate integer bit extraction, GP/FP destination view
rendering, FP immediate scratch materialization, inline-asm line construction,
and immediate-publication instruction construction for that boundary.

Remaining immediate scalar uses intentionally stay outside the owner:
`lower_before_call_immediate_binding` owns prepared scalar literal lookup,
destination authority, ordinary register immediate moves, synthetic
`CallBoundaryMove` construction, and stack-slot immediate stores;
`make_scalar_call_argument_immediate` remains the generic immediate operand
adapter consumed by both ordinary call-boundary moves and value moves;
`make_call_boundary_move_instruction` remains generic record construction for
all call-boundary move source kinds; value stack moves and indirect
callee/edge-publication helpers remain generic scalar materialization rather
than immediate call-argument publication.

## Suggested Next

Execute Step 4 by running the focused behavior proof for the immediate scalar
publication owner route and recording the result.

## Watchouts

- The owner boundary is intentionally narrower than all immediate operands:
  only same-block cast publication of a prepared register call argument is
  in-scope.
- Ordinary before-call prepared immediate moves and stack immediate stores are
  excluded by the plan and remain in `lower_before_call_immediate_binding`.
- Aggregate/byval transport, f128 carriers, ordinary before-call moves, stack
  immediate stores, generic `CallBoundaryMove` construction, and generic scalar
  edge/indirect-callee materialization remain outside this owner.
- The same-block cast lookup remains local to the owner and still searches only
  before the current instruction in the current BIR block for the already
  prepared source-home value name.

## Proof

Ran delegated documentation-only proof because `calls.cpp` did not change:

```bash
git diff --name-only
```

Result: only `todo.md` changed.

Proof log path: none expected for a `todo.md`-only audit packet.

Additional check: `git status --short` shows only `M todo.md`.
