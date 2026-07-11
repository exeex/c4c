# Feature Views And Optional Contracts

This document answers Step 2's optional-feature question: how calls, variadic
entry, special carriers, atomics, intrinsics, inline asm, and object data
should be exposed without making them part of `PreparedMirCoreView`.

The common rule is explicit presence plus fail-closed lowering. If a target
encounters an instruction, function, or module output mode that requires a
feature view, absence or incomplete facts must reject that function/module
instead of reconstructing shared prepared truth from raw BIR or diagnostic
metadata.

## Optional View Access Pattern

The core view should provide named feature queries rather than a generic raw
module escape hatch:

```cpp
class PreparedMirFeatureSet {
 public:
  [[nodiscard]] std::optional<PreparedMirCallView>
  calls(FunctionNameId function) const;
  [[nodiscard]] std::optional<PreparedMirVariadicEntryView>
  variadic_entry(FunctionNameId function) const;
  [[nodiscard]] std::optional<PreparedMirI128CarrierView>
  i128_carriers(FunctionNameId function) const;
  [[nodiscard]] std::optional<PreparedMirF128CarrierView>
  f128_carriers(FunctionNameId function) const;
  [[nodiscard]] std::optional<PreparedMirAtomicView>
  atomics(FunctionNameId function) const;
  [[nodiscard]] std::optional<PreparedMirIntrinsicView>
  intrinsics(FunctionNameId function) const;
  [[nodiscard]] std::optional<PreparedMirInlineAsmView>
  inline_asm(FunctionNameId function) const;
  [[nodiscard]] std::optional<PreparedMirObjectDataView>
  object_data() const;
};
```

Each optional view should expose prepared records with the same cursor keys
used by the core instruction cursor: `FunctionNameId`, `block_index`,
`instruction_index`, and, where present today, `BlockLabelId`. A feature view
may be empty and present if the producer proves the function does not need that
feature; it is incomplete when records exist with missing ids, `Missing`
carrier kinds, or non-empty `missing_required_facts`.

## Feature Contract Table

| Feature contract | Current prepared backing field family | Classification | Optional view name | Required presence check | Fail-closed behavior | Target-local candidates |
| --- | --- | --- | --- | --- | --- | --- |
| Calls and call-boundary publication | `call_plans`; related call-plan lookups from `make_prepared_function_lookups()` and `make_prepared_call_plan_lookups()`; publication helpers in `calls.hpp` | Instruction dependent, and required for any lowered call instruction. Target-dependent in final ABI spelling. | `PreparedMirCallView` | Before lowering a call cursor, require `features.calls(function)` and a call plan for that cursor. Before using call argument/result publication, require the matching argument/result plan. | Reject the call or enclosing function with a prepared-call-contract-missing diagnostic. Do not infer ABI assignments, clobbers, source freshness, or late result publication from raw operands. | Final call instruction sequence, scratch-register choices, target ABI register spelling, and target-specific external-call admission. Shared argument/result identity and freshness/publication facts should remain shared. |
| Variadic entry and variadic helper homes | `variadic_entry_plans`; helper home lookups such as `find_prepared_variadic_entry_helper_operand_homes()` | Target dependent and instruction/function dependent. Required for variadic function entry setup and for `va_start`, `va_arg`, and `va_copy` helper lowering. | `PreparedMirVariadicEntryView` | Require a view for any variadic function that needs entry materialization and for any cursor recognized as a variadic helper. Require the specific helper homes for the cursor. | Reject the helper/function as unsupported-prepared-variadic-contract-missing. Do not synthesize `va_list` homes from stack layout alone. | Target-specific `va_list` field layout, helper instruction expansion, and calling convention details. Shared helper cursor identity and operand-home facts should remain shared until all targets can own them. |
| I128 value carriers and helper boundaries | `i128_carriers`; `i128_runtime_helpers` | Instruction dependent and target dependent. Required when I128 values need transport, lane operations, comparisons, or helper boundaries. | `PreparedMirI128CarrierView` and `PreparedMirI128RuntimeHelperView` | For any I128 cursor or I128 value materialization, require the function carrier view and a carrier by `PreparedValueId` or `ValueNameId`. For helper calls, require the runtime-helper view and matching helper boundary. | Reject with a missing-i128-carrier/helper diagnostic when a carrier is absent, has kind `Missing`, or lists missing required facts. Do not split I128 values by ad hoc BIR type inspection. | Exact register pair names, instruction selection for pair operations, helper-call sequence, and target-specific lane moves. Shared value-to-carrier identity and lane-role semantics should stay in the view. |
| F128 value carriers and helper boundaries | `f128_carriers`; `f128_runtime_helpers` | Instruction dependent and target dependent. Required for F128 transport, memory materialization, compare/convert paths, and helper boundaries. | `PreparedMirF128CarrierView` and `PreparedMirF128RuntimeHelperView` | For any F128 cursor or F128 value materialization, require the carrier view and matching carrier. For helper lowering, require the runtime-helper view. | Reject with a missing-f128-carrier/helper diagnostic when facts are absent, `Missing`, or incomplete. Do not treat F128 constants, stack homes, or full-width registers as interchangeable without carrier authority. | Target register class spelling, vector/scalar move sequence, helper ABI expansion, and constant-pool mechanics. Shared carrier identity, constant payload attachment, and helper-boundary facts should remain shared. |
| Atomics | `atomic_operations` | Instruction dependent and target dependent. Required for any atomic instruction a target claims to lower. | `PreparedMirAtomicView` | For any atomic cursor, require the function atomic view and a `PreparedAtomicOperationCarrier` with `Complete` kind for the cursor. | Reject the atomic instruction/function when the carrier is absent, kind is `Missing`, width/order/result facts are missing, or required facts are incomplete. Do not lower atomics from raw BIR memory operands alone. | Target instruction family, acquire/release barrier mapping, LL/SC vs single-instruction selection, and scratch-register policy. Shared operation kind, ordering, width, address space, and operand/result identity should remain shared. |
| Intrinsics | `intrinsic_carriers`; some intrinsic records cross-check `call_plans` | Instruction dependent and target dependent. Required for intrinsic instructions that survive to MIR. | `PreparedMirIntrinsicView` | For any intrinsic cursor, require the intrinsic view and a complete carrier for the cursor. If the intrinsic carrier says it has a prepared call plan, require the call view too. | Reject as unsupported or missing-prepared-intrinsic-contract when the carrier is absent, incomplete, or lacks a required feature. Do not lower based only on callee spelling or generic call shape. | Target feature detection, concrete opcode selection, vector register class, and fallback helper sequence. Shared intrinsic family/operation, operand roles, immediates, memory access shape, and result home should remain shared. |
| Inline asm | `inline_asm_carriers` | Instruction dependent and strongly target dependent. Required for inline asm instructions. | `PreparedMirInlineAsmView` | For any inline-asm cursor, require the inline-asm view and a `Complete` carrier for the cursor. Require tied-home authority for tied operands before assigning shared registers. | Reject inline asm when the carrier is absent, kind is `Missing`, constraints cannot be represented, tied homes are missing, or required facts are incomplete. Do not parse constraints independently in each target as semantic authority. | Constraint-to-register allocation, final template substitution, clobber encoding, and target-specific unsupported constraint policy. Shared parsed operands, tied-home authority, side-effect flag, result home, and clobber list should remain visible through the view. |
| Object data and module data records | `object_data`; core `globals()` and `string_constants()` remain available for text assembly paths | Module dependent and target/output-mode dependent. Required for relocatable object emission and for globals needing prepared relocation/section semantics. | `PreparedMirObjectDataView` | Require the module-level object-data view when the target is emitting object records or when a global requires relocation slots, zero-fill/publication identity, or unsupported object-data markers. | Reject object emission when a required global object record is absent, contradictory, unsupported without an explicit unsupported marker, or lacks emitted bytes/zero-fill/relocation facts it requires. Text-only assembly may fail closed to unsupported data shape rather than object emission. | ELF section layout, relocation encoding, symbol binding details, and target object-writer policy. Shared global publication identity, object byte ranges, emitted bytes, zero fill, and relocation targets can remain shared. |

## Feature Presence Rules

Targets should make feature presence checks before lowering, preferably in a
small admission pass per function or module:

- a function with no calls may observe that `PreparedMirCallView` is absent;
  a function with any call cursor must require it;
- a function that is not variadic and has no variadic helper cursor may ignore
  `PreparedMirVariadicEntryView`;
- I128 and F128 views are required only when the function uses those values or
  helper boundaries, but targets should not special-case one narrow instruction
  and leave nearby carrier uses unexamined;
- atomic, intrinsic, and inline-asm views are required when their instruction
  families appear in the BIR instruction stream;
- object data is required by output mode and by global data shape, not by
  per-function instruction traversal.

The admission result should be a typed missing-feature diagnostic or an
unsupported-feature result. It should not be a silent fallback to raw
`PreparedBirModule` fields.

## Incomplete Feature Facts

A present feature view is incomplete when any required record contains:

- invalid function, block, value, slot, object, or link ids;
- a carrier kind named `Missing`;
- non-empty `missing_required_facts`;
- a cursor position that cannot be matched to the core instruction cursor;
- a required related view is absent, such as intrinsic facts that require a
  call plan or special-carrier helper facts that require runtime-helper
  boundaries.

Incomplete facts must fail closed the same way as absent views. A target may
emit a diagnostic explaining the missing fact, but it must not treat the
diagnostic text itself as lowering authority.

## Target-Local Versus Shared Truth

The optional contracts should move only target-specific mechanics out of
shared prealloc. Good target-local candidates are instruction selection,
register spelling, scratch allocation, final prologue/call sequences,
constraint admission policy, object relocation encoding, and feature-specific
opcode selection.

Shared truth should remain in named views when it records durable semantic
identity or cross-target correctness facts: prepared value ids, function ids,
cursor positions, ABI argument/result identity, freshness/publication
authority, stack object identity, carrier completeness, helper-boundary
identity, intrinsic operand roles, inline-asm tied homes, and object-data byte
ranges.

That boundary lets x86 start with the smallest core view while RV64 and AArch64
can request richer feature views without making every prepared fact part of
the required core ABI.
