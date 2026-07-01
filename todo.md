Status: Active
Source Idea Path: ideas/open/512_stack_passed_parameter_home_publication.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Consume Published Homes Through RV64

# Current Packet

## Just Finished

Step 5 of `plan.md` completed producer/prealloc homes for metadata-authorized
ordinary RV64 stack-passed formals that still lacked unique prepared homes.

Implementation boundary:

- `regalloc.cpp` now runs a producer-side completion pass after ordinary
  allocation and before regalloc stack slots are published.
- The pass assigns a prepared stack slot only for RV64 fixed formals whose
  BIR formal carries explicit ordinary-C `param.abi->passed_on_stack`
  authority and whose type, size, alignment, ABI class, and value kind all
  agree.
- The existing unique spill-slot path remains the first authority path for
  formals such as `%p.B`, `%p.fdB`, and `%p.fdC`.
- The new completion path rejects missing or ambiguous formal identity,
  missing ABI, register-passed ABI, byval, sret, variadic functions/formals,
  memory-class aggregates, I128, F128, VRM/vector-like values, zero-sized or
  zero-aligned ABI records, and unsupported ABI classes.
- RV64 object emission, unsupported markers, expectations outside the focused
  dump contract, allowlists, runtime comparison, and scan accounting were not
  changed.

Representative before/after facts for `tests/c/external/gcc_torture/src/20001017-1.c`:

- Before Step 5: `%p.B`, `%p.fdB`, and `%p.fdC` had stack homes at
  `slot#9+stack32`, `slot#10+stack40`, and `slot#11+stack44`; `%p.b` and
  `%p.C` were still `kind=none`.
- After Step 5: `%p.B`, `%p.fdB`, and `%p.fdC` remain on the same stack homes.
- After Step 5: `%p.b` publishes `kind=stack_slot slot_id=12 offset=48` and
  `%p.C` publishes `kind=stack_slot slot_id=13 offset=56` from producer
  prealloc slots.
- Caller ABI bindings for indexes 8-12 remain explicit stack offsets `0`,
  `8`, `16`, `24`, and `32`.
- No RV64 consumer reconstruction, parameter-index fallback, testcase-shape
  matching, or raw name-derived offset inference was added.

## Suggested Next

Step 6 should route RV64 consumption through the now-published prepared formal
homes and verify the representative route advances past the old
`unsupported_param_home` family without reconstructing offsets in RV64.

## Watchouts

- The producer homes are frame slots for callee-side prepared publication,
  not incoming call-argument ABI offsets. RV64 consumption must still use the
  prepared home records instead of deriving incoming offsets from argument
  index or parameter spelling.
- `20001017-1.c` object codegen still fails at the existing non-goal
  `unsupported_stack_frame ... fpr:fs1` diagnostic.
- Existing byval and memory aggregate paths stayed outside the completion
  pass and were covered by the focused preservation subset.

## Proof

- `cmake --build build --target c4cll` passed.
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20001017-1.c`
  was captured before and after under
  `build/agent_state/512_step5_producer_homes/20001017-1/`.
- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20001017-1.c -o build/agent_state/512_step5_producer_homes/20001017-1/20001017-1.o`
  still fails with rc 2 at the existing `fpr:fs1` unsupported stack-frame
  diagnostic.
- `cmake -S . -B build` refreshed CTest metadata after the focused dump
  contract changed.
- `ctest --test-dir build -j --output-on-failure -R 'stack_passed_parameter_home|prepare_frame_stack_call_contract|prealloc_formal_publications|backend_riscv_object_emission|riscv64_byval|rv64_runtime_riscv64_byval|riscv64_aggregate' | tee test_after.log`
  passed, 22/22. `test_after.log` is the canonical proof log for this packet.
- `git diff --check -- src/backend/prealloc/regalloc.cpp tests/backend/CMakeLists.txt todo.md`
  passed.
