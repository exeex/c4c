Status: Active
Source Idea Path: ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Scalar-Control-Flow Producer Admission

# Current Packet

## Just Finished

Step 2 - Repair Scalar-Control-Flow Producer Admission: added scalar phi-edge
producer materialization at the predecessor boundary. When a scalar `i1` phi
incoming names a compare result produced earlier in the controlling CFG path,
the predecessor block now gets a real BIR `BinaryInst` before its terminator so
Route 7 branch-condition lookup and Route 5 phi-edge publication both observe
the semantic producer.

## Suggested Next

Delegate the Step 3 RV64 representative proof packet for scalar-control-flow
rows, starting with `src/20000314-3.c` / `attr_eq` or the supervisor-selected
current substitute.

## Watchouts

- Keep function-signature work in
  `ideas/open/561_bir_function_signature_semantic_producer_admission.md`.
- Keep scalar-binop work in
  `ideas/open/562_bir_scalar_binop_semantic_producer_admission.md`.
- Do not claim scalar-control-flow progress through expectation rewrites,
  unsupported downgrades, allowlist edits, or named-case shortcuts.
- This packet needed a narrow private declaration in
  `src/backend/bir/lir_to_bir/lowering.hpp` for the pending scalar phi producer
  map; the implementation remains in `module.cpp`.
- The repair intentionally materializes only scalar `i1` compare producers for
  phi-edge/control-flow admission and does not widen into scalar-binop,
  function-signature, RV64 ABI, or object-emission lowering.

## Proof

Ran `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$'; } 2>&1 | tee test_after.log`:
passed. Supervisor acceptance also ran
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`:
passed, 346/346 backend tests.
