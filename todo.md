# Execution State

Status: Active
Source Idea Path: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Repair The Selected Semantic/BIR Seam
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Step 2.2 repaired the bootstrap scalar-floating-global seam in semantic
`lir_to_bir`. Scalar `float`/`double` globals now lower through the minimal
global path into admitted `F32`/`F64` BIR initializers from the LLVM-style FP
initializer text that our frontend emits, and `backend_lir_to_bir_notes` now
proves that admitted floating-global lane directly. Under the packet proof,
`00119` and `00123` no longer fail with the old bootstrap-global diagnostic and
instead advance into a downstream x86 emitter restriction on the return shape.

## Suggested Next

Treat scalar floating globals as repaired for idea 58 and rehome `00119` and
`00123` to the downstream x86 emitter route they now expose. Keep the next
idea-58 packet on remaining semantic/BIR failures that still stop before the
prepared-module handoff instead of pulling these two cases back upstream.

## Watchouts

- The scalar-global fix is semantic/BIR-only; it does not widen the downstream
  x86 same-module data or return-shape emitter support.
- `00119` and `00123` now fail in the same downstream x86 emitter family, so
  they should move with that route rather than being treated as remaining
  bootstrap-global work.
- The notes coverage proves both admitted `float` and `double` scalar-global
  initializers through the minimal global lane, which is the durable protection
  against regressing back to the old bootstrap-global failure.
- Keep this runbook on upstream semantic/BIR gaps; do not add emitter-side
  shortcuts for floating globals under idea 58.

## Proof

Ran the packet proof command and preserved `/workspaces/c4c/test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|c_testsuite_x86_backend_src_00119_c|c_testsuite_x86_backend_src_00123_c)$' | tee test_after.log`.
Current result: `backend_lir_to_bir_notes` passes with the new admitted scalar
floating-global lane covered. `00119` and `00123` no longer fail with the old
bootstrap-global diagnostic from `test_before.log`; the after-log shows both
cases now fail downstream with the x86 emitter's unsupported return-shape
restriction instead. The delegated proof therefore remains nonzero overall
because these cases have advanced into the next downstream route, not because
scalar floating globals are still unadmitted. `test_after.log` is the canonical
proof artifact for this packet.
