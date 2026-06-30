Status: Active
Source Idea Path: ideas/open/427_rv64_scalar_select_join_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Evidence And Select A First Owned Shape

# Current Packet

## Just Finished

Step 1 inspected the select/join evidence without changing implementation
code. The first coherent implementation packet is ordinary small-integer
scalar select materialization in the RV64 object route, using `src/pr43236.c`
as the row-family anchor:
`%t13.store0 = bir.select eq i64 %t12, 0, i8 0, %t13.elt0`.

Observed rejection point: `build/rv64_gcc_c_torture_backend/src_pr43236.c/case.log`
still reports `unsupported_instruction_fragment: BIR instruction requires
unsupported RV64 object lowering`. AST-backed inspection showed the event path
enters `src/backend/mir/riscv/codegen/object_emission.cpp` through
`fragment_for_prepared_instruction`, classifies selects with
`classify_prepared_object_select_consumer`, then calls
`fragment_for_prepared_select` for `OrdinarySelect` or
`PreparedJoinTransferCarrier`. The current object select fragment rejects
`pr43236` before emission because it only admits `I32` and `I64` select result
types, while the representative row is an `I8` select-store. Existing object
helpers already have scalar memory sizes and stack load/store encodings for
`I8` and `I16`.

## Suggested Next

Implement Step 2 for the ordinary small-integer scalar select-store family.
Owned implementation target: `src/backend/mir/riscv/codegen/object_emission.cpp`,
starting at `fragment_for_prepared_select` and the local destination/value
move path it uses. Owned tests should include `tests/backend/CMakeLists.txt`
and a focused backend case under `tests/backend/case/`, preferably an `i8`
local-array select-store sibling of the existing
`riscv64_i16_local_array_select_store.c`.

Packet target: make object emission accept coherent `I8` and `I16` ordinary
scalar `bir.select` results when the compare operands, selected values, and
destination home are already materializable by prepared facts. Do not touch
join-transfer carrier behavior in this first packet except to preserve the
existing classification gate. Keep unsupported non-integer/F128/pointer or
missing-home cases fail-closed.

Supervisor-selected proof command for the implementation packet:
`cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'`.

Suggested proof refinement only, if the supervisor wants a narrower inner loop
before the full delegated subset: include the existing
`backend_codegen_route_riscv64_i16_local_array_select_store` and
`backend_rv64_runtime_riscv64_i16_local_array_select_store` targets plus the
new `i8` object-route/codegen test that exercises the `pr43236` shape.

## Watchouts

- `src/pr43236.c` is evidence, not a filename-shaped special case. The packet
  should generalize through `TypeKind::I8`/`I16` scalar select support in the
  object fragment path.
- The prepared-function text route has `emit_riscv_simple_select` support for
  `I16`, `I32`, and `Ptr`; the object route currently has a separate encoded
  fragment path and should reuse existing object helpers rather than copy text
  emission.
- `fragment_for_prepared_select` currently accepts `I32`/`I64`; its helper
  stack load/store and size paths already recognize `I8`/`I16`.
- Keep join-transfer carrier materialization for a later packet after ordinary
  select-store has a focused object-route proof.
- Do not edit expectations, allowlists, unsupported markers, runtime comparison, or pass/fail accounting.
- Keep F128, call-adjacent publication, aggregate ABI, pointer provenance, and global memory repair out of this route.
- Reject filename-shaped fixes for representative gcc_torture rows.

## Proof

Ran `git diff --check > test_after.log 2>&1`; `test_after.log` is the delegated
proof log for this Step 1 documentation-only packet.
