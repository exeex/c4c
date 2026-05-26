# RISC-V Stack-Destination Fail-Closed Route Review

## Scope

- Active source idea: `ideas/open/30_riscv_prepared_edge_publication_stack_destination_policy_broadening.md`
- Active plan: `plan.md`
- Review base: `79333fa35` (`[plan] Activate RISC-V stack-destination policy broadening plan`)
- Commits reviewed: 2 (`79333fa35..HEAD`)
- Focus: `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` and `todo.md`

## Base Selection

`79333fa35` is the correct active-idea checkpoint. It created the active
`plan.md` and `todo.md` for source idea 30. The later lifecycle commit
`993a33507` is Step 1 inventory only, and `HEAD` (`7dde9be5e`) is the Step 2
fail-closed packet under review. There is no history ambiguity.

## Findings

### Medium: Validation artifact referenced by todo is absent

`todo.md:35-38` records a passing focused proof and says the result was written
to `test_after.log`, but the worktree currently has no root-level
`test_after.log`. Root logs present are `test_before.log` and
`test_baseline.log`. This does not invalidate the route judgment, because the
review question is about Step 2 alignment and the code/test diff is narrow, but
the supervisor should refresh or restore canonical proof evidence before
acceptance/closure.

### Low: Step 2 is fail-closed evidence, not source-idea completion

The source idea acceptance criteria ask for at least one additional
source-to-`StackSlot` form to be implemented, while the active plan also allows
recording a concrete RISC-V policy reason that remaining candidate forms must
stay fail-closed for now. The Step 1 record identified the concrete missing
scratch-register/scratch-reservation policy. The Step 2 diff then tightens the
unsupported assertions for `RematerializableImmediate -> StackSlot`,
`StackSlot -> StackSlot`, and `PointerBasePlusOffset -> StackSlot` by requiring
both `UnsupportedDestinationHome` and empty instruction text in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:22` and
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:608`.

This is acceptable as a policy-blocker packet and is not testcase-overfit: it
does not weaken existing supported behavior, add fixture-shaped backend
matching, or claim new support through expectation rewrites. It should not be
treated as final completion of the source idea unless the lifecycle owner
explicitly accepts the recorded blocker as the durable close condition or
creates follow-up scope for scratch-register policy.

## Judgments

- Idea alignment: matches source idea for a Step 2 fail-closed policy packet
- Runbook transcription: plan matches idea enough to continue, with the known
  completion caveat above
- Route alignment: on track
- Technical debt: acceptable
- Validation sufficiency: needs broader proof before closure; focused proof is
  claimed but the `test_after.log` artifact is currently missing
- Testcase-overfit: no

## Recommendation

Continue the current route. The next packet should either introduce an explicit
RISC-V scratch-register policy before supporting one of the deferred
source-to-stack forms, or keep source-to-stack broadening deferred and route
that decision through lifecycle closure/follow-up handling. Before accepting a
closure slice, regenerate or restore canonical validation evidence so
`todo.md` and root logs agree.
