Status: Active
Source Idea Path: ideas/open/271_aarch64_variadic_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish The Current Variadic Boundary

# Current Packet

## Just Finished

Completed Plan Step 1, `Establish The Current Variadic Boundary`.

Current compiled ownership is split rather than absent:

- Direct extern variadic call-boundary facts are prepared in `PreparedCallPlan`
  as `wrapper_kind == DirectExternVariadic` plus
  `variadic_fpr_arg_register_count`; `calls.cpp` copies both into
  `CallInstructionRecord`, and treats `DirectExternVariadic` as a call record's
  `is_variadic` source. Normal direct-call printing still emits only the call
  mnemonic and callee label, so the FPR count is preserved as metadata, not
  rendered behavior.
- `dispatch.cpp` owns variadic helper recognition for `llvm.va_start.p0`,
  `llvm.va_copy.p0.p0`, `llvm.va_arg.*`, and `llvm.va_arg.aggregate`.
  It also owns the fail-closed prepared-fact gate for variadic entry plans,
  helper operand homes, and helper-specific diagnostics.
- `prealloc` currently owns prepared variadic entry/helper facts: named GP/FP
  counts, `va_list` layout, register-save area, overflow area, helper resources,
  and helper operand homes/access plans. That is live prepared data, not legacy
  markdown policy.
- `instruction.hpp` owns the target record structs for `va_start`, scalar
  `va_arg`, aggregate `va_arg`, and `va_copy`, plus the variadic fields inside
  `CallInstructionRecord`.
- `calls.cpp` owns record construction and printer behavior for the helper
  family. It builds records only from prepared entry/helper carriers, records
  memory side effects, selects `VariadicVaStart`, `VariadicVaArgScalar`,
  `VariadicVaArgAggregate`, or `VariadicVaCopy` opcodes, and prints structured
  provenance lines. Missing structured provenance returns unsupported
  diagnostics.
- `prologue.cpp`, `memory.cpp`, and `module_compile.cpp` do not appear to own
  variadic ABI policy in the live route. `module_compile.cpp` only coordinates
  traversal/dispatch; ordinary memory/prologue ownership remains focused on
  their non-variadic frame and memory responsibilities.
- `variadic.md` is historical only. It describes old `va_list`/register-save
  lowering, but the accepted contract says current lowering must use prepared
  carriers and must not reconstruct save areas from legacy prologue text or
  local parameter scanning.

Step 2 recommendation: create `variadic.hpp`/`variadic.cpp` as a narrow compiled
owner for existing live variadic boundary behavior. The API should be
behavior-preserving movement, not a no-op and not a new ABI implementation.
Expose helpers along these lines:

- `variadic_entry_helper_kind(std::string_view)` for helper classification.
- `require_prepared_variadic_entry_plan(...)` or a small gate/result helper that
  owns the current prepared-entry diagnostics.
- `variadic_helper_operand_homes_complete(...)` and
  `variadic_helper_missing_consumption_fact_message(...)`.
- record builder helpers for `VariadicVaStartRecord`,
  `VariadicScalarVaArgRecord`, `VariadicAggregateVaArgRecord`, and
  `VariadicVaCopyRecord`.
- printer helpers for each variadic helper branch, or one
  `print_variadic_call(...)` that returns `std::nullopt`/fallback when the call
  is not a variadic entry helper.

Keep `PreparedCallPlan`/`PreparedVariadicEntryPlanFunction` as the only ABI fact
authority. Leave legacy `va_list` layout derivation, register-save-area
construction, stack-overflow-area construction, and new `va_start`/`va_arg`/
`va_copy` semantics explicitly deferred unless future prepared carriers already
publish the needed facts.

## Suggested Next

Execute Step 2 by adding `src/backend/mir/aarch64/codegen/variadic.hpp` and
`variadic.cpp`, wiring `variadic.cpp` into `src/backend/CMakeLists.txt`, and
moving only the existing helper classification, prepared-fact gating,
helper-record builders, and variadic helper printer branches out of
`dispatch.cpp`/`calls.cpp` at narrow call sites. This should preserve emitted
output, diagnostics, opcodes, side effects, and unsupported contracts.

## Watchouts

Do not rebuild historical `va_list`, register-save-area, stack-overflow-area,
`va_start`, `va_arg`, or `va_copy` behavior from `variadic.md`. The live route
already has structured helper records and prepared-fact gates; Step 2 should
move that behavior behind a variadic owner without changing the contract.
Watch the existing contract note that a direct `CallRecord` field for
`variadic_fpr_arg_register_count` was once listed as deferred in docs, while
the compiled `CallInstructionRecord` already has that field.

## Proof

Audit-only packet. No build or backend proof was required or run; no
`test_after.log` was produced.
