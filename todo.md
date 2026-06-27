Status: Active
Source Idea Path: ideas/open/418_prepared_target_consumer_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Draft The Audit Artifact

# Current Packet

## Just Finished

Completed Step 2 by drafting
`docs/prepared_fact_contracts/target_consumer_boundary_audit.md` from the
Step 1 inventory and idea 413 taxonomy matrix.

The audit artifact now records row-level classifications with audit row IDs,
target/helper surfaces, consumed taxonomy rows, owner classification, current
behavior, required consumer behavior, and downstream owner ideas. It separates
acceptable coherent-fact lowering rows from suspected target recovery or
producer-gap rows:

- Coherent/reference rows:
  `418-AUD-RV64-CALL-COHERENT-001`,
  `418-AUD-RV64-BYVAL-COHERENT-001`,
  `418-AUD-RV64-INLINEASM-COHERENT-001`,
  `418-AUD-A64-VALUE-COHERENT-001`,
  `418-AUD-A64-CALL-COHERENT-001`,
  `418-AUD-A64-VARIADIC-COHERENT-001`, and
  `418-AUD-A64-INLINEASM-COHERENT-001`.
- Recovery/review rows:
  `418-AUD-RV64-FRAME-SLOT-CALL-RECOVERY-001`,
  `418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001`,
  `418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001`,
  `418-AUD-A64-FRAME-SLOT-PUBLICATION-REVIEW-001`, and
  `418-AUD-A64-STACK-PRESERVE-RECOVERY-001`.

The artifact also includes downstream handoff rows for ideas 414, 415, 416,
and 417. For Step 3, it identifies RV64 global/object-data recovery as the
clearest low-risk cleanup or owner-decision candidate, with file-reference
evidence for raw initializer-byte and fallback symbol reconstruction.

## Suggested Next

Execute Step 3 by making one low-risk cleanup or owner decision. Suggested
packet: record a concrete owner decision for
`418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001` and
`418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001` in the audit artifact, unless the
supervisor chooses a small code cleanup instead.

## Watchouts

- `docs/prepared_fact_contracts/target_consumer_boundary_audit.md` is now the
  canonical row map for this active plan's downstream handoff.
- RV64 global/object helpers remain the clearest first Step 3 decision point:
  `prepared_global_memory_emit.cpp` reconstructs initializer words, labels,
  and fallback symbol names; `object_emission.cpp` reconstructs object bytes
  and sections from raw BIR globals/string constants.
- AArch64 value operands, variadic helpers, and inline asm rows are coherent
  reference rows, not current cleanup targets.
- The AArch64 frame-slot publication and stack-preservation rows should not be
  changed without first deciding which silent skip paths are optional
  non-applicable routes versus required-but-missing producer facts.

## Proof

Docs-only packet. No build or test command was required or run. Proof is the
row/file-reference evidence in
`docs/prepared_fact_contracts/target_consumer_boundary_audit.md`; no
`test_after.log` was created or modified.
