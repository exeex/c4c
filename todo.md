Status: Active
Source Idea Path: ideas/open/356_semantic_bir_pointer_derived_string_loads.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Semantic Dynamic-Load Loss

# Current Packet

## Just Finished

Completed Plan Step 1 evidence capture for dynamic pointer-derived string/byte
loads in `00173`.

First bad semantic boundary:

- Source expression: `*b` in the `for (b = a; *b != 0; b++)` condition/body
  and `*src` / `*src++` in the copy loop in
  `tests/c/external/c-testsuite/src/00173.c`.
- LLVM/LIR source shape: `load i8, ptr %t10`, `load i8, ptr %t19`,
  `load i8, ptr %t22`, `load i8, ptr %t29`, and `load i8, ptr %t35`, where
  those pointer operands are loaded from `%lv.b` or `%lv.src` and should
  represent the current pointer value.
- Pointer carrier: `%lv.a` is initialized from the `@.str0` string address,
  then copied into `%lv.b` and `%lv.src`; loop increments write `%t17` back to
  `%lv.b` and `%t36` back to `%lv.src`.
- Expected dynamic memory base: the current loaded pointer carrier
  `%t10`/`%t19`/`%t22`/`%t29`/`%t35`, or an equivalent pointer-value memory
  address using that current pointer as the base.
- Actual semantic BIR fixed form: `%t11 = bir.load_global i8 @.str0` in
  `for.cond.1`, `%t20` and `%t23 = bir.load_global i8 @.str0, offset 1` in
  `block_1`, and `%t30`/`%t37 = bir.load_global i8 @.str0` in the copy loop.
- Producer path: `lower_memory_gep_inst` records global-backed scalar array
  provenance for string-derived GEPs in `dynamic_global_scalar_arrays_`;
  `lower_memory_load_inst` then reaches `load_dynamic_global_scalar_array_value`
  and materializes every selected byte as `LoadGlobalInst` candidates instead
  of preserving the runtime pointer-value base for pointer-carrier loads.
- Downstream loop consumer: the `for.cond.1` and `block_3` compare chains
  (`sext` then `ne ... 0`) repeatedly consume fixed `@.str0` bytes, and the
  `block_4` store copies `%t37` from `@.str0` through `%t38` into `dest`.
- Owner classification: semantic LIR-to-BIR memory/provenance lowering for
  global-backed pointer-derived scalar byte loads, specifically the boundary
  between global-backed pointer provenance and dynamic scalar-array
  materialization. This is before prepared BIR and AArch64 lowering.

Nearby shape check:

- A local-array dynamic pointer-byte probe keeps the expected dynamic pointer
  base: `char arr[4]; char *p=&arr[0]; p++; return *p;` lowers to
  `bir.load_local i8 %t11.addr, addr %t10`.
- A legitimate fixed global-byte expression such as `"hi"[1]` lowers to
  `bir.load_global i8 @.str0, offset 1`, so fixed global-byte loads remain a
  valid form when the source access is actually constant.
- Existing focused coverage in
  `tests/backend/bir/backend_lir_to_bir_notes_test.cpp` exercises
  dynamic-global-scalar array materialization and currently expects
  `LoadGlobalInst` records for that array-select path; the next repair must
  distinguish that fixed/global-array selection contract from pointer-carrier
  dereferences like `*b` and `*src`.

## Suggested Next

Add focused semantic coverage for a string/global-backed pointer carrier that
is incremented and then dereferenced, asserting the BIR load uses the current
pointer base instead of fixed `bir.load_global @.str0` bytes.

## Watchouts

- Do not special-case `00173`, one string literal, one global symbol, or one
  loop body.
- Keep direct fixed global/string byte loads distinct from dynamic pointer
  loads.
- Preserve existing dynamic-global-scalar array coverage that intentionally
  materializes global array element candidates with `LoadGlobalInst`; the
  failing owner is pointer-carrier dereference provenance, not every global
  scalar-array access.
- Do not reopen AArch64 address-valued memory, recursive `00181` publication,
  runner behavior, expectations, timeout policy, or CTest registration.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00173_c)$' | tee test_after.log`

Result: build was up to date; `backend_aarch64_instruction_dispatch` and
`backend_aarch64_memory_operand_contract` passed; `c_testsuite_aarch64_backend_src_00173_c`
still failed with runtime output containing only `copied string is`.
Proof log: `test_after.log`.
