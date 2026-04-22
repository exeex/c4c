# Execution State

Status: Active
Source Idea Path: ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Idea-61 Ownership And Confirm The Next Prepared-Module Seam
Plan Review Counter: 0 / 4
# Current Packet

## Just Finished

Step `2.2` from the retired idea-68 runbook repaired the ownership boundary so
`c_testsuite_x86_backend_src_00204_c` no longer stops in the authoritative
prepared local-slot family. The delegated proof kept
`backend_cli_trace_mir_00204_myprintf_rejection` green while full `00204.c`
now fails only with the downstream idea-61 prepared-module restriction
`x86 backend emitter only supports a single-function prepared module or one
bounded multi-defined-function main-entry lane with same-module symbol calls
and direct variadic runtime calls through the canonical prepared-module
handoff`.

## Suggested Next

Work Step `1` of the new idea-61 runbook: confirm the exact prepared-module
seam now blocking `00204.c`, starting with the bounded multi-defined-function
main-entry lane, same-module symbol-call classification, and direct variadic
runtime-call contract in `src/backend/mir/x86/codegen/prepared_module_emit.cpp`.
Keep the proof on
`backend_cli_trace_mir_00204_myprintf_rejection|c_testsuite_x86_backend_src_00204_c`
until the next owned seam is explicit.

## Watchouts

- Do not reopen idea 68 unless the top-level `00204.c` failure regresses back
  to the authoritative prepared local-slot family.
- Treat the focused `myprintf` rejection as a guardrail; do not accept a
  bounded entry-lane repair that breaks that route or bypasses authoritative
  prepared call ownership.
- Reject `main + helper` or named-call special casing that only admits one
  topology without improving generic prepared-module traversal or call-bundle
  consumption.

## Proof

Latest delegated proof run:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_trace_mir_00204_myprintf_rejection|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`

Observed state for lifecycle routing:
`backend_cli_trace_mir_00204_myprintf_rejection` passed, and
`c_testsuite_x86_backend_src_00204_c` failed only with
`x86 backend emitter only supports a single-function prepared module or one
bounded multi-defined-function main-entry lane with same-module symbol calls
and direct variadic runtime calls through the canonical prepared-module
handoff`.
Proof log path: `test_after.log`.
