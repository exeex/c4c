# Intrinsic And Inline-Assembly Binding Route

## Diagnostic Trace

First diagnostic action: enumerate `LirInlineAsmValueBinding` producers and
intrinsic lowerers, then identify verifier and lowerer consumers plus
malformed-binding coverage.

Primary locations:

- `src/codegen/lir/ir.hpp` defines `LirInlineAsmValueBinding`,
  `LirInlineAsmValueRole`, `LirInlineAsmOp`, and intrinsic-like instruction
  families such as memory/VA operations, stack save/restore, `LirAbsOp`, and
  ordinary `LirCallOp` builtin forms.
- `src/codegen/lir/hir_to_lir/stmt.cpp` constructs inline-assembly bindings
  from HIR inline asm operands, records `original_asm_text`,
  `original_constraint_text`, clobbers, ordinary inputs, ordinary results, and
  `.insn r` metadata.
- `src/codegen/lir/verify.cpp` verifies inline-assembly value bindings, scalar
  output-only authority, read/write pairing, constraint ordering, result
  uniqueness, type agreement, and downstream store-use type agreement.
- `src/backend/bir/lir_to_bir.cpp` validates and imports selected inline-asm
  forms through `InlineAsmSpec`, using `original_asm_text`,
  `original_constraint_text`, clobbers, and structured input/result bindings.
- `src/codegen/lir/hir_to_lir/call/builtin.cpp` lowers builtin/intrinsic-like
  calls including `ffs`, `ctz`, `clz`, `popcount`, `memcpy`, VA operations,
  alloca, overflow helpers, bswap, and floating predicates/math.
- `src/backend/bir/lir_to_bir.cpp` contains selected Raw-BIR receiver patterns
  for builtin `ctz`/`clz`/`ctpop`/`ffs` chains, inline-asm output stores, VA
  and memory authority rows, and stack-save/restore rows.

## Binding Inventory

| Family | Producer and binding representation | Verifier boundary | Lowerer / receiver boundary | Disposition |
| --- | --- | --- | --- | --- |
| Inline asm ordinary inputs/results | `stmt.cpp` builds `ordinary_inputs` and `ordinary_results` as `LirInlineAsmValueBinding` with `LirOperand`, `LirTypeRef`, role, and constraint index. | `verify.cpp` checks operand kind, module type refs, role/list agreement, constraint count/order, unique results, read/write input/result pairing, and scalar integer output-only result authority. | `validate_inline_asm_shape` admits bounded structured forms, and import builds `InlineAsmSpec` from original text, constraints, clobbers, structured inputs, and result types. | Native binding route exists for selected inline-asm value bindings. |
| Inline asm template/constraint/clobber payload | Producer copies `asm_template`, rendered constraints, `original_asm_text`, `original_constraint_text`, and clobbers. | Verifier counts constraints only to validate binding indices; it does not parse templates or constraints for value, type, ABI, or dispatch facts. | Raw-BIR import stores original template/constraint text and clobbers as opaque payload. | Opaque by policy; no repair should parse this text for authority. |
| Inline asm `.insn r` metadata | `stmt.cpp` optionally records opcode, funct3, funct7, and three operand indices. | Verifier checks operand indices are inside the constraint list. | Current Raw-BIR selected inline-asm receiver rejects `.insn_r` in `validate_inline_asm_shape`. | Native metadata exists, but no selected Raw-BIR receiver route is authorized here. |
| Builtin integer intrinsics `ffs`, `ctz`, `clz`, `popcount` | `call/builtin.cpp` prepares integer arguments and emits selected native call/result/final-use chains. | LIR verifier coverage is distributed through ordinary result/use authority checks and prior bounded producer handoffs; this research did not find one complete family-wide verifier inventory for all builtin integer forms. | Raw-BIR importer has selected pattern checks and receiver paths for accepted `ffs`, `ctz`, `clz`, and `ctpop` rows. | Selected rows are already bounded elsewhere; family-wide binding inventory remains incomplete. |
| Memory and VA intrinsics (`memcpy`, `memset`, `va_start`, `va_end`, `va_copy`, `va_arg`) | `call/builtin.cpp` emits dedicated LIR ops and selected authority carriers for some memory/VA rows. | `verify_native_memory_va_authority` verifies selected memory/VA pointer and integer authority shapes. | Raw-BIR has selected receiver rows for accepted memory/VA handoffs, including recently accepted direct-local `va_start`; other rows remain fail-closed. | Bounded selected rows exist; this route does not authorize a combined memory/VA sweep. |
| Stack save/restore and dynamic alloca | Dedicated LIR stack ops and alloca authority carriers exist for selected VLA/lifetime rows. | LIR verifier checks selected native stack/local-object authority in the relevant bounded routes. | Raw-BIR has selected `StackSaveAuthority` and `StackRestoreAuthority` receiver paths. | Already bounded by prior owner routes, not a new 849 repair target. |
| Other builtin floating/math/overflow/bswap/classification forms | `call/builtin.cpp` emits a mix of direct lowered operations, helper calls, and rendered call strings. | This audit did not find a complete native binding/verifier map for every form. | No single Raw-BIR receiver family covers all of these as intrinsic bindings; accepted selected rows remain separate. | Missing evidence; future work must select one singular 796 or 846 family. |

## Positive And Malformed Evidence

Positive inline-asm binding evidence:

- `LirInlineAsmValueBinding` keeps value identity, type, role, and constraint
  index separate from template and constraint text.
- LIR verifier accepts structured output-only scalar integer inline asm when it
  has exactly one output binding with matching type and result value ID.
- Raw-BIR importer lowers selected inline asm into `InlineAsmSpec` using
  structured inputs/results and opaque original payload.

Malformed inline-asm rejection evidence:

- Missing semantic constraints with structured bindings reject unless the
  special scalar output-only case applies.
- Binding roles must match their input/result list or be explicit read/write.
- Constraint indices must be in range and strictly ordered.
- Produced result identities must be unique and distinct from ordinary inputs.
- Read/write results require matching read/write inputs with the same type.
- Downstream store use of inline-asm output must match the binding type.
- Raw-BIR validation rejects `.insn_r`, unsupported native output types,
  duplicate result IDs, unsupported structured value shapes, unmatched
  read/write pairs, and selected outputs not consumed by exactly one admitted
  store.

Positive selected-intrinsic evidence:

- Existing Raw-BIR receiver tests cover bounded selected builtin chains for
  `ffs`, `ctz`, `clz`, and `ctpop`.
- Existing memory/VA/object/lifetime routes cover selected `memcpy` and
  `va_start` authority rows, while selected VLA stack routes cover stack
  save/restore.

Missing or unproved evidence:

- There is no complete producer-to-verifier-to-lowerer inventory for every
  builtin/intrinsic-like form in `call/builtin.cpp`.
- Floating predicate/math, overflow, bswap, classification, and other helper
  forms are not proven by this audit as a single native binding family.
- Inline asm `.insn_r` has metadata validation but no selected Raw-BIR receiver
  route in this evidence.

## Opacity Boundary

Inline-assembly templates and constraints remain opaque outward payload. The
verifier counts constraint positions only to validate structured binding
indices, and Raw-BIR import stores original template and constraint text rather
than parsing it. Binding, type, value identity, ABI, and dispatch facts must
come from `LirInlineAsmValueBinding`, typed LIR operands, selected authority
carriers, or dedicated native metadata. They must not be reconstructed from
template text, rendered constraints, rendered arguments, or LLVM call spelling.

## Return Relation

796 is the correct future owner for a singular residual instruction,
terminator, or inline-assembly value/type handoff when the selected family has
native facts and malformed-authority proof. Candidate examples from this audit
include a bounded `.insn_r` receiver route or another selected inline-asm
binding family, but only after a dedicated source idea names exact facts and
proof.

846 is the correct future owner when the missing work is verifier, dispatch, or
printer overload migration for an already-published native fact. Candidate
examples include family-specific verifier/dispatch coverage for builtin forms
whose producer facts already exist.

734 remains downstream and receives nothing directly from 849. 797 remains
downstream of 734. A future repair must first be accepted by 796 or 846 with a
singular family, native fact, and exact return path.
