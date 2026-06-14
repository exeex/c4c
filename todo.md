Status: Active
Source Idea Path: ideas/open/249_phase_f3_route6_call_identity_parity_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Build the riscv Applicability Matrix

# Current Packet

## Just Finished

Executed Step 4 of `plan.md`: built the riscv applicability matrix for the
selected scalar i32 Route 6 call argument source fact.

Evidence commands used:

- `command -v c4c-clang-tool && command -v c4c-clang-tool-ccdb`
- `rg -n "call_plans|PreparedBirModule::call_plans|PreparedFunctionLookups::call_plans|find_prepared_call_plans|route6_call|route6_find_call_argument_source|call_argument_source|ConsumedPlans|consume_plans" src/backend/mir/riscv src/backend/bir src/backend -g '*.cpp' -g '*.hpp'`
- `rg -n "call|callee|argument|arg|wrapper|extern|prepared" src/backend/mir/riscv -g '*.cpp' -g '*.hpp'`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb type-refs /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp PreparedFunctionLookups build/compile_commands.json`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/calls.cpp build/compile_commands.json` (failed because `riscv/codegen/calls.cpp` is not present in `build/compile_commands.json`; direct targeted reads were used for that file)
- `sed -n '1,540p' src/backend/mir/riscv/codegen/calls.cpp`
- `sed -n '430,780p' src/backend/mir/riscv/codegen/emit.cpp`
- `sed -n '470,710p' src/backend/mir/riscv/codegen/riscv_codegen.hpp`
- `rg -n "emit_call_|call_abi_config_impl|CallArgClass|operand_to_t0|direct_name|func_ptr|find_prepared_call_plans|make_prepared_function_lookups|call_plan|route6|source_value_id|source_name|source_value" src/backend/mir/riscv/codegen/calls.cpp src/backend/mir/riscv/codegen/riscv_codegen.hpp src/backend/mir/riscv/codegen/emit.cpp`
- `rg -n "PreparedFunctionLookups|call_plans|edge_publications|consume_edge_publication_move_intent|append_edge_publication_move_instruction|emit_prepared_module|route5|route3|Route6|route6" src/backend/mir/riscv/codegen/emit.cpp src/backend/mir/riscv/codegen/emit.hpp src/backend/mir/riscv -g '*.cpp' -g '*.hpp'`
- `c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp consume_edge_publication_move_intent build/compile_commands.json`

Conclusion:

No bounded riscv consumer currently applies to the selected scalar i32 Route 6
call argument source fact. Riscv call lowering owns ABI/register/stack policy
from generic call operands and `CallArgClass`, while the prepared riscv emission
path consumes `PreparedFunctionLookups::edge_publications` plus optional
Route 5/Route 3 edge-publication agreement. It does not consume
`PreparedFunctionLookups::call_plans`, `PreparedBirModule::call_plans`,
`find_prepared_call_plans`, `ConsumedPlans`, `route6_find_call_argument_source`,
or `route6_call_argument_source_matches_argument_value_record`.

The riscv classification is concrete non-applicability with fail-closed
behavior: the selected shared semantic fact remains valid as x86-only evidence
for now, but no riscv call-plan adapter/readiness can be inferred from it. Any
future riscv application needs a separate implementation idea that introduces a
real riscv call-plan/Route 6 consumer rather than borrowing x86 authority.

riscv applicability matrix:

| riscv row | Evidence | Classification |
| --- | --- | --- |
| Public prepared call-plan surfaces in riscv | Targeted `rg` found no `PreparedBirModule::call_plans`, `PreparedFunctionLookups::call_plans`, `find_prepared_call_plans`, or `ConsumedPlans` use under `src/backend/mir/riscv`. Hits for these surfaces are in shared prealloc/BIR, x86, and aarch64, not riscv. | Non-applicable |
| Route 6 call-use/source records in riscv | Targeted `rg` found no riscv use of `route6_find_call_argument_source`, `route6_call_argument_source_matches_argument_value_record`, or Route 6 call-use/source index APIs. | Non-applicable |
| Ordinary riscv call argument lowering | `RiscvCodegen::emit_call_reg_args_impl(...)` and `emit_call_stack_args_impl(...)` consume `std::vector<Operand>`, `CallArgClass`, and target-local register/stack layout. Scalar integer register args are materialized through `operand_to_t0(args[i])` and moved to `RISCV_ARG_REGS[...]`; stack args similarly materialize generic operands. | Target policy, not selected semantic fact |
| Riscv direct/indirect call instruction emission | `emit_call_instruction_impl(...)` emits `call <direct_name>` for direct calls or materializes `func_ptr` and emits `jalr ra, t2, 0`; it does not consult prepared call plans or Route 6 source identity. | Target policy |
| Riscv result handling | `emit_call_store_result_impl(...)` stores from `a0`/`a1`/`fa0` or moves to assigned registers/slots based on target return policy, not the selected call argument source fact. | Target policy |
| Prepared riscv module emission | AST signatures and direct reads show `emit_prepared_module(...)` builds `PreparedFunctionLookups` and iterates `lookups.edge_publications.publications`, calling `append_edge_publication_move_instruction(...)`; it never iterates `lookups.call_plans` or `module.call_plans`. | Retained prepared edge-publication compatibility |
| Prepared riscv agreement path | `RiscvEdgePublicationMoveAdapter` optionally attaches Route 5/Route 3 agreement through `route5_edge_source_agrees_with_prepared_publication(...)` and `route3_source_memory_agrees_with_prepared_publication(...)`. This is edge-publication/source-memory authority, not call-use/source identity. | Separate semantic family |
| Prepared riscv fail-closed gates | `consume_prepared_backed_move_intent()` returns `MissingSharedLookups`, `MissingPublication`, `UnsupportedPublication`, `UnsupportedSourceHome`, or `UnsupportedDestinationHome`; `emit_prepared_module(...)` skips unavailable publications. These gates prevent accidental prepared-only emission when the edge-publication contract is missing or unsupported, and there is no riscv call-plan path to accidentally consume the selected fact. | Fail-closed non-applicability |
| Compile database coverage for riscv calls | `c4c-clang-tool-ccdb function-signatures` could not load `riscv/codegen/calls.cpp` because the file is absent from `build/compile_commands.json`; the source was inspected with targeted `sed`/`rg` instead. | Tooling limitation recorded |

## Suggested Next

Execute Step 5: define fail-closed and mismatch requirements for the selected
fact family. Include the x86 agreement gate rows and the riscv non-applicability
proof so public prepared call-plan surfaces stay blocked unless a future packet
adds a real riscv consumer.

## Watchouts

For Step 5, keep the selected positive fact narrow: scalar named-i32 call
argument source identity after Route 6/prepared `source_value_id` agreement.
Negative rows must cover missing/mismatched Route 6 or prepared evidence,
non-named/non-i32 arguments, unsupported prepared homes, and public output
stability without renaming or weakening statuses.

Riscv remains a concrete non-applicability proof, not a readiness proof. Do not
infer riscv adapter safety from x86, aarch64, Route 5/Route 3 edge-publication
agreement, or generic riscv ABI call lowering.

## Proof

No build or test proof was run. The delegated packet was analysis-only and
explicitly required no proof log; no `test_after.log` was produced.
