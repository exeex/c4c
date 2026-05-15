Status: Active
Source Idea Path: ideas/open/238_aarch64_atomic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Atomic Authority And Fail-Closed Gaps

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for
`ideas/open/238_aarch64_atomic_machine_nodes.md`.

## Suggested Next

Begin Step 1 `Inventory Atomic Authority And Fail-Closed Gaps`: inspect current
BIR, prepared, AArch64 selection, and printer support for atomic ordering,
fences, RMW, and compare-exchange facts, then add focused fail-closed tests or
notes without weakening unsupported expectations.

## Watchouts

- Do not infer atomic semantics from volatile flags, rendered text, or archived
  fixed scratch-register snippets.
- Keep unsupported or partial atomic facts fail-closed.
- Do not fold intrinsic, inline-assembly, binary128, scalar FP, or i128 behavior
  into this route.

## Proof

Lifecycle-only activation. No build or test proof required.
