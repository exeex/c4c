Status: Active
Source Idea Path: ideas/open/aarch64-codegen-03-alu-fallback-operand-phase-extraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract The Phase-Local Fallback Boundary

# Current Packet

## Just Finished

Step 2 - Extract The Phase-Local Fallback Boundary completed a
behavior-preserving in-file extraction in `alu.cpp`.

`make_scalar_fallback_operand` now delegates to a local
`ScalarFallbackOperandSelector` in the anonymous namespace. The selector owns
the scalar ALU fallback decision boundary while preserving the exact previous
selection order:

- immediate value
- already-emitted scalar register
- unpublished same-block load-local prepared memory source
- resolved named scalar operand
- prepared scalar load source
- final named scalar fallback

No `alu.hpp`, control-publication, materialization, test, or expectation files
were changed.

## Suggested Next

Delegate Step 3 - Rewire Call Sites Without Semantic Drift to decide whether
`lower_scalar_instruction` should instantiate or otherwise expose the local
fallback selector at the fallback call-site boundary, while keeping behavior and
control-publication paths unchanged.

## Watchouts

- `make_scalar_fallback_operand` remains the only fallback call-site helper
  used by `lower_scalar_instruction`; Step 2 did not require call-site rewiring.
- Keep `make_control_publication_operand` and all `materialize_control_*` /
  `append_control_*` helpers outside this fallback boundary.
- If Step 3 would require changing control-publication materialization, stop for
  plan review instead of widening the extraction.

## Proof

Delegated proof passed and wrote `test_after.log`:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result|backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value)$"; } 2>&1 | tee test_after.log'`

`git diff --check` passed.
