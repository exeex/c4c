Status: Active
Source Idea Path: ideas/open/377_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit First Instruction Fragment

# Current Packet

## Just Finished

Step 1: Audit First Instruction Fragment completed for
`tests/c/external/gcc_torture/src/20000217-1.c`.

The first rejected ordinary prepared instruction fragment is in `main`, block
`entry`, at the same-module direct `bir.call i16 showbug(ptr %lv.x, ptr %lv.y)`.
The prepared call arguments are local frame address materializations for
`%lv.x` and `%lv.y`, selected from frame slots `#4` and `#5` and passed in ABI
GPRs `a0` and `a1`. The rejected result shape is scalar integer call-result
publication from ABI register `a0` to `%t2`'s stack-slot home: slot `#12`,
stack offset `4`, type `i16`, `late_publication=yes`,
`late_source_register=yes`.

Rejection site: `fragment_for_prepared_instruction()` dispatches the call to
`fragment_for_prepared_call()`. That helper accepts the same-module/direct
wrapper and the local-frame-address GPR arguments, but rejects this result plan
because non-FPR call results currently require register-to-register
publication. The specific diagnostic helper has no call-result case, so the
route falls through to the generic
`unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering`.

Evidence artifacts:
- `build/agent_state/377_step1_instruction_fragment_audit.txt`
- `build/agent_state/377_step1_20000217-1.prepared-bir.txt`
- `build/agent_state/377_step1_20000217-1.codegen-obj.log`
- `build/agent_state/377_step1_diag_probe.log`

## Suggested Next

Implement focused RV64 object lowering for supported scalar integer call-result
publication from an ABI result register to a prepared stack-slot home in
`fragment_for_prepared_call()`, with focused backend tests for the admitted
shape and fail-closed adjacent result shapes.

## Watchouts

- Do not key behavior on testcase names, value ids, frame slots, registers,
  instruction indexes, source syntax, or prepared-BIR text matching.
- Do not reopen idea 375 compare/trunc or idea 376 move-bundle ownership.
- The diagnostic probe was temporary and removed before completion; it only
  identified `main` block index `0`, instruction index `2` as the generic
  rejection owner.
- The next slice should use prepared call-result metadata, not the probed
  indices or this representative's concrete value names.

## Proof

Audit-only packet. Regenerated prepared and object-route evidence under
`build/agent_state/377_step1_*`.

Commands run:

```sh
build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000217-1.c > build/agent_state/377_step1_20000217-1.prepared-bir.txt 2>&1
build/c4cll --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000217-1.c -o build/agent_state/377_step1_20000217-1.o > build/agent_state/377_step1_20000217-1.codegen-obj.log 2>&1
cmake --build build --target c4cll
build/c4cll --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000217-1.c -o build/agent_state/377_step1_diag_probe.o > build/agent_state/377_step1_diag_probe.log 2>&1
cmake --build build --target c4cll
```

The first `--codegen obj` and diagnostic probe commands exited nonzero as
expected for the unsupported object-route shape. No root-level logs were
written.
