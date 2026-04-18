Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Revalidated the active `plan.md` Step 3 `00210` packet against a fresh rebuild.
The delegated proof still leaves `backend_lir_to_bir_notes` and
`backend_x86_handoff_boundary` green while
`c_testsuite_x86_backend_src_00210_c` fails on `error: x86 backend emitter
only supports a single-function prepared module through the canonical
prepared-module handoff`. Extra rebuilt-source probes also showed that simpler
external-call cases such as `00131` and `00211` are not currently admitted by
source once the binary is rebuilt, so the honest blocker is broader than the
current `00210`-only runbook framing.

## Suggested Next

Implement the repaired `plan.md` Step 3 route against the smaller proving
cluster: restore bounded single-defined-function prepared-module direct
external-call coverage for cases like `00131` and `00211`, while keeping
multi-defined-function `00210` explicit as the next adjacent boundary instead
of the packet's proving anchor.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- Fresh semantic-BIR inspection shows `00210` already lowers the helper calls
  as direct `@actual_function` calls, so the current indirect-callee-identity
  framing in the repaired runbook is stale against rebuilt source.
- The surviving `00210` gate in rebuilt source is still the multi-defined-
  function prepared-module rejection, and adjacent rebuilt probes show the
  source also lacks simpler one-defined-function external-call coverage.
- The repaired runbook now treats `00131` and `00211` as the next proving
  cluster and keeps `00210` as an adjacent multi-defined-function boundary.
- Do not satisfy the next packet by merely deleting the
  `functions.size() != 1` rejection or by adding a testcase-shaped `00210`
  matcher. The route must match the real prepared-BIR family.
- Keep `00189`, `00057`, and `00124` out of this packet; they remain adjacent
  indirect-runtime, emitter/control-flow, and scalar-control-flow families.

## Proof

Delegated proof rerun:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00210_c)$' | tee test_after.log`

Result: `backend_lir_to_bir_notes` passed, `backend_x86_handoff_boundary`
passed, and `c_testsuite_x86_backend_src_00210_c` failed on the single-function
prepared-module gate. Proof log: `test_after.log`.

Exploratory rebuilt-source spot checks outside the delegated proof:
- `build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00210.c -o /tmp/00210.s`
- `build/c4cll --codegen asm --backend-bir-stage semantic --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00210.c -o /tmp/00210.semantic.txt`
- `build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00131.c -o /tmp/00131.s`
- `build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00211.c -o /tmp/00211.s`
