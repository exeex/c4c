# Prepared Dynamic-Frame Callee-Saved Slot Placement

Status: Open
Type: Implementation
Parent: `ideas/open/613_abi_call_result_stack_frame_lowering.md`
Related:
- `ideas/open/613_abi_call_result_stack_frame_lowering.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
Owning Layer: Prepared frame authority producer
Queue Order: 26
Prerequisites: dynamic/fixed frame metadata must already identify saved callee-saved registers and frame requirements
Estimated Evidence Breadth: dynamic/fixed GPR stack-frame rows blocked before RV64 object-route consumption
Proof Surface: prepared dumps for dynamic-stack functions with callee-saved GPR saves, plus RV64 consumer guard rows

## Goal

Publish explicit prepared callee-saved save-slot placements for dynamic/fixed
frame rows so RV64 object emission can consume stack-frame plans without
deriving save locations from final frame size, register order, or testcase
shape.

## Why This Exists

Idea 613 is a consumer-only ABI/RV64 route. Its Step 3 probes for
`src/20040811-1.c`, `src/pr43220.c`, and `src/vla-dealloc-1.c` confirmed that
the prepared frame facts publish frame size/alignment, dynamic stack operation
metadata, and saved callee-saved GPR names, but do not publish concrete
`slot_placement` save-slot offsets and sizes for those saved registers.

Without those placements, RV64 object emission cannot lower the frame
semantically. Inferring the slots from frame size, register order, or the known
source file would move prepared-authority production into the RV64 consumer and
violate idea 613's scope.

## In Scope

- Identify where prepared dynamic/fixed frame facts are produced for
  callee-saved GPR save/restore plans.
- Publish explicit save-slot offsets and sizes for saved GPRs such as `s1` and
  `s2` when the compiler already has enough frame-layout authority.
- Preserve dynamic stack operation metadata, fixed-slot frame-pointer metadata,
  and existing frame-size/alignment facts.
- Add producer-level tests or diagnostics that prove the prepared facts contain
  concrete slot placements before RV64 consumption.
- Keep RV64 object emission fail-closed when these prepared placements are
  absent.

## Out Of Scope

- RV64 object-route frame lowering itself.
- FPR save/restore placement and FPR dynamic-frame policy.
- Generic move-bundle authority production.
- Local/global memory producer repair.
- Runtime/library/variadic call policy.
- Expectation, unsupported-marker, allowlist, timeout, or accounting changes.

## Acceptance Criteria

- Dynamic/fixed frame rows with callee-saved GPRs publish concrete prepared
  save-slot offsets and sizes in the frame facts.
- At least one representative row from the `src/20040811-1.c`,
  `src/pr43220.c`, or `src/vla-dealloc-1.c` family exposes the new placement
  authority before RV64 object emission.
- Rows without enough upstream frame-layout authority remain rejected with a
  missing-authority diagnostic instead of inferred placement.
- Existing non-GPR, FPR-heavy, local/global, move-bundle, runtime, library, and
  variadic guards remain outside this idea.

## Reviewer Reject Signals

- Reject any change that makes RV64 object emission infer callee-saved save
  slots from frame size, register order, source filename, or final assembly
  shape.
- Reject named-case-only handling for `20040811-1.c`, `pr43220.c`, or
  `vla-dealloc-1.c`.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes claimed as producer progress.
- Reject broad frame rewrites that fold FPR save/restore, local/global
  producer repair, move-bundle authority, or call policy into this idea.
- Reject helper renames or diagnostic wording changes that leave the prepared
  frame facts without concrete callee-saved GPR save-slot placements.
