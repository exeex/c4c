Status: Active
Source Idea Path: ideas/open/117_bir_printer_structured_render_context.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Printer Spelling Authority

# Current Packet

## Just Finished

Completed `plan.md` Step 1 inventory for BIR dump spelling authority.

Classification:
- Structured-ready: scalar type spellings from `bir::TypeKind` through
  `render_type`; opcodes from `BinaryOpcode`/`CastOpcode`; immediates from
  `Value`; call argument types from lowered `Value::type`; call ABI suffixes
  (`sret`/`byval` size and align) from `CallArgAbiInfo`; return, branch, and
  conditional branch operand types from `Value::type`; indirect call targets
  through `callee_value`.
- Final spelling data: function names, parameter names, SSA result names, block
  labels, local slot names, global symbol names, string constant names reached
  through memory addresses or rewritten call args, direct call callee names,
  memory address base names, and phi observation labels/results are already
  carried as final BIR spellings rather than type-rendering authority.
- Legacy/raw-type fallback: `bir::CallInst::return_type_name` is the active
  spelling authority in `render_call_type_name()` whenever non-empty. Direct
  call lowering fills it from `call.return_type.str()` except for sret-as-void;
  runtime helper placeholders sometimes set `"void"` explicitly; inline asm
  uses trimmed `inline_asm.ret_type` raw text. The fallback to
  `render_type(call.result->type)` or `"void"` is structured but only runs when
  `return_type_name` is empty.
- Unresolved/not currently printed by `bir::print`: `Module::globals`,
  `Module::string_constants`, `target_triple`, and `data_layout` are carried in
  BIR but not rendered by the semantic BIR printer today. Prepared BIR embeds
  `bir::print(module.module)`, so the same classification governs prepared
  semantic text before prepared metadata.

First code-changing target for Step 2: add a minimal BIR-facing structured type
spelling context, preferably a narrow `bir::Module` extension populated during
LIR-to-BIR lowering from existing `LirStructDecl`/`StructNameTable` and backend
structured layout data. Keep `bir_printer.cpp` behavior-preserving at first:
scalar spellings still use `TypeKind`, final names stay unchanged, and
`CallInst::return_type_name` remains the compatibility fallback until Step 3
routes covered call return rendering through the structured context.

## Suggested Next

Delegate Step 2 to introduce the minimal structured type spelling context on
the BIR side and populate it from the existing structured layout handoff,
without changing `--dump-bir` output.

## Watchouts

- Preserve existing `--dump-bir` text.
- Keep legacy strings as fallback until idea 118 removes them.
- Do not make `bir_printer.cpp` parse legacy type declaration bodies.
- Raw-string printer fallbacks to cover here or defer to idea 118:
  `CallInst::return_type_name` for direct calls, explicit `"void"` helper
  placeholders, inline asm return text, and inline asm `raw_args` text. The
  direct-call return type path is the first fallback to cover in this runbook;
  inline asm raw text can be deferred unless Step 3 naturally needs it.

## Proof

`git diff --check -- todo.md` passed. This inventory-only packet did not run a
build or test subset; the delegated proof is sufficient for the owned
`todo.md` edit and does not produce `test_after.log`.
