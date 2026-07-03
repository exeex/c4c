Status: Active
Source Idea Path: ideas/open/561_prepared_local_address_base_plus_offset_boundary_evidence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce Local Address Evidence

# Current Packet

## Just Finished

Step 1 reproduced retained representative `src/20000519-1.c` from
`tests/c/external/gcc_torture/src/20000519-1.c` through same-snapshot semantic
BIR, prepared BIR, MIR debug observation, and RV64 object routing.

Commands:

```sh
./build/c4cll --dump-bir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20000519-1.c > build/agent_state/561_step1_20000519_1/semantic_bir.txt 2> build/agent_state/561_step1_20000519_1/semantic_bir.stderr
./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20000519-1.c > build/agent_state/561_step1_20000519_1/prepared_bir.txt 2> build/agent_state/561_step1_20000519_1/prepared_bir.stderr
./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20000519-1.c -o build/agent_state/561_step1_20000519_1/rv64.o > build/agent_state/561_step1_20000519_1/rv64_obj.stdout 2> build/agent_state/561_step1_20000519_1/rv64_obj.stderr
./build/c4cll --trace-mir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20000519-1.c > build/agent_state/561_step1_20000519_1/trace_mir.txt 2> build/agent_state/561_step1_20000519_1/trace_mir.stderr
./build/c4cll --dump-mir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20000519-1.c > build/agent_state/561_step1_20000519_1/dump_mir.txt 2> build/agent_state/561_step1_20000519_1/dump_mir.stderr
```

Results: semantic BIR, prepared BIR, trace MIR, and dump MIR exited 0; RV64
object route exited 2.

Tied failing access:

- Semantic BIR: `bar`, `block_1`, instruction 2:
  `%t2 = bir.load_local i32 %t2.addr, addr %t0`
  in `build/agent_state/561_step1_20000519_1/semantic_bir.txt`.
- Prepared BIR: matching prepared-addressing row:
  `access block=block_1 inst_index=2 base=pointer_value result=%t2 pointer=%t0 offset=0 size=4 align=4 base_plus_offset=yes layout_authority=opaque_compatibility range_verdict=unknown_compatible`
  in `build/agent_state/561_step1_20000519_1/prepared_bir.txt`.
- Prepared storage for the pointer base: `%t0` is published as
  `encoding=frame_slot ... slot_id=#6 stack_offset=24`, not as a GPR base, in
  `build/agent_state/561_step1_20000519_1/prepared_bir.txt`.
- RV64 rejection point:
  `--codegen obj failed: RISC-V backend object route unsupported prepared module shape: unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing`
  in `build/agent_state/561_step1_20000519_1/rv64_obj.stderr`.

Evidence tie: yes. The same `bar` load through pointer value `%t0` is visible
in semantic BIR and prepared BIR. Prepared publishes the access as
`base_plus_offset=yes`, but RV64 object emission rejects the module at the
local-memory consumer boundary because the pointer-value base `%t0` has no
usable register home for `prepared_pointer_value_base_offset`.

## Suggested Next

Execute Step 2 by classifying whether the first owner is prepared publication
of a pointer-value base usable from a stack home, or the RV64 prepared
local-memory consumer's refusal to materialize that published stack-home
pointer base.

## Watchouts

- Do not reconstruct local-memory facts from RV64 target-specific instruction
  shapes.
- Do not combine this route with direct-call metadata repair.
- Do not weaken unsupported accounting, expected output, tests, or prepared
  admission contracts.
- Do not use named-case shortcuts for retained torture representatives.
- Keep `review/557_step13_vector_local_memory_review.md` untouched unless the
  supervisor explicitly brings it into scope.
- The object-route diagnostic is not annotated with a function or instruction
  index; the access tie above uses BIR traversal order plus the RV64 local
  memory helper predicate that requires a frame-slot absolute offset or a
  pointer-value base register.

## Proof

Step 1 validation:

```sh
git diff --check -- todo.md && scripts/plan_review_state.py show
```

Result: passed. Plan-review state reported `current_step_id` as `1` and
`current_step_title` as `Reproduce Local Address Evidence`.
