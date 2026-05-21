Status: Active
Source Idea Path: ideas/open/365_aarch64_string_literal_pointer_value_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Pointer-Value Publication Boundary

# Current Packet

## Just Finished

Plan Step 1 localized the first bad pointer-value publication boundary without
implementation edits.

For `tests/c/external/c-testsuite/src/00173.c`, the LLVM route still has the
expected literal pointer value:
`%t2 = getelementptr [6 x i8], ptr @.str0, i64 0, i64 0`, followed by
`store ptr %t2, ptr %lv.a`. The semantic BIR boundary has already lost the
`.str0` producer fact: `bir.store_local %lv.a, ptr %t2` appears with no
corresponding `%t2` address-materialization/global-symbol definition.

Prepared BIR then records the local pointer store as ordinary frame-slot
storage, not string/global address publication:
`access block=entry inst_index=4 base=frame_slot stored=%t2 frame_slot=#11`.
The producer carrier is `%t2`, value id 2, classified as
`storage %t2 ... spill_slot=slot#26+stack72` / `home %t2 ... offset=72`, and
the first call preserves it through `route=stack_slot spill_slot=slot#26+stack72`.
The generated AArch64 publishes that stack-spill address into `%lv.a`:
`add x9, sp, #72; str x9, [sp, #16]`. The expected store was the `.str0`
address via `adrp/add ... .str0`; `.str0` is emitted in `.rodata` but never
materialized before the local pointer store.

The first runtime consumer is `printf("%s\n", a)`: generated code reloads
`a` from `[sp, #16]`, moves it to `x1`, and calls `printf`, so the consumer
receives `sp+72` instead of `.str0`. Later dynamic byte consumers still remain
dynamic pointer-derived loads, for example prepared BIR records `*a`, `*b`,
and `*src` as `base=pointer_value` accesses (`pointer=%t6`, `%t10`, `%t29`);
this keeps idea 356's dynamic pointer-derived load behavior distinct from this
pointer-address publication fault.

Nearby shape check: `tests/c/external/gcc_torture/src/ptr-arith-1.c` has
`char *str = "abcdefghijkl"`; LLVM has `%t0 = getelementptr ... @.str0` then
`store ptr %t0, ptr %lv.str`, while prepared BIR similarly records only
`access block=entry inst_index=0 base=frame_slot stored=%t0 frame_slot=#0`
with no string address materialization for the local pointer store. Its
generated AArch64 stores `x20` to `[sp]` before any `.str0` materialization.

Owner classification: semantic BIR string/global pointer-value
materialization/provenance is the first bad boundary. Prepared stack layout
and AArch64 codegen already have address-materialization machinery for direct
string/global call arguments and frame-slot addresses, but the `%t2` local
pointer-store producer arrives without the `.str0` identity needed to publish
the literal/global address as a pointer value.

## Suggested Next

Execute Plan Step 2: add focused coverage for `char *p = "literal"` or an
equivalent global-data pointer flowing into a local pointer and then into a
dynamic pointer consumer. The test should fail on the current frame-slot
publication and also assert that the later byte load remains a
`PointerValue`/dynamic load rather than becoming a fixed `.str0` byte load.

## Watchouts

- Do not special-case `00173`, `.str0`, one literal, one stack slot, one stack
  offset, one ABI register, one loop body, or one emitted instruction
  sequence.
- Preserve idea 356's repaired dynamic pointer-derived byte-load behavior.
- Keep direct fixed global/string byte loads distinct from local pointer
  address publication.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.
- The repair route should carry string/global identity through the local
  pointer-store producer, not teach consumers to rediscover one named literal
  from a later load.

## Proof

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00173_c)$' | tee test_after.log`

Result: build was up to date; `backend_lir_to_bir_notes`,
`backend_aarch64_instruction_dispatch`, and
`backend_aarch64_memory_operand_contract` passed. The representative
`c_testsuite_aarch64_backend_src_00173_c` still fails with
`[RUNTIME_NONZERO] ... exit=Segmentation fault`, preserving the current first
bad fact. Proof log: `test_after.log`.
