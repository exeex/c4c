# Current Packet

Status: Active
Source Idea Path: ideas/open/826_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify the selected truthiness-comparison LHS authority contract
你該做code review了

## Just Finished

- Step 1 selection complete: select one unchanged current-function
  `DirectScalar` integer parameter as `LirCmpOp.lhs` in the native truthiness
  compare emitted by `StmtEmitter::to_bool_operand`; its exact consumer
  relation is the parameter `LirValueId`/type on the comparison LHS, with
  integer `ne` against the authoritative zero immediate. The native definition
  supplies owner, index, type, and ABI. No authority has been published yet.

## Suggested Next

- Step 2: publish and verifier-check only the selected direct-scalar integer
  truthiness-comparison-LHS authority row, keeping every other comparison and
  parameter form fail closed.

## Watchouts

- Do not edit Raw-BIR/importer/receiver code or reopen accepted 734 parameter
  rows.
- Do not derive identity from names, signatures, rendered operands,
  diagnostics, `monostate`, or testcase shape.
- `LirCondBr.condition` is a comparison-result authority, not the direct
  parameter-use seam; keep the row bounded to `LirCmpOp.lhs`.

## Proof

- No proof ran: this Step 1 evidence packet makes no code change or authority
  publication. Step 2 needs a supervisor-selected focused producer proof.
