Status: Active
Source Idea: ideas/open/13_x86_backend_pure_bir_handoff_and_legacy_guardrails.md
Source Plan: plan.md

# Current Packet

## Just Finished

Extended `x86::emit_prepared_module(...)` to support one additional honest
prepared value-flow family: a minimal x86_64 single-function `i32` parameter
returned after a single immediate bitwise-or through the canonical
prepared-module boundary. The focused handoff proof now covers the
immediate-return case, the direct single-parameter passthrough case, the
single-parameter add-immediate case, the single-parameter sub-immediate case,
the single-parameter mul-immediate case, the single-parameter and-immediate
case, the single-parameter or-immediate case, and the single-parameter
xor-immediate case by checking
prepared/public/generic route equality against the same canonical x86 assembly
per shape.

## Suggested Next

Extend `x86::emit_prepared_module(...)` to the next honest prepared-module
shapes behind the same canonical boundary with one bounded single-block
single-parameter `i32` shift-immediate family that still consumes the shared
prepared contract, without reintroducing any direct-BIR/public-entry fallback
path.

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
- keep the current support semantic and minimal: one prepared function, one
  block, and only direct `i32` return families already modeled by the
  canonical prepared boundary, with no hidden mixed fallback
- keep immediate-op support limited to the same one-op prepared shape; do not
  widen into multi-instruction prep or mixed fallback behavior unless the
  canonical prepared route and proof stay equally honest
- shift-immediate follow-up work must stay equally bounded: one block, one
  returned SSA value, and a direct `edi` to `eax` lowering shape through the
  prepared-module consumer

## Proof

Ran `cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$'` and wrote the proving
output to `test_after.log`.
