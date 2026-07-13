Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Freeze inventory, constraint table, and schema checkpoint

# Current Packet

## Just Finished

- Lifecycle activation only; no implementation packet has completed for this
  idea yet.

## Suggested Next

- Execute plan Step 1: produce the evidence-backed inline-asm flow inventory,
  closed constraint table, and BIR/MIR/regalloc/late-assembler schema checkpoint.

## Watchouts

- Do not implement before the target-confirmed `VR`/`VRM*` group set,
  alignment, and contiguity rules are frozen and reviewed.
- Keep asm instruction/template payload opaque; constraint normalization is a
  separate early semantic operation and must not become mnemonic parsing.
- Do not apply `stash@{0}` or revive legacy BIR, prealloc, MIR, or `c4c-as`.

## Proof

- Lifecycle-only packet; `git diff --check` passed and local review state was
  reset to Step 1 with no pending review or baseline reminder.
