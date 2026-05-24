Status: Active
Source Idea Path: ideas/open/aarch64-codegen-forward-migration-second-wave-audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Generate Focused Follow-Up Ideas

# Current Packet

## Just Finished

Step 3: Generate Focused Follow-Up Ideas completed as an audit-only lifecycle
packet. Created focused follow-up implementation ideas from the Step 2
candidate table:

- `ideas/open/shared-mir-same-block-producer-select-queries.md`
  extracts shared same-block producer and select-chain query ownership.
- `ideas/open/target-neutral-publication-plan-record.md`
  introduces a target-neutral scalar publication-plan record with target-local
  emission hooks.
- `ideas/open/prealloc-call-boundary-ordering-republication.md`
  extends prealloc call-plan ownership for generic call-boundary ordering and
  republication intent.
- `ideas/open/prealloc-store-source-publication-planning.md`
  moves store-source planning facts into prealloc while leaving memory operand
  spelling and final store emission target-local.

Each new idea includes intent, why it exists, in-scope work, out-of-scope work,
acceptance criteria, proof expectations, concrete `## Reviewer Reject Signals`,
and an explicit x86/RISC-V reuse path. Each also keeps AArch64 instruction
spelling, ABI lane policy, stack-pointer sequences, memory operand spelling,
and final machine instruction construction out of scope except for target-local
hook/adaptor work.

## Suggested Next

Step 4 keep-local/defer recording should list candidates intentionally not
promoted into implementation ideas yet:

- Dynamic-stack helper recognition and operand-home planning: defer until the
  hook boundary around stack-pointer/frame-pointer sequencing, alignment, and
  scratch availability is clearer.
- Branch-fusion eligibility and support-instruction filtering: defer until the
  target hook boundary for condition sets, immediate encodability, compare
  forms, and final branch spelling is narrow enough.
- Generic `dispatch.hpp` helper API cleanup: defer until one concrete
  implementation move lands, then move declarations with the owning helper.
- Final AArch64 spelling, ABI lane rules, stack-pointer sequences, memory
  operands, ADRP/ADD, CMP/CSEL/branch forms, and machine instruction
  construction: keep target-local.

## Watchouts

- This is an audit run; do not implement broad code movement.
- Step 4 should not reopen the four created follow-up ideas unless a wording
  fix is required; it should record keep-local/defer decisions for the
  remaining tempting candidates.
- Do not create follow-up ideas for target-local final emission. The audit
  explicitly keeps AArch64 register spelling, ABI lane policy, stack-pointer
  sequences, ADRP/ADD, CMP/CSEL/branch forms, memory operands, and machine
  instruction construction in the AArch64 layer.
- The safest follow-up activation remains producer/select-chain query
  extraction. Publication planning, call-boundary ordering, and store-source
  planning are valuable but have higher hook-boundary risk.

## Proof

Audit-only idea creation and `todo.md` update. No build or test proof required
by the packet, and no `test_after.log` was created.
