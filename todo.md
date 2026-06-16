Status: Active
Source Idea Path: ideas/open/287_post_286_prepared_boundary_interface_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Post-286 Carriers and Bridges

# Current Packet

## Just Finished

Completed Step 1 inventory for the post-286 prepared-boundary carriers and
bridges. No implementation files were changed.

Boundary inventory:

- Call argument type identity and ABI suffixes:
  - `src/codegen/lir/ir.hpp:286` defines `LirCallArg` with rendered `type`,
    `operand`, structured `LirTypeRef type_ref`, and AArch64 HFA lane fields;
    `src/codegen/lir/ir.hpp:296` defines `LirCallOp` with legacy
    `callee_type_suffix`/`args_str`, `arg_type_refs`, optional
    `callee_signature`, and `structured_args`.
  - `src/codegen/lir/call_args.hpp:39` mirrors owned typed call arguments as
    rendered type/operand plus `LirTypeRef` and HFA lane metadata for call
    formatting/parsing helpers.
  - `src/codegen/lir/hir_to_lir/call/args.cpp:253` still emits AArch64
    variadic HFA carrier-array arguments as rendered text with an
    `alignstack(...)` suffix and no `LirTypeRef`; fixed AArch64 HFA lanes at
    `src/codegen/lir/hir_to_lir/call/args.cpp:297` carry structured scalar
    lane metadata instead.
  - `src/backend/bir/lir_to_bir/calling.cpp:543` defines
    `strip_call_arg_abi_type_suffix`, currently stripping only trailing
    `alignstack(...)` from rendered call-argument type text.
  - `src/backend/bir/lir_to_bir/calling.cpp:605` prefers
    `structured_args`/`callee_signature` in `parse_typed_call`, but still falls
    back through parsed rendered suffixes and `args_str`; direct-global calls
    follow the same pattern at `src/backend/bir/lir_to_bir/calling.cpp:810`.
  - `src/backend/bir/lir_to_bir/calling.cpp:1251` resolves byval aggregate
    layouts. The structured path uses `LirTypeRef`/`StructNameId` when present,
    but the no-ref and hand-built LIR compatibility branches at
    `src/backend/bir/lir_to_bir/calling.cpp:1260` and
    `src/backend/bir/lir_to_bir/calling.cpp:1307` still consume rendered
    byval/carrier text after `strip_call_arg_abi_type_suffix`.
  - `src/backend/bir/lir_to_bir/calling.cpp:1554` lowers parsed direct calls
    into BIR `CallInst` fields `args`, `arg_types`, and `arg_abi`; the
    indirect path starts at `src/backend/bir/lir_to_bir/calling.cpp:1667`.
    Both paths use `strip_call_arg_abi_type_suffix` before scalar type
    lowering and push structured ABI into `lowered_call.arg_abi` at
    `src/backend/bir/lir_to_bir/calling.cpp:1798`.
  - `src/backend/bir/lir_to_bir/call_abi.cpp:194` lowers signature aggregate
    layout from rendered type text plus optional `LirTypeRef`; the no-ref
    bridge at `src/backend/bir/lir_to_bir/call_abi.cpp:200` is explicitly a
    rendered-signature compatibility path.
  - Prepared/prealloc consumers read the BIR ABI carrier, not the rendered
    string: `src/backend/prealloc/regalloc/call_return_abi.cpp:296` resolves
    `CallArgAbiInfo` from `call.arg_abi`, and
    `src/backend/prealloc/call_plans.cpp:1190` plans prepared call argument
    destinations from that ABI plus move-bundle bindings.

- AArch64 variadic HFA lane expansion:
  - Frontend/LIR variadic aggregate preparation identifies AArch64 HFA varargs
    in `src/codegen/lir/hir_to_lir/call/args.cpp:244`, emits a carrier array
    load, and marks the carrier argument by rendered `alignstack(...)` text.
  - Semantic call lowering reconstructs HFA payload shape with
    `collect_va_arg_hfa_payload_lanes` at
    `src/backend/bir/lir_to_bir/calling.cpp:282` and
    `aapcs64_va_arg_hfa_payload_shape` at
    `src/backend/bir/lir_to_bir/calling.cpp:346`.
  - Call-argument expansion into lane BIR values happens in
    `append_aarch64_variadic_hfa_carrier_arg_lanes` at
    `src/backend/bir/lir_to_bir/calling.cpp:1201`: it requires AArch64,
    a variadic call, an SSA aggregate alias, matching local aggregate slots,
    lane-count/slot-count agreement, and lane slot types matching the computed
    HFA lane type. It then appends one BIR argument per leaf slot and sets
    `CallArgAbiInfo::aarch64_hfa_lane_count/index` at
    `src/backend/bir/lir_to_bir/calling.cpp:1243`.
  - The expansion is invoked from both direct and indirect semantic call
    lowering for aggregate/byval fallback paths at
    `src/backend/bir/lir_to_bir/calling.cpp:1620`,
    `src/backend/bir/lir_to_bir/calling.cpp:1639`,
    `src/backend/bir/lir_to_bir/calling.cpp:1710`, and
    `src/backend/bir/lir_to_bir/calling.cpp:1732`.
  - Prepared call-plan sizing keeps those lane ABI facts visible:
    `src/backend/prealloc/regalloc/call_return_abi.cpp:101` and
    `src/backend/prealloc/call_plans.cpp:221` special-case
    `aarch64_hfa_lane_count` for stack size/alignment, and
    `src/backend/prealloc/call_plans.cpp:604` counts variadic FPR register
    arguments from `call.arg_abi`.
  - Aggregate `va_arg` publication uses a separate BIR call fact:
    `src/backend/bir/lir_to_bir/calling.cpp:1899` computes HFA payload shape
    for `llvm.va_arg.aggregate` and writes `va_arg_hfa_lane_count` /
    `va_arg_hfa_lane_size_bytes` at
    `src/backend/bir/lir_to_bir/calling.cpp:1916`. Prepared planning consumes
    those fields in `src/backend/prealloc/variadic_entry_plans.cpp:593` and
    switches the source to the FP register-save area with per-lane destination
    homes.

- Opaque pointer byte-offset provenance:
  - `src/backend/bir/lir_to_bir/memory/memory_types.hpp:42` defines
    `LocalSlotAddress` with a scalar `value_type`, signed byte offset, and
    compatibility `storage_type_text`/`type_text`; `PointerAddress` at
    `src/backend/bir/lir_to_bir/memory/memory_types.hpp:134` carries
    `base_value`, `value_type`, byte offset, optional dynamic element metadata,
    and the same rendered type-text compatibility fields.
  - `src/backend/bir/lir_to_bir/memory/coordinator.cpp:86` reconstructs runtime
    pointer addresses from `pointer_value_addresses`, `local_slot_pointer_values`,
    local pointer slots, and array-base metadata, carrying byte offsets and
    rendered storage/type text into pointer-provenance side tables.
  - `src/backend/bir/lir_to_bir/memory/provenance.cpp:53` owns scalar subobject
    admission. The specific opaque-pointer rule is
    `allow_opaque_ptr_base && stored_type == I8` at
    `src/backend/bir/lir_to_bir/memory/provenance.cpp:73`, with an additional
    zero-offset opaque base allowance for empty/`ptr`/`i8` text at
    `src/backend/bir/lir_to_bir/memory/provenance.cpp:79`.
  - The same helper falls back to rendered aggregate type text and legacy type
    declarations at `src/backend/bir/lir_to_bir/memory/provenance.cpp:87`
    because no `LirTypeRef`/`StructNameId` reaches this provenance helper.
  - Global scalar pointer accesses call the helper with
    `allow_opaque_ptr_base = false` in
    `resolve_linear_addressed_global_scalar_access` at
    `src/backend/bir/lir_to_bir/memory/provenance.cpp:356`; the load/store
    lowering paths using that decision are at
    `src/backend/bir/lir_to_bir/memory/provenance.cpp:501` and
    `src/backend/bir/lir_to_bir/memory/provenance.cpp:669`.
  - Prepared memory access records preserve only prepared address facts:
    `src/backend/prealloc/addressing.hpp:49` stores base kind/id, byte offset,
    size, align, and base-plus-offset policy in `PreparedAddress`; `PreparedMemoryAccess`
    at `src/backend/prealloc/addressing.hpp:190` wraps the address with result,
    stored value, address space, and volatility.
  - `PreparedFunctionLookups` exposes `memory_accesses` at
    `src/backend/prealloc/prepared_lookups.hpp:15`; derived publication facts
    copy base kind, pointer value, byte offset, size, align, and policy from a
    prepared memory access at `src/backend/prealloc/prepared_lookups.cpp:449`.
    No prepared byte-offset record currently carries a structured base-object
    extent or `LirTypeRef` provenance back to semantic lowering.

## Suggested Next

Proceed to Step 2 classification. Classify each surface as a safe local cleanup,
structured-carrier prerequisite, retained target/prepared policy, or separate
follow-up idea; start with `strip_call_arg_abi_type_suffix` because it is the
clearest rendered-string carrier, but do not assume it is removable until the
structured owner for `alignstack`, byval/carrier shape, and struct identity is
named.

## Watchouts

The HFA lane expansion path is not just a formatting issue: it currently joins
AArch64 target policy, local aggregate slot shape, and `CallArgAbiInfo` lane
metadata. The opaque pointer path is also not cleanup-ready without a
structured base-object extent/range carrier; prepared `memory_accesses` preserve
byte offsets and addressing policy, but not enough structured provenance to
replace the semantic compatibility rule.

## Proof

Not run (inventory-only). No `test_after.log` was created because this packet
made only documentation/state updates in `todo.md`.
