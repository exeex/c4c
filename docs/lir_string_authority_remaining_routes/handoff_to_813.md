# 812 unresolved semantic-route handoff

Evidence revision: `d58b8d44c9b64d2005d2b3760a0592b1b47ebd03`.

This handoff is a direct selection from the Step 2
[disposition matrix](field_callsite_disposition_matrix.md).  It contains all
and only rows whose disposition is `unowned semantic route` or `insufficient
evidence`.  At this revision the selection is three `insufficient evidence`
rows; there are no `unowned semantic route` rows.  The keys below are stable
routing keys, not successor numbers or a lifecycle change.

| Stable routing key | Matrix row and missing evidence | First diagnostic action / proof boundary | Boundary |
| --- | --- | --- | --- |
| `global.policy-identity-evidence` | Global linkage, visibility, qualifier policy, and symbol identity have dedicated fields and `LinkNameId` where present, but this audit has not established policy-field producer-to-BIR-receiver coverage. | Inspect every policy-field producer and BIR receiver; begin with `rg` of `LirGlobal` fields into globals lowering.  A later route must prove each covered policy fact reaches its receiver without inferring it from rendered declaration text. | This does not reopen the checked `llvm_type`/declaration-shadow rows, absent-metadata compatibility, initializer scans, or existing global/extern ownership. |
| `instruction.intrinsic-binding-evidence` | Ordinary inline-assembly values need `LirInlineAsmValueBinding`, while other intrinsic forms vary; a complete producer/lowerer binding audit is absent. | Enumerate `LirInlineAsmValueBinding` producers and intrinsic lowerers, then require malformed-binding proof. | Inline-assembly templates and constraints remain opaque outward payload; their text must not be parsed for binding, type, value, ABI, or dispatch facts.  Existing verifier/dispatch ownership is unchanged. |
| `cfg.phi-raw-bindings-evidence` | Block/value IDs exist where modeled, but coverage of raw terminator/PHI operand seams is not established. | Map every `LirPhi` and terminator operand through verifier and BIR receivers, including malformed predecessor/value pairs. | This is limited to CFG/PHI operand and incoming-binding evidence.  It does not claim call/body-parameter, general operand, or receiver-family ownership. |

## Selection check

The matrix has 29 rows: 3 `insufficient evidence`, 0 `unowned semantic route`,
19 `existing open owner`, 4 `intentional opaque/render text`, 2
`structured-authoritative`, and 1 `checked mirror`.  The three table keys are
the complete qualifying-key set and no existing-owner or intentional-text row
is included.
