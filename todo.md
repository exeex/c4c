# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now admits arbitrary positive producer-valid pointer depth for
  direct integer/floating scalar globals and fixed scalar-base array elements.
  Typed BIR preserves exact scalar base, width, depth, ordered dimensions,
  opaque `ptr` parity, and existing object/initializer facts through Foundation,
  Raw BIR, and Canonical BIR.
- Existing depth-zero scalar array elements and depth-one pointer rows remain
  covered alongside a depth-two initialized direct-global definition and a
  depth-three weak extern declaration. Zero/negative staged pointer facts,
  negative producer depth, aggregate bases, function pointers,
  pointer-to-array/inner-rank shapes, unexpected mirrors, and spelling
  conflicts still reject transactionally.

## Suggested Next

- Audit the next producer-valid Plan Step 3 globals/objects family, or make the
  Step 3 checkpoint decision if no additional producer-emitted family remains.

## Watchouts

- Multi-level pointer depth is carried only by existing typed depth fields;
  opaque LLVM spelling remains exactly `ptr` at every positive depth and must
  not be parsed. Scalar-base, declarator-shape, and object-coherence exclusions
  remain authoritative.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records all 4/4 backend
  tests passing.
