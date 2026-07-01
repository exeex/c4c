Status: Active
Source Idea Path: ideas/open/512_stack_passed_parameter_home_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Publish Ordinary-C ABI Metadata For Stack Parameters

# Current Packet

## Just Finished

Step 4 of `plan.md` published ordinary-C RV64 stack-parameter ABI metadata
before RV64 consumption.

Implementation boundary:

- `lir_to_bir/module.cpp` now applies an RV64 ordinary-C ABI pressure pass
  after BIR lowering and before prealloc consumption.
- The pass marks only eligible fixed scalar call arguments and formals beyond
  the current eight-slot RV64 ordinary-C ABI register budget as
  `passed_on_stack`.
- The pass skips variadic functions/calls, byval, sret, memory-class
  aggregates, I128, F128, VRM/vector-like values, missing ABI records, and
  zero-sized or zero-aligned ABI records.
- Step 3 remains the prealloc authority boundary: caller offsets and formal
  homes still require explicit `passed_on_stack` ABI metadata. RV64 object
  emission, unsupported markers, expectation files, allowlists, runtime
  comparison, and scan accounting were not changed.

Representative before/after facts for `tests/c/external/gcc_torture/src/20001017-1.c`:

- Before: caller stack ABI bindings for indexes 8-12 had
  `destination_storage=stack_slot` but no `stack_offset`.
- After Step 4: caller stack ABI bindings for indexes 8-12 publish explicit
  offsets `0`, `8`, `16`, `24`, and `32`.
- Before: callee homes for `%p.B`, `%p.fdB`, `%p.b`, `%p.C`, and `%p.fdC`
  were all `kind=none`.
- After Step 4: `%p.B`, `%p.fdB`, and `%p.fdC` publish stack-slot homes at
  `slot#9+stack32`, `slot#10+stack40`, and `slot#11+stack44`.
- `%p.b` and `%p.C` remain `kind=none`: the producer metadata now marks them
  stack-passed, but prealloc has no unique regalloc spill-slot home for those
  values in this representative. No index/name fallback or RV64 inference was
  added to manufacture those homes.

## Suggested Next

Supervisor should review whether Step 5 can proceed with the now-published
caller offsets and partial callee homes, or whether `%p.b`/`%p.C` need a narrow
producer home-publication packet first.

## Watchouts

- The callee helper still deliberately requires explicit
  `param.abi->passed_on_stack` plus a unique matching `regalloc.spill_slot`
  object and frame slot. Missing ABI, ambiguous, duplicate, byval, sret,
  varargs, F128, dynamic, or aggregate forms remain fail-closed.
- `%p.b` and `%p.C` are now metadata-authorized stack-passed formals but still
  lack unique prepared homes. Do not let Step 5 synthesize their home offsets
  in RV64.
- `20001017-1.c` object codegen still fails before parameter-home consumption
  at `unsupported_stack_frame ... fpr:fs1`; do not treat this plan repair as
  object advancement.
- Existing byval/sret/memory aggregate paths remain outside this metadata pass
  and were covered by the focused byval subset.

## Proof

- `cmake --build build --target c4cll` passed.
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20001017-1.c`
  was captured before and after under
  `build/agent_state/512_step4_stack_parameter_metadata/20001017-1/`.
- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20001017-1.c -o build/agent_state/512_step4_stack_parameter_metadata/20001017-1/20001017-1.o`
  still fails with rc 2 at the existing `fpr:fs1` unsupported stack-frame
  diagnostic.
- `ctest --test-dir build -j --output-on-failure -R 'stack_passed_parameter_home|prepare_frame_stack_call_contract|prealloc_formal_publications|backend_riscv_object_emission|riscv64_byval|rv64_runtime_riscv64_byval' | tee test_after.log`
  passed, 15/15. `test_after.log` is the canonical proof log for this packet.
- `git diff --check -- src/backend/bir/lir_to_bir/module.cpp tests/backend/CMakeLists.txt todo.md`
  passed.
