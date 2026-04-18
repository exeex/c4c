Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by naming the next bounded packet as the
multi-defined-function prepared-module lane centered on `00210`. The proving
cluster for that lane is `backend_lir_to_bir_notes`,
`backend_x86_handoff_boundary`, `c_testsuite_x86_backend_src_00131_c`,
`c_testsuite_x86_backend_src_00211_c`, and
`c_testsuite_x86_backend_src_00210_c`: `00131` and `00211` remain the landed
single-defined-function baseline while `00210` remains the current
multi-defined-function boundary. `00189` stays adjacent but out of scope
because it layers indirect/global-function-pointer and variadic-runtime
behavior on top of the same top-level gate, while `00057` and `00124` remain
separate emitter and scalar-control-flow families.

## Suggested Next

Execute `plan.md` Step 3 by widening the prepared-module consumer honestly for
the bounded multi-defined-function lane centered on `00210`, while keeping the
Step 2 proving cluster fixed and leaving `00189`, `00057`, and `00124`
outside the packet.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- Do not satisfy the next packet by globally deleting the prepared-module
  defined-function gate; widen only the bounded multi-defined-function lane
  required by `00210`.
- String-backed direct-call pointer provenance is now carried from the LIR
  string pool into the semantic-BIR result for this bounded lane, but the
  route still depends on the pointer operand resolving honestly to same-module
  string/global data instead of opaque runtime plumbing.
- Do not add a testcase-shaped `00210` recognizer; any change must generalize
  across the Step 2 proving cluster and preserve `00131` / `00211` as the
  baseline floor.
- Keep `00189`, `00057`, and `00124` out of this packet; they remain adjacent
  indirect-runtime, emitter/control-flow, and scalar-control-flow families.

## Proof

Step 2 proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00131_c|c_testsuite_x86_backend_src_00211_c|c_testsuite_x86_backend_src_00210_c)$' > test_after.log 2>&1`

Result: `backend_lir_to_bir_notes`, `backend_x86_handoff_boundary`,
`c_testsuite_x86_backend_src_00131_c`, and
`c_testsuite_x86_backend_src_00211_c` pass. The current boundary remains
`c_testsuite_x86_backend_src_00210_c`, which still fails with `error: x86
backend emitter only supports a single-function prepared module through the
canonical prepared-module handoff`. Proof log path: `test_after.log`.
