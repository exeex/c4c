# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 scalar-global producer-alignment packet now imports actual
  `lower_global` rows whose optional `llvm_type_ref` is absent, using the exact
  structured `LirGlobal::type` as target-aware authority for the already
  supported integer and floating scalar family.
- A single validated global-type helper enforces non-void well-formed receipt,
  canonical `llvm_type` parity, and exact agreement from any present structured
  mirror. Import reuses that result without dereferencing an absent optional.

## Suggested Next

- Execute one bounded remaining Step 3 global, external-symbol, or initializer
  completeness packet selected from the runbook, keeping aggregate and
  flexible-special-type global support separate.

## Watchouts

- Scalar authority is deliberately limited to the existing target-aware
  `lower_constant_type` domain: plain integer/floating bases with no pointer,
  reference, array, pointer-to-array, inner-rank, or function-pointer shape.
- Present `llvm_type_ref` remains mirror evidence and must lower identically;
  rendered `llvm_type` is parity-only and is never parsed into semantic type.
- Mirror-free pointers, arrays, named/literal aggregates, flexible-special
  types, and all linkage, visibility, qualifier, and initializer expansion
  remain fail-closed and separate.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof covers mirror-free integer declarations and
  floating definitions, I686 target-aware `long` width, typed views with
  visibility/initializer facts intact, strict present-mirror parity, and
  Raw/Canonical rollback for pointer, array, aggregate, and text conflicts.
