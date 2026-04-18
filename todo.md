Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 1 for the frontier re-baseline checkpoint. The
current evidence shows that `00210` is the smallest honest next
prepared-module boundary: it still fails exactly at the multi-defined-function
prepared-module gate. `00189` hits the same top-level gate but remains an
adjacent out-of-scope neighbor because it also layers indirect/global-function-
pointer and variadic-runtime behavior on top of that boundary, while `00057`
and `00124` remain separate emitter and scalar-control-flow families.

## Suggested Next

Execute `plan.md` Step 2 and name the next bounded packet as the
multi-defined-function prepared-module lane centered on `00210`, while keeping
`00189` explicit as an adjacent indirect/global-pointer/variadic-runtime
neighbor and leaving `00057` and `00124` in their existing out-of-scope
families.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- This packet intentionally admits only one defined function plus declaration-
  only neighbors; it does not widen into multi-defined-function prepared
  modules.
- String-backed direct-call pointer provenance is now carried from the LIR
  string pool into the semantic-BIR result for this bounded lane, but the
  route still depends on the pointer operand resolving honestly to same-module
  string/global data instead of opaque runtime plumbing.
- `00210` remains the adjacent multi-defined-function boundary; do not satisfy
  the next packet by globally deleting the prepared-module defined-function
  gate or by adding a testcase-shaped `00210` recognizer.
- Keep `00189`, `00057`, and `00124` out of this packet; they remain adjacent
  indirect-runtime, emitter/control-flow, and scalar-control-flow families.

## Proof

Step 1 evidence:
`ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_x86_backend_src_00210_c|c_testsuite_x86_backend_src_00189_c|c_testsuite_x86_backend_src_00057_c|c_testsuite_x86_backend_src_00124_c|backend_lir_to_bir_notes|backend_x86_handoff_boundary)$'`

Result: `backend_lir_to_bir_notes` passed and `backend_x86_handoff_boundary`
passed. `c_testsuite_x86_backend_src_00210_c` and
`c_testsuite_x86_backend_src_00189_c` both fail with the single-function
prepared-module gate. `c_testsuite_x86_backend_src_00057_c` still fails in the
minimal emitter family, and `c_testsuite_x86_backend_src_00124_c` still fails
in scalar-control-flow lowering.
