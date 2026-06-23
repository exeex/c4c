Status: Active
Source Idea Path: ideas/open/318_rv64_byval_aggregate_call_abi.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Byval Aggregate Call Coverage

# Current Packet

## Just Finished

Completed idea 318 Step 2 by adding focused expected-repair backend coverage
for RV64 byval aggregate call ABI transport and callee-side byval home loads.

New focused fixture:

- `tests/backend/case/riscv64_byval_aggregate_fixed_call.c`

New expected-repair tests:

- `backend_dump_riscv64_byval_aggregate_fixed_call`
- `backend_codegen_route_riscv64_byval_aggregate_fixed_call_expected_repair`

The fixture is a fixed-arity, non-variadic call using a small aggregate with
two integer lanes and one pointer lane. It proves the same Step 1 boundary
without copying `src/00140.c`, `%p.f`, its field names, or fixed stack homes.

Dump coverage asserts:

- a callee signature with `ptr byval(size=16, align=8)`
- callee-side aggregate-param and byval copy loads from `addr %p.value`
- pointer-lane byval home load at offset `+8`
- caller-side byval call and `aggregate_address` arg0 plan
- before-call `destination_storage=stack_slot` ABI movement
- `source_kind=byval_param` stack object and `base=pointer_value` callee
  access records

Codegen coverage asserts the current bad RV64 route stays captured: the callee
emits only its prologue and the caller truncates before a `call consume_parcel`
or any `ret`, while still showing the initialized aggregate payload stores.
That rejects fake callee-local values by requiring the prepared byval caller and
callee access facts, not just a direct local aggregate read.

## Suggested Next

Repair the fixed-arity RV64 byval aggregate path: caller payload transport into
the byval ABI slot and callee-side byval home loads should emit enough assembly
for the focused fixture to become positive codegen/runtime coverage.

## Watchouts

- Do not special-case `src/00140.c`, `%p.f`, field names, or fixed stack-slot
  homes.
- Do not fake callee-local aggregate values; prove caller-side payload
  transport across the call boundary.
- Keep aggregate-local, function-pointer, external-call, and select/control
  repairs out of scope unless evidence proves a true dependency.
- Float-lane byval ABI coverage should split if the fixed integer/pointer
  byval repair exposes a separate FPR/float aggregate lane mechanism; this
  Step 2 fixture stays focused on the first bad callee/caller byval transport
  boundary.

## Proof

Focused proof passed 2/2:
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R 'backend_(dump|codegen_route)_riscv64_byval_aggregate_fixed_call'`.

Delegated proof ran:
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The
fresh proof log is `test_after.log`.
