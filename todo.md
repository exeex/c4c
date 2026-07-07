Status: Active
Source Idea Path: ideas/open/581_rv64_ordinary_floating_cast_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove Representative Route Advancement

# Current Packet

## Just Finished

Completed `plan.md` Step 4 by rerunning the retained ordinary floating-cast
representatives after Step 3 and recording their current RV64 object routes.

Representative route results:
- `src/920618-1.c`: prepared dump rc `0`; RV64 object route rc `1`; advanced
  past the prior `unsupported_floating_cast` owner to
  `unsupported_terminator_fragment: BIR terminator requires unsupported RV64
  object lowering`.  Fresh prepared context still contains the ordinary
  `%t0 = bir.fptrunc double 0x3FF199999999999A to float` in `@main`, then a
  fused `sle float 0x00000000, %t0` branch from `entry` to `block_1`/`block_2`.
- `src/ieee/pr67218.c`: prepared dump rc `0`; RV64 object route rc `1`;
  advanced past the prior `unsupported_floating_cast` owner to
  `unsupported_terminator_fragment: BIR terminator requires unsupported RV64
  object lowering`.  Fresh prepared context contains the retained
  `@foo` chain `%t1 = bir.uitofp i32 %t0 to float` and
  `%t2 = bir.fpext float %t1 to double`, plus `@main` with an immediate
  F32-to-F64 cast used by a double compare.
- `src/pr23941.c`: prepared dump rc `0`; RV64 object route rc `1`; advanced
  past the prior `unsupported_floating_cast` owner to
  `unsupported_terminator_fragment: BIR terminator requires unsupported RV64
  object lowering`.  Fresh prepared context contains the retained ordinary
  `%t0 = bir.fptrunc double 0x4000000000000000 to float`, F32 arithmetic,
  `%t2 = bir.fpext float %t1 to double`, local store/load, and a fused double
  compare branch in `@main`.

The route diagnostic for the later terminator owner is generic and does not
print function/block/instruction coordinates.  The prepared dumps saved under
the Step 4 artifact directory provide the available CFG context.  F128 and
long-double rows remain quarantined and were not used as completion evidence.

## Suggested Next

Execute `plan.md` Step 5 by running the supervisor-selected backend validation
subset and recording whether the focused coverage plus representative route
advancement are ready for plan-owner closure evaluation.  Treat the later
`unsupported_terminator_fragment` owner as a separate downstream route, not as
more floating-cast work.

## Watchouts

- Keep this lane limited to ordinary F32/F64 casts.
- All three retained representatives now advance beyond
  `unsupported_floating_cast`; do not widen this lane to terminator lowering.
- The focused `UIToFP i32 -> F32` plus `FPExt F32 -> F64` chain was already
  supported before Step 3; `pr67218.c` now also clears the representative-route
  floating-cast owner and stops at a downstream terminator owner.
- Immediate floating-source materialization is now supported for ordinary
  F32/F64 width casts only.
- Do not use F128, long-double, soft-float helper, scalar compare, or variadic
  helper work as justification for this idea.
- Do not claim progress through unsupported-marker changes, expectation
  rewrites, route allowlist edits, named-case checks, or residual filename
  shortcuts.

## Proof

Proof output is in `test_after.log`.

- `cmake --build --preset default --target c4cll`: pass.
- Fresh prepared dumps for `src/920618-1.c`, `src/ieee/pr67218.c`, and
  `src/pr23941.c`: all rc `0`.
- Fresh RV64 object routes for those three representatives: all rc `1`, each
  now at downstream `unsupported_terminator_fragment` rather than the old
  `unsupported_floating_cast` owner.
- Step 4 artifacts are under
  `build/agent_state/581_rv64_ordinary_floating_cast_lowering/step4/`.
