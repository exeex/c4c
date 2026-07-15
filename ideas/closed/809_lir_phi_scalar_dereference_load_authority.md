# LIR PHI Scalar Dereference-Load Authority

Status: Closed
Type: bounded PHI producer-handoff blocker
Blocked Parent: `ideas/closed/806_lir_phi_residual_producer_family_authority_trace.md`

## Goal

Repair only the evidenced scalar `UnaryOp::Deref` load-result authority
handoff required for `20060910-1.c` to supply the existing conditional-PHI
incoming contract.

## Why This Exists

806 Step 3 traced `check_header`'s ternary true expression to
`*((deeper)->buffer_position)++`. Conditional lowering preserves an incoming
native authority when present, but its enclosing scalar `UnaryOp::Deref`
emits `LirLoadOp` through a fresh temporary string and loses the immediate
`LirValueId` before the conditional PHI consumes it. This is distinct from
806's accepted postfix old-value producer route.

## In Scope

- Make the smallest native current-function `LirValueId` handoff from the
  scalar dereference-load result into the existing conditional-PHI path.
- Add nearby scalar-dereference same-family positive and malformed/foreign,
  stale, or unknown-authority coverage consistent with the existing PHI
  contract.
- Obtain fresh build and focused proof, then provide the evidence required for
  the supervisor to decide whether to run the full baseline.

## Out Of Scope

- Postfix old-value, floating unary-minus `fneg`, scalar bit-not `xor`, or
  804 scalar unary-minus work.
- CFG, PHI schema/verifier, predecessor/edge semantics, generic conditional
  lowering, aggregate/vector or pointer/object provenance rewrites.
- Text-derived identity, testcase-name branching, expectation downgrades, or
  declaring 806/804/754 clear without a supervisor-accepted 100% full
  baseline.

## Acceptance Criteria

- The scalar dereference `LirLoadOp` result reaches the existing conditional
  PHI with a checked current-function `LirValueId`; missing, foreign, stale,
  and unknown authority remain rejected.
- Nearby same-family positive and malformed-authority coverage passes after a
  fresh build, including focused `llvm_gcc_c_torture_src_20060910_1_c` proof.
- A supervisor accepts a fresh 100% full baseline before 806 resumes at its
  recorded full-baseline return step.

## Reviewer Reject Signals

- Reject a generic provenance, conditional-lowering, CFG, PHI-verifier, or
  instruction-family rewrite presented as this dereference-load repair.
- Reject testcase-shaped branches, rendered-text or temporary-name identity
  recovery, and expectation downgrades.
- Reject changes that merely classify the load without preserving a checked
  native current-function handoff through the actual conditional PHI path.
- Reject focused-only success as parent clearance or reopening the accepted
  postfix, `fneg`, or `xor` routes.

## Parent Return Contract

806 Steps 1–2 remain accepted, including postfix repair `961ce9fda`. On an
accepted 809 repair and supervisor-accepted fresh 100% full baseline,
reactivate 806 at its recorded Step 3 full-baseline return point; do not
repeat classification or accepted producer repairs.

## Closure Record

Capability complete. Commit `4d29f7b3e` repairs scalar `UnaryOp::Deref` by
giving its `LirLoadOp` result a fresh native current-function `LirValueId`.
Fresh direct `frontend_lir_call_type_ref_test` coverage passed; the matching
focused before/after guard improved from 1/2 to 2/2 with no new failures; and
the supervisor accepted the fresh
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure`
full baseline at 3037/3037. That accepted baseline fulfills the parent-return
contract and releases 806's recorded Step 3 gate without repeating this route.
