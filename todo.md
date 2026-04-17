Status: Active
Source Idea: ideas/open/13_x86_backend_pure_bir_handoff_and_legacy_guardrails.md
Source Plan: plan.md

# Current Packet

## Just Finished

Removed the remaining minimal parameter-side x86 shadow-ownership seam inside
the canonical prepared-module consumer. `bir::Param` now carries canonical
call-argument ABI metadata, lowering/legalize populate that metadata for the
prepared route, and `x86::emit_prepared_module(...)` consumes the prepared
parameter ABI instead of recomputing one locally from the type. The focused
handoff test now derives its expected parameter register from prepared
metadata, and the broader `^backend_` checkpoint stayed green after the slice.

## Suggested Next

No further concrete competing ownership seam is exposed in the current bounded
x86 prepared-module route. Hand lifecycle state back for near-close assessment
instead of widening emitter support or adding more narrow proof families.

## Watchouts

- keep new support inside `x86::emit_prepared_module(...)`; do not recreate a
  raw-BIR or public-entry special-case path like the reverted direct-return
  route
- `backend_x86_handoff_boundary` now proves success by comparing the canonical
  prepared/public/generic route outputs against the same bounded x86 function
  result; keep future proof route-oriented instead of falling back to generic
  substring checks
- the canonical x86 handoff is now the prepared-module consumer boundary, so
  new work should extend `x86::emit_prepared_module(...)` rather than adding
  new public/backend-side fallback renderers
- public x86 entry and direct BIR entry already funnel through the canonical
  prepared-module consumer; future cleanup should retire residual mixed
  ownership around that seam instead of reopening the boundary question
- the current join support is still tightly bounded: one prepared function, one
  entry compare, two empty branch-to-join leaf blocks, and one join block whose
  phi has already been legalized into a single `bir.select`
- the branch support currently assumes `BinaryOpcode::Eq` against zero with the
  sole parameter as the compared value; widen only if the prepared-module
  consumer can stay semantic and explicit
- the joined branch support only accepts select arms that resolve to the sole
  parameter, an immediate, or one supported parameter-immediate binary already
  materialized into the join block by prepare legalize
- the joined branch support now has focused proof for two trailing join-local
  supported parameter-immediate arithmetic cases (`add` and `mul`), three
  logical cases (`xor`, `and`, and `or`), and three shift cases (`shl`,
  `lshr`, and `ashr`) whose named operand is exactly the select result; do not
  widen beyond one trailing op in the same block
- do not treat more narrow handoff proofs as closure-quality evidence by
  themselves; a broader `^backend_` checkpoint is required before near-close
  acceptance
- keep control-flow proof route-oriented by comparing prepared/public/generic
  outputs against the same canonical assembly instead of falling back to loose
  substring assertions
- do not widen into nested select trees, forwarded successor joins, or any case
  that requires interpreting raw phi structure instead of the canonical
  prepared-module output

## Proof

Ran `cmake --build --preset default -j4`, then
`ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`,
then `ctest --test-dir build -j --output-on-failure -R '^backend_'`, and all
checks passed.
