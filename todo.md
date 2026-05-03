# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
reran the temporary `TypeSpec::tag` deletion probe after migrating the shared
helper boundary. The probe again failed as expected, then the temporary
`ast.hpp` edit was reverted.

New first remaining boundary:
- `src/codegen/lir/hir_to_lir/const_init_emitter.cpp:791-915` still uses
  `TypeSpec::tag` while lowering constant global-address member and indexed
  member expressions into final GEP strings and field-walk offsets.
- `src/codegen/lir/hir_to_lir/const_init_emitter.cpp:1458` still gates
  aggregate byte encoding on `cur_ts.tag`.

Additional same-wave residuals visible after that first failure:
- `src/codegen/lir/hir_to_lir/core.cpp:285`, `315-325`, and `638-639`
  still read `TypeSpec::tag` in structured layout observation and legacy
  lookup/HFA helpers.
- `src/codegen/lir/hir_to_lir/expr/coordinator.cpp:68-69` still uses
  `TypeSpec::tag` for member function-pointer signature discovery.

## Suggested Next

Next coherent packet: migrate the const-init address/GEP field-walk residuals
in `const_init_emitter.cpp` so final GEP string construction and offset walking
obtain owner tags through structured metadata or explicit final-spelling helper
paths instead of direct `TypeSpec::tag` reads. Keep final emitted LLVM GEP text
unchanged.

## Watchouts

- The deletion build is intentionally not committed; `TypeSpec::tag` is restored
  in the working tree.
- The const-init residual is partly final LLVM GEP spelling and partly semantic
  field walking. Preserve emitted GEP text while moving the field-owner lookup
  and current aggregate owner tracking to structured metadata or explicit
  compatibility helpers.
- `core.cpp` and `expr/coordinator.cpp` are next-wave residuals, not part of the
  immediate const-init first boundary unless the const-init route proves a
  shared helper is missing.

## Proof

Deletion probe command:
`cmake --build --preset default > test_after.log 2>&1`

Expected probe failure was captured in `test_after.log`; first compile failure
is `src/codegen/lir/hir_to_lir/const_init_emitter.cpp:791` because
`TypeSpec::tag` no longer exists. The temporary `ast.hpp` edit was reverted
after the probe, and `cmake --build --preset default` then passed to confirm
the workspace returned to a buildable state.
