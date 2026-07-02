# RV64 Object Emission Structure Baseline

Source idea: `ideas/open/519_rv64_object_emission_cleanup_umbrella.md`
Plan step: Step 1 - Establish Structure Baseline

This is an analysis-only baseline for
`src/backend/mir/riscv/codegen/object_emission.cpp`. No implementation files,
tests, expectations, or root-level logs were changed.

## File Size

Current line count:

```text
12678 src/backend/mir/riscv/codegen/object_emission.cpp
```

## Tool Availability

Both repo clang tools are available on `PATH`:

```sh
which c4c-clang-tool && which c4c-clang-tool-ccdb
```

Result:

```text
/home/vscode/.local/bin/c4c-clang-tool
/home/vscode/.local/bin/c4c-clang-tool-ccdb
```

The target file is present in `build/compile_commands.json`, so Step 1 used
the compile-database entrypoint:

```sh
rg -n "object_emission.cpp" build/compile_commands.json
```

## AST Query Log

Structure inventory:

```sh
c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp build/compile_commands.json | jq -r '.results[] | select(.is_definition==true) | "\(.line):\(.name) -> \(.return_type)"'
c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp build/compile_commands.json | jq -r '.results[] | select((.kind=="struct" or .kind=="class" or .kind=="enum") and .is_definition==true) | "\(.line):\(.kind):\(.name)"'
```

Caller/callee probes:

```sh
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp build_rv64_prepared_text_object_module_with_diagnostics build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp prepare_rv64_object_function build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp prepared_function_to_object_function build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp fragment_for_prepared_instruction build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp fragment_for_prepared_call build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp append_rv64_prepared_data_objects build/compile_commands.json
```

The `prepare_rv64_object_function` query returned `ok: false` with
`function not found in translation unit`; the actual function-level prepared
object anchor is `prepared_function_to_object_function` at line 11036.

Type-reference probes:

```sh
c4c-clang-tool-ccdb type-refs /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp RiscvEncodedFragment build/compile_commands.json
c4c-clang-tool-ccdb type-refs /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp RiscvObjectFunction build/compile_commands.json
c4c-clang-tool-ccdb type-refs /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp PreparedBirModule build/compile_commands.json
c4c-clang-tool-ccdb type-refs /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp ObjectModule build/compile_commands.json
```

Narrow source reads were used only after the AST pass to confirm region
boundaries around lines `1-140`, `780-840`, `2120-2165`, `10860-11120`, and
`12060-12678`.

## Structural Anchors

Top-level local structs reported by AST:

| Line | Symbol | Notes |
| --- | --- | --- |
| 168 | `PreparedObjectCompare` | Sort/compare support for prepared object identity. |
| 232 | `RiscvPreparedObjectFunctionResult` | Function admission/result wrapper. |
| 808 | `RiscvLaidOutFragment` | Text-section layout bookkeeping. |
| 2139 | `PreparedSelectEdgeSourceProducerFragment` | Select-edge source producer fragment/result bridge. |
| 2368 | `PreparedBeforeReturnStackToRegisterKey` | Before-return move bundle key. |
| 2377 | `PreparedBeforeReturnStackToRegisterKeyHash` | Hash for before-return key. |
| 3053 | `Rv64NormalizedBranchPredicate` | Normalized branch predicate representation. |
| 4196 | `PreparedFrameSlotAddressArgumentPublication` | Call argument frame-slot address publication bridge. |
| 5453 | `PreparedSretStackPointerAccess` | Sret stack pointer access description. |

Major function regions from the filtered signature pass:

| Line range | Region | Representative anchors |
| --- | --- | --- |
| 34-174 | constants, instruction encoders, fragment append helpers, fixup kind mapping | `encode_u_type`, `encode_i_type`, `append_le32`, `append_rv64_fragment`, `symbol_kind_for_fixup_target` |
| 182-801 | prepared symbol naming, diagnostics, variadic admission, module/image rejection helpers | `prepared_call_argument_object_symbol`, `rv64_variadic_*`, `make_rv64_prepared_module_rejection` |
| 813-1763 | register/home lookup, stack slot offsets, basic load/store/move helpers, stack adjustment, variadic GPR publication | `rv64_register_number`, `prepared_stack_slot_home_offset`, `append_rv64_store_register_to_stack`, `fragment_for_rv64_variadic_incoming_gpr_publications` |
| 1796-2876 | variadic materialization and prepared move bundle handling | `fragment_for_prepared_variadic_va_start`, `fragment_for_prepared_out_of_ssa_moves`, `fragment_for_prepared_move_bundle` |
| 2971-3468 | move-to-register, branch helpers, inline asm parsing/encoding, return-immediate fragment | `append_rv64_move_value_to_register`, `make_rv64_block_label_fragment`, `fragment_for_rv64_insn_r_inline_asm`, `fragment_for_rv64_insn_d_inline_asm` |
| 3475-3804 | call-frame/prologue/epilogue and fixed frame sizing | `rv64_call_frame_size`, `make_rv64_call_frame_prologue_fragment`, `append_rv64_stack_frame_epilogue`, `rv64_object_stack_frame_size` |
| 3824-5064 | call argument publication, call lowering, and return lowering | `prepared_frame_slot_call_argument_offset`, `fragment_for_prepared_call`, `fragment_for_prepared_return` |
| 5266-6909 | local/global memory, address materialization, casts, local/global load/store | `prepared_memory_access_for_instruction`, `fragment_for_prepared_store_local`, `fragment_for_prepared_load_global`, `fragment_for_prepared_store_global`, `fragment_for_prepared_cast` |
| 6968-9763 | scalar binary/select/edge-publication logic and diagnostics | `fragment_for_prepared_binary`, `fragment_for_prepared_select_edge_binary_with_cast_dependencies`, `fragment_for_prepared_select`, `rv64_select_publication_bundle_rejection_diagnostic` |
| 10063-11036 | formal entry homes, branch/terminator/instruction dispatch, object traversal diagnostics, function emission | `make_rv64_formal_entry_home_fragment`, `fragment_for_prepared_terminator`, `fragment_for_prepared_instruction`, `prepared_function_to_object_function` |
| 11552-11985 | parallel-copy diagnostics, inline asm substitution/classification, ELF relocation mapping | `diagnose_unplaced_parallel_copy_obligations`, `substitute_prepared_riscv_inline_asm_operands`, `encode_rv64_ev_insn_d_inline_asm`, `rv64_elf_relocation_type` |
| 12002-12678 | object module assembly, prepared data object emission, public object/ELF entrypoints | `rv64_relocatable_elf_config`, `build_rv64_text_object_module`, `append_rv64_prepared_data_objects`, `build_rv64_prepared_text_object_module_with_diagnostics`, `write_rv64_prepared_relocatable_elf_object` |

## Dependency And Helper-Family Map

### Encoding, fragment, and relocation primitives

The first region defines U/I/S/R/B/J encoders and byte append helpers, then
reuses `RiscvEncodedFragment` throughout the file. The `RiscvEncodedFragment`
type-reference probe found references from the primitive helpers at lines
`96`, `125`, and a dense spread through all fragment-producing regions. This
family is low-level and broad: moving it first would affect nearly every
emission helper, but it has relatively simple dependencies compared with the
prepared-module logic.

### Prepared function admission and function-to-object conversion

`build_rv64_prepared_text_object_module_with_diagnostics` calls
`diagnose_unplaced_parallel_copy_obligations`,
`find_defined_bir_function`, `prepared_function_to_object_function`,
`build_rv64_text_object_module`, and `append_rv64_prepared_data_objects`.

`prepared_function_to_object_function` is the central per-function bridge. Its
callee probe showed direct dependencies on prepared lookups, publication-plan
collection, addressing/frame/storage/inline-asm lookup, stack-frame sizing,
variadic admission, param-home diagnostics, traversal construction, move/select
consumer classification, instruction and terminator fragment dispatch, and
diagnostic assembly. This is a late split candidate unless extracted around a
stable context object.

### Prepared instruction dispatch

`fragment_for_prepared_instruction` fans out to most semantic emission families:
address materialization, scalar binary, select, casts, local/global memory,
variadic helpers, call lookup, inline asm, and prepared object traversal
diagnostics. It also calls external prepared-object traversal classifiers and
BIR predicates such as compare and VRM-register checks. This dispatch cluster
is high-coupling and should not be treated as an early pure-helper move.

### Calls, variadic handling, and frame/prologue/return

`fragment_for_prepared_call` pulls in inline asm carriers, call boundary-effect
planning, saved-register effects, local frame address materialization routes,
sret argument offsets, stack/load/store/move helpers, FPR/GPR home lookups, and
symbol address fixups. The call family crosses existing destination candidates
`calls.cpp`, `variadic.cpp`, `prologue.cpp`, `returns.cpp`, and
`prepared_frame_emit.cpp`; Step 2 should compare this against AArch64 before
choosing destination ownership.

### Local/global memory and address materialization

The memory region depends on prepared memory access facts, BIR type sizes,
frame-slot absolute offsets, byval/sret stack-pointer routes, pointer-value
base-plus-offset facts, direct global materialization symbols, and object
fixups. It is a candidate for staged extraction only after destination
comparison distinguishes local memory from global memory and symbol-address
materialization.

### Select, scalar binary, and edge publication

The scalar/select region includes binary emission, cast dependency authority,
select-edge source producers, publication move classification, predecessor
edge-publication fragments, and diagnostic helpers. It is large and depends on
prepared publication plans and consumer classifiers, so it should likely become
one or more later follow-up ideas rather than an initial file split.

### Object module and data object assembly

`build_rv64_text_object_module` owns text-section layout, local label binding,
function symbol definition, undefined symbol declaration, relocation
attachment, and ELF relocation type selection. `append_rv64_prepared_data_objects`
owns string constants, globals, selected object-data contract verification,
`.rodata`/`.data`/`.bss` section selection, zero-fill reservation, emitted
bytes, symbol-pointer relocations, and final object symbols. This region is
separable from per-instruction fragment emission but has sensitive symbol and
fixup behavior.

## Major Dependency Clusters

| Cluster | Evidence | Risk note |
| --- | --- | --- |
| Prepared module model | `PreparedBirModule` type refs at lines 183, 329, 675, 771, 1797, 1866, 4205, 4235, 4271, 4409, 5615, 5769, 5817, 5842, 6186, 6350, 6377, 6847, 6910, 10109, 10449, 10581, 11037, 12253, 12266, 12344, 12428, 12584, 12639, 12650, 12674 | Cross-cutting context. Prefer context/facade cleanup before moving high-level prepared emission dispatch. |
| Fragment model | `RiscvEncodedFragment` refs span primitive appenders, most fragment factories, dispatch, and final function assembly | Early helper movement is possible only if the destination is still shared by all emission families. |
| Object module model | `ObjectModule` refs cluster around `build_rv64_text_object_module`, relocation symbol declaration, prepared data objects, and public module/ELF entrypoints | Symbol/fixup behavior is sensitive; defer broad movement until after AArch64 comparison and follow-up scoping. |
| Prepared object function | `RiscvObjectFunction` refs at lines 150, 233, 11070, 12074, 12585 | Function object is the bridge between per-function fragments and text-module assembly. |
| Prealloc lookups and publication plans | Callee probes show `make_prepared_function_lookups`, `collect_prepared_dependency_operand_authorities`, select carrier/source placement collection, and prepared-object traversal classifiers | This is the main reason instruction/select/call regions are not simple independent helpers. |
| BIR core model | Instruction dispatch and diagnostics use `bir::Inst`, `Block`, `Function`, `Value`, `BinaryInst`, `CallInst`, local/global load/store, casts, selects, terminators, globals, and string constants | Splits must not hide BIR model coupling behind new filenames. |
| Object writer and ELF | `rv64_elf_relocation_type`, `rv64_relocatable_elf_config`, `write_rv64_relocatable_elf_object`, and `write_relocatable_elf` are concentrated late in the file | Likely a late or separate object/ELF assembly slice; preserve relocation constants and ABI flags. |

## Step 2 Comparison Packet Signal

Suggested next packet: compare this RV64 region map against
`src/backend/mir/aarch64/codegen/` and the existing RV64 destination candidates
listed in `plan.md`. The comparison should produce a table that distinguishes:

- reusable existing RV64 owners such as `calls.cpp`, `memory.cpp`,
  `globals.cpp`, `returns.cpp`, `prologue.cpp`, `variadic.cpp`, and
  `rv64_line_assembler.cpp`;
- AArch64 boundaries that are useful only as review references;
- RV64-specific prepared-object paths that should intentionally remain
  different from AArch64;
- symbol/fixup/data-object assembly that should move late or stay central.
