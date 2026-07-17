# Lowering-Produced LIR Switch Selector Authority

Status: Closed
Type: narrow lowering-produced selector-authority correction
Blocked Parent: `ideas/open/821_frontend_lir_manual_switch_modelled_result_authority.md`,
Step 3

## Goal

Correct only the lowering-produced `LirSwitch` selector-authority route that
still fails the frontend focused test after the manual fixture has been
isolated and verified.

## Why This Exists

The focused `frontend_lir_call_type_ref` probe continues to abort at
`LirSwitch.selector: must identify a current-function integer value definition`
after the pending local manual-fixture correction. Isolating that fixture body
verifies it, and the remaining abort occurs later in
`lower_lir_module_for_target(...)` for a lowering-produced switch. This is a
separate producer/authority route, not a reason to broaden Idea 821.

## In Scope

- Trace the lowering-created `LirSwitch` selector and its defining
  current-function integer value authority in `lower_lir_module_for_target(...)`.
- Make the smallest structured authority publication/correction needed for
  that produced selector to satisfy the existing fail-closed verifier contract.
- Add nearby same-feature coverage for the lowering-produced route when the
  existing focused test surface supports it.
- Build and run the exact focused frontend test far enough to establish the
  selector route is cleared; return Idea 821 to Step 3 without accepting its
  pending manual-fixture slice on this idea's behalf.

## Out of Scope

- The manual `make_switch()` fixture correction and its pending local patch
  owned by Idea 821.
- Idea 820 DirectScalar work, including its later
  `LirBinOp.scalar_lhs_parameter_authority` abort.
- Generic switch redesign, Raw-BIR/importer/builder work, broad value-authority
  rewrites, or testcase-specific verifier exceptions.
- Deriving selector identity or type from display text, rendered diagnostics,
  or named-test conditions.

## Acceptance Criteria

1. The lowering-produced switch selector identifies a current-function integer
   value definition through structured authority under the existing verifier
   contract.
2. The correction is limited to the lowering-produced selector route and does
   not modify or accept Idea 821's pending manual-fixture patch.
3. A fresh build and exact focused
   `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
   pass beyond the former lowering-produced selector abort; any later existing
   DirectScalar failure is recorded as Idea 820 scope, not repaired here.
4. The durable return record tells Idea 821 to resume at Step 3 with its
   pending slice still unaccepted and no fabricated commit/proof reference.

## Reviewer Reject Signals

- Reject changes to the manual `make_switch()` fixture or any claim that its
  pending local patch is accepted by this idea.
- Reject DirectScalar, Raw-BIR, importer, builder, generic switch redesign, or
  broad value-authority work presented as a lowering-selector correction.
- Reject named-case bypasses, expectation downgrades, test ordering changes,
  or verifier exceptions that conceal the missing selector authority.
- Reject textual/display-name classification or a default integer result type
  in place of structured current-function authority.
- Reject retaining the exact lowering-produced selector abort behind renamed
  helpers or a testcase-shaped abstraction.

## Resumption Record: Next DirectScalar authority blocker

Status: deactivated for the separately scoped Idea 825 parameter-authority
route. Do not resume this source until that route has supplied an isolated,
accepted DirectScalar switch-selector authority outcome or a new isolation
owner has separated the shared dirty worktree material.

- Last accepted progress: Step 1 trace only. No implementation slice from this
  idea has an accepted focused proof or acceptance commit.
- Completed runbook steps: Step 2 remains a dirty, pending and unaccepted
  lowering-route implementation plus nearby coverage in the shared worktree.
  Preserve it exactly; do not claim it accepted, discard it, overwrite it, or
  attach a commit reference.
- Interrupted step: Step 3, *Prove the route and return to 821*.
- First bad fact and scope boundary: the fresh focused
  `^frontend_lir_call_type_ref$` CTest is 0/1 before and 0/1 after under the
  matching regression guard. The dirty 822 materializing-add slice causally
  advances the failure from `LirSwitch.selector: must identify a
  current-function integer value definition` to
  `LirBinOp.scalar_lhs_parameter_authority: is required when LirBinOp.lhs uses
  a native direct-scalar parameter`. This DirectScalar parameter-authority
  failure is outside 822's lowering-produced-selector scope; closed Idea 820
  only owns the earlier `ull` publication and does not authorize this repair.
  The non-increasing failure count rejects acceptance of the dirty slice.
- Exact return action: preserve the 822 materializing-add/test material while
  Idea 825 owns the distinct native `LirSwitch.selector` parameter-authority
  row. If 825 cannot work without touching the preserved material, create a
  narrow shared-worktree isolation owner before either source resumes. Once
  the material is isolated and the relevant owner has accepted focused proof,
  reconstruct this source at Step 3, rerun its build and exact focused CTest,
  and return Idea 821 only after separate supervisor acceptance.
- Proof and commit status: `cmake --build --preset default` passed. The exact
  CTest and matching guard are diagnostic and non-accepting; there is no
  accepted proof or implementation commit for this idea.

## Closure Record: superseded by accepted selector-authority handoff

Status: closed as intentionally concluded stale cleanup.

This source has no current executable resumption route. Its Step 2 lowering
slice remained pending and unaccepted, and its own record required either an
accepted Idea 825 DirectScalar switch-selector authority outcome or a separate
shared-worktree isolation owner before any resume. Closed
`ideas/closed/828_shared_worktree_direct_call_authority_isolation.md`
preserved the unaccepted 821/822 material in
`review/828_preserved_821_822_frontend_slice.patch` and removed it from the
active shared route without accepting semantic progress. Closed
`ideas/closed/825_lir_next_body_parameter_authority_handoff.md` then accepted
the dedicated native `LirSwitch.selector_parameter_authority` producer/verifier
handoff and focused proof, which supersedes this older lowering-produced
selector patch route.

Disposition: archive this source without activation. Do not reconstruct Step
3, restore the preserved patch as accepted progress, or return to 821 from
this route. Future lowering-produced selector work requires a new source idea
with fresh evidence outside the accepted 825 authority contract.
