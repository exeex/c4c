# RV64 Object Data Symbol Fixup And Module Assembly Cleanup

## Goal

Move or narrow RV64 object data, symbol/fixup, relocation, and final module assembly boundaries only after fragment producers expose stable fixup contracts.

## Why This Exists

The late object-emission region owns the highest-risk behavior: text module layout, local labels, undefined symbols, relocation mapping, data object emission, section selection, zero-fill reservation, symbol bindings, ELF config, and public object entrypoints. It needs an explicit late-boundary idea so earlier cleanup does not accidentally absorb it.

## In Scope

- Owned files:
  - `src/backend/mir/riscv/codegen/object_emission.cpp`
  - `src/backend/mir/riscv/codegen/object_emission.hpp`
  - possible new compiled object-data or object-module assembly files
- Move late/central boundaries only when earlier fragment producers have clear structured fragment and fixup contracts.
- Review data-object relocation and text relocation ownership as separate sub-slices unless one tiny owner demonstrably preserves both contracts.
- Preserve public object/ELF entrypoints through API-compatible wrappers if movement occurs.

## Out Of Scope

- Semantic RV64 capability repair, gcc_torture expectation changes, unsupported marker changes, target-side inference, or producer helper extraction that belongs in earlier ideas.
- Combining final module assembly with call, scalar, memory, select, or function traversal movement.
- Changing object bytes, relocations, ELF flags, symbol bindings, section names, section alignment, or zero-fill behavior.

## Acceptance Criteria

- Symbol/fixup/module assembly remains central until prerequisites are complete, or a tiny reviewed sub-slice proves a narrower owner.
- Any movement preserves object bytes, relocation entries, symbol binding, section layout, ELF flags, and public entrypoints.
- Validation includes:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|object_model_records|cli_riscv64_.*obj|obj_runtime_rv64_|rv64_roundtrip_contract)'`
- Escalate to broader `^backend_` proof before accepting a slice that changes object module layout, relocation attachment, section emission, or ELF writing.

## Reviewer Reject Signals

- The slice changes object bytes, relocation entries, symbol binding, section names, section alignment, zero-fill reservation, public entrypoints, or ELF flags while claiming behavior preservation.
- The diff combines final module assembly with semantic RV64 capability repair or gcc_torture expectation updates.
- Text fixups and data-object pointer relocations are merged before stable producer contracts exist.
- Unsupported markers, expected outputs, or runtime results are weakened.
- A new object-module file hides the same central coupling without narrower interfaces.
