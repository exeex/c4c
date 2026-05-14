Status: Active
Source Idea Path: ideas/open/227_aarch64_module_phoenix_replacement_drafts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Draft Operand Authority

# Current Packet

## Just Finished

Completed plan Step 3 by adding
`src/backend/mir/aarch64/module/operand_resolution.cpp.md` as the Stage 3
operand authority replacement draft. The draft defines typed prepared
value-location authority, target register and `MachineOperand` conversion,
storage precedence normalization, immediates, labels, symbols, memory forms,
fail-closed compatibility register fallback, owned inputs/outputs, indirect
queries, forbidden knowledge, classification, and downstream consumption by
instruction, branch/control, and call lowering without broad prepared tables,
source spellings, broad optional public records, or flat `machine_nodes` as
semantic inputs.

## Suggested Next

Delegate Step 4 to draft instruction and branch/control lowering against the
typed operand authority seam, keeping operation-specific lowering out of
operand resolution.

## Watchouts

- Stage 3 is draft-only; do not edit real `.cpp`, `.hpp`, build, or test files.
- Do not delete, disable, or build-disconnect the compiled legacy `module.cpp`.
- Do not add component-level public headers. `helper.hpp` is the only allowed
  header exception and must be justified before drafting.
- Do not make `FunctionRecord::machine_nodes`, cached display strings, spelling
  recovery, or broad public inspection records semantic lowering authority.
- Do not introduce a target render API named `__repr__`.
- If the Stage 2 artifact map appears wrong, stop and report a Stage 2 contract
  repair blocker instead of silently adding or removing files.
- Step 3 should keep storage precedence inside operand resolution so
  instruction, branch, and call lowering do not choose among broad optional
  public records.
- Register spelling fallback belongs only in fail-closed compatibility
  diagnostics; structured prepared facts must remain authoritative.
- Do not let operand resolution reintroduce public `FunctionRecord` records or
  flat `machine_nodes` as semantic inputs.
- Step 4 consumers should pass use-specific context to operand resolution and
  consume typed `MachineOperand`/`MachineRegister` values rather than rechecking
  value-home, storage-plan, regalloc, ABI, or source spelling tables.

## Proof

Markdown-only proof written to `test_after.log` with concise read-only `rg`
checks over `operand_resolution.cpp.md` and `todo.md`. The proof confirms the
operand draft exists as a Stage 3 replacement draft; covers typed prepared
value-location authority, target register/operand conversion, storage
precedence normalization, immediates, labels, symbols, memory forms,
fail-closed compatibility register fallback, owned inputs/outputs, indirect
queries, forbidden knowledge, classification, downstream typed operand
consumption, and rejection of broad optional public records and flat
`machine_nodes` as semantic inputs. No build was required for this
markdown-only draft packet.
