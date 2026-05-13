# Current Packet

Status: Active
Source Idea Path: ideas/open/205_aarch64_arm_reference_layout_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Current AArch64 And ARM Reference Surfaces

## Just Finished

Activation created the runbook and initialized executor-compatible packet
state. No implementation or documentation contract work has started.

## Suggested Next

Begin Step 1 by auditing the ARM reference layout, current AArch64 markdown
contracts, and live `api/`, `abi/`, and `module/` files. Record the chosen
ledger target, covered feature families, and exact proof method here before
moving to Step 2.

## Watchouts

- Do not edit `ideas/open/205_aarch64_arm_reference_layout_contract.md` unless
  source intent itself proves wrong.
- Do not add instruction selection, assembly emission, object writing, linker
  behavior, or target-local workarounds for missing BIR/prepared facts.
- Do not expand `module/module.cpp` or `module/module.hpp` as a convenient
  destination without first deciding ownership in the ledger.
- Do not recover backend facts from rendered names, printed BIR, old markdown
  examples, assembly-string parsing, parser operands, or legacy shape matching.
- Split required missing BIR/prepared carriers into separate `ideas/open/`
  source ideas with concrete reviewer reject signals.
- Preserve unrelated dirty files and transient `review/` artifacts.

## Proof

Lifecycle activation only; no build or test proof was required for this packet.
