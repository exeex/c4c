Status: Active
Source Idea Path: ideas/open/aarch64-codegen-03-alu-fallback-operand-phase-extraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Rewire Call Sites Without Semantic Drift

# Current Packet

## Just Finished

Step 3 - Rewire Call Sites Without Semantic Drift audited the scalar binary
fallback call sites in `lower_scalar_instruction` after the Step 2 extraction.

No `alu.cpp` code change was needed. The existing
`make_scalar_fallback_operand` calls are already the correct call-site
boundary: they keep `ScalarFallbackOperandSelector` private to the anonymous
namespace, avoid exposing fallback internals in `lower_scalar_instruction`, and
preserve the exact fallback selection order:

- immediate value
- already-emitted scalar register
- unpublished same-block load-local prepared memory source
- resolved named scalar operand
- prepared scalar load source
- final named scalar fallback

No `alu.hpp`, control-publication, materialization, test, or expectation files
were changed.

## Suggested Next

Delegate the next plan step to validate the completed extraction boundary and
decide whether this runbook is ready for broader supervisor-side validation or
plan-owner review.

## Watchouts

- `make_scalar_fallback_operand` remains the only scalar binary fallback
  call-site helper used by `lower_scalar_instruction`.
- Keep `make_control_publication_operand` and all `materialize_control_*` /
  `append_control_*` helpers outside this fallback boundary.
- Rewiring `lower_scalar_instruction` to instantiate
  `ScalarFallbackOperandSelector` directly would be churn and would leak a
  private fallback concept into the call-site surface.

## Proof

Delegated no-code-change proof passed:

`git diff --check`

No code changed, so the delegated build/ctest command was not run and no log
file was created or touched for this packet.
