Status: Active
Source Idea Path: ideas/open/243_phase_f0_x86_riscv_bir_portability_convergence_audit.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Audit Diagnostic, Oracle, and String Authority

# Current Packet

## Just Finished

Completed plan.md Step 5 analysis for diagnostic, oracle, and string authority.
This packet identified the prepared-shape proof points that must remain stable
while future x86 Route 6 and riscv Route 5/Route 3 follow-ups move semantic
authority toward BIR facts.

Inspected source, test, and planning surfaces:

- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`:
  existing readiness rule for prepared printer, CLI dump, x86 route-debug,
  helper-oracle, and wrapper compatibility replacement.
- `src/backend/mir/x86/debug/debug.cpp` and
  `tests/backend/bir/backend_x86_route_debug_test.cpp`: x86
  `summarize_prepared_module_routes(...)`, `trace_prepared_module_routes(...)`,
  route-debug headers, focus behavior, prepared-dump equivalence, and Route 6
  scalar argument rows.
- `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`
  and `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`: x86 Route 6
  `ConsumedPlans` helper-oracle agreement, absence, source-name mismatch,
  prepared mismatch, and prepared fallback contracts.
- `src/backend/mir/riscv/codegen/emit.hpp`,
  `src/backend/mir/riscv/codegen/emit.cpp`, and
  `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`: riscv
  `EdgePublicationMoveIntentStatus`, instruction-text wrapper output, shared
  prepared lookup authority, missing-publication fallback, unsupported source
  and destination fail-closed behavior, stack-source, memory-source, and
  pointer-base cases.
- `src/backend/prealloc/publication_plans.hpp`,
  `src/backend/prealloc/publication_plans.cpp`,
  `src/backend/prealloc/prepared_lookups.cpp`, and
  `src/backend/prealloc/value_locations.hpp`: prepared publication,
  source-memory, typed-stack-source, aggregate-stack-source, and current-block
  publication status name surfaces.
- `src/backend/bir/bir.hpp` and `src/backend/bir/bir.cpp`: Route 4/5/6 status
  names and the BIR route status families that future diagnostics must expose
  without weakening prepared compatibility strings.
- `tests/backend/CMakeLists.txt` and `tests/backend/OWNERSHIP.md`: baseline
  ownership for backend route, prepared-BIR, x86 route-debug, x86 wrapper, riscv
  wrapper, CLI dump, and c_testsuite-style supported-path expectations.

AST-backed inspection used:

- `c4c-clang-tool-ccdb function-signatures
  /workspaces/c4c/src/backend/mir/x86/debug/debug.cpp
  build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callees
  /workspaces/c4c/src/backend/mir/x86/debug/debug.cpp
  trace_prepared_module_routes build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callees
  /workspaces/c4c/src/backend/mir/x86/debug/debug.cpp
  summarize_prepared_module_routes build/compile_commands.json`

Diagnostic/oracle replacement candidates:

| Surface | Current prepared/string authority | Future BIR replacement candidate | String stability and baseline requirements |
| --- | --- | --- | --- |
| x86 route-debug summary/trace | `summarize_prepared_module_routes(...)` and `trace_prepared_module_routes(...)` still own externally tested headers and rows: `x86 route summary`, `x86 route trace`, `target triple: x86_64-unknown-linux-gnu`, `route owner: x86/debug`, `module emitter: x86/module`, `status: contract-first debug surface; structured lane diagnostics still deferred`, focus function/block/value headers, function rows, and the selected Route 6 row `route6 scalar arg call#0 block=entry inst#2 callee=addip0 arg#0 source=%t1 kind=ArgumentValue`. | Add x86 route-native diagnostic rows for Route 6 scalar call argument source identity, sourced from Route 6 facts and labeled separately from compatibility-derived rows until prepared agreement is no longer required. | Preserve all existing x86 route-debug header, focus, dump-equivalence, function-row, and absent-row expectations. For missing Route 6, invalid source id, duplicate Route 6, prepared mismatch, and source-name mismatch, keep the trace header/function row but keep the Route 6 source row absent. Do not relabel or weaken route-debug tests as proof of replacement. |
| x86 Route 6 direct extern wrapper oracle | `backend_x86_handoff_boundary_direct_extern_call_test.cpp` and `backend_prepared_lookup_helper_test.cpp` prove that `find_consumed_scalar_i32_call_argument_source(...)` only returns an agreed scalar `i32` `ArgumentValue` Route 6 source when Route 6 source name/id and prepared call argument source id agree. Prepared call argument selection remains fallback when Route 6 is absent or mismatched. | A narrow x86 Route 6 call-wrapper replacement can use Route 6 as the source-name authority after it proves the same agreement/fallback matrix and keeps ABI/call-bundle policy target-owned. | Preserve failure strings covering malformed fixtures, threading through `ConsumedPlans`, fail-closed absence, prepared call argument selector preservation, source-name mismatch, Route 6/prepared id mismatch, and prepared fallback asm equality. Baseline must include the wrapper asm from `expected_minimal_direct_extern_call_lane_asm()` and the helper-oracle statuses `available`, `missing_call`, `wrong_call`, `missing_argument`, `missing_source_relationship`, `missing_source_value`, `abi_bound_excluded`, `duplicate_relationship`, and `no_match` where applicable. |
| riscv prepared edge-publication wrapper | `EdgePublicationMoveIntentStatus` is a public helper contract with statuses `Available`, `MissingSharedLookups`, `MissingPublication`, `UnsupportedPublication`, `UnsupportedSourceHome`, and `UnsupportedDestinationHome`; tests also assert target wrapper text such as `mv a1, a0`, `li a1, 42`, `lw a1, 16(sp)`, `ld a1, 32(sp)`, `li t6, 4096\n    add t6, sp, t6\n    lw a1, 0(t6)`, `addi a1, s2, 12`, `mv a1, s2`, and `li a1, 4096\n    add a1, s2, a1`. | A riscv Route 5/Route 3 adapter can introduce route-fact comparison or route-native lookup behind prepared agreement, but prepared lookup authority remains the compatibility source until Route 5 edge/source identity and Route 3 memory-source identity prove parity. | Preserve `EdgePublicationMoveIntentStatus` enum names, missing-publication behavior after clearing `publications_by_edge_destination`, unsupported publication/source/destination fail-closed behavior, and exact instruction text for available cases. Baseline must keep unsupported subword, unsigned, F32, dynamic stack, aggregate-width, non-move, pointer-base-as-address, and large-offset cases fail-closed with no accidental `lb`/`lbu`/`lh`/`lhu`/`lwu`/`flw`/scalar aggregate-load emission. |
| Prepared publication/source-memory statuses | Prepared status names are compatibility strings: `PreparedEdgePublicationSourceMemoryAccessStatus` uses `unavailable`, `available`, `missing_prepared_memory_access`, `incomplete_prepared_memory_access`; `PreparedTypedStackSourcePublicationStatus` uses `available`, `missing_publication`, `unsupported_publication`, `unsupported_source_home`, `missing_concrete_stack_source`, `missing_same_width_i32_type`, `unsupported_destination_storage`, `unsupported_move_authority`, `missing_destination_register_placement`, `missing_destination_gpr_bank`, `missing_destination_register_view`; `PreparedAggregateStackSourceAuthorityStatus` includes `missing_aggregate_copy_authority`; `PreparedCurrentBlockEntryPublicationStatus` includes `publication_unavailable`. | Route 5 diagnostics should expose edge/source/publication status separately, and Route 3 diagnostics should expose memory-source identity/status separately, while prepared status names remain compatibility output until the route-native surfaces cover all positive, negative, mismatch, and fallback cases. | Do not rename prepared status strings or claim route ownership from status-string equality. Future route-native diagnostics must preserve prepared status baselines and add explicit Route 5 names such as `available`, `memory_source`, `missing_source_memory_access`, `incomplete_source_memory_access`, `missing_source_value`, and `no_match`, plus Route 3 memory-source statuses for unavailable/incomplete/missing memory identity. |
| Prepared lookup helper oracle | `backend_prepared_lookup_helper_test.cpp` still aggregates Route 3/4/5/6 helper-oracle behavior, prepared-vs-BIR agreement assertions, fallback semantics, missing/mismatch/duplicate statuses, and x86 `ConsumedPlans` compatibility. | Add route-native helper-oracle families one route fact at a time, starting with x86 Route 6 call source and riscv Route 5/Route 3 edge-source/memory-source candidates. | Existing helper-oracle failure strings and statuses must remain unchanged. Replacement proof must include positive, missing fact, invalid input, duplicate, mismatch, wrong-key, and fallback cases without reading prepared facts as route-native truth unless the row is explicitly labeled compatibility fallback. |
| Wrapper output and supported-path contracts | x86 direct extern wrapper asm and riscv prepared edge-publication asm are externally asserted output. Supported-path contracts include keeping prepared fallback available, unsupported cases fail-closed, and not downgrading a supported path to unsupported to simplify route migration. | Wrapper-input tests may be added from route-native views only after the underlying Route 6 or Route 5/Route 3 facts are proven by helper/oracle tests. | Preserve wrapper text, fallback behavior, fail-closed unsupported behavior, and baseline tests. Any expectation rewrite, output relabel, supported-path downgrade, or named-fixture shortcut is testcase-overfit and should be rejected. |

Step 5 disposition:

- x86 Route 6 call-wrapper split is blocked on a route-native diagnostic/oracle
  that keeps current route-debug rows, helper-oracle agreement checks, prepared
  fallback, and wrapper asm stable.
- riscv Route 5/Route 3 edge-publication adapter is blocked on a diagnostic
  adapter that compares Route 5 edge/source and Route 3 memory-source facts
  against prepared publication/source-memory authority while preserving
  `EdgePublicationMoveIntentStatus` and exact target-local instruction text.
- No prepared diagnostic, helper-oracle, wrapper, status, fallback, supported
  path, or baseline surface is deletion-ready from this audit packet.

## Suggested Next

Execute Step 6 as a lifecycle packet: write the convergence map and focused
follow-up ideas for the x86 Route 6 call-wrapper diagnostic/oracle replacement,
the riscv Route 5/Route 3 edge-publication adapter, retained prepared
publication/status compatibility, and any safe final prepared field-group
demotion/deletion candidates. Do not implement the adapters in this runbook.

## Watchouts

- Step 6 should cite
  `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
  as prior planning evidence, but the active closure map must be specific to
  F0 x86/riscv convergence.
- Do not weaken x86 route-debug rows or helper-oracle strings to make Route 6
  migration look complete. Absence of the Route 6 source row is currently the
  expected fail-closed signal for absent, invalid, duplicate, mismatched, and
  source-name-mismatched Route 6 facts.
- Do not rename or collapse `EdgePublicationMoveIntentStatus` or prepared
  publication/source-memory status names during riscv adapter work.
- Route 5 may own edge/source identity and Route 3 may own memory/source
  identity, but riscv register names, stack offsets, signed-12-bit decisions,
  scratch registers, load/store/address instruction spelling, and wrapper text
  remain target policy.
- A future diagnostic/oracle follow-up must add route-native proof beside
  prepared compatibility proof. It must not replace prepared expected strings
  with route-native labels as its only evidence.
- Any supported-path downgrade, output-only relabel, expectation weakening, or
  fixture-shaped string shortcut is testcase-overfit.

## Proof

Analysis-only packet; no build or ctest required by delegated proof. Used
read-only inspection with `rg`, targeted source/test/doc reads, `git status`,
and the clang-tool queries listed above. No `test_after.log` was generated
because this packet has no compile/test proof requirement and changed only
`todo.md`.
