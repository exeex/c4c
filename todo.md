Status: Active
Source Idea Path: ideas/open/97_bir_prealloc_call_abi_authority_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory BIR Call ABI Facts

# Current Packet

## Just Finished

Completed Step 1 - Inventory BIR Call ABI Facts. Inspected
`src/backend/bir/bir.hpp`,
`src/backend/bir/lir_to_bir/calling.cpp`,
`src/backend/bir/lir_to_bir/call_abi.cpp`, and
`src/backend/bir/lir_to_bir/module.cpp`; no implementation files were edited.

### BIR Call ABI Fact Inventory

| Fact | BIR carrier | Producer site | Intended authority boundary | Needs prealloc-consumer tracing |
| --- | --- | --- | --- | --- |
| Calling convention | `bir::CallInst::calling_convention`, `bir::Function::calling_convention` in `bir.hpp` | Struct fields currently default to `CallingConv::C`; no inspected lowering assignment changes them in `calling.cpp`, `call_abi.cpp`, or `module.cpp`. | BIR can carry target-neutral source calling-convention intent, but this packet found only default C convention authority in these producers. Target-specific lowering remains prealloc/backend authority unless later traces find consumers assuming richer BIR facts. | Yes: check whether prealloc treats all BIR calls/functions as C or re-inferrs conventions elsewhere. |
| Direct vs indirect callee identity | `CallInst::callee`, `callee_link_name_id`, `callee_value`, `is_indirect` | `calling.cpp` resolves direct global calls through `parse_lir_direct_global_callee`, validates metadata-rich direct calls against `FunctionSymbolSet`, writes `callee` plus `callee_link_name_id`; indirect calls lower the callee operand into `callee_value` and set `is_indirect`. Runtime/intrinsic placeholders intentionally leave `callee_link_name_id` invalid. | BIR owns target-neutral callee identity: link-name ids are semantic direct-call authority when present; string callee is compatibility/display for unresolved or synthesized placeholders; `callee_value` is the indirect-call operand. | Yes: trace whether prealloc consumes `callee_link_name_id`/`is_indirect` or re-parses `callee` strings for directness/helper identity. |
| Call argument values and lowered parameter types | `CallInst::args`, `CallInst::arg_types` | `calling.cpp` builds these from `parse_direct_global_typed_call`/`parse_typed_call`, structured args, byval aggregate layout, pointer values, and scalar/function-pointer type lowering. Sret inserts an implicit first pointer arg. | BIR owns target-neutral call operand order after LIR signature normalization, including implicit sret/byval pointer materialization. Physical location assignment is outside this boundary. | Yes: trace whether prealloc uses `arg_types`/`arg_abi` directly or reconstructs argument types from `Value`/callee metadata. |
| Call argument ABI classification | `CallArgAbiInfo` and `CallInst::arg_abi` | `call_abi.cpp::compute_call_arg_abi` classifies scalar/pointer/fp/i128/f128 args from `TargetProfile`; `calling.cpp` attaches per-call ABI entries, customizes byval aggregate size/align/class and sret flags, and copies AArch64 HFA lane metadata from structured args. | BIR already carries ABI-class facts needed to describe logical call arguments: type, size/align, primary/secondary class, register-vs-stack preference, byval copy, sret pointer, and AArch64 HFA lane grouping. Later physical register/stack slot legalization remains prealloc authority. | Yes: priority trace for duplicated classification in prealloc call plans and call moves. |
| Function parameter ABI classification | `Param::abi`, `Param::is_sret`, `Param::is_byval`, `Param::is_varargs`, `Function::params` | `call_abi.cpp::lower_function_params_with_layouts` creates function params from structured signature metadata or legacy signature text, inserts synthetic sret parameter, computes scalar/byval ABI, records byval/sret flags, and sets variadic status. | BIR owns normalized target-neutral function signature facts plus initial ABI class metadata for params. Prealloc may legalize where those params land, but should not need to rediscover byval/sret/variadic semantics if the BIR facts are sufficient. | Yes: trace variadic entry plans, wrapper params, and call-plan param classification against `Function::params[*].abi` and flags. |
| Function and call return ABI classification | `CallResultAbiInfo`, `CallInst::result_abi`, `Function::return_abi`, `return_type`, `return_size_bytes`, `return_align_bytes`, `result_lanes` | `call_abi.cpp::lower_return_info_from_type`, `infer_function_return_info`, and `lower_signature_return_info` compute scalar, HFA, and sret return info; `module.cpp::lower`/`try_lower_canonical_select_function`/`calling.cpp::lower_decl_function` write `Function::return_abi`; `calling.cpp` writes call result/result lanes/result ABI and sret storage. | BIR owns target-neutral return semantic shape plus ABI class summary: scalar type, aggregate size/align, sret memory return, HFA lane count, and result lane names. Prealloc owns helper/wrapper and physical result moves. | Yes: priority trace for prealloc result-plan inference, sret handling, HFA lanes, and i128/f128 helper paths. |
| Variadic call/function markers | `CallInst::is_variadic`, `Function::is_variadic`, vararg `Param::is_varargs` | `calling.cpp::parse_typed_call` and `parse_direct_global_typed_call` derive call variadic-ness from LIR callee signature or parsed `...`; `call_abi.cpp::lower_function_params_with_layouts` sets function variadic status from structured signature params or legacy signature parsing. | BIR owns target-neutral variadic semantic marker. Target-specific variadic save-area/register-home planning is prealloc authority. | Yes: trace prealloc variadic entry planning for correct consumption versus rediscovery. |
| Sret storage and implicit sret argument | `CallInst::sret_storage_name`, `sret_storage_name_id`, `CallArgAbiInfo::sret_pointer`, `CallResultAbiInfo::returned_in_memory`, `Param::is_sret`, `Function::return_abi` | `calling.cpp` allocates aggregate return local storage, inserts a pointer arg with `sret_pointer`, records `sret_storage_name`, and marks memory return result ABI; `call_abi.cpp` inserts function `%ret.sret` param for sret returns. | BIR owns semantic sret recognition and the logical implicit pointer argument/storage relation. Prealloc should own physical address/register moves and any target wrapper mechanics. | Yes: trace prealloc special-carrier/wrapper/sret result code for re-derivation. |
| Byval aggregate argument facts | `CallArgAbiInfo::byval_copy`, `size_bytes`, `align_bytes`, `Param::is_byval`, lowered pointer arg/value | `calling.cpp` lowers direct/indirect byval call args through aggregate layout and emits pointer args with customized ABI; `call_abi.cpp::lower_function_params_with_layouts` creates byval params and ABI from structured signature metadata or legacy fallback. | BIR owns byval semantic fact, aggregate size/align, and logical pointer carrier. Exact physical copy placement and stack/register legalization remain prealloc authority. | Yes: trace prealloc byval copy/classification and stack-placement code. |
| AArch64 fixed HFA lane pressure facts | `CallArgAbiInfo::aarch64_hfa_lane_count`, `aarch64_hfa_lane_index`, `passed_in_register`, `passed_on_stack` | `module.cpp::apply_aarch64_fixed_hfa_pressure` mutates BIR params and direct-call arg ABI after module lowering, using callee link-name/name lookup and lane metadata/name parsing to mark lanes spilled when fixed FP register pressure exceeds 8. | This is already target-specific ABI pressure encoded into BIR. It should be treated as existing BIR authority for later audit comparison, not a new place to add more target legalization. | Yes: high-priority trace for whether prealloc repeats AArch64 HFA pressure or ignores/overrides these BIR facts. |
| Runtime/intrinsic placeholder call metadata | `CallInst::intrinsic`, `inline_asm`, placeholder `callee`, arg/result ABI | `calling.cpp::lower_runtime_intrinsic_inst` synthesizes va_start/va_end/va_copy/va_arg, inline asm, stacksave/stackrestore, fabs, and AArch64 semantic intrinsic placeholder calls; these carry arg/result ABI where applicable and intentionally invalid link-name ids. | BIR owns semantic placeholder identity and typed metadata for intrinsics/inline asm. Runtime-helper expansion, helper-call ABI legalization, and special carrier movement are prealloc/backend authority unless they duplicate an existing BIR fact. | Yes: trace runtime helper and special-carrier plans later, especially i128/f128, variadic, and intrinsic/helper call planning. |
| Module function symbol set for call resolution | `Function::link_name_id`, module `NameTables`, direct call `callee_link_name_id` | `module.cpp::lower_module` imports link-name tables, builds `FunctionSymbolSet` from extern decls/functions, lowers externs/declarations before definitions, and uses it to validate metadata-rich direct calls in `calling.cpp`. | BIR owns direct-call semantic identity when link-name ids resolve across the module. Raw names are compatibility indices. | Yes: trace whether prealloc preserves link-name authority or falls back to raw names when planning calls. |

## Suggested Next

Execute Step 2 - Inventory Prealloc Call ABI Inference. Start with
`src/backend/prealloc/calls.hpp`, `call_plans.cpp`, `legalize.cpp`, and
`regalloc/call_moves.cpp`; compare each prealloc classifier against the BIR
facts above, especially `arg_abi`, `result_abi`, sret/byval flags,
variadic markers, direct callee identity, and AArch64 HFA pressure bits.

## Watchouts

- This is an audit-only plan; do not edit implementation files during routine
  audit packets.
- Separate target-neutral BIR call facts from target-sensitive prealloc ABI
  legalization.
- Do not create follow-up ideas unless a concrete duplicated authority or
  contract gap is traced to named files and proof expectations.
- `CallArgAbiInfo`/`CallResultAbiInfo` are not purely target-neutral: BIR
  producers already use `TargetProfile` for ABI class, fp register class,
  i128/f128 memory classification, and AArch64 HFA pressure. Later packets
  should classify prealloc overlap against this existing fact boundary, not
  assume BIR has only semantic call shape.
- Runtime/intrinsic placeholder calls intentionally lack `callee_link_name_id`;
  do not classify that invalid id as missing direct-callee authority without
  checking whether the call is a synthesized placeholder.

## Proof

Analysis-only packet. No build, CTest, or `test_after.log` was required by the
delegated proof because only `todo.md` changed.
