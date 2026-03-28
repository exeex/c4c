# Backend Binary Utils Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

This idea should remain later than the target codegen bring-up ideas.

## Goal

Replace the external assembler/linker fallback with built-in binary utilities only after target codegen is already functionally useful.

## Why This Is Separate

Built-in assembler and linker work are not just follow-up cleanup.

They are substantial projects with their own correctness surface:

- instruction parsing
- encoding
- ELF object writing
- relocation handling
- archive handling
- symbol resolution
- executable layout

Keeping them separate prevents early backend bring-up from being blocked by binary-format work.

## Scope

### Built-in assembler

- decide whether assembler work is still syntax-driven or can consume a more structured internal form
- implement per-target object emission in roadmap priority order
- validate emitted objects against external tool output

### Built-in linker

- implement shared ELF object and archive reading support
- implement relocation application per target
- produce working linked executables comparable to external linker output

## Suggested Further Split

If this idea becomes active, it will likely need immediate subdivision into:

- `backend_builtin_assembler_plan.md`
- `backend_builtin_linker_plan.md`

## Validation

- object files disassemble correctly
- linked executables run equivalently to externally linked outputs
- relocation-heavy tests have explicit coverage

## Good First Patch

Document and test the exact external assembler/linker contract first so the built-in replacements have a concrete compatibility target.
