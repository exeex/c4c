# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now admits producer-valid `TB_VA_LIST` globals through direct,
  ordinary pointer, and ordinary fixed-array routes.
- Target-shaped pointer-object versus `%struct.__va_list_tag_` storage, exact
  size/alignment, structured declaration identity, pointer depth, dimensions,
  object/linkage/visibility/alignment, and initializer facts survive Foundation,
  Raw BIR, and Canonical BIR.

## Suggested Next

- Audit the remaining producer-emitted global `TypeSpec` families and make the
  Step 3 checkpoint decision if no additional coherent family remains.

## Watchouts

- Va-list pointee/element facts carry the target storage form and require the
  canonical structured declaration identity and exact unpacked/nonopaque
  producer layout on struct-backed targets; Foundation cross-resolves direct
  and nested identities, and no rendered spelling is parsed for authority.
- Split pointer-to-array shapes, references, function pointers, direct mirrors,
  residual enum/vector/VRM facts, and negative/unsized/computed bounds remain
  closed.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records all 4/4 backend
  tests passing.
