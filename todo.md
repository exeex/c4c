Status: Active
Source Idea Path: ideas/open/312_rv64_local_stack_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Local Effective Address Materialization

# Current Packet

## Just Finished

Step 2 coverage packet added compact backend cases for the selected local
address publication boundary:

- `tests/backend/case/riscv64_prepared_local_array_base_pointer.c` covers
  array decay/base address materialization stored into a pointer local and then
  used through `p[0]`/`p[1]`.
- `tests/backend/case/riscv64_prepared_local_array_subobject_pointer.c` covers
  a constant local array subobject address (`&arr[1][3]`) stored into a pointer
  local and dereferenced.

Added prepared-BIR dump tests that pass today and prove the cases lower through
pointer-local store/load and frame-slot addressing without using c-testsuite
filenames. Added codegen-route xfail contracts for the desired repair shape:
the route must form the local address from `sp`, store that pointer into the
pointer local, reload/use through the pointer path, and complete the return
tail.

Current route evidence remains the expected pre-repair failure. The base-array
case emits `sd t0, 0(sp)` without first materializing `t0` from `sp` and stops
before the final return tail. The subobject case similarly emits `sd t0,
32(sp)` before any local address formation, even though the later direct load
from the subobject stack slot completes.

## Suggested Next

Implement local frame-slot address publication for address-taken local
array/subobject values stored into pointer locals. The focused route contracts
should be flipped from xfail to ordinary passing tests once the emitter
materializes the pointer value before `store_local %lv.p`.

## Watchouts

- Do not special-case candidate filenames, variable names, or fixed stack
  offsets.
- The route tests intentionally use `WILL_FAIL` as coverage-before-repair.
  Remove that property in the implementation packet when the desired address
  formation snippets are present.
- The prepared-BIR dump contract currently proves the store/load shape and
  frame-slot accesses, not the final address publication. The route xfail tests
  are the semantic emission contract.
- Keep `src/00143.c` out of the first implementation packet; its indexed
  array/select-update chains are broader than the pointer-local publication
  boundary covered here.

## Proof

Focused proof passed:

`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route)_riscv64_prepared_local_array_(base|subobject)_pointer'`

Result: `4/4` passed. The two dump tests pass normally; the two route tests
pass via explicit `WILL_FAIL` expected-failure properties. No broad backend
guard was requested, and `test_after.log` was not produced or modified.
