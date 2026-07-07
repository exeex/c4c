Status: Active
Source Idea Path: ideas/open/583_rv64_pointer_arithmetic_result_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce Pointer Arithmetic Owner

# Current Packet

## Just Finished

Step 1 reproduced the current RV64 pointer arithmetic owner for
`src/20000819-1.c` with fresh build, prepared dump, and object-route evidence.
The object route still stops at `unsupported_pointer_arithmetic` in function
`foo`, block `entry`, block index `0`, instruction index `7`,
`instruction_kind=BinaryInst`, owner `ptr %t4`.

Prepared facts for the first failing instruction:

- Pointer-valued instruction: `%t4 = bir.add ptr %t1, %t4.byte_offset`.
- Pointer base: `%t1 = bir.load_local ptr %lv.param.sp`; prepared home
  `value_id=3`, register `s1`.
- Integer byte-offset source: `%t4.byte_offset = bir.mul i64 %t3, 4`, with
  `%t3 = bir.sub i64 0, %t2`; prepared home `value_id=6`, register `s2`.
- Destination owner/home: `%t4`; prepared home `value_id=7`, register `t0`.
- Publication clue: `store_source function=foo block=entry inst=8 source=%t4
  status=available intent=store_local_publication source_producer=binary
  source_producer_block=entry source_producer_inst=7`.

Code-path clue: the diagnostic is emitted in
`src/backend/mir/riscv/codegen/object_emission.cpp` after the prepared
pointer-result/frame-address/binary fragment helpers decline to consume the
pointer-valued binary instruction.

## Suggested Next

Execute Step 2 from `plan.md`: add focused RV64 object-emission coverage for
prepared pointer-result add/sub materialization and destination publication
from a pointer base plus integer byte offset, preserving precise unsupported
diagnostics for non-lowerable forms.

## Watchouts

- Do not select or mutate deferred `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`.
- Do not use filename-, function-, block-, value-name-, or diagnostic-string-shaped shortcuts for the pointer arithmetic repair.
- Preserve unsupported diagnostics for pointer arithmetic forms outside the prepared pointer-base plus integer byte-offset contract.
- Step 1 also shows a nearby `%t12 = bir.add ptr %t11, 4` in `for.latch.1`,
  but the first object-route owner is `%t4` at entry instruction `7`; Step 2
  coverage should target the semantic contract, not the representative names.

## Proof

`test_after.log` records:

- `cmake --build --preset default --target c4cll`: passed, rc `0`.
- Prepared dump for `tests/c/external/gcc_torture/src/20000819-1.c`: passed,
  rc `0`.
- RV64 object route for the same representative: rc `1`, expected for Step 1
  classification, with current owner `unsupported_pointer_arithmetic`.

Artifacts are under
`build/agent_state/583_rv64_pointer_arithmetic_result_publication/step1/`,
including `summary.md`, per-command `.cmd` files, return-code files,
`dump-prepared-bir.txt`, and `object-route.log`.
