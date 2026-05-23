Status: Active
Source Idea Path: ideas/open/aarch64-codegen-cpp-family-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit CPP Family Merge Candidates

# Current Packet

## Just Finished

Completed `plan.md` Step 1 audit for AArch64 codegen `.cpp` merge
candidates. Current source-list scan shows every AArch64 codegen `.cpp` is
enumerated in `src/backend/CMakeLists.txt`; deleting a `.cpp` in an
implementation slice must remove that entry.

First implementation target:

- Merge `src/backend/mir/aarch64/codegen/dispatch_publication_common.cpp`
  into `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`.
- Delete the obsolete `dispatch_publication_common.cpp` only after moving the
  helper definitions and removing its `src/backend/CMakeLists.txt` entry.
- Destination rationale: the helper file is a 303-line dispatch-publication
  connective translation unit declared in the `dispatch_publication_common`
  section of `dispatch.hpp`; the nearest durable owner is the 557-line
  `dispatch_publication.cpp`, while the helper declarations remain visible
  through `dispatch.hpp` for existing dispatch callers.

Candidate notes:

- Dispatch publication: `dispatch_publication_common.cpp` is the first merge
  target. `dispatch_publication.cpp` stays as the destination owner.
- Calls family: defer. `calls_common.cpp` and `calls_effects.cpp` are smaller
  connective candidates, but they are shared by `calls.cpp`,
  `calls_moves.cpp`, and `calls_printing.cpp`; choose a calls-family target
  only after the dispatch-publication slice lands.
- Prepared-boundary fallout: no additional trivial `.cpp` target emerged from
  the size scan beyond already expected tiny/deferred files.

Explicit non-targets / stay separate for this slice:

- `dispatch.cpp`, `dispatch_calls.cpp`, `dispatch_value_materialization.cpp`,
  `dispatch_entry_formals.cpp`, `dispatch_edge_copies.cpp`,
  `dispatch_branch_fusion.cpp`, `dispatch_store_sources.cpp`,
  `dispatch_dynamic_stack.cpp`, `dispatch_producers.cpp`, and
  `dispatch_lookup.cpp` stay separate because they are semantic dispatch
  lowering modules or independently edited dispatch responsibilities.
- `calls_moves.cpp`, `calls_argument_sources.cpp`,
  `calls_byval_aggregates.cpp`, `calls_preservation.cpp`,
  `calls_printing.cpp`, and `calls.cpp` stay separate for now because they are
  substantive call ABI, move, printing, or coordinator owners.
- Large feature modules stay separate: `alu.cpp`, `memory.cpp`, `i128_ops.cpp`,
  `f128.cpp`, `machine_printer.cpp`, `instruction.cpp`, `variadic.cpp`,
  `cast_ops.cpp`, `intrinsics.cpp`, `inline_asm.cpp`, `atomics.cpp`,
  `globals.cpp`, `comparison.cpp`, `returns.cpp`, `float_ops.cpp`,
  `asm_emitter.cpp`, `operands.cpp`, and `prologue.cpp`.
- Tiny but intentionally deferred files stay separate: `peephole.cpp`,
  `compatibility_projection.cpp`, `module_compile.cpp`, and
  `dispatch_diagnostics.cpp`.

## Suggested Next

Execute `plan.md` Step 2: perform the behavior-preserving
`dispatch_publication_common.cpp` merge into `dispatch_publication.cpp`, delete
the obsolete source file, and remove its `src/backend/CMakeLists.txt` entry.

## Watchouts

- Keep the source idea stable unless durable intent changes.
- Do not merge large semantic lowering modules just to reduce file count.
- Do not hide target-independent logic in AArch64 during consolidation.
- Keep slices behavior-preserving and reviewable.
- `dispatch_publication_scalar_type_size_bytes` is currently referenced from
  `dispatch_calls.cpp` and `dispatch_value_materialization.cpp`; keep its
  declaration in `dispatch.hpp` and preserve external linkage when moving the
  definition.
- Because `src/backend/CMakeLists.txt` explicitly enumerates
  `dispatch_publication_common.cpp`, the implementation slice must update that
  build list when deleting the file.

## Proof

Audit-only packet; no build required and no `test_after.log` was produced.

Source/reference scans run:

```sh
wc -l src/backend/mir/aarch64/codegen/*.cpp | sort -n
rg -n "dispatch_publication_common|dispatch_publication|calls_(common|moves|printing|effects|preservation|argument_sources|byval_aggregates)|src/backend/mir/aarch64/codegen|aarch64/codegen" CMakeLists.txt src CMakePresets.json cmake 2>/dev/null
rg -n "dispatch_publication_(load|store|global|scalar|return|formal|call|stack|memory|type)|record_current_block_entry_publication_registers|record_publication" src/backend/mir/aarch64/codegen/*.cpp src/backend/mir/aarch64/codegen/*.hpp
rg -n "(append_call_diagnostic|make_unknown_call_effect|outgoing_stack_argument_bytes|classify_abi_register|abi_register_for_binding|make_call_boundary_machine_instruction|publish_prepared_call_preserve_effects|call_effect_from_operand)" src/backend/mir/aarch64/codegen/calls*.cpp src/backend/mir/aarch64/codegen/calls.hpp
```

Exact proof command for the implementation step:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log
```
