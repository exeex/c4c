# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Close target-preparation and verifier-profile boundaries

## Just Finished

- Plan Step 7 closed the target-preparation and verifier-profile boundary in
  the normative C2-C9 order.
- Defined `CanonicalBir` as exactly the target-independent B8 publication and
  `VerifiedPreparationInput` as the non-mutating C1 gate binding that exact
  stage stamp to one validated target fingerprint while containing no prepared
  facts.
- Aligned the C2 layout and C3-C8 planner inputs, ordered predecessor keys,
  consumers, failure atomicity, and transitive invalidation rules; C7 remains
  tables-only.
- Defined the sole C9 public API as the all-module `bind_constraints`
  transaction, which reads original descriptions and ordinary identities from
  the exact Canonical snapshot and publishes one immutable
  `BoundConstraintSet` or nothing.

## Suggested Next

- Execute Plan Step 8, "Add and index the complete D2 subordinate contract."

## Watchouts

- Step 8 must consume the exact C3/C4 plans, cumulative preparation bundle, and
  immutable Canonical C9 binding without reparsing constraints or weakening
  their keys.
- `PreparedInput` is a target-bound input capability, not a published BIR
  revision and not `PreparedBir`; target eligibility and every prepared fact
  remain owned by C2-C9.
- C9's `BoundConstraintSet` is the immutable Canonical binding. Per-mutating-
  revision projection remains Plan Step 10 and must not be silently folded into
  Step 8.
- Step 14 and implementation remain forbidden pending completion of the repair
  route and a new blocker-free independent Step 13 review.

## Proof

- Passed: `git diff --check && ! rg -n 'Which C9|What exact pass promise distinguishes|remain open.*(C9|PreparedInput)|unresolved.*(C9|PreparedInput)' src/backend/bir/regalloc/constraints/README.md src/backend/bir/verify/README.md src/backend/bir/preparation/README.md && rg -n 'Canonical|PreparedInput|VerifiedPreparationInput|C2|C3|C4|C5|C6|C7|C8|C9|bind_constraints|BoundConstraintSet|PipelineStageStamp|TargetFingerprint|transaction|invalidat' src/backend/bir/target_layout/README.md src/backend/bir/preparation/README.md src/backend/bir/preparation/{abi,calls,variadic,address,inline_asm,runtime_helpers}/README.md src/backend/bir/regalloc/constraints/README.md src/backend/bir/verify/README.md`.
- The supervisor-selected documentation proof was sufficient; this packet did
  not create or modify `test_after.log` because regression logs were explicitly
  outside packet ownership.
