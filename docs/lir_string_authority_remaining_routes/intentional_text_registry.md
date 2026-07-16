# 812 intentional-text registry

Evidence revision: `d58b8d44c9b64d2005d2b3760a0592b1b47ebd03`.

These are not blanket exemptions. Each is text that is intrinsically payload or one-way final rendering, and it is forbidden to become semantic input for identity, lookup, type/owner recovery, verification, or dispatch.

| Text family | Current location | Rationale | Forbidden semantic use |
| --- | --- | --- | --- |
| Inline-assembly template and constraints | `LirInlineAsmOp` printing at `src/codegen/lir/lir_printer.cpp:132-134`; 761 closure. | Target-language payload; ordinary bindings have separate `LirInlineAsmValueBinding` facts. | Parse template/constraints to recover operand type, value identity, ABI role, or dispatch facts. |
| Raw literal/data bytes and global initializer spelling | `ir.hpp:1092-1098`, `lir_printer.cpp:627-630`. | LLVM data payload/final output spelling. | Infer global symbol/type/owner/reference authority from rendered initializer text. |
| Final LLVM rendering and link-name presentation | `lir_printer.cpp:24-49,542-545`. | A one-way output bridge; `LinkNameId` is stated semantic identity. | Parse rendered headers/names to repair declaration, callee, or signature authority. |
| Diagnostics and display/mangled names | Display paths and emitted textual diagnostics. | Human-facing output; semantic IDs are separately carried where supported. | Use wording/name shape/testcase spelling as verifier, lookup, or routing input. |
| Data-layout/target spellings | Backend target/data-layout configuration surfaces. | Target ABI configuration payload, not a LIR value/type identity. | Classify a LIR type or reconstruct an owner/value identity from target text. |
| Compatibility call text | `call_args.hpp` / `call_args_ops.hpp`. | Retained only while structured mirrors are incomplete. | This is not exempt when parsers/collectors consume it; Step 2 must classify those semantic consumers. |

The final row is deliberately a non-exemption: its existence keeps parsers and reference scans in the exhaustive matrix. Intrinsic text may render outward; it may not become an inward semantic authority channel.
