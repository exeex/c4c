Status: Active
Source Idea Path: ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative Integration

# Current Packet

## Just Finished

Completed Step 4 by proving representative integration after the Step 3
object-route call-argument fix. Evidence lives under
`build/agent_state/648_step4_representative_integration/`, with summary at
`build/agent_state/648_step4_representative_integration/summary.md`.

The focused call-argument subset remains green: dump, text-route, and object
CLI coverage for `riscv64_call_arg_local_frame_address_materialization` all
passed.

Representative RV64 object emission for
`tests/c/external/gcc_torture/src/20000722-1.c` now succeeds and writes
`build/agent_state/648_step4_representative_integration/20000722-1.o`.
Fresh `llvm-objdump` evidence for `bar` shows the call setup now materializes
the selected local frame-slot address directly in `a0`:

```text
98: 00010513      mv a0, sp
9c: 00000097      auipc ra, 0x0
a0: 000080e7      jalr ra <.Lpcrel_hi_string_local_load_4_1_7+0x1c>
```

This removes the prior representative stale call-argument shape `mv s2, sp`
followed by `mv a0, s2` before the `foo` call. The proof grep still finds a
later `mv a0, s1` in `foo` near return setup, but that instruction is not the
`bar` call-argument owner for idea 648. No remaining downstream owner was
identified by this Step 4 proof.

## Suggested Next

Execute Step 5 by running the supervisor-selected broader validation for the
affected RV64 backend bucket and then requesting lifecycle close or park. The
current Step 4 evidence supports closure consideration for idea 648 unless
broader validation exposes a distinct downstream owner.

## Watchouts

- Use the fresh object disassembly from
  `build/agent_state/648_step4_representative_integration/20000722-1.objdump.txt`
  for representative call evidence.
- The later `foo` return-path `mv a0, s1` is not the `bar` call-argument setup.
  Do not treat that line as a renewed idea 648 failure without a fresh owner
  classification.
- Preserve the `prepared_frame_slot_address_call_argument_offset(...)`
  fail-closed checks; do not bypass them with source-register or stack-offset
  assumptions.
- Do not reopen idea 656 local-memory policy or string-label pointer admission.
- Do not infer frame-slot address materialization from source spelling, stack
  offsets, final assembly, testcase identity, or diagnostic text.
- Do not special-case `src/20000722-1.c`, `%lv._clit_`, `foo`, `bar`, `s1`,
  `s2`, or `a0`.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.

## Proof

Ran the delegated proof:

`bash -lc 'set -o pipefail; mkdir -p build/agent_state/648_step4_representative_integration && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_(dump|codegen_route|cli)_riscv64_call_arg_local_frame_address_materialization$" && build/c4cll --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000722-1.c -o build/agent_state/648_step4_representative_integration/20000722-1.o && llvm-objdump -d build/agent_state/648_step4_representative_integration/20000722-1.o > build/agent_state/648_step4_representative_integration/20000722-1.objdump.txt && rg -n "<bar>|<foo>|mv\s+a0|addi\s+a0,\s*sp|jalr|auipc" build/agent_state/648_step4_representative_integration/20000722-1.objdump.txt; } 2>&1 | tee test_after.log'`

Result: passed. The build was up to date, all three focused tests passed, and
representative RV64 object emission plus objdump completed:
`backend_dump_riscv64_call_arg_local_frame_address_materialization`,
`backend_codegen_route_riscv64_call_arg_local_frame_address_materialization`,
and `backend_cli_riscv64_call_arg_local_frame_address_materialization`.
Proof log: `test_after.log`. Representative artifacts:
`build/agent_state/648_step4_representative_integration/20000722-1.o`,
`build/agent_state/648_step4_representative_integration/20000722-1.objdump.txt`,
and `build/agent_state/648_step4_representative_integration/summary.md`.
