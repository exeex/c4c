# X86 Assembler Contract

`src/backend/mir/x86/assembler/` is an explicit deferred subsystem contract.
The live implementation is not present yet, but the backend layout now treats
assembler ownership as a first-class x86 subsystem instead of letting codegen
quietly absorb it.

## Intended Ownership

- accept staged x86 assembly text or a future structured assembly request
- own object emission and assembler-tool invocation boundaries
- publish assembler results back to backend orchestration

## Must Not Own

- prepared BIR consumption
- x86 semantic lowering
- route-debug classification
- linker orchestration

## Current Status

The current x86 backend is still in compile-only contract-first mode. Until the
assembler live seam exists, `api/` may stage assembly text but must not
pretend object emission already belongs to the generic x86 emission layer.
