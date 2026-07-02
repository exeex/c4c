# RV64 Object Emission AArch64 Comparison

Source idea: `ideas/open/519_rv64_object_emission_cleanup_umbrella.md`
Plan step: Step 2 - Compare Against AArch64 Codegen Layout

This is an analysis-only comparison. No implementation files, tests,
expectations, or root-level logs were changed.

## Evidence Log

Step 2 started from
`docs/rv64_object_emission_cleanup/structure_baseline.md`, then used the
following AST-backed and targeted source reads:

```sh
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/object_emission.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/module_compile.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/memory.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/globals.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/returns.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/prologue.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/variadic.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/prepared_call_emit.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/prepared_frame_emit.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/prepared_function_emit.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/rv64_line_assembler.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/prepared_module_emit.cpp build/compile_commands.json
```

Targeted source reads checked:

- `src/backend/mir/aarch64/codegen/README.md`
- `src/backend/mir/riscv/codegen/README.md`
- `src/backend/mir/aarch64/codegen/object_emission.{hpp,cpp}`
- `src/backend/mir/aarch64/codegen/module_compile.hpp`
- `src/backend/mir/riscv/codegen/object_emission.{hpp,cpp}`
- `src/backend/mir/riscv/codegen/prepared_module_emit.{hpp,cpp}`
- `src/backend/mir/riscv/codegen/prepared_function_emit.{hpp,cpp}`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.hpp`
- `src/backend/mir/riscv/codegen/emit.cpp`
- `src/backend/mir/riscv/codegen/mod.cpp`

The legacy RV64 files `calls.cpp`, `memory.cpp`, `globals.cpp`,
`returns.cpp`, `prologue.cpp`, and `variadic.cpp` are present in the tree but
did not load from `build/compile_commands.json` in this pass. Treat them as
layout references only until a later packet proves they are live destination
owners or explicitly revives them.

## AArch64 Layout Signal

| Concern | AArch64 owner shape | Useful RV64 lesson | RV64 caveat |
| --- | --- | --- | --- |
| Prepared-module entry | `module_compile.cpp` is the thin coordinator from accepted prepared facts into target MIR/module records. | Keep high-level module admission separate from family lowering when a stable target-MIR route exists. | RV64 object emission currently consumes prepared BIR directly for the object route, not a compiled RV64 target-MIR object stream. |
| Object writer | `object_emission.cpp` is compact: fragment encoders, selected-machine-instruction fragment conversion, text-module assembly, relocation mapping, ELF config/write. | A late RV64 cleanup should leave object module assembly as a narrow object/fixup owner instead of a semantic lowering owner. | RV64 `object_emission.cpp` also owns prepared BIR traversal, data objects, inline asm classification, labels, and prepared diagnostics. |
| Calls | `calls.cpp` is a large prepared-call lowering owner that emits machine instructions and call-boundary records. | RV64 call preparation has a plausible existing owner in `prepared_call_emit.cpp` for text-route helpers and a future prepared-call object fragment owner. | RV64 object-route call fragments also create `RiscvEncodedFragment` fixups and depend on prepared function traversal context; they are not just text-call helpers. |
| Memory | `memory.cpp` owns prepared memory facts, load/store record construction, result retargeting, local/global stack publication, pointer base-plus-offset, and transport. | Split RV64 memory work by local memory, global/address materialization, and publication/writeback instead of one monolithic move. | RV64 object-route memory fragments currently combine byte encoders, direct PC-relative fixups, frame offsets, prepared access validation, and BIR instruction dispatch. |
| Globals/address materialization | `globals.cpp` owns address materialization records, global load address policy, relocation operand spelling, and local slot address publication. | RV64 global symbol/address materialization should be separated from local stack memory when follow-up ideas are drafted. | RV64 data-object emission lives late in object emission and is different from global load/store text helpers. |
| Returns | `returns.cpp` owns prepared return operand/effect records and return instruction lowering. | Return fragments are plausible low-risk follow-up slices once prologue/frame facts are stable. | RV64 object-route return handling includes immediate return fragments plus before-return move bundles and prepared publication dependencies. |
| Prologue/frame | `prologue.cpp` owns frame boundary nodes, saved-register facts, formal-entry publications, and fixed-frame sizing. | RV64 frame/prologue helpers should be staged around `prepared_frame_emit.cpp` and only then around object fragments. | RV64 object route calculates call frame, stack frame, formal entry homes, and saved-register behavior inside the same large file. |
| Variadic | `variadic.cpp` owns `va_start`, scalar/aggregate `va_arg`, `va_copy`, and prepared helper validation. | RV64 variadic helpers should be isolated from calls before moving call lowering wholesale. | RV64 variadic entry and incoming GPR publications interact with prepared module admission and stack/frame helpers. |
| Line assembly/encoding | AArch64 object emission hand-encodes a small supported subset directly. | RV64 has `rv64_line_assembler.cpp` as an existing live owner for parser/encoder primitives. | The object route should not parse emitted text for semantic object work; only low-level line/word encoders are plausible early owners. |

## RV64 Destination Map Draft

| Step 1 RV64 region | Existing destination candidate | Draft owner decision | Notes |
| --- | --- | --- | --- |
| Constants, U/I/S/R/B/J encoders, endian append helpers | `rv64_line_assembler.cpp` for duplicated low-level encoders; possibly a new shared object-fragment utility if object-only APIs need labels/fixups | Plausible early extraction, but do not move all fragment helpers blindly. | `rv64_line_assembler.cpp` already owns `encode_*`, `append_le32`, `append_le64`, field parsing, and line encoding. Object-fragment labels/fixups are different and may need an object-fragment owner rather than the line parser. |
| Fixup kind mapping and ELF config/write | Keep in `object_emission.cpp` or move late to an object-module assembly owner | Late/stay central. | Symbol kind, relocation type, ELF flags, `.text` layout, local labels, and undefined symbol declaration must stay coupled until all fragment producers expose stable fixup contracts. |
| Prepared symbol naming, module/image rejection, diagnostics | `prepared_module_emit.cpp` for module-level prepared admission patterns; object-route-specific diagnostics may stay in `object_emission.cpp` | Plausible module-admission follow-up, but not first movement. | Text-route `prepared_module_emit.cpp` validates external call policy and delegates to function/global emitters; object route has richer `PreparedObjectConsumerDiagnosticCategory` reporting. |
| Register/home lookup, stack-slot offsets, basic stack load/store/move | `prepared_frame_emit.cpp` for frame/stack offset helpers; `prepared_local_memory_emit.cpp` for access-specific local memory | Split by frame helper versus memory operation. | Shared stack offset predicates are good candidates; semantic local memory fragments should wait until local/global/address ownership is separated. |
| Variadic materialization and incoming GPR publications | Future `prepared_variadic_*` owner or revive `variadic.cpp` only after live-owner proof | Plausible mid-stage but currently no live compiled `variadic.cpp` destination. | AArch64 has live `variadic.cpp`; RV64 legacy `variadic.cpp` is not in the compile database from this pass. Do not route code there without activation work. |
| Move bundles and edge publications | `prepared_edge_publication_emit.cpp`; maybe `prepared_scalar_emit.cpp` for scalar source materialization | Plausible after scalar/publication boundary review. | This file already owns edge-publication move intent and stack/register publication helpers, but object-route bundles are mixed with before-call/before-return and selected producers. |
| Inline asm parsing, substitution, and EV/INSN.D encoding | `rv64_line_assembler.cpp` for parsing/encoding primitives; maybe a dedicated inline-asm object helper later | Plausible staged split after keeping public object APIs stable. | `object_emission.hpp` currently exposes substitution/classification/encoding entrypoints; moving internals must preserve those APIs. |
| Call-frame, prologue, epilogue, fixed frame sizing | `prepared_frame_emit.cpp`; perhaps `prologue.cpp` only after live-owner proof | Plausible early-to-mid helper extraction for pure frame calculations. | AArch64 prologue owner is live and broad; RV64 `prologue.cpp` is not compile-db live here. |
| Call argument publication, call lowering, direct/indirect call fragments | `prepared_call_emit.cpp` | Plausible mid/late staged owner. | `prepared_call_emit.cpp` owns text-route simple call support; object-route call fragments carry encoded bytes, fixups, sret/byval/register preservation, and prepared object diagnostics. |
| Return lowering | `prepared_scalar_emit.cpp` for simple text returns; potential future object return helper | Small dedicated follow-up possible after frame/before-return dependencies are isolated. | `prepared_scalar_emit.cpp` has `emit_riscv_simple_return`, but object route return also consumes prepared before-return move bundles and call-preserved homes. |
| Local memory and pointer-value access | `prepared_local_memory_emit.cpp` | Plausible staged owner after offset/base facts are isolated. | Keep pointer base-plus-offset and string-constant address materialization out until global/address split is defined. |
| Global loads/stores and direct symbol materialization | `prepared_global_memory_emit.cpp` for text-route global helpers; object data stays separate | Plausible staged owner for load/store/address fragments, not for final data sections. | Text-route `append_prepared_global_storage_asm` is assembly output; object-route `.rodata`/`.data`/`.bss` section creation is a different responsibility. |
| Scalar binary, casts, selects, compare branches | `prepared_scalar_emit.cpp`; `prepared_edge_publication_emit.cpp` for publication-source moves | Plausible but large. Split scalar arithmetic/casts before select-edge publication. | Select and binary fragments depend heavily on prepared publication plans, BIR producers, and diagnostics; avoid a catch-all scalar move. |
| Formal entry homes and function traversal | `prepared_function_emit.cpp` for text-route simple function traversal; object route may need a new prepared-object-function owner | Late. | `prepared_function_to_object_function` is the central high-coupling anchor and should not be moved until helpers around it are peeled off. |
| Terminator and instruction dispatch | Keep central until all family helpers expose stable fragment APIs | Late/stay central. | `fragment_for_prepared_instruction` fans out to almost every semantic family. Moving it early would hide coupling behind a new filename. |
| Parallel-copy diagnostics | Keep near prepared object admission or future diagnostics owner | Late. | It feeds `build_rv64_prepared_text_object_module_with_diagnostics` directly. |
| Prepared data objects and object sections | New object-data owner or late object-module assembly owner | Late. | Needs contract verification, selected object data, string constants, section kind, zero fill, symbol-pointer relocations, and object symbol definitions. |
| Public object/ELF entrypoints | `object_emission.cpp` | Stay central until follow-up ideas prove replacement owners. | These are the public API surface in `object_emission.hpp`; movement should be API-preserving and late. |

## Existing RV64 Files As Owners

| Candidate | Live evidence in this pass | Best use in follow-up drafting |
| --- | --- | --- |
| `prepared_module_emit.cpp` | Compile-db AST signatures load. Owns text-route prepared module entry, external-call rejection, global text emission, function delegation, and edge publication append. | Reference for module-level admission shape, not a drop-in object-module owner. |
| `prepared_function_emit.cpp` | Compile-db AST signatures load. Owns text-route prepared function traversal, formal homes, inline asm substitution callout, simple terminator/instruction text path. | Useful for formal-entry and simple traversal helpers; object-function conversion remains higher coupling. |
| `prepared_frame_emit.cpp` | Compile-db AST signatures load. Owns stack alignment, frame slot offsets, block label spelling, simple stack load/store snippets. | Good candidate for pure frame/offset helper extraction. |
| `prepared_call_emit.cpp` | Compile-db AST signatures load. Owns simple call text route, frame-slot address args, byval aggregate address args, preserved GPR args, callee-saved preservation. | Best existing owner for call-specific prepared helper families after object fragment API decisions. |
| `prepared_local_memory_emit.cpp` | Compile-db AST signatures load. Owns simple scalar/pointer frame-slot accesses, pointer address materialization, string constant address materialization, simple load/store local. | Best existing owner for local memory helper families after separating local from global/address. |
| `prepared_global_memory_emit.cpp` | Compile-db AST signatures load. Owns prepared globals, object label spelling for text, simple global storage asm, direct global/function address materialization, simple load/store global. | Useful for global access/address helpers; final object data sections probably need a separate late owner. |
| `prepared_scalar_emit.cpp` | Compile-db AST signatures load. Owns immediates, compare branches, select-to-location, move-to-register/location, casts, pointer add, binary, simple return. | Candidate for scalar/cast/simple-return slices, but select/publication should not be mixed in a first scalar move. |
| `prepared_edge_publication_emit.cpp` | Compile-db AST signatures load. Owns stack/register edge publication intent and move instruction assembly. | Candidate for selected edge-publication move helpers after scalar/call dependencies are isolated. |
| `rv64_line_assembler.cpp` | Compile-db AST signatures load. Owns line parser and low-level encoder functions. | Candidate for duplicated encode/append primitives if public APIs stay clear. |
| `calls.cpp`, `memory.cpp`, `globals.cpp`, `returns.cpp`, `prologue.cpp`, `variadic.cpp` | Files exist but did not load from compile commands. | Treat as historical layout references only in this plan unless a later activation packet makes them live. |

## Intentional RV64/AArch64 Divergences

- AArch64 object emission consumes selected target MIR machine records.
  RV64 object emission currently consumes prepared BIR and prepared object
  traversal facts directly. Cleanup should not pretend RV64 has the same
  target-MIR object boundary unless a separate idea creates it.
- AArch64 `object_emission.cpp` can stay compact because calls, memory,
  globals, returns, prologue, and variadic lowering already have live owners.
  RV64 has live `prepared_*_emit.cpp` helpers, but they are text-route helpers
  and do not yet own encoded object fragments.
- RV64 carries PC-relative AUIPC/ADDI label-pair fixups, local labels, branch
  and JAL fixups, and selected object data sections. These are materially
  different from the AArch64 ADRP/ADD/CALL26 model and should not be copied
  mechanically.
- RV64 final data-object emission is tied to prepared selected object-data
  contract verification. AArch64 comparison does not provide a matching data
  object owner here.
- RV64 legacy files named like the AArch64 owners are not automatically live
  destinations. Follow-up ideas should prefer compiled `prepared_*` owners or
  propose explicit live-owner creation.

## Risk Notes

### Symbol And Fixup Handling

- Keep `RiscvObjectFixupKind`, `RiscvObjectFixupTargetKind`, local label
  binding, `rv64_elf_relocation_type`, and `.text` relocation attachment
  coupled until fragment producers expose stable fixup contracts.
- The AUIPC/LO12 pair uses a local label as the LO12 relocation target, unlike
  the simpler AArch64 symbol-pair fixup shape. Early movement must preserve
  label uniqueness and section-offset math.
- Data-object symbol-pointer initializers attach `R_RISCV_64` relocations in
  `.data`/`.rodata`, which is separate from text fixup handling. Do not merge
  that with instruction-fragment relocation cleanup.

### Prepared Admission

- `build_rv64_prepared_text_object_module_with_diagnostics` is the current
  admission gate for diagnostics, function-name checks, BIR function matching,
  object-function conversion, object-module construction, and prepared data
  object emission. Moving semantic helpers must not weaken its diagnostic
  categories or replace contract checks with silent unsupported behavior.
- `prepared_function_to_object_function` and `fragment_for_prepared_instruction`
  remain the high-coupling anchors. Follow-up ideas should peel off leaf helper
  families before attempting to move either anchor.
- Existing text-route helpers can guide ownership, but object-route admission
  must continue to consume prepared object facts rather than infer missing
  facts from BIR or emitted assembly.

### Runtime Behavior

- This plan must not change object bytes, relocations, ELF flags, symbol
  binding, section alignment, zero-fill reservation, public entrypoints, or
  unsupported diagnostics.
- Follow-up slices should validate object-emission tests that cover the moved
  family and at least one prepared-module runtime path when behavior changes
  could affect executable output.
- Avoid text-assembly round trips for object emission. `rv64_line_assembler`
  is a useful low-level encoder owner, but object emission should continue to
  operate on structured fragments and fixups.

## Suggested Step 3 Packet

Draft staged follow-up ideas from this comparison and the Step 1 baseline.
Start with low-risk helper movement candidates:

1. Encoder/byte append helper ownership, preserving `object_emission.hpp`
   public APIs and structured `RiscvEncodedFragment` fixups.
2. Pure frame/offset helpers into `prepared_frame_emit.*`.
3. Local/global/address helper slices that separate local memory from global
   address materialization and final data-object section emission.
4. Scalar/cast/simple-return helpers before select-edge publication.
5. Call/variadic/prologue and object-data/module assembly late, with
   `prepared_function_to_object_function` and instruction dispatch retained
   central until their dependencies are peeled away.

Step 3 should not create implementation moves. It should produce concrete
behavior-preserving follow-up idea drafts with owned files, prerequisites,
validation expectations, and reviewer reject signals.
