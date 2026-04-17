Status: Active
Source Idea: ideas/open/13_x86_backend_pure_bir_handoff_and_legacy_guardrails.md
Source Plan: plan.md

# Current Packet

## Just Finished

Implemented the first real prepared-module consumer slice inside
`x86::emit_prepared_module(...)` for a minimal single-function `return i32
<imm32>` case. `backend_x86_handoff_boundary` now proves that the canonical
prepared-module consumer succeeds and that the public/generic x86 BIR routes
return the same canonical x86 assembly through that boundary.

## Suggested Next

Extend `x86::emit_prepared_module(...)` to the next honest prepared-module
shapes behind the same canonical boundary, starting with additional minimal
prepared return/value-flow cases that still avoid reintroducing any
direct-BIR/public-entry fallback path.

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
  block, direct immediate `i32` return, no hidden mixed fallback

## Proof

Ran `cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$'` and wrote the proving
output to `test_after.log`.
