Status: Active
Source Idea Path: ideas/open/314_rv64_aggregate_local_subobject_and_byval_flow.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Aggregate-Local Coverage

# Current Packet

## Just Finished

Step 2 focused aggregate-local expected-repair coverage for idea 314.

Added focused source cases:

- `tests/backend/case/riscv64_aggregate_local_self_pointer_chain.c`
- `tests/backend/case/riscv64_aggregate_local_anonymous_union_fields.c`

Added four backend tests:

- `backend_dump_riscv64_aggregate_local_self_pointer_chain_expected_repair`
- `backend_codegen_route_riscv64_aggregate_local_self_pointer_chain_expected_repair`
- `backend_dump_riscv64_aggregate_local_anonymous_union_fields_expected_repair`
- `backend_codegen_route_riscv64_aggregate_local_anonymous_union_fields_expected_repair`

The self-pointer dump test proves prepared BIR carries the semantic chain:
store the local aggregate self pointer, load through the aggregate pointer
twice, and read the nested `int` field. Its route test captures the current
gap: RV64 assembly stops after storing the self pointer and contains no final
load/return.

The anonymous-union dump test proves prepared BIR carries 4-byte stores and
reloads for overlapping/nested local aggregate fields, while prepared
addressing currently reports out-of-bounds subobject accesses. Its route test
captures the current gap: RV64 assembly stops after the first scalar store and
does not emit the later loads/return.

## Suggested Next

Proceed to Step 3 by repairing nested aggregate subobject flow. Start with a
fail-closed guard or complete emission path for currently truncated RV64
aggregate-local functions, then repair the self-pointer pointer-value load
chain and anonymous-union 4-byte field store/load addressing without
special-casing source filenames, struct names, or fixed field offsets.

## Watchouts

- Do not special-case filenames, fixed offsets, or struct/union names.
- Do not reopen generic local frame-slot address publication unless aggregate
  evidence proves it is still incomplete.
- Keep broad vararg and floating aggregate ABI repair out of scope unless it is
  only being classified as a residual.
- The current RV64 emitter can emit/link truncated aggregate-local functions
  and only fail later under qemu; focused tests should catch missing emission
  before relying on qemu symptoms.
- `src/00140.c` includes byval aggregate copy and same-module call argument
  flow; do not fold broad byval/variadic/floating aggregate ABI work into the
  first nested-subobject repair packet.
- The new route tests intentionally assert the current truncated output and are
  labeled `expected_repair`; after the real repair they should be flipped to
  successful semantic emission/runtime contracts instead of being weakened.

## Proof

Ran:

- `cmake --build --preset default -j`
- `ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route)_riscv64_aggregate_local_(self_pointer_chain|anonymous_union_fields)_expected_repair'`
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Results:

- Build passed.
- Focused expected-repair subset passed `4/4`.
- Delegated backend proof wrote `test_after.log` and failed with the existing
  `backend_riscv_prepared_edge_publication` failure as the only failing
  backend test.
