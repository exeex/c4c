Status: Active
Source Idea Path: ideas/open/653_stack_carried_pointer_source_publication_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative Integration

# Current Packet

## Just Finished

Step 4, `Prove Representative Integration`: refreshed semantic BIR,
prepared-BIR, RV64 object, disassembly, and runtime evidence for the two idea
653 representative rows.

- Evidence is in
  `build/agent_state/653_step4_representative_integration/summary.md`.
- For `src/20140828-1.c` / `%t6`, semantic BIR now has the explicit producer
  `%t6 = bir.add ptr %lv.a.0, 2`, prepared output has available RHS branch
  stack-load authority for `%t6` value id `18` in slot `#16+stack8`, prepared
  access metadata records `address_materialization ... result=%t6 ... offset=6`,
  ASM/object materialize the branch RHS as `sp+6`, and object emission exits 0.
- `20140828-1.c` advances to runtime: clang RV64 exits 0, while the c4c RV64
  binary exits 134 under qemu. This is downstream of `%t6` source
  publication/materialization and points at runtime correctness for the
  preceding `f(a, 1, &d)` callee/result or frame-slot value path.
- For `src/loop-2e.c` / `%t23`, semantic BIR still reaches
  `%t24 = bir.ne ptr %t20, %t23` without an explicit `%t23` producer in the
  dump. Prepared output has stack freshness for `%t23` value id `27` in slot
  `#46+stack336`, but call preservation remains stack-slot-only with no
  `source_selection`.
- `loop-2e.c` still stops before object/disassembly/runtime on the c4c route:
  `--codegen obj` exits 2 with `unsupported_terminator_fragment: BIR
  terminator requires unsupported RV64 object lowering`.

## Suggested Next

Supervisor review/commit for the Step 4 evidence slice. Next execution should
choose whether to continue idea 653 with the `loop-2e.c` `%t23` producer gap or
split/park Step 4 because `20140828-1.c` has moved to a distinct downstream
runtime correctness owner.

## Watchouts

- Do not treat `20140828-1.c` runtime abort as missing `%t6` branch source
  publication; the refreshed branch evidence is explicit and materialized.
- Do not claim `loop-2e.c` integration from ASM printing alone; the c4c object
  route still fails before disassembly/runtime, and `%t23` lacks an explicit
  semantic/prepared source-selection chain.
- Do not weaken expectation files, unsupported markers, allowlists, or runtime
  accounting to claim progress.

## Proof

`test_after.log`: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'`.

Result: build completed and the delegated backend subset remains red with 32
failed tests out of 365, matching `test_before.log` by failed test name. No new
backend failure names were introduced. `test_after.log` is the preserved proof
log.

Additional focused probes:

- `build/c4cll --target riscv64-linux-gnu --dump-prepared-bir
  tests/c/external/gcc_torture/src/20140828-1.c` wrote
  `build/agent_state/653_step4_representative_integration/20140828-1.prepared.txt`
  and shows `%t6` RHS branch authority as `status=available`.
- `build/c4cll --target riscv64-linux-gnu --codegen obj
  tests/c/external/gcc_torture/src/20140828-1.c -o
  build/agent_state/653_step4_representative_integration/20140828-1.o` exited
  0; disassembly is
  `build/agent_state/653_step4_representative_integration/20140828-1.objdump.txt`.
- `qemu-riscv64 -L /usr/riscv64-linux-gnu
  build/agent_state/653_step4_representative_integration/20140828-1.c4c.bin`
  exited 134 while the clang RV64 binary exited 0.
- `build/c4cll --target riscv64-linux-gnu --dump-prepared-bir
  tests/c/external/gcc_torture/src/loop-2e.c` wrote
  `build/agent_state/653_step4_representative_integration/loop-2e.prepared.txt`
  and shows `%t23` RHS branch stack freshness as `status=available` but without
  `source_selection`.
- `build/c4cll --target riscv64-linux-gnu --codegen obj
  tests/c/external/gcc_torture/src/loop-2e.c -o
  build/agent_state/653_step4_representative_integration/loop-2e.o` exited 2;
  diagnostic log:
  `build/agent_state/653_step4_representative_integration/loop-2e.obj.stderr.txt`.
