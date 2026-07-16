# 812 evidence baseline

Evidence revision: `d58b8d44c9b64d2005d2b3760a0592b1b47ebd03` (captured 2026-07-16).

This is a read-only inventory baseline for 812, not a disposition matrix or a semantic-completion claim. Step 2 must use this revision when tracing each producer through its consumers.

## Surfaces scanned

| Surface | Traceable current evidence | Why it remains in the Step 2 audit |
| --- | --- | --- |
| Type carrier | `src/codegen/lir/types.hpp:93-140` has `runtime_text` and named compatibility factories; `:189-251` exposes mutable `str()` and implicit string conversions; `:410-474` parses/classifies text. | Builtin/composite facts can be structural, but the compatibility escape hatch and nested/dynamic forms must be traced before asserting text cannot classify semantics. |
| Operand carrier | `src/codegen/lir/operands.hpp:78-81,119-149,247` classifies text and offers `LirOperand::raw`, mutable text, and implicit conversions. | Raw/rendered operand spelling can reach verifier/lowering consumers. |
| Calls and collection | `src/codegen/lir/call_args.hpp:156-244,442-550,633-737` parses typed arguments, scans value/global references, and rewrites text; `call_args_ops.hpp:21-29,157-284` chooses structured views or parses `callee_type_suffix`/`args_str`. | The 761 structured mirror is bounded; fallback parsing and text scans are still live. |
| LIR schema/verifier/printer | `src/codegen/lir/ir.hpp:1054,1092-1098,1305-1308` retains `signature_text`, `llvm_type`, `init_text`, and `type_decls`; `verify.cpp:33-53` verifies operand kind; `lir_printer.cpp:24-49,132-134,542-630` renders/signature-rewrites and emits inline-asm, global, and declaration text. | Separates checked/final mirrors from semantic fallback paths. |
| HIR producers | `src/codegen/lir/hir_to_lir/` and the named `hir_*_type_text` factories in `types.hpp:128-140`. | HIR-derived aggregate, field, signature, and inline-asm text must be traced to first semantic consumer. |
| LIR-to-BIR consumers | `src/backend/bir/lir_to_bir/{types,aggregate,calling,globals,global_initializers,context}.cpp`; e.g. `types.cpp:442-646`, `calling.cpp:1031-1400`, `globals.cpp:445-575`, `global_initializers.cpp:12-188`. | Current lowering still parses type/call/global initializer text at named compatibility paths; structured alternatives must be proved per consumer. |

## Closure and open-owner inventory

Closure reconciliation is in [closure_reconciliation.md](closure_reconciliation.md). The current open scope and return chains are in [existing_owner_map.md](existing_owner_map.md). The text that is intrinsically payload/render-only is separately bounded in [intentional_text_registry.md](intentional_text_registry.md). No entry upgrades an untraced consumer to structured authority.

## Baseline constraints for later steps

- Use the captured revision above, not historical test counts, to identify current declarations and consumers.
- Treat a structural outer `LirTypeRef` as evidence only for that outer fact; inspect element, field, parameter, return, and child refs independently.
- Do not recover identity/type/owner/reference facts from printer output, names, diagnostics, or testcase spelling.
- Existing open owners retain their exact routes; this inventory creates no successor and changes no lifecycle state.
