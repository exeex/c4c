Status: Active
Source Idea Path: ideas/open/653_stack_carried_pointer_source_publication_materialization.md
Source Plan Path: plan.md
Current Step ID: 4A
Current Step Title: Publish The `%t23` Compare Pointer Source Chain

# Current Packet

## Just Finished

Step 4A, `Publish The %t23 Compare Pointer Source Chain`, published the real
`src/loop-2e.c` compare pointer producer and carried it into prepared BIR.

- LIR-to-BIR now preserves structured pointer-address facts through
  `ptrtoint`/integer add/sub-immediate/`inttoptr` chains and admits those
  nonzero named-base pointer facts as compare-operand source producers.
- `src/loop-2e.c` semantic BIR now has `%t23 = bir.add ptr %t21, 156` before
  `%t24 = bir.ne ptr %t20, %t23`; prepared BIR carries the same producer.
- `%t23` is now produced after the call from `%t21`, so the old stack-carried
  preservation shape `%t23` value id `27` / `slot #46+stack336` is no longer
  the active authority chain. The refreshed prepared shape is `%t23` value id
  `26` / `slot #50+stack368`.
- RV64 object emission advances past the previous
  `unsupported_terminator_fragment` and past the prepared pointer-arithmetic
  diagnostic. The current precise owner is RHS branch stack-load authority:
  `unsupported_branch_stack_load_authority`, `authority_status=missing_stack_clobber_safety`.
- Evidence is in
  `build/agent_state/653_step4a_loop_t23_pointer_source_chain/summary.md`.

## Suggested Next

Delegate the next packet to repair the prepared/RV64 branch stack-load
authority for the new `%t23` producer shape: prove the stack-clobber safety or
consume the freshly materialized `%t23 = %t21 + 156` producer directly at the
fused branch without re-inferring from stack offsets or testcase shape.

## Watchouts

- Do not chase the old `%t23` value id `27` / `slot #46+stack336` preservation
  shape as if it were still current; the semantic producer changes value
  numbering and moves `%t23` to value id `26` / `slot #50+stack368`.
- Do not treat the remaining failure as a missing semantic producer:
  `%t23 = bir.add ptr %t21, 156` is present in semantic and prepared BIR.
- Do not broaden Step 4A into the parked `%t6` runtime owner; that owner is
  downstream of the pointer-source publication/materialization boundary.
- Do not weaken expectation files, unsupported markers, allowlists, or runtime
  accounting to claim progress.

## Proof

`test_after.log`: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'`.

Result: build completed and the delegated backend subset remains red with 32
failed tests out of 365, matching `test_before.log` by failed test name. No new
backend failure names were introduced. `test_after.log` is the preserved proof
log.

Focused proof:

- `cmake --build --preset default --target c4cll backend_lir_to_bir_notes_test
  && ./build/tests/backend/bir/backend_lir_to_bir_notes_test` passed.
- `build/c4cll --target riscv64-linux-gnu --dump-bir
  tests/c/external/gcc_torture/src/loop-2e.c` wrote
  `build/agent_state/653_step4a_loop_t23_pointer_source_chain/loop-2e.bir.txt`
  and shows `%t23 = bir.add ptr %t21, 156`.
- `build/c4cll --target riscv64-linux-gnu --dump-prepared-bir
  tests/c/external/gcc_torture/src/loop-2e.c` wrote
  `build/agent_state/653_step4a_loop_t23_pointer_source_chain/loop-2e.prepared.txt`
  and shows the same `%t23` producer.
- `build/c4cll --target riscv64-linux-gnu --codegen obj
  tests/c/external/gcc_torture/src/loop-2e.c -o
  build/agent_state/653_step4a_loop_t23_pointer_source_chain/loop-2e.o` exits
  2 with `unsupported_branch_stack_load_authority` /
  `authority_status=missing_stack_clobber_safety`, recorded in
  `build/agent_state/653_step4a_loop_t23_pointer_source_chain/loop-2e.obj.stderr.txt`.
