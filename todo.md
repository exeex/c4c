# Current Packet

Status: Active
Source Idea Path: ideas/open/582_rv64_va_start_stack_backed_destination.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair Destination-Address Materialization

## Just Finished

Step 3 from `plan.md` is complete.

Repaired RV64 `va_start` helper lowering for the semantic stack-backed
`destination_va_list_address` shape.

Implementation surface:
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

The helper diagnostic now accepts prepared destination-address homes in either
a GPR or a validated stack slot. Stack-backed destination-address homes use
`t0` as the destination-address helper scratch while retaining `t1` for the
overflow-area pointer value. The lowering still computes the destination base
from the prepared `destination_va_list` stack home and still fails closed for
unsupported address homes, scratch-register aliases, malformed va_list stack
homes, and destination-address stack slots that alias the destination va_list
slot.

The focused backend fixture was converted from the old broad rejection to
positive RV64 object emission. It verifies the stack-backed destination-address
shape, exact helper bytes, and no relocations.

## Suggested Next

Execute Step 4 from `plan.md`: rebuild `c4cll`, rerun prepared dumps and RV64
object-route logs for `tests/c/external/gcc_torture/src/va-arg-21.c`, and
record whether the representative advances past the old
`unsupported_variadic_helper_lowering` owner or lands on a narrower downstream
owner.

## Watchouts

- Preserve the semantic helper shape; do not match `src/va-arg-21.c` by
  filename, function, block, value name, or source text.
- The focused test now proves helper support for distinct stack slots for the
  va_list object and the stack-backed destination address.
- Keep unsupported non-stack, malformed, or aliasing helper forms fail-closed.
- The Step 4 representative may still fail later in the route; the key check is
  advancement past the prior broad GPR-home `va_start` destination-address
  owner.
- Preserve unrelated open ideas, including untracked
  `ideas/open/583_rv64_pointer_arithmetic_result_publication.md`.

## Proof

Proof log: `test_after.log`.

Command:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`

Result: passed, `backend_riscv_object_emission` 1/1.
