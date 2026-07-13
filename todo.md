Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reconcile BIR READMEs and prepare the closure audit

# Current Packet

## Just Finished

- Completed Plan Step 5: reconciled the three scoped BIR READMEs with the final
  proven bounded inline-asm value transport, payload authority, importer
  transaction/rejection boundary, and current verifier checks.

## Suggested Next

- Ask the plan owner to perform the runbook closure/deactivation audit while
  preserving the Closure Note Audit below in the lifecycle note.

## Watchouts

- The reconciled current sections are intentionally narrower than the target
  design sections; do not read the target schema as implemented behavior.
- Register allocation, spill/reload, MIR, general opcode import, parsed target
  constraints, and asm-goto remain outside this completed runbook.

## Closure Note Audit

- Intentional deferred scope: complete parsed constraint alternatives,
  symbolic operands, symbol/address-space facts, and asm-goto topology remain
  target schema beyond the implemented generic non-goto SSA edges.
- Intentional deferred scope: specialized inline-asm structure, constraint,
  tie, clobber, and effect verifier rules remain target contract; the current
  verifier enforces closed opcode/payload shape and generic value integrity.
- Intentional deferred scope: general LIR opcode/module import remains outside
  the bounded zero-parameter, void, inline-asm-only importer surface.
- Intentional deferred scope: target constraint preparation, register
  allocation, spill/reload, frame facts, target opcodes, and MIR remain later
  stage ownership and are not BIR state.
- Accidental desynchronization: none remain in the three Step 5 README surfaces
  after reconciliation against the final implementation.

## Proof

- Passed: `git diff --check -- src/backend/bir/core/README.md
  src/backend/bir/lir_to_bir/README.md src/backend/bir/verify/README.md
  todo.md`.
- Documentation-only proof; no regression log was changed.
