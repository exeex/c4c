# Inline-Assembly Transport Checkpoint

This directory freezes the Step 1 contract for
`ideas/open/731_inline_asm_transport_and_regalloc_contract.md`.

- [01_active_inventory.md](01_active_inventory.md) traces the active source,
  HIR, LIR, and new-BIR boundaries and labels quarantined evidence.
- [02_closed_constraint_contract.md](02_closed_constraint_contract.md) is the
  closed RV64 constraint, group, template-reference, and rejection table.
- [03_schema_checkpoint.md](03_schema_checkpoint.md) freezes the BIR, MIR,
  allocator, late-assembler, ownership, and proof shapes.

The governing rule is that decoded source template bytes remain opaque through
allocation. Constraint normalization is a separate semantic operation. No
stage before the late assembler may recognize a mnemonic, directive, or
`.insn` form.
