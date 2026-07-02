Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 13
Current Step Title: Repair The Next Remaining Semantic Family

# Current Packet

## Just Finished

Step 13 - Repair The Next Remaining Semantic Family inspection subpacket
completed the GEP-family boundary read for `src/20000717-4.c`; no code or test
files were edited.

Exact boundary:

- The current case log still reports semantic `lir_to_bir` failure in function
  `x`, classified as `gep local-memory`.
- The LLVM-path shape is a global struct-member array chain:
  `%t0 = gep %struct._anon_0, @s, 0, 1`, then
  `%t2 = gep %struct.slot, %t0, 0`, then
  `%t3 = gep %struct.slot, %t2, 0, 0`, followed by the dynamic scalar element
  GEP `%t9 = gep i32, %t3, %t8`.
- The constant global GEP chain reaches the `[6 x i32]` member through
  `global_pointer_slots`, but it does not publish a
  `dynamic_global_scalar_arrays` authority for that scalar array subobject.
  The final dynamic `i32` GEP therefore cannot consume global scalar-array
  provenance/range facts and falls through to `fail_gep()`.

## Suggested Next

Recommended next packet: repair the global scalar-array subobject publication
boundary for `src/20000717-4.c` by publishing or admitting dynamic
global-scalar-array authority when a constant global GEP chain lands on a
scalar array member and a later GEP indexes that member dynamically.

Focused BIR test gap to add or extend:

- Add a `20000717-4`-style fixture, for example
  `expect_global_struct_member_scalar_array_dynamic_gep_publishes_authority`,
  with `%struct.slot = type { [6 x i32] }`,
  `%struct._anon_0 = type { i32, [4 x %struct.slot] }`, global `@s`, the
  constant GEP chain to `s.slot[0].field`, and a dynamic `i32` element GEP plus
  load.
- Assert semantic lowering succeeds without `gep local-memory`, and pin that
  the dynamic scalar element pointer is backed by available
  `GlobalStaticGepAuthorityRecord` / `DynamicGlobalScalarArray` facts rather
  than by RV64/MIR inference.

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, runtime-comparison
changes, and RV64/MIR inference.

Do not route this as a local-slot repair: the representative source uses a
global object, but the shared semantic failure bucket still reports
`gep local-memory`.

The existing dynamic global member-array fixture starts from a top-level global
array and already passes. The missing shape is a global struct field that is an
array of structs, then a nested scalar array member indexed dynamically.

Keep the downstream `src/20000314-1.c` and `src/20001026-1.c` object-route
failures out of this producer packet.

## Proof

Proof log preserved: `test_after.log`.

Commands run:

- Inspection only; no build or CTest run was required by the delegated packet.
- `./build/c4cll --codegen llvm --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000717-4.c -o /tmp/20000717-4.ll`
- `./build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000717-4.c`
  reproduced the semantic `gep local-memory` failure.

Inspected case log:
- `build/rv64_gcc_c_torture_backend/src_20000717-4.c/case.log`

Recommended RV64 representative proof command after repair:

- `cmake --build --preset default && ALLOWLIST=build/agent_state/557_step13_20000717.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`
