Status: Active
Source Idea Path: ideas/open/583_rv64_pointer_arithmetic_result_publication.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair Pointer Result Materialization

# Current Packet

## Just Finished

Step 3 repaired RV64 object emission for supported pointer-result `add` and
`sub` shapes where one prepared pointer base combines with one prepared integer
byte-offset operand and the result has a prepared pointer destination home.

The encoded object fragment path now accepts `Ptr` add/sub before the scalar
integer-only gate, emits `addi`/`add`/`sub` from prepared register or immediate
offset facts, and publishes the pointer result into the prepared destination
register or stack home. The text helper path also accepts register destination
homes for the same supported shape. Focused backend coverage now builds both
add and sub objects and proves the later pointer-value local-memory store uses
the published `%result.ptr` owner; a missing result home still fails closed
with the pointer-arithmetic-specific diagnostic.

## Suggested Next

Execute Step 4 from `plan.md`: rebuild `c4cll`, rerun prepared dumps and the
RV64 object route for `tests/c/external/gcc_torture/src/20000819-1.c`, and
record whether the representative advances past the old
`unsupported_pointer_arithmetic` owner or lands on a distinct downstream owner.

## Watchouts

- Do not select or mutate deferred `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`.
- The focused fixture is semantic: do not repair it with filename, function,
  block, value-name, or diagnostic-string shortcuts.
- Preserve fail-closed behavior for pointer arithmetic forms outside prepared
  pointer base plus integer byte-offset add/sub with a prepared destination
  home.
- The Step 3 object-fragment repair intentionally stays inside prepared facts:
  unsupported operand type combinations, missing homes, and unprepared offsets
  remain rejected by the pointer-arithmetic diagnostic path.
- Step 4 should compare against the Step 1 owner coordinates for
  `20000819-1.c`: `function=foo`, `block=entry`, `instruction_index=7`,
  `owner=ptr %t4`.

## Proof

`test_after.log` records the delegated proof:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`

Result: passed, `backend_riscv_object_emission` 1/1.
