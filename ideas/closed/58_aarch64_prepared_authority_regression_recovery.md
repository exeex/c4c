# AArch64 Prepared Authority Regression Recovery

## Goal

Repair the current AArch64 regression set introduced after the prepared
authority repair sequence so the latest full-suite baseline no longer fails:

- `backend_aarch64_instruction_dispatch`
- `backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`
- `c_testsuite_aarch64_backend_src_00196_c`
- `c_testsuite_aarch64_backend_src_00207_c`

## Why This Exists

The latest log,
`log/baseline_83d5470731cc32247c364d73ad03d8ddc478ec90.log`, shows these
four failures. The log history points at two durable regression boundaries:

- `db1581b38` / `log/baseline_db1581b3833ae7d45fb379623428490cf60a4b36.log`
  was still green: `100% tests passed, 0 tests failed out of 3411`.
- `c92708627` / `log/baseline_c9270862701eed736e54a77234bd8060ec318e64.log`
  first introduced the persistent failures for instruction dispatch,
  dynamic-stack fixed-slot FP anchoring, and `00207`.
- `78730af2f` / `log/baseline_78730af2f15da3f272aaa7d138bffd886908cbd0.log`
  first introduced the persistent `00196` failure, producing the same
  four-test failure shape that remains in the latest baseline.

The suspected ownership is not a single named testcase. The failing shape
spans AArch64 instruction-dispatch expectation, dynamic stack fixed-slot
anchoring, and c-testsuite runtime behavior after ALU/load-local and variadic
register-save publication authority changes.

## In Scope

- Reproduce and inspect the four failing tests from the latest baseline.
- Use the log-derived boundaries above to compare behavior before and after
  `c92708627` and `78730af2f`.
- Repair the semantic AArch64 lowering or prepared-fact consumption that
  causes the current failures.
- Preserve the prepared-authority direction from the preceding closed ideas:
  consumers should use shared prepared facts where appropriate instead of
  restoring duplicate local scans as permanent authority.
- Add or adjust focused coverage only when it captures the repaired semantic
  capability rather than the exact failing testcase shape.

## Out Of Scope

- Do not re-open unrelated architecture backends.
- Do not broad-rewrite ALU, memory, dispatch, calls, comparison, or variadic
  lowering unless the failing behavior proves the shared prepared authority
  contract itself is wrong.
- Do not use this idea to continue the previous prepared-authority cleanup
  sequence once these regressions are repaired.
- Do not mark any of the four failing tests unsupported or weaken their
  contracts without explicit user approval.
- Do not treat `00196` or `00207` as isolated named cases if nearby
  same-feature behavior remains broken.

## Acceptance Criteria

- The focused failure set passes:
  `backend_aarch64_instruction_dispatch`,
  `backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`,
  `c_testsuite_aarch64_backend_src_00196_c`, and
  `c_testsuite_aarch64_backend_src_00207_c`.
- A regression guard or equivalent matching before/after proof shows the four
  failures are fixed without introducing new failures in the chosen AArch64
  backend scope.
- The repair explains which boundary was responsible for each failing family:
  `c92708627`, `78730af2f`, or a shared later interaction.
- Any new helper or prepared lookup has a semantic owner and is consumed by
  the relevant lowering path without testcase-name matching.
- `todo.md` records any remaining known failures separately if the full suite
  still has unrelated red tests after this idea's scope is complete.

## Reviewer Reject Signals

- Reject changes that key behavior to `00196`, `00207`, specific c-testsuite
  filenames, exact temporary names, literal labels, or exact emitted stack
  offsets instead of repairing the lowering rule.
- Reject downgrading `backend_aarch64_instruction_dispatch`,
  `backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`,
  `c_testsuite_aarch64_backend_src_00196_c`, or
  `c_testsuite_aarch64_backend_src_00207_c` to unsupported, weaker
  expectations, or narrower contracts without explicit user approval.
- Reject expectation rewrites, helper renames, log-only classification, or
  todo-only updates claimed as capability progress.
- Reject restoring broad duplicate local scans as the durable fix when an
  existing prepared fact or a small shared prepared lookup should own the
  semantic query.
- Reject broad rewrites outside AArch64 prepared fact consumption and lowering
  paths unless the regression proof shows the shared prepared contract itself
  must change.
- Reject a patch that makes only the four named tests pass while nearby
  same-feature AArch64 dynamic-stack, load-local, variadic register-save, or
  prepared-publication cases remain unexamined or newly red.

## Paused Route Note

The active Step 3 route was paused on 2026-05-28 after
`backend_aarch64_instruction_dispatch` became the monolithic route driver. The
dirty implementation stack had moved the first bad dispatch assertion through
store-global publication ownership, fused compare materialization,
call/outgoing stack arguments, direct edge `LoadLocal` publication, and then
GOT-required `LoadGlobal` materialization while the focused proof remained
`2/4` passing. Idea 58 remains open: `backend_aarch64_instruction_dispatch`
and `c_testsuite_aarch64_backend_src_00196_c` are still failing, and the
original four-test recovery criteria are not satisfied.

Follow-up decomposition work now lives in
`ideas/closed/60_aarch64_dispatch_prepared_publication_decomposition.md`.

## Returned Route Note

Idea 60 closed on 2026-05-28 after the post-Step-10 review accepted the
dispatch/prepared-publication seam sequence. The focused proof is now `3/4`:
`backend_aarch64_instruction_dispatch`,
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`, and
`c_testsuite_aarch64_backend_src_00207_c` pass, while
`c_testsuite_aarch64_backend_src_00196_c` still fails with the known
`joe() && fred()` runtime mismatch. Resume this idea from the remaining
`78730af2f` family; keep the closed idea 60 seams as context only unless the
`00196` investigation proves a shared semantic owner.

## Closure Note

Closed on 2026-05-28 after Step 6 closure readiness review. The source idea's
acceptance criteria are satisfied:

- `c92708627` family: handled by the closed idea 60
  dispatch/prepared-publication seam work.
- `78730af2f` family: repaired by commit `57f1996c4` through AArch64
  block-entry edge-publication clobber avoidance in
  `dispatch_edge_copies.cpp`.
- Focused proof: commit `b8aef7247` recorded the four target tests passing
  together.
- Broader guard: commit `d8e8e2a9` recorded the matching `^backend_`
  regression guard passing with before `169/169`, after `169/169`, and no new
  failures.

No reject signal applies: the final repair is tied to prepared edge-publication
source materialization and register-clobber avoidance, not testcase names,
literal labels, exact temporaries, or fixed stack offsets.
