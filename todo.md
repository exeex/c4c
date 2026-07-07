# Current Packet

Status: Active
Source Idea Path: ideas/open/582_rv64_va_start_stack_backed_destination.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Backend Closure Readiness

## Just Finished

Step 5 from `plan.md` is complete.

Ran the supervisor-selected backend closure-readiness proof after the Step 3
`va_start` destination-address repair and Step 4 representative route proof.

Fresh backend result:

- `cmake --build --preset default`: rc `0`
- `ctest --test-dir build -j --output-on-failure -R '^backend_'`: rc `0`
- `100% tests passed, 0 tests failed out of 346`

Closure-readiness evidence:

- Focused semantic backend coverage for a stack-slot `destination_va_list` plus
  distinct stack-backed `destination_va_list_address` passed after the repair.
- The Step 4 representative route for
  `tests/c/external/gcc_torture/src/va-arg-21.c` no longer stops at the old
  `unsupported_variadic_helper_lowering` owner.
- Step 4 advanced the representative to the downstream ordinary call-lowering
  owner:
  `unsupported_instruction_fragment; function=doit; block=entry; instruction_index=2; instruction_kind=CallInst; owner=ptr %t1`.
- Prepared context for that downstream owner is
  `%t1 = bir.call ptr malloc(i64 4)`, before the later `llvm.va_start.p0`
  helper calls.
- The `va_start` helper operands retained the stack-backed destination-address
  shape, including `dst_va_list_addr` stack slots `#17` and `#18`.
- Case-local f128 carrier/helper sections remained empty, so this proof stayed
  inside the `va_start` destination-address boundary.

The focused coverage, representative route advancement, and backend subset
proof satisfy the source idea acceptance criteria. The source idea is ready for
supervisor/plan-owner closure evaluation.

## Suggested Next

Supervisor should ask the plan owner to evaluate lifecycle closure for
`ideas/open/582_rv64_va_start_stack_backed_destination.md`.

## Watchouts

- The representative still exits nonzero, but the remaining owner is a later
  ordinary call-lowering gap for `malloc`, not the old `va_start`
  destination-address helper owner.
- Preserve the semantic helper support and fail-closed checks from Step 3.
- Do not expand this plan into the downstream `malloc` call-lowering owner;
  that is outside the `va_start` destination-address boundary.
- Plan-owner closure should use matching close-gate logs; this packet only
  refreshed the executor-owned `test_after.log`.
- Preserve unrelated open ideas, including untracked
  `ideas/open/583_rv64_pointer_arithmetic_result_publication.md`.

## Proof

Proof log: `test_after.log`.

Command:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` records `100% tests passed, 0 tests failed out of 346`.
