# 01. How Target Information Enters The Pipeline

Question: How does target information currently enter the pipeline, from
`TargetProfile` through BIR call ABI metadata, prepared register placements,
and final target emission?

## Target Triple/Profile Creation To BIR Lowering Context

Target information enters through `TargetProfile`, not through a late backend
string lookup. The public profile type lives in `src/target_profile.hpp` and
stores the original triple, architecture, OS, backend ABI kind, relocation
model, and whether the ABI has float argument/result registers. The canonical
construction path is `target_profile_from_triple()` in
`src/target_profile.cpp`: it parses the arch from the triple, derives the OS,
selects `BackendAbiKind` (`Aapcs64`, `RiscvLp64*`, etc.), and sets
`has_float_arg_registers` / `has_float_return_registers`.

The CLI creates and threads that profile before frontend or backend lowering:

- `src/apps/c4cll.cpp` initializes `target_profile` from
  `default_host_target_triple()`, accepts `--target` by calling
  `target_profile_from_triple()`, and applies PIC/PIE to the profile.
- The same profile is passed to the preprocessor, sema, HIR/LIR lowering, and
  backend entry points through `BackendOptions{.target_profile =
  target_profile}`.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp::lower()` copies
  `hir_mod.target_profile` into `LirModule::target_profile` and derives the
  LLVM data layout from it.
- `src/backend/bir/lir_to_bir/context.cpp::make_lowering_context()` selects
  `module.target_profile` unless it is unknown, then falls back to the default
  host triple. The result is stored in `BirLoweringContext::target_profile`.
- `src/backend/bir/lir_to_bir/module.cpp::lower_module()` writes
  `module.target_triple = llvm_target_triple(context.target_profile)` and
  passes `context.target_profile` into global lowering, extern/function
  signature lowering, function lowering, and target-specific ABI pressure
  passes.

The backend public boundary preserves the same direction. `BackendOptions` in
`src/backend/backend.hpp` carries a `TargetProfile`; `src/backend/backend.cpp`
uses `profile_or_default()` / `resolve_public_lir_target_profile()` for public
fallbacks, then routes BIR/LIR modules to x86, AArch64, or RV64 entry points
based on the resolved profile.

## BIR Call Arg/Result ABI Fact Production

BIR call ABI facts are produced during LIR-to-BIR lowering from the
`BirLoweringContext::target_profile`.

The compact ABI payloads are defined in `src/backend/bir/bir.hpp`:

- `CallArgAbiInfo` carries the lowered argument type, size, alignment, primary
  and secondary ABI classes, register/stack classification, byval/sret flags,
  and AArch64 HFA lane metadata.
- `CallResultAbiInfo` carries the result type, ABI classes, memory-return
  classification, and result register count.
- `Param`, `CallInst`, and `Function` embed those facts as `abi`, `arg_abi`,
  `result_abi`, `va_arg_payload_abi`, and `return_abi`.

The scalar computation surface is
`src/backend/bir/lir_to_bir/call_abi.cpp`:

- `lower_call_arg_abi(const TargetProfile&, TypeKind)` sets size/alignment,
  decides register versus stack passing, and uses
  `target_profile.has_float_arg_registers` plus `target_profile.arch` for float
  and `F128` behavior.
- `lower_function_return_abi(const TargetProfile&, TypeKind, bool)` handles
  sret memory returns, float-return register availability, integer returns,
  and `I128` memory returns.
- Public wrappers `lir_to_bir_detail::compute_call_arg_abi()` and
  `compute_function_return_abi()` expose those facts to the rest of lowering.

The richer call/signature lowering is in
`src/backend/bir/lir_to_bir/calling.cpp`. It calls
`lower_return_info_from_type(..., context_.target_profile, ...)`, pushes
`compute_call_arg_abi(context_.target_profile, ...)` results into
`lowered_arg_abi`, assigns `lowered_call.arg_abi`, and assigns
`lowered_call.result_abi`. Declaration lowering similarly fills
`Function::return_abi` and parameter `abi` using the target profile. Module
post-processing in `src/backend/bir/lir_to_bir/module.cpp` applies AArch64 HFA
pressure and RV64 ordinary C stack pressure to those BIR ABI records.

## Prepared/Prealloc Mapping To Target Register Placements

The abstract BIR ABI facts become physical target register placements in
prealloc. `src/backend/backend.cpp::prepare_semantic_bir_pipeline()` calls
`prepare::prepare_semantic_bir_module_with_options()`, which constructs
`BirPreAlloc(module, target_profile, options).run()` in
`src/backend/prealloc/prealloc.cpp`. `PreparedBirModule` stores both the
semantic BIR module and the `TargetProfile` in
`src/backend/prealloc/module.hpp`.

`BirPreAlloc::run()` executes legalization, stack layout, liveness, SSA
destruction, register allocation, then `publish_contract_plans()`.
The important publication surfaces for target ABI placement are:

- `populate_regalloc_placement_identity()` in
  `src/backend/prealloc/regalloc_placement_identity.cpp`, which normalizes
  assigned register and spill authority into `PreparedRegisterPlacement` using
  the prepared module's `target_profile`.
- `populate_call_plans()` in `src/backend/prealloc/call_plans.cpp`, which
  walks BIR calls and creates `PreparedCallPlan` records. For every argument,
  `plan_call_argument_destination()` combines BIR `call.arg_abi`, move-bundle
  ABI bindings, and `target_profile` to produce
  `destination_register_name`, `destination_register_bank`, stack offsets, and
  `destination_register_placement`. For call results, the call-result plan
  uses `call.result_abi` and `call_result_destination_register_*()` to derive
  the ABI source register and placement.
- `src/backend/prealloc/target_register_profile.cpp`, which owns the concrete
  register spelling and placement policy. It maps AArch64 argument registers
  to `x0`-`x7` / `sN` / `dN` / `qN`, RV64 argument registers to `a0`-`a7` or
  `fa0`-`fa7` when the profile has float arg registers, and result registers
  to `x0`/`a0` or `fa0`/AArch64 float names. It also returns
  `PreparedRegisterPlacement{bank, pool, slot_index, contiguous_width}` for
  call-argument and call-result slots.

The physical identity contract is explicit but uneven. `PreparedRegisterPlacement`
is defined in `src/backend/prealloc/frame.hpp`; `PreparedTargetRegisterIdentity`
is defined in `src/backend/prealloc/regalloc.hpp`; and `PreparedValueHome` can
carry `target_register_identity` in
`src/backend/prealloc/value_locations.hpp`. Today
`target_register_identity_for_abi_register_placement()` in
`target_register_profile.cpp` only emits ABI physical identities for RV64
call-argument/result placements, and
`src/backend/prealloc/regalloc/value_homes.cpp` publishes RV64 scalar register
home identity. AArch64 still consumes prepared register names/placements in
its lowering path, but this specific target-register identity helper is RV64
only.

## RV64 And AArch64 Prepared-Surface Consumption

RV64 and AArch64 both consume the prepared surface after target-profile-driven
BIR and prealloc, but they do not consume the exact same subset.

For public assembly:

- `src/backend/backend.cpp::emit_riscv_bir_module_entry()` and
  `emit_riscv_lir_module_entry()` prepare the module and call
  `riscv::codegen::emit_prepared_module(prepared)`.
- `src/backend/mir/riscv/codegen/emit.cpp::emit_prepared_module()` forwards
  to `emit_prepared_module_text()`.
- `src/backend/mir/riscv/codegen/prepared_module_emit.cpp::emit_prepared_module_text()`
  builds per-function prepared lookups with
  `make_prepared_function_lookups()` and emits functions from
  `module.control_flow.functions`.
- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp::emit_riscv_simple_call()`
  reads `PreparedCallPlan`, `PreparedCallArgumentPlan`,
  before/after-call `PreparedMoveBundle`s, and call-boundary effects. It emits
  RV64 argument moves to the plan's `destination_register_name`, emits direct
  or indirect calls, then copies the result from the call plan's ABI source
  register to the planned destination register.
- The RV64 object route in
  `src/backend/mir/riscv/codegen/object_emission.cpp` consumes the same
  prepared module family, including prepared call plans and move bundles, and
  reports prepared-consumer diagnostics when the prepared object shape is not
  supported.

For AArch64:

- `src/backend/backend.cpp::emit_aarch64_bir_module_entry()` and
  `emit_aarch64_lir_module_entry()` prepare the module, clear
  `prepared.regalloc.functions`, and call
  `aarch64::codegen::print_prepared_machine_nodes(prepared)`.
- `src/backend/mir/aarch64/codegen/asm_emitter.cpp::print_prepared_machine_nodes()`
  validates the prepared module handoff, resolves the target profile from the
  prepared module, and lowers prepared functions to selected machine nodes.
- `src/backend/mir/aarch64/codegen/module_compile.cpp::compile_prepared_module()`
  / `build_module()` provide the object-route version: validate handoff,
  create a module shell from the prepared module and resolved target profile,
  then lower the module body.
- `src/backend/mir/aarch64/codegen/calls.cpp` consumes prepared call plans via
  `find_prepared_call_plan()`, prepared move bundles, value-home lookups, and
  prepared edge-publication lookups. Its call-argument/result lowering checks
  prepared ABI plans before emitting or rejecting machine nodes.

The backend object front door reinforces the split. In
`src/backend/backend.cpp::emit_target_bir_module_object()`, RV64 prepares and
passes the module to `emit_rv64_prepared_object_module(prepared)`, which calls
`write_rv64_prepared_relocatable_elf_object_with_diagnostics(prepared)`.
AArch64 prepares, clears `regalloc.functions`, then calls
`emit_aarch64_prepared_object_module(std::move(prepared))`, which compiles the
prepared module before building an AArch64 text object module.

## Direction Of Information Flow

The current direction is intentional and mostly healthy: target information is
available before BIR lowering, BIR ABI facts are computed from the target
profile, prealloc maps those facts into prepared call plans and target register
placements, and target backends consume the prepared surface rather than
recomputing the ABI from raw triples.

The main caveat is not direction but contract completeness and consistency.
RV64 has stronger explicit physical identity publication for ABI register
placements (`PreparedTargetRegisterIdentity`) and keeps regalloc/prepared
placement data through its current consumers. AArch64 consumes prepared call
plans and register names, but the public AArch64 handoff clears regalloc data
and the shared target-register-identity helper currently returns identities
only for RV64 ABI placements. That does not make the flow backward; it means
later research questions need to test whether the prepared contract fields are
rich enough and consumed consistently enough for the shared AArch64/RV64
backend contract.
