Status: Active
Source Idea Path: ideas/open/271_aarch64_variadic_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Route Or Record Integration

# Current Packet

## Just Finished

Completed Plan Step 3, `Route Or Record Integration`.

Reviewed the compiled route after Step 2. The integration hooks that should be
owned by `variadic.{hpp,cpp}` already route through that compiled boundary:

- `dispatch.cpp` calls `variadic_entry_helper_kind`,
  `require_prepared_variadic_entry_plan`,
  `variadic_helper_operand_homes_complete`, and
  `variadic_helper_missing_consumption_fact_message` for helper classification,
  prepared entry gating, operand-home completeness, and helper-specific missing
  fact diagnostics.
- `calls.cpp` calls `complete_variadic_call_record`,
  `variadic_call_selection_status`, and `print_variadic_call` for structured
  helper record completion, machine-node selection status, and helper printer
  branches.
- `src/backend/CMakeLists.txt` compiles `variadic.cpp`, so the compiled owner is
  discoverable by the backend build.

The remaining live facts are intentionally still records/carriers rather than
new variadic policy owners: `PreparedCallPlan::wrapper_kind ==
DirectExternVariadic` and `PreparedCallPlan::variadic_fpr_arg_register_count`
stay in the prepared call plan, while `CallInstructionRecord` retains the
direct call wrapper/FPR-count fields plus the structured variadic helper record
payloads needed by generic instruction construction. `instruction.hpp` remains
the shared record surface for opcodes, machine pseudo/mnemonic enums,
`VariadicVaStartRecord`, scalar and aggregate `VariadicVaArgRecord`,
`VariadicVaCopyRecord`, and `CallInstructionRecord`; it does not reconstruct
legacy `va_list` behavior.

`calls.cpp` remains scoped to call sequencing, generic machine-node effect
assembly, and ordinary direct/indirect call printing. `dispatch.cpp` remains
scoped to routing BIR calls through prepared call and helper provenance.
`prologue`, `memory`, and `module_compile` have no live compiled variadic hooks
after the Step 2 route, so they remain focused on frame, memory, and module
driver responsibilities rather than variadic save-area or overflow-area policy.

No additional code movement is needed before Step 4. Step 4 can delete
`src/backend/mir/aarch64/codegen/variadic.md` as a historical shard without
additional integration work, provided the deletion packet confirms no active
references still treat that markdown file as implementation authority.
This remains an ownership redistribution only: no new `va_start`, `va_arg`,
`va_copy`, register-save-area, overflow-area, or `va_list` semantics were
implemented or requested.

## Suggested Next

Execute Step 4 by deleting `src/backend/mir/aarch64/codegen/variadic.md` and
checking that remaining references do not name it as an active implementation
artifact. Keep the packet deletion-only unless the supervisor explicitly
authorizes another movement.

## Watchouts

Preserve the explicit deferral of full variadic function-entry behavior. Do not
turn the structured helper records or printer diagnostics into new AAPCS64
`va_start`, `va_arg`, `va_copy`, register-save-area, overflow-area, or
`va_list` semantics during the deletion packet. Direct extern variadic call
metadata still belongs to prepared/call records.

## Proof

No build or test proof was run; the delegated packet was a todo-only
integration decision and explicitly required no proof unless implementation
changes became unavoidable. `test_after.log` was not touched.
