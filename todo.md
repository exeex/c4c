Status: Active
Source Idea Path: ideas/open/aarch64-codegen-forward-migration-second-wave-audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Record Keep-Local And Deferred Decisions

# Current Packet

## Just Finished

Step 4: Record Keep-Local And Deferred Decisions completed as an audit-only
decision record. The audit intentionally did not promote these remaining
second-wave candidates into implementation ideas:

- Dynamic-stack helper recognition and operand-home planning: defer for now.
  The reusable portion is plausible, but the safe boundary is still entangled
  with target-specific stack-pointer/frame-pointer sequencing, alignment,
  scratch-register availability, and final address materialization. Revisit
  only when there is a narrower hook contract or an adjacent x86/RISC-V
  consumer that needs the same planning facts.
- Branch-fusion eligibility and support-instruction filtering: defer for now.
  Reusable same-block reasoning may exist, but eligibility depends on
  target-specific condition sets, immediate encodability, compare forms,
  support-instruction legality, and final conditional-branch spelling. Do not
  create a migration slice until those hooks are narrow enough to avoid moving
  AArch64 instruction-selection policy into shared code.
- Generic `dispatch.hpp` helper cleanup: defer as standalone work. Header
  cleanup should follow a concrete implementation move and transfer
  declarations with their owning helper, not become an independent API shuffle.
- Final AArch64 emission, ABI, and instruction spelling: keep target-local.
  This includes ABI lane policy, AArch64 register spelling, stack-pointer
  sequences, ADRP/ADD forms, CMP/CSEL/branch forms, memory operand spelling,
  and final machine instruction construction. Future shared or prealloc ideas
  may expose target hooks, but final AArch64 spelling remains in the AArch64
  layer.

## Suggested Next

Step 5 lifecycle handoff. Ask the plan owner to decide whether this audit
source idea is satisfied and should close, or whether lifecycle should switch
to one generated follow-up idea. The cleanest immediate activation candidate is
`ideas/open/shared-mir-same-block-producer-select-queries.md` because it has
the narrowest hook boundary and the least target-specific emission risk.

## Watchouts

- This is an audit run; do not implement broad code movement.
- Do not create follow-up ideas for target-local final emission. The audit
  explicitly keeps AArch64 register spelling, ABI lane policy, stack-pointer
  sequences, ADRP/ADD, CMP/CSEL/branch forms, memory operands, and machine
  instruction construction in the AArch64 layer.
- Do not treat dynamic-stack or branch-fusion deferral as rejection forever;
  they need tighter target-hook boundaries or adjacent x86/RISC-V proof before
  becoming migration work.
- Publication planning, call-boundary ordering, and store-source planning are
  already captured as follow-up ideas. They are valuable but have higher
  hook-boundary risk than producer/select-chain query extraction.

## Proof

Audit-only `todo.md` update. No build or test proof required by the packet, and
no `test_after.log` was created.
