Status: Active
Source Idea Path: ideas/open/240_aarch64_inline_asm_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebaseline Remaining Inline-Asm Gap Matrix

# Current Packet

## Just Finished

Plan Step 1 - Rebaseline Remaining Inline-Asm Gap Matrix: recorded the
remaining AArch64 inline-asm support matrix and selected the first narrow
implementation packet.

| Surface | Current supported path | Diagnostic-only now | Follow-up ownership |
| --- | --- | --- | --- |
| Named operands | Operand names already survive in BIR metadata, prepared carriers, and selected `InlineAsmMachineOperandRecord::name` for register inputs when the source/LIR metadata provides them. | `%[name]` template references still fail closed through `has_named_operand_references` in dispatch and printer; missing/unknown names must stay diagnostic-only. | Current backend carrier path can support register and immediate `%[name]` substitution by resolving names against selected structured operands. Duplicate-name policy or absent source/LIR names should remain diagnostic-only unless source/LIR grows stronger name authority. |
| Clobbers | Clobber spellings are visible as constraints and prepared fail-closed facts; unrelated direct-call helper clobber machinery already has structured machine-effect modeling. | Inline-asm `Clobber` operands and clobber lists are rejected with structured-clobber-authority diagnostics; no inline-asm clobber carrier exists yet. | Backend can add carriage only after source/LIR/BIR provides structured clobber facts. Register-pool filtering, callee-saved save/restore, or allocator-visible clobber policy belongs to allocator/source-LIR follow-up, not this printer/dispatch packet. |
| Memory/address constraints | Existing prepared memory/address records support ordinary load/store and intrinsic memory operands, but inline-asm carriers currently support only register, tied, and integer-immediate operands. | `m` and `~{memory}` remain unsupported facts; inline-asm memory operands have no selected memory/address operand record. | Backend carrier path can only accept memory/address forms that reuse existing prepared pointer/frame-slot authority. Any scratch register, spill, address materialization, or constraint-alternative decision needs allocator/source-LIR ownership. |
| Tied output coallocation | Numeric tied inputs already carry `tied_output_index`; current fixtures use matching prepared homes and printer substitutes the selected output operand. | Missing tie index, missing selected tied output, and missing homes fail closed; there is no allocator decision to force coallocation. | Current backend path can tighten diagnostics by verifying tied input/output homes are the same before selection. Creating shared homes when they differ is allocator-policy follow-up. |

## Suggested Next

Implement named operand substitution as the first code packet. Target `%[name]`
references for already-selected register and integer-immediate operands by
removing the blanket backend rejection, resolving the bracketed name against
`InlineAsmMachineOperandRecord::name`, and preserving explicit diagnostics for
unknown, duplicate, malformed, or unsupported named operands. Suggested proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_instruction_dispatch|aarch64_machine_printer|prepared_printer)'`.

## Watchouts

- Do not touch `plan.md` or `ideas/open/240_aarch64_inline_asm_machine_nodes.md`
  for this Step 1 slice.
- Preserve completed positional register, single output, numeric tie, integer
  immediate, `%wN`/`%xN`, and side-effect behavior.
- Named operands are the cleanest backend-local packet because name metadata is
  already present on selected operands; clobbers and memory/address constraints
  are likely to need upstream/source-LIR or allocator ownership before real
  support.
- Tied coallocation must not be "fixed" by printing the output register when
  prepared homes disagree; mismatches should be diagnostics unless allocator
  ownership supplies a shared home.

## Proof

Inventory-only lifecycle/progress update. No build or test command was
required, and `test_after.log` was intentionally not touched.
