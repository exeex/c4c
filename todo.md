Status: Active
Source Idea Path: ideas/open/355_aarch64_address_valued_memory_call_argument_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Value-Kind Boundary

# Current Packet

## Just Finished

Step 1 localized the first repair boundary for idea 355 to AArch64 publication
of prepared `pointer_value` memory accesses and their downstream ABI consumers.
The smallest failing representative is `00020`.

`00020` prepared BIR is already semantically correct:

- producer chain: `%t0 = bir.load_local ptr %lv.pp`, then
  `%t1 = bir.load_local ptr %t1.addr, addr %t0`, then
  `%t2 = bir.load_local i32 %t2.addr, addr %t1`
- prepared homes: `%t0` in `x21`, `%t1` in stack slot `stack+40`, `%t2`
  in `x21`
- prepared addressing: inst 4 is `base=pointer_value result=%t1 pointer=%t0`;
  inst 5 is `base=pointer_value result=%t2 pointer=%t1`
- consumer: return move wants value id 4, `%t2`, in `x0`

Generated AArch64 instead performs the first pointee pointer load into `x9`,
stores it to `stack+40`, and then returns `x21`:

```asm
ldr x21, [sp, #16]
ldr x9, [x21]
str x9, [sp, #40]
mov x0, x21
```

The second load, `ldr w?, [x9-or-stack+40]`, is missing, and return
publication uses the intermediate pointer `%t0` rather than the final `i32`
pointee `%t2`. `00103` has the same prepared-BIR shape with `void *foo` /
`void **bar`, so it is adjacent to the same minimal indirect-memory boundary.

Call-argument adjacency:

- `00170` has `deref_uintptr(&epos)`. Prepared call plan for inst 8 records
  `arg0 source_encoding=frame_slot source_value_id=12 source_stack_offset=8`
  and prepared addressing records `address_materialization ... kind=frame_slot
  result=%lv.epos offset=0`. Generated AArch64 emits `ldr x0, [sp, #8]`
  before the call, but no pointer value was stored at `stack+8`; the consumer
  needs the materialized address of the local slot, not a reload from a
  preserved spill slot.
- `00189` has `fprintfptr(stdout, "%d\n", (*f)(24))`. Prepared BIR loads the
  `stdout` global pointer into `%t1` with home `stack+0`, preserves it across
  the nested indirect call, and uses it as call arg0 for the later indirect
  `fprintfptr` call. Generated AArch64 loads `stdout` into `x9`, overwrites
  `x9` while preparing `f`, and later emits `ldr x0, [sp]`; the preserved
  pointer stack home was never published.

Rejected residuals:

- `00005` is out of scope for this packet because it fails before generated
  AArch64 with `FRONTEND_FAIL` in semantic `lir_to_bir`.
- `00173` is adjacent pointer-loop work, but the current first-bad evidence is
  broader loop pointer advancement and it timed out in the delegated proof; it
  should wait until the minimal indirect-memory and call-argument boundaries
  are repaired.
- `00181` remains an adjacent global-array/pointer-recursion case, not the
  smallest repair driver for Step 2.

## Suggested Next

Implement Step 2 against the minimal `00020` / `00103` indirect-memory
boundary: when a prepared access is `base=pointer_value`, make lowering publish
the loaded pointer value and then the final pointee value, so the return
consumer sees `%t2` rather than the intermediate pointer `%t0`.

## Watchouts

- Do not edit expectations, unsupported classifications, runners, timeout
  policy, CTest registration, or proof-log behavior.
- Do not special-case c-testsuite filenames, source function names, stack
  offsets, symbol names, or emitted instruction neighborhoods.
- Keep the two value kinds distinct in Step 2 and Step 3:
  `00020`/`00103` need pointer-value followed by pointee load publication;
  `00170` needs materialize-address for an address-exposed local passed as a
  call argument; `00189` needs publication of a loaded global pointer into its
  preserved stack home before a later call-argument reload.
- Keep scalar compare/select, floating variadic scalar, composite ABI,
  dynamic-stack/goto, and aggregate initializer residuals out of this owner
  unless fresh first-bad-fact evidence justifies a lifecycle split.

## Proof

Delegated proof command:

```sh
ctest --test-dir build -j10 --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00020_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c|c_testsuite_aarch64_backend_src_00005_c|c_testsuite_aarch64_backend_src_00103_c|c_testsuite_aarch64_backend_src_00173_c|c_testsuite_aarch64_backend_src_00181_c|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract)$' | tee test_after.log
```

Result: 4/11 passed, 7/11 non-passing. The backend unit/CLI contracts passed.
The external residuals were `00005` `FRONTEND_FAIL`, `00020`
`RUNTIME_NONZERO exit=152`, `00103` `RUNTIME_NONZERO exit=136`, `00170`
segmentation fault, `00173` timeout, `00181` segmentation fault, and `00189`
runtime mismatch missing the final `42` line.

Proof log: `test_after.log`.
