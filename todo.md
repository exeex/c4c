Status: Active
Source Idea: ideas/open/13_x86_backend_pure_bir_handoff_and_legacy_guardrails.md
Source Plan: plan.md

# Current Packet

## Just Finished

Proved one more honest joined prepared-module control-flow shape on the
canonical x86 handoff boundary without widening the emitter contract: a
minimal x86_64 single-parameter `i32` compare-against-zero branch whose join
block contains one legalized `bir.select` plus one trailing supported
parameter-immediate logical op on that select result. The focused handoff
proof now covers the joined add/sub-then-xor case while still proving
prepared/public/generic route equality against the same canonical x86
assembly.

## Suggested Next

Extend `x86::emit_prepared_module(...)` only if it can honestly support the
next bounded joined prepared-module shape behind the same canonical boundary:
one x86_64 single-parameter `i32` compare-against-zero branch whose prepared
join block still contains exactly one legalized `bir.select`, but the trailing
join-local op is a different already-supported immediate family such as `and`,
only if that can stay inside the same explicit join consumer without nested
selects, forwarded joins, or fallback rendering.

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
- the current join support is still tightly bounded: one prepared function, one
  entry compare, two empty branch-to-join leaf blocks, and one join block whose
  phi has already been legalized into a single `bir.select`
- the branch support currently assumes `BinaryOpcode::Eq` against zero with the
  sole parameter as the compared value; widen only if the prepared-module
  consumer can stay semantic and explicit
- the joined branch support only accepts select arms that resolve to the sole
  parameter, an immediate, or one supported parameter-immediate binary already
  materialized into the join block by prepare legalize
- the joined branch support now has focused proof for one trailing join-local
  supported parameter-immediate arithmetic case (`add`) and one logical case
  (`xor`) whose named operand is exactly the select result; do not widen
  beyond one trailing op in the same block
- keep control-flow proof route-oriented by comparing prepared/public/generic
  outputs against the same canonical assembly instead of falling back to loose
  substring assertions
- do not widen into nested select trees, forwarded successor joins, or any case
  that requires interpreting raw phi structure instead of the canonical
  prepared-module output

## Proof

Ran `cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$'` and wrote the passing
proving output to `test_after.log`.
