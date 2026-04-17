Status: Active
Source Idea: ideas/open/13_x86_backend_pure_bir_handoff_and_legacy_guardrails.md
Source Plan: plan.md

# Current Packet

## Just Finished

Extended `x86::emit_prepared_module(...)` to support one additional honest
prepared control-flow family: a minimal x86_64 single-function `i32`
compare-against-zero branch in the entry block that routes to two leaf
immediate-return blocks through the canonical prepared-module boundary. The
focused handoff proof now covers that bounded compare-and-branch case, and it
still proves prepared/public/generic route equality against the same canonical
x86 assembly for each supported shape.

## Suggested Next

Extend `x86::emit_prepared_module(...)` to the next honest prepared-module
control-flow shape behind the same canonical boundary: one bounded x86_64
single-parameter `i32` compare-against-zero branch whose two leaf blocks
return the parameter itself versus an immediate, if that can still be modeled
semantically inside the prepared-module consumer without adding any raw-BIR or
public-entry fallback renderer.

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
- keep the current branch support equally bounded: one prepared function, one
  entry compare, one `cond_br`, and two leaf return blocks with no join/phi or
  hidden mixed fallback
- the branch support currently assumes `BinaryOpcode::Eq` against zero with the
  sole parameter as the compared value and immediate `i32` returns in both leaf
  blocks; widen only if the prepared-module consumer can stay semantic and
  explicit
- keep control-flow proof route-oriented by comparing prepared/public/generic
  outputs against the same canonical assembly instead of falling back to loose
  substring assertions
- if the next packet adds a non-immediate leaf return, preserve the canonical
  prepared-module boundary and avoid sneaking in a second renderer for
  parameter moves or direct-BIR shortcuts

## Proof

Ran `cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$'` and wrote the proving
output to `test_after.log`.
