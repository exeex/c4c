# 113 BIR/Prealloc Aggregate Call-Boundary Contract Audit

## Goal

Analyze the aggregate call-boundary lessons from idea 112 and produce focused
follow-up ideas that decide which responsibilities belong in shared
BIR/prealloc contracts versus AArch64-specific ABI lowering.

This idea is analysis-only. It should not directly perform broad code
reorganization.

## Why This Exists

Idea 112 repaired the post-closure AArch64 regressions in
`c_testsuite_aarch64_backend_src_00216_c` and
`c_testsuite_aarch64_backend_src_00204_c`. Its closure note identified two
important boundary failures:

- `00216`: direct aggregate address materialization for frame-slot-backed local
  aggregate pointer operands.
- `00204`: outgoing AArch64 stack argument reservation before `x16`-relative
  stores, plus stable callee-side `va_list` home preservation for aggregate
  `va_arg`.

Both repairs are AArch64-visible, but the underlying contracts are not purely
AArch64-specific. x86 and RISC-V will also need coherent aggregate-by-address,
byval, outgoing stack argument, and variadic aggregate payload authority. Before
adding new target backends, the shared BIR/prealloc boundary should be audited
so each architecture consumes common facts instead of rediscovering or
duplicating them in target codegen.

The full-suite baseline has been refreshed after idea 112:

```text
Baseline Commit: 5daac44dc41119b9ea3d2a8c1d91e00da78f6aec
Baseline Subject: Close AArch64 post-closure regression plan
Baseline Regex: <full-suite>
100% tests passed, 0 tests failed out of 3427
```

## In Scope

- Read idea 112's closure notes and the relevant commits:
  - `5054b6999 Repair local aggregate call address selection`
  - `cdfc33cac Repair AArch64 variadic aggregate call handling`
  - `1362a467b Protect AArch64 recovered stack call targets`
  - `9e78e96e7 Refresh AArch64 byval stack route contracts`
- Classify each repaired behavior as one of:
  - shared BIR semantic fact
  - shared prealloc/planning fact
  - target ABI placement decision
  - target codegen emission detail
- Audit the current boundary for:
  - frame-slot-backed aggregate address materialization
  - local aggregate pointer operand selection
  - outgoing stack argument area reservation and address ownership
  - byval aggregate stack routes
  - variadic aggregate `va_arg` payload/home preservation
  - prepared dump visibility needed to review those facts
- Compare nearby closed ideas to avoid duplicating already-settled work,
  especially ideas 97, 101, 102, 104, 106, 110, and 112.
- Produce a small set of follow-up ideas, each with a bounded owner and proof
  route.

## Out Of Scope

- Directly changing `src/backend/mir/aarch64/codegen` in this analysis idea.
- Implementing x86 or RISC-V aggregate call lowering.
- Reopening idea 112's runtime repair unless the audit finds a real current
  regression.
- Broad BIR/prealloc rewrites without a concrete contract gap.
- Renumbering or rewriting older closed ideas.

## Expected Outputs

The closure note for this analysis should create follow-up ideas only where the
audit finds concrete gaps. Good follow-up candidates include:

- a shared prealloc contract for aggregate address materialization consumed by
  call planning;
- an outgoing stack argument area ownership contract that targets can consume
  without inventing architecture-local reservation state;
- a variadic aggregate payload/home preservation contract separating shared
  va-arg shape facts from target ABI register-save layout;
- prepared dump/contract visibility for aggregate call-boundary facts when
  reviewability is currently too weak.

If a candidate is already fully covered by a closed idea and current tests, the
analysis should explicitly record "no new idea" instead of creating duplicate
work.

## Acceptance And Completion Criteria

- The analysis identifies the current owner layer for each 112 repair behavior.
- Follow-up ideas are created only for concrete unresolved contract gaps.
- Each follow-up idea names:
  - the shared or target owner boundary;
  - the files or modules likely involved;
  - the proof route;
  - reject signals against testcase-overfit.
- No implementation files are changed by this analysis idea.
- The current full-suite clean baseline remains accepted as the starting point.

## Reviewer Reject Signals

- The analysis turns into code changes instead of producing bounded follow-up
  ideas.
- It claims that a behavior belongs in shared BIR/prealloc without identifying
  what target ABI detail remains architecture-specific.
- It creates duplicate ideas for work already closed and proven by nearby
  contract tests.
- It treats AArch64's `x16` scratch addressing convention as a shared contract
  instead of separating it from the shared outgoing-stack area requirement.
- It proposes x86/RISC-V target work before the shared aggregate call-boundary
  facts are made explicit.
