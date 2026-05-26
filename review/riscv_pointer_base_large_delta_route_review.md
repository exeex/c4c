# RISC-V Pointer-Base Large-Delta Route Review

## Scope

- Delegated role: `c4c-reviewer`
- Active source idea: `ideas/open/29_riscv_prepared_edge_publication_pointer_base_policy_broadening.md`
- Active plan: `plan.md`
- Current todo state: Step 3, `Review Route Quality`
- Requested focus: Step 2 large-delta implementation in
  `src/backend/mir/riscv/codegen/emit.cpp`,
  `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`, and
  `todo.md`

## Review Base

Chosen base commit: `4daafe873` (`[plan] Activate RISC-V pointer-base policy broadening plan`).

Reason: this is the lifecycle activation commit for the current active plan and
the current source idea, followed by the Step 1 inventory commit
`d2e42e4c1`, the Step 2 backend implementation commit `2dd914fd6`, and the
Step 3 todo advancement commit `f2460c7ba`. No later plan or source-idea
checkpoint resets the active idea before `HEAD`.

Commits since base: 3.

Reviewed diff: `git diff 4daafe873..HEAD`.

## Findings

### Medium: Recorded focused proof cannot be independently confirmed from the canonical log

- `todo.md:38-42` records a passing focused command that redirects output to
  `test_after.log`.
- The current workspace does not contain `test_after.log`; `find . -maxdepth 2
  -name 'test_after.log' -o -name 'test_before.log'` found only
  `./test_before.log`.

This is not an implementation-route blocker, but it leaves the proof evidence
incomplete for acceptance. The focused command and test bucket are appropriate
for the Step 2 route-quality question; the supervisor should rerun the exact
focused command or restore the canonical `test_after.log` before accepting the
slice. Broader backend validation is still required before lifecycle closure
per the active plan.

### No blocking testcase-overfit finding

- `src/backend/mir/riscv/codegen/emit.cpp:94-112` still derives pointer-base
  source facts from the prepared source home and prepared value-home lookup.
  It does not scan predecessor/successor blocks or build a RISC-V-local edge
  publication table.
- `src/backend/mir/riscv/codegen/emit.cpp:184-201` implements a general
  target-local policy for large deltas: zero remains `mv`, signed-12-bit
  deltas remain `addi`, and out-of-range deltas use `li dst, delta` followed
  by `add dst, base, dst` only when `dst != base`.
- The aliasing large-delta case remains fail-closed without inventing scratch
  semantics: `src/backend/mir/riscv/codegen/emit.cpp:192-201`.
- The tests add a positive large-delta distinct-destination case at
  `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:471-489`
  and preserve fail-closed coverage for aliasing large deltas at
  `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:704-716`.
- Existing fail-closed surfaces remain exercised for missing base names,
  absent deltas, unresolved bases, non-register base homes, stack destinations,
  missing shared lookups/publications, and non-move publications at
  `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:668-749`
  and `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:752-863`.

The implementation is not matching named tests, edge labels, fixture value ids,
or a single hard-coded large constant. The added `4096`, `2048`, and `-2049`
test values exercise the boundary class rather than driving source logic.

## Judgments

- Idea alignment: `matches source idea`
- Runbook transcription: `plan matches idea`
- Route alignment: `on track`
- Technical debt: `acceptable`
- Validation sufficiency: `needs broader proof`
- Reviewer recommendation: `continue current route`

## Answer To Review Question

The Step 2 implementation avoids testcase-overfit, preserves shared prepared
edge-publication authority, and keeps unsupported forms fail-closed. The
focused test scope is the right proof shape for route-quality acceptance, but
the canonical `test_after.log` referenced by `todo.md` is absent in the
workspace, so the proof evidence should be regenerated or restored before the
supervisor accepts the slice. Broader backend validation remains a Step 4
closure requirement, not a blocker for continuing from this route review.
