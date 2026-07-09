# RV64 String-Label Pointer Runtime Object Correctness

Status: Open
Type: Research
Parent: `ideas/closed/630_string_constant_local_memory_policy.md`
Related:
- `ideas/closed/618_runtime_mismatch_ownership_investigation.md`
- `ideas/closed/630_string_constant_local_memory_policy.md`
- `docs/runtime_mismatch_ownership/03_followup_implementation_queue.md`
Owning Layer: RV64 runtime/object correctness evidence for string-label pointer rows
Queue Order: 38
Prerequisites: object emission must already consume explicit string-label
pointer authority; do not reopen string-constant local-memory admission
Proof Surface: `src/20000722-1.c` after object/link success and runtime
segfault

## Goal

Determine the first concrete owner of the `src/20000722-1.c` runtime/object
failure after string-label pointer authority has been consumed.

## Why This Exists

Idea 630 moved `src/20000722-1.c` past the string-constant local-memory
authority blocker. Step 8 shows the row now publishes
`layout_authority=string_constant_label_pointer`, reaches object and link, and
then fails with `[RV64_BACKEND_RUNTIME_MISMATCH]` / c4c segfault.

That symptom is outside string-constant local-memory admission, but it is close
enough to the string-label pointer route that the next owner should be proven
before opening an implementation fix.

## In Scope

- Reproduce `src/20000722-1.c` with current object, link, and runtime logs.
- Capture enough runtime/object evidence to distinguish bad string-label
  address materialization, local/global memory lowering, stack layout, ABI/call
  setup, branch/control flow, or true runtime support.
- Compare the row against the runtime mismatch ownership lanes from idea 618
  and classify the first owner with concrete artifacts.
- If a single implementation owner is proven, create or recommend a separate
  implementation idea with a focused proof surface.

## Out Of Scope

- Reopening string-constant local-memory authority or broadening
  `StringConstantLabelPointer` admission.
- Implementing a runtime fix in this research idea.
- Grouping all segfault or string rows under one generic runtime-support owner.
- Expectation, unsupported-marker, allowlist, timeout, runtime-comparison, or
  pass/fail accounting changes.

## Acceptance Criteria

- The row is rerun from a fresh build with captured object/link/runtime
  artifacts.
- The first owner is classified as one of ABI, layout, local/global memory,
  call lowering, branch/control flow, true runtime support, or unresolved with
  explicit missing evidence.
- Any recommended implementation follow-up is split by single owner and proof
  surface.
- The investigation does not claim progress from changing expectations,
  allowlists, timeout policy, or runtime comparison behavior.

## Reviewer Reject Signals

- Reject treating the segfault alone as proof of a runtime-support fix.
- Reject implementation edits or broad runtime rewrites inside this research
  idea.
- Reject reopening idea 630 unless fresh evidence shows a missing
  string-label pointer authority fact before object emission.
- Reject named-case-only fixes for `src/20000722-1.c` without a semantic owner
  and focused proof route.
- Reject expectation, unsupported-marker, allowlist, timeout/accounting, or
  runtime-comparison changes as progress.
