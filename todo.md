Status: Active
Source Idea Path: ideas/open/prealloc-regalloc-coordinator-contraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Coordinator And Helper Families

# Current Packet

## Just Finished

Completed Step 1 from `plan.md`: inventoried
`src/backend/prealloc/regalloc.cpp` against the existing
`src/backend/prealloc/regalloc/` helper families.

Current ownership boundaries:
- Placement and assignment mechanics already have homes in
  `regalloc/assignment.*`, `regalloc/classification.*`, and
  `target_register_profile.*`; `regalloc.cpp` still coordinates pool order and
  active caller/callee-saved sets.
- Spill/reload records, phi moves, consumer moves, call/return moves, runtime
  helper mappings, value homes, pointer carriers, f128 constants, and storage
  classification already route through existing helper-family files.
- Interval ranking and overlap facts live in `regalloc/intervals.*`, while
  source liveness facts stay in `liveness.*`.
- Prepared printer meaning remains split between
  `prepared_printer/regalloc.cpp` for spill/reload output and
  `prepared_printer/value_locations.cpp` for homes, move bundles, and ABI
  bindings.
- `regalloc.cpp` still owns frame-slot seed values, stack-slot publication into
  `PreparedStackLayout`, final frame-size updates, and the high-level
  per-function phase order.

Rejected broad moves:
- Do not move the allocation lambda or pool ordering wholesale; it closes over
  mutable coordinator state, target-profile access, liveness call points, and
  spill-point side effects.
- Do not split call ABI publication yet; `append_prepared_call_abi_bindings`
  bridges call ABI facts and value-location printing, so moving it before a
  smaller proven boundary risks mixing call-plan and printer responsibilities.
- Do not fragment `regalloc.hpp`; current helper files still depend on the
  aggregate allocation-plan structures.

## Suggested Next

Run Step 2 as one small code-changing packet: move the file-local
`expire_completed_assignments(...)` helper from `regalloc.cpp` into
`regalloc/assignment.*` next to `ActiveRegisterAssignment` and assignment span
selection. This is the safest first contraction because it depends only on the
active-assignment vector, the current start point, and the existing
call-boundary pressure boolean.

If that proves too small to be useful after implementation, the next candidate
is a separate later packet around stack-slot publication helpers
(`next_frame_slot_id`, frame extent/alignment lookup, and
`publish_regalloc_stack_slots`) into `regalloc/stack_slots.*`; do not combine it
with assignment expiry.

## Watchouts

Keep Step 2 behavior-preserving: active assignment expiry must preserve the
existing strict/end-point-at-start behavior and the
`preserve_call_boundary_pressure` path. The stack-slot helper cluster looks
related, but moving it touches frame layout publication and should be saved for
a second validated packet. No printer changes are implied by the Step 2
assignment-helper move.

## Proof

Ran `git diff --check`; passed. No `test_after.log` was created because this
was a no-code inventory packet.
