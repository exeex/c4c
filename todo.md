Status: Active
Source Idea Path: ideas/open/425_scalar_fpr_salvage_from_try_gcc_torture.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Publish Salvage Handoff And Follow-Up Ideas

# Current Packet

## Just Finished

Step 4 - Publish Salvage Handoff And Follow-Up Ideas completed the scalar FPR
salvage publication packet without closing active idea 425.

Salvage shortlist and ordering:

1. Accepted: residual scalar F32/F64 cast object lowering.
   - New source idea:
     `ideas/open/517_residual_scalar_f32_f64_cast_object_lowering.md`.
   - Owning layer: RV64 prepared object lowering for scalar FPR/GPR conversion
     casts, with prepared/BIR producer facts treated as inputs rather than
     inferred by the object emitter.
   - Evidence and bucket impact: the fresh 2026-07-01 scan has exactly `3`
     `unsupported_floating_cast` rows in
     `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv`:
     `src/cvt-1.c`, `src/920618-1.c`, and `src/pr66233.c`.
   - Branch evidence to mine, not cherry-pick: `9d0f64883`, `b0700c2c3`,
     `88076c4ec`, `18c41c9c3`, `92d77cf2c`, `8bbaf8eb7`, and `f95ec37b9`.

Quarantine and rejection notes:

- Older scalar residual rows `src/ieee/930529-1.c` and
  `src/ieee/pr72824.c` remain supporting context only. They are too composite
  and old-row-specific to drive this first scalar-FPR salvage follow-up:
  `930529-1` mixes `double` division with select/local-byte checks, while
  `pr72824` mixes `float` local storage, `fptrunc`, `llvm.copysign.f32`, and a
  floating branch guard.
- F128/helper ABI/local-memory, aggregate/byval, stack-frame, and
  `conversion.c`-only work remain quarantined from scalar F32/F64 cast salvage.
- Scalar FPR binary arithmetic, branch guard lowering, select/materialization,
  local-store/reload publication, same-module scalar FPR calls, and return
  move bundles are not follow-up ideas from this packet unless a future scan
  proves one of them is the first owner.

The new follow-up idea includes concrete goal, why, scope, out-of-scope,
acceptance criteria, and reviewer reject signals. It also preserves the Step 3
precondition that row inspection must split a producer/prepared-fact idea
instead of widening RV64 object emission if the three fresh rows do not have
explicit prepared scalar-cast facts.

## Suggested Next

Delegate Step 5 - Validate And Handoff. The executor should run formatting or
diff checks for `todo.md` and the new idea file, then record whether active
idea 425 is ready for plan-owner closure.

## Watchouts

Active idea 425 is not closed by this packet. The new idea 517 should not be
activated until the supervisor or plan owner closes or otherwise transitions
the active 425 lifecycle state.

If executor row inspection later proves the three fresh rows have different
first owners, split before implementation instead of widening RV64 object
emission. Keep the older scalar residual rows as context, not acceptance
criteria.

## Proof

Lifecycle/source-idea publication step. No build or CTest was required.

- `git diff --check -- todo.md ideas/open/517_residual_scalar_f32_f64_cast_object_lowering.md`
