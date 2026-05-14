# Common MIR Container And Target Printer Boundary

Status: Closed
Created: 2026-05-14
Last-Updated: 2026-05-14

## Closure Note

Closed after the active runbook completed Step 7 with full-suite validation.
Backend MIR now has a common instruction-stream carrier under
`src/backend/mir/`, AArch64 public assembly generation walks that carrier, and
target spelling is delegated through AArch64 printer hooks. Remaining markdown
shard conversion work stays with idea 229 and is not part of this closure.

## Lifecycle Note

This idea was the parent context for the common MIR/container boundary failure
family, and was superseded during execution by the AArch64 module phoenix
rebuild sequence:

- `ideas/open/225_aarch64_module_phoenix_extract_legacy_evidence.md`
- `ideas/open/226_aarch64_module_phoenix_review_replacement_layout.md`
- `ideas/open/227_aarch64_module_phoenix_replacement_drafts.md`
- `ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md`

The superseding route follows the user clarification that prepared BIR should
lower directly to MIR machine nodes and that
`src/backend/mir/aarch64/module/module.cpp` is legacy evidence for extraction,
review, draft, and conversion, not the design to patch in place.

## Intent

Make backend MIR an assembly-shaped, target-independent instruction stream:
`Function -> Block -> Instruction`, with generic walking/printing code that
delegates target-specific instruction, operand, and register spelling to each
backend.

The MIR stream should be the primary data source for MIR printers, terminal
assembly printers, and future assembler/encoder handoff. Target-specific
snapshot records may still exist as prepared-contract metadata, but they should
not be confused with the main MIR instruction carrier.

## Owned Failure Family

This idea owns backend MIR layering problems where:

- the main MIR carrier is a flat target-local `machine_nodes` vector instead of
  a block-ordered instruction stream
- terminal assembly generation reads target-local node vectors directly instead
  of walking a common MIR container
- target metadata records such as frame, call, register, and prepared snapshot
  facts obscure the actual instruction stream
- common MIR code would need to know target mnemonics, register spellings, or
  memory operand syntax in order to print
- target printers pre-store display strings in MIR records instead of deriving
  them from IDs, register references, operands, and target print methods

## Scope Notes

Expected repair themes include:

- define common MIR containers under `src/backend/mir/`, not under a single
  target backend
- define common printable MIR nodes and operands under `src/backend/mir/`,
  including target opcode ids, physical registers, immediates, memory operands,
  symbols, and labels
- represent the primary stream as functions containing blocks containing
  instructions
- provide a generic MIR walker/printer that owns ordering, labels, indentation,
  section/function/block structure, and newline policy
- define a target printer interface for instruction/operator, operand, memory,
  register class/view, symbol, immediate, and label spelling
- migrate AArch64 assembly output to walk the common MIR container and delegate
  instruction spelling to AArch64 target code
- keep target-specific instruction records and selection status as the
  instruction payload while the common container settles, then narrow the shape
  if useful
- keep compatibility flat views only as temporary migration aids
- remove display-cache fields from target MIR records when the spelling is
  recoverable from IDs, name tables, register references, or source facts

## Boundaries

This idea does not own:

- adding broad new AArch64 instruction coverage beyond what is needed to prove
  the printer/container boundary
- changing prepared authority semantics for frame, call, register allocation,
  spill/reload, or storage plans
- replacing target-specific instruction selection with a full TableGen-like
  ISA description
- object encoding, ELF relocation emission, or linker behavior after assembly
  text has been produced
- x86 or RISC-V migration except as consumers of the common MIR container
  interface after the AArch64 path proves the shape

## Completion Signal

This idea is complete when backend MIR has a common instruction-stream carrier
under `src/backend/mir/`, AArch64 public assembly generation walks that carrier
instead of a flat target-local vector, and instruction/operand/register spelling
is delegated through target printer methods rather than being baked into common
MIR records or precomputed display strings.
