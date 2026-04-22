# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.2.3
Current Step Title: Migrate Canonical Return-Lane Handoff And Audit Remaining Holdouts
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Completed step 2.2.3 by auditing the post-move call/return surface and marking
the only live residual non-policy holdout explicitly: the prepared call-bundle
ABI selector helpers exported from `lowering/call_lowering.cpp` through
`x86_codegen.hpp` remain a step-3 compatibility surface for
`prepared_local_slot_render.cpp`. `return_lowering.cpp` now records that it
owns only return policy flow and epilogue sequencing while raw register
publication helpers stay in reviewed owner `core/x86_codegen_output.cpp`.

## Suggested Next

Have the supervisor decide whether step 2.2.3 is now exhausted; if a follow-on
packet is needed, keep it limited to the next explicit step-3 prepared seam
that can absorb the call-bundle selector compatibility surface without widening
back into dormant legacy owners.

## Watchouts

- The backend target still does not compile `shared_call_support.cpp` or
  `mod.cpp`; the newly annotated prepared call-bundle selectors must remain a
  compatibility holdout until a reviewed prepared seam replaces them.
- `return_lowering.cpp` is no longer hiding raw return publication ownership;
  only return-path policy flow, including F128-special handling and epilogue
  sequencing, remains there.
- Keep `call_lowering.cpp`, `memory.cpp`, and `mod.cpp` non-owning for this
  route; do not widen into ABI policy, prepared-route rewrites, or dormant
  utility reattachment just to retire the compatibility note.

## Proof

Step 2.2.3 canonical return-lane handoff audit and compatibility
classification on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
