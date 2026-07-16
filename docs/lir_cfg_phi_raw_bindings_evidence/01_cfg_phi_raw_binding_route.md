# CFG And PHI Raw-Binding Route

## Diagnostic Trace

First diagnostic action: map every `LirPhi` and terminator operand through
verifier and BIR receiver paths, including malformed predecessor/value pairs.

Primary locations:

- `src/codegen/lir/ir.hpp` defines `LirPhiIncoming`, `LirPhiOp`,
  `LirBr`, `LirCondBr`, `LirRet`, `LirSwitch`, `LirIndirectBr`,
  `LirIndirectBrOp`, and `LirUnreachable`.
- `src/codegen/lir/verify.cpp` verifies PHI incoming values, predecessor block
  IDs, successor occurrences, direct and conditional successor IDs, switch
  selector/successor authority, return value/type authority, and indirect
  branch address/successor authority.
- `src/backend/bir/lir_to_bir.cpp` validates the same current-function IDs and
  lowers them to Raw-BIR PHI and terminator forms.
- `tests/backend/bir/backend_lir_to_bir_interface_test.cpp` contains accepted
  bounded receiver evidence for direct branch, conditional branch, switch,
  indirect branch, PHI edge authority, loop/backedge PHIs, and scalar returns.

## Per-Form Map

| Form | Native fields | Raw/display fields | Verifier boundary | Raw-BIR receiver boundary | Disposition |
| --- | --- | --- | --- | --- | --- |
| `LirPhiIncoming` / `LirPhiOp` | `value` as authoritative `LirOperand`, `predecessor` as `LirBlockId`, `successor_occurrence` as typed occurrence, `boundary_value_type`, result `LirValueId`. | `label` is predecessor display mirror; `type_str` is checked against boundary type. | Verifier requires known current-function value IDs or closed special tokens, exact predecessor block, matching display label, valid successor occurrence selecting the PHI block, no duplicate selected occurrence, and complete coverage of every predecessor occurrence to the PHI block. | Import reserves PHI result by `LirValueId`, maps predecessor block ID and successor occurrence to a Raw-BIR `PhiEdge`, materializes special tokens, and appends `PhiSpec`. | Accepted bounded receiver evidence exists; no raw predecessor/value recovery is needed for modeled PHIs. |
| Direct branch `LirBr` | `successor` as `LirBlockId`. | `target_label` is display shadow. | Verifier requires one current-function block and matching label. | Import resolves the block ID and emits a Raw-BIR direct jump. | Accepted bounded receiver evidence exists. |
| Conditional branch `LirCondBr` | `condition` as current-function `LirValueId`, `true_successor` and `false_successor` as `LirBlockId`s. | `cond_name`, `true_label`, and `false_label` are display shadows. | Verifier requires the condition to identify the comparison result and display name to match; both successors must identify current-function blocks and matching labels. | Import requires condition to resolve to `i1`, resolves both successor IDs, and emits `CondJumpTerm`; parallel true/false successor occurrences remain distinct edges. | Accepted bounded receiver evidence exists. |
| Switch `LirSwitch` | `selector` as `LirValueId`, `selector_type_ref`, `default_successor`, ordered `case_successors`, optional selected parameter authority. | `selector_name`, `selector_type`, `default_label`, and case labels are display shadows. | Verifier requires structured selector type, selector value, one successor per case, matching labels, and selected parameter authority when applicable. | Import requires selector to resolve to an integer value, validates default and ordered case successors, and emits `SwitchTerm`. | Accepted bounded receiver evidence exists for terminator and selected parameter selector rows; no family-wide new handoff is authorized here. |
| Return `LirRet` | `value_str` may carry authoritative operand alternatives; `type_str` is structured type; optional selected return-value parameter authority exists for bounded parameter rows. | Existing field name remains source compatible; raw text remains allowed only for unsupported compatibility. | Verifier checks void/non-void coherence, authoritative integer scalar returns, immediate range, SSA value authority, and selected parameter return authority when present. | Import supports void returns and selected scalar integer returns whose value resolves exactly to signature type. | Accepted bounded receiver evidence exists for scalar returns and selected parameter returns; aggregate/raw returns remain outside this route. |
| Terminator `LirIndirectBr` | `addr` as `LirValueId`, ordered target `LirBlockId`s. | None in this compact terminator form. | Verifier validates pointer address and unique current-function targets. | Import resolves pointer address and targets to `IndirectJumpTerm`. | Native compact form is modeled, but current selected receiver evidence often uses `LirIndirectBrOp`. |
| Instruction carrier `LirIndirectBrOp` | `addr_value` as pointer `LirValueId`, ordered `successors` as `LirBlockId`s. | `addr` and `targets` are display mirrors. | Verifier requires address identity, direct-constant consistency where present, pointer definition, display mirror match, one successor per target, unique current-function successors, and matching labels. | Import requires the op to be final instruction with unreachable sentinel, resolves address and successors, then emits an indirect jump terminator. | Accepted bounded receiver evidence exists. |
| `LirUnreachable` | No value or block payload. | None. | Supported as a terminator sentinel. | Imported directly or used as the sentinel behind `LirIndirectBrOp`. | No raw seam. |

## Positive And Malformed Evidence

Positive evidence:

- Direct branch receiver tests prove `LirBr.successor` maps to the Raw-BIR jump
  target without label recovery.
- Conditional branch tests prove `LirCondBr.condition`,
  `true_successor`, and `false_successor` preserve condition and ordered edge
  authority, including parallel successor occurrences.
- Switch tests prove selector, default successor, and ordered case successor
  authority reach Raw BIR, including distinct repeated destinations.
- PHI tests prove incoming value/predecessor/successor-occurrence authority
  reaches Raw-BIR PHI edges, including loop/backedge and parallel-edge cases.
- Indirect branch tests prove current-function pointer address and target block
  IDs reach Raw-BIR indirect jump form.

Malformed evidence expectations already represented by verifier/importer/tests:

- PHI rejects missing value authority, invalid/foreign predecessor blocks,
  mismatched predecessor labels, missing or incoherent successor occurrence,
  duplicate selected occurrence, and incomplete coverage of predecessor
  occurrences.
- Direct and conditional branches reject invalid, foreign, or mismatched
  successor IDs and display shadows; conditional branch also rejects missing,
  foreign, or non-boolean condition authority.
- Switch rejects missing selector authority, non-integer selector type,
  mismatched case successor count, invalid default or case successors, and
  selected parameter authority mismatch.
- Indirect branch rejects missing pointer address authority, invalid direct
  label-address constants, duplicate or unresolved successors, and mismatch
  between display targets and selected block IDs.
- Return rejects void/non-void mismatches, out-of-range immediates, unsupported
  scalar authority alternatives, wrong signature type, and selected
  return-parameter authority mismatch.

## Accepted 734 Receipts Versus Remaining Raw Seams

Accepted 734 bounded receipts already cover the modeled CFG/PHI forms above at
the Raw-BIR receiver boundary. They are not reopened by this evidence route.

The remaining risk is not a specific unreceived `LirPhi` or terminator form in
the current modeled surface; it is evidence discipline for any future raw or
legacy seam. A future producer handoff is required only if a new or currently
unsupported form still carries block, value, type, owner, predecessor, or edge
facts solely in rendered text or raw operands.

No exact new producer handoff is proved by this document. Therefore no new 734
receiver row is authorized by 850.

## Return Relation

If future work discovers a concrete raw CFG/PHI seam, a separately scoped
producer/verifier owner must first publish native block/value/type/edge facts
and malformed-boundary proof. Only after that handoff may 734 receive one
bounded Raw-BIR row. 797 remains downstream of the 734 disposition.

Text-based reconstruction of block labels, value names, predecessor labels,
case labels, or LLVM terminator spelling remains rejected as authority.
