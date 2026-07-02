# RV64 Object Encoding And Byte Helper Cleanup

## Goal

Extract or share RV64 object-route U/I/S/R/B/J encoding helpers and little-endian byte append helpers without changing emitted object bytes or public object-route behavior.

## Why This Exists

`src/backend/mir/riscv/codegen/object_emission.cpp` carries low-level encoding and byte append helpers that overlap with `rv64_line_assembler.*`. These helpers are a leaf-like cleanup candidate, but object emission must keep structured fragments, labels, and fixups rather than routing through text parsing.

## In Scope

- Owned files:
  - `src/backend/mir/riscv/codegen/object_emission.cpp`
  - `src/backend/mir/riscv/codegen/object_emission.hpp`
  - `src/backend/mir/riscv/codegen/rv64_line_assembler.cpp`
  - `src/backend/mir/riscv/codegen/rv64_line_assembler.hpp`
- Compare duplicate low-level encoder and endian append helper signatures.
- Move or share pure encoder/byte append helpers only when the API stays independent of prepared BIR, object module assembly, label binding, and fixup attachment.
- Preserve `RiscvEncodedFragment` structure and object-route callers.

## Out Of Scope

- Symbol kind mapping, relocation mapping, local-label binding, object module assembly, ELF writing, and prepared data-object emission.
- Any RV64 capability repair, gcc_torture expectation change, unsupported marker change, or target-side inference.
- Routing object emission through text assembly parsing.

## Acceptance Criteria

- The helper ownership boundary is smaller and reviewable, with no broad object-emission rewrite.
- Object-route public APIs in `object_emission.hpp` remain compatible unless wrappers preserve existing callers.
- Validation includes:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|rv64_roundtrip_contract|cli_riscv64_return_zero_writes_elf_obj|cli_riscv64_vrm_insn_d_source_obj)'`
- Add one affected runtime object filter if branch or call word emission is touched.

## Reviewer Reject Signals

- The diff moves symbol kind mapping, relocation mapping, label binding, object module assembly, or ELF writing.
- The slice routes object emission through emitted text parsing rather than structured fragments.
- Object bytes change without a separate approved encoding-fix idea and proof.
- The change is mainly helper renaming or expectation rewriting while claiming cleanup progress.
- The same old monolithic coupling remains behind a new helper or filename.
