# Current Packet

Status: Active
Source Idea Path: ideas/open/828_lir_direct_scalar_unary_fneg_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace unary fneg publication and verifier shape

## Just Finished

- Switched from 827 Step 2 after its required binary definition-type/operation-type mismatch was disproven by the first parent diagnostic operation; no code/test changes or acceptance proof were made in that interrupted step.

## Suggested Next

- Execute Step 1 only: trace unary lowering and the verifier relation for the known `double` ternary unary-minus diagnostic anchor, then record the smallest structured producer/verifier decision. Do not implement in this packet.

## Watchouts

- Preserve the dirty 821/822/825-related worktree changes. Do not broaden into binary producers, switch selectors, Raw-BIR/importer, or generic rows. The parent composite CTest is diagnostic only.

## Proof

- This is a diagnosis packet. Do not create or roll forward canonical regression logs. Select any implementation proof only after the trace establishes the bounded contract.
