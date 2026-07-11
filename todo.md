# Current Packet

Status: Active
Source Idea Path: ideas/open/716_prealloc_current_block_routing_authority_closure.md
Source Plan Path: plan.md
Current Step ID: 6.2
Current Step Title: Repair complete invariant owner-prepared fact coverage

## Just Finished

- Repaired the runbook after the Step 6.2 owner-only consumption proof exposed
  incomplete owner-prepared fact coverage; bounded AArch64 adoption is now
  Step 6.3 and remains gated on the new Step 6.2 prerequisite.

## Suggested Next

- Execute plan Step 6.2 as a preallocation-owned packet: prove complete,
  invariant owner-prepared facts for the unchanged incoming-expression
  contract before retrying owner-only AArch64 consumption in Step 6.3.

## Watchouts

- Do not reconstruct authority in AArch64, promote Route 5, rewrite source
  identity while appending facts, add a target-local authority builder or
  fallback, or change supported expectations.
- Do not accept scalar-operand or immediate-destination authority without a
  focused semantic contract covering positive, unrelated, mismatched, and
  multiple-applicable-edge cases.
- Preserve or prove the transfer-level destination invariant; owner attachment
  alone is not proof of complete authority.
- Ideas 713 and 705 remain blocked until idea 716 completes its handback.

## Proof

- Lifecycle-only runbook repair; no implementation proof run.
