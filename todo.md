Status: Active
Source Idea: ideas/open/13_x86_backend_pure_bir_handoff_and_legacy_guardrails.md
Source Plan: plan.md

# Current Packet

## Just Finished

Proved the rest of the bounded joined right-shift family already present in the
canonical x86 prepared-module consumer: minimal x86_64 single-parameter `i32`
compare-against-zero branches whose join block contains one legalized
`bir.select` plus one trailing supported parameter-immediate `lshr` or `ashr`
on that select result. The focused handoff proof now covers joined add/sub
followed by `shl`, `lshr`, and `ashr` while still proving
prepared/public/generic route equality against the same canonical x86
assembly, and it did so without adding any fallback or widening the emitter
boundary because the existing bounded prepared-module consumer already handled
those shift families.

## Suggested Next

Extend focused proof only if the same bounded prepared-module consumer already
honestly supports another single trailing join-local parameter-immediate op on
the legalized select result behind the canonical boundary, and only if that
still stays inside the same explicit join consumer with no nested selects,
forwarded joins, commuted forms, or fallback rendering. If no similarly
bounded family remains, the next packet should pivot from proof expansion back
to retiring mixed ownership or other active-route cleanup required by the
source idea.

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
- the joined branch support now has focused proof for two trailing join-local
  supported parameter-immediate arithmetic cases (`add` and `mul`), three
  logical cases (`xor`, `and`, and `or`), and three shift cases (`shl`,
  `lshr`, and `ashr`) whose named operand is exactly the select result; do not
  widen beyond one trailing op in the same block
- keep control-flow proof route-oriented by comparing prepared/public/generic
  outputs against the same canonical assembly instead of falling back to loose
  substring assertions
- do not widen into nested select trees, forwarded successor joins, or any case
  that requires interpreting raw phi structure instead of the canonical
  prepared-module output

## Proof

Ran `cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
and wrote the passing proving output to `test_after.log`.
