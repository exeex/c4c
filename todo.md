Status: Active
Source Idea: ideas/open/13_x86_backend_pure_bir_handoff_and_legacy_guardrails.md
Source Plan: plan.md

# Current Packet

## Just Finished

Locked Step 1's current seam in code and tests: the active x86/public backend
entry is `backend::emit_module -> emit_target_bir_module -> prepared semantic
BIR text`, while the translated x86 direct-BIR entry in
`src/backend/mir/x86/codegen/emit.cpp` still rejects direct BIR input.

## Suggested Next

Start Step 2 by choosing the canonical x86 consumer boundary, then replace the
prepared-BIR text seam in `emit_target_bir_module` with a real x86 handoff
instead of preserving the current non-canonical text route.

## Watchouts

- `backend_x86_handoff_boundary` now proves the exact seam to remove; do not
  rewrite that test into a text-shape acceptance story once x86 handoff work
  begins
- the public route still bypasses target-local x86 emission entirely, so Step 2
  must change route ownership rather than only broadening semantic-BIR output
- keep the next slice focused on raw-BIR vs prepared-module ownership, not on
  unrelated x86 emission cleanup outside the handoff boundary

## Proof

Ran `cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_'` and wrote the proving output to
`test_after.log`.
