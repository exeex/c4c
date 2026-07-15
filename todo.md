# Current Packet

Status: Active
Source Idea Path: ideas/open/807_lir_phi_floating_unary_minus_fneg_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish native authority for the `fneg` result

## Just Finished

- 809 is capability-complete and archived after accepted commit `6cf72bd9e`:
  ternary PHI construction retains returned operand authority while existing
  missing, unknown, foreign, and stale rejection remains intact. 807 resumes
  from its durable Step 2 return point; no 807 implementation slice is yet
  accepted or committed.

## Suggested Next

- Execute 807 Step 2 using the retained scoped `fresh_value(ctx)` floating-
  `fneg` hunk; add nearby floating-`fneg` positive and malformed-authority
  coverage, then obtain the focused proof.

## Watchouts

- Keep the retained `src/codegen/lir/hir_to_lir/expr/misc.cpp` floating-`fneg`
  hunk scoped to its producer handoff. Do not reopen 809's ternary seam,
  modify PHI/verifier contracts, or absorb other unary families.

## Proof

- 809 accepted evidence: commit `6cf72bd9e`; fresh
  `cmake --build --preset default`; matching before/after
  `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
  with monotonic guard; and broader `-R '^backend_'` 5/5. For 807, obtain a
  fresh build and focused same-family positive plus malformed-authority proof.
