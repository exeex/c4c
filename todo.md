Status: Active
Source Idea Path: ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Add Scalar-Control-Flow BIR Coverage

# Current Packet

## Just Finished

Step 1 - Add Scalar-Control-Flow BIR Coverage: added focused BIR coverage for
the scalar-control-flow producer boundary. The new coverage asserts that
same-block scalar comparison operands and fused branch conditions publish
materialized producer facts today, while phi operands and predecessor-block
condition values remain outside the admitted producer boundary.

## Suggested Next

Delegate the producer repair packet for scalar-control-flow phi/cross-CFG
producer admission, using the new BIR coverage as the expected boundary.

## Watchouts

- Keep function-signature work in
  `ideas/open/561_bir_function_signature_semantic_producer_admission.md`.
- Keep scalar-binop work in
  `ideas/open/562_bir_scalar_binop_semantic_producer_admission.md`.
- Do not claim scalar-control-flow progress through expectation rewrites,
  unsupported downgrades, allowlist edits, or named-case shortcuts.
- Current coverage documents the repair boundary only; it does not implement
  phi or predecessor-block producer admission.

## Proof

Ran `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$'; } 2>&1 | tee test_after.log`:
passed. Proof log: `test_after.log`.
