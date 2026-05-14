Status: Active
Source Idea Path: ideas/open/232_aarch64_variadic_function_entry_carriers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Variadic Entry Inputs And Existing Prepared Boundaries

# Current Packet

## Just Finished

Step 1, Inspect Variadic Entry Inputs And Existing Prepared Boundaries, completed as an
inspection-only packet. No implementation files, tests, plan files, idea files, or
logs were changed.

Current variadic-function-entry inputs:
- BIR function-level authority is `bir::Function::is_variadic` plus fixed named
  parameters in `bir::Function::params` (`src/backend/bir/bir.hpp`). This is the
  current place to know a callee definition is variadic and to count named
  parameters before target lowering.
- LIR-to-BIR sets function variadic state from structured LIR signature metadata
  (`LirFunction::signature_is_variadic`, `signature_params`,
  `signature_param_type_refs`) in `src/backend/bir/lir_to_bir/call_abi.cpp`.
  This path can identify fixed parameter count and fixed parameter ABI data, but
  it does not currently publish AAPCS64 variadic-entry facts.
- BIR call-level authority is `bir::CallInst::is_variadic`, `arg_abi`,
  `calling_convention`, and the runtime helper callee names
  `llvm.va_start.p0`, `llvm.va_arg.<type>`, `llvm.va_arg.aggregate`, and
  `llvm.va_copy.p0.p0` from `src/backend/bir/lir_to_bir/calling.cpp`.
- Existing helper BIR carries pointer/aggregate helper-call ABI metadata, but
  not the callee-entry register-save area, named GP/FP counts, overflow-area
  base, or `va_list` field layout.

Prepared insertion points:
- Add a separate function-entry carrier under prepared/shared state, preferably
  as a new `PreparedVariadicEntryPlan` / `PreparedVariadicEntryPlanFunction`
  family on `PreparedBirModule`, keyed by `FunctionNameId`. Keep it distinct
  from `PreparedCallPlan`.
- `PreparedFramePlanFunction` is the right neighboring boundary for frame-owned
  stack facts and dump grouping, but it should not silently absorb all variadic
  entry semantics unless the new carrier is explicit. A nested optional field
  there is acceptable only if the type remains named and function-entry-specific.
- Populate after function identities, frame plan, stack layout, regalloc, and
  value locations are available, near `populate_call_plans`, because entry facts
  need function metadata plus ABI/frame information but are not callsite facts.
- Existing `PreparedCallPlan::wrapper_kind` and
  `variadic_fpr_arg_register_count` are call-boundary metadata only. They must
  not be reused as evidence of complete variadic callee-entry support.

Prepared dump and test surfaces to edit later:
- Add summary/detail output in `src/backend/prealloc/prepared_printer.cpp`,
  likely alongside `append_function_summaries` and either `append_frame_plan`
  or a new `--- prepared-variadic-entry-plan ---` section.
- Extend prepared/BIR tests first: `backend_prepare_liveness`,
  `backend_prepare_frame_stack_call_contract`, and `backend_prepared_printer`
  are the best narrow dump/carrier surfaces.
- Extend AArch64 record/guard tests later after carriers exist:
  `backend_aarch64_instruction_dispatch`,
  `backend_aarch64_target_instruction_records`, and
  `backend_aarch64_machine_printer` for fail-closed diagnostics/records only.

Representative proof candidates:
- Integer entry metadata: variadic callee with fixed integer named parameter and
  `va_start`/`va_arg int`, e.g. `tests/backend/case/variadic_sum2.c`.
- FP entry metadata: variadic callee with fixed named parameter and FP varargs,
  e.g. `tests/backend/case/variadic_double_bytes.c`.
- Aggregate entry metadata: variadic callee consuming aggregate varargs, e.g.
  `tests/backend/case/variadic_pair_second.c`, plus the existing aggregate
  helper checks around `llvm.va_arg.aggregate`.
- `va_copy` metadata: `tests/backend/case/variadic_va_copy_accumulate.c` and
  existing `backend_prepare_liveness` helper checks for
  `llvm.va_copy.p0.p0`.

Missing semantic inputs that AArch64 target lowering must not fabricate:
- named GP and FP argument counts after AAPCS64 classification of fixed
  parameters, including sret/byval/HFA effects where applicable
- register-save-area size, alignment, slot/range ownership, and negative offset
  policy
- overflow-area base and alignment policy
- concrete AArch64 `va_list` field layout and initial field values
- helper scratch/resource requirements for `va_start`, `va_arg`, and `va_copy`
- structured aggregate identity for variadic aggregate `va_arg`; the current
  path still has a rendered-type compatibility bridge in BIR lowering

## Suggested Next

Start Step 2 by adding typed prepared variadic-entry carrier records in
`prealloc.hpp` and plumbing them through `PreparedBirModule` without target
lowering consumption.

## Watchouts

- Keep call-boundary variadic metadata separate from full variadic function
  entry carriers.
- Do not infer `va_list` layout, register-save areas, named GP/FP counts,
  negative offsets, overflow-area bases, or helper scratch policy inside
  AArch64 target codegen.
- Existing AArch64 diagnostics are generic (`MissingPreparedCallPlan` and
  `UnsupportedInstructionFamily`) and call-centric. Add a specific missing
  prepared variadic-entry diagnostic only after the carrier type exists.
- Reject expectation downgrades, unsupported-path weakening, named-case
  shortcuts, and text-only printer patches as progress.

## Proof

Inspection-only packet. No build or test proof was required, and no
`test_after.log` was created or modified.
