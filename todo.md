Status: Active
Source Idea Path: ideas/open/161_bir_memory_access_identity_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Memory Access Surfaces

# Current Packet

## Just Finished

Lifecycle activation created the runbook for Step 1 of
`ideas/open/161_bir_memory_access_identity_annotation_schema.md`.

## Suggested Next

Execute Step 1: inventory direct memory access, same-block global-load
identity, and same-block load-local source surfaces, then record concrete BIR
instruction/value schema targets, oracle helpers, positive and negative
coverage, and the narrow proof command for the first code-changing packet.

## Watchouts

- Keep Route 3 annotations target-neutral and semantic.
- Do not import frame slot ids, byte offsets, size/align layout, relocation
  spelling, TLS register details, addressing-mode legality, or AArch64 memory
  operand formation.
- Do not copy `PreparedAddress` or `PreparedMemoryAccess` wholesale as the BIR
  schema shape.
- Preserve explicit negative cases for volatile, address-space, local-source,
  and no-source behavior where applicable.

## Proof

Activation only. No build or test proof required.
