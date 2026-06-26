Status: Active
Source Idea Path: ideas/open/370_rv64_object_route_byval_aggregate_param_homes.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement Byval Home Repair or Refined Boundary

# Current Packet

## Just Finished

Step 1 audit completed for the `src/20030914-2.c` RV64 object-route byval
aggregate parameter-home boundary. The representative prepared dump names
`f(ptr byval(size=72, align=4) %p.pa, i32 %p.pb)`, with `%p.pa` published as a
prepared stack-slot home (`home %p.pa value_id=0 kind=stack_slot slot_id=0
offset=0`) and stack object `#18` as `source_kind=byval_param type=ptr size=72
align=4 address_exposed=yes requires_home_slot=yes permanent_home_slot=yes`.
The callee frame publishes slot `#0` at offset 0, size 72, align 4, and the
incoming byval payload is already scalarized into explicit 4-byte
`load_local`/`store_local` pairs for offsets 0 through 68. The caller-side
argument plan also records byval aggregate-address passing for the call to `f`
and a stack-slot call-boundary move for ABI index 0.

## Suggested Next

Execute Step 2 from `plan.md`. The supervisor accepted the current
`riscv64_byval_preserved_pointer_args` backend failures as pre-existing for this
audit packet because the monotonic regression guard showed no before/after
delta. The smallest supportable Step 2 shape is RV64 byval aggregate formal
entry where the byval parameter is a default-address-space prepared stack-slot
home backed by a permanent `source_kind=byval_param` frame object with explicit
size/align and scalarized in-bounds `load_local`/`store_local` payload facts.

## Watchouts

The missing object-route lowering rule is callee-entry materialization for a
byval formal whose home is a prepared stack slot. The current object-route guard
rejects this shape before emission with `unsupported_byval_param_home`.
Adjacent shapes should stay fail-closed when the byval formal lacks a prepared
stack-slot home, lacks a `source_kind=byval_param` permanent frame object,
has missing or ambiguous size/align/frame-slot facts, uses non-default or
dynamic address-space/layout facts, or requires inferring aggregate bytes from
ABI stack layout instead of prepared scalar payload facts. The synthetic
`make_prepared_byval_stack_slot_param_module()` fixture covers the same
72-byte/align-4 stack-slot byval parameter shape and currently expects the same
precise unsupported diagnostic.

## Proof

Artifacts and commands used:

- Existing failure evidence:
  `build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log`
- Representative prepared dump command:
  `build/c4cll --dump-prepared-bir -I tests/c/external/gcc_torture --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20030914-2.c`
- Existing focused fixture inspected:
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- Delegated proof command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Result: build passed, `test_after.log` exists, but the delegated backend CTest
subset failed in existing byval prepared-call-boundary coverage:
`backend_codegen_route_riscv64_byval_preserved_pointer_args` rejected forbidden
snippet `mv a1, s1`, and
`backend_rv64_runtime_riscv64_byval_preserved_pointer_args` segfaulted under
QEMU. This packet did not touch implementation or tests. Supervisor monotonic
comparison against `test_before.log` passed with no new failures:
before passed=322 failed=2 total=324; after passed=322 failed=2 total=324.
