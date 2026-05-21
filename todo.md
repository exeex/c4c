Status: Active
Source Idea Path: ideas/open/356_semantic_bir_pointer_derived_string_loads.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative Progress And Decide Lifecycle

# Current Packet

## Just Finished

Completed Plan Step 4 localization for the new `00173` runtime segmentation
fault after the pointer-carrier reload repair.

New first bad fact:

- Semantic BIR no longer contains the old fixed-global byte loads for dynamic
  `*b` / `*src` consumers; the loop loads are dynamic pointer-value loads such
  as `%t11 = bir.load_local i8 %t11.addr, addr %t10`, `%t30 = bir.load_local
  i8 %t30.addr, addr %t29`, and prepared addressing records them as
  `base=pointer_value`.
- The bad producer is earlier: `char *a = "hello"` lowers to
  `bir.store_local %lv.a, ptr %t2`, but semantic/prepared BIR has no
  `.str0` / string-constant address materialization for `%t2`.
- Prepared BIR allocates `%t2` as a normal frame spill
  (`storage %t2 ... spill_slot=slot#26+stack72`) and records the first
  `printf("%s\n", a)` argument as a register sourced from `%t4`, not as a
  `.str0` symbol address.
- Generated AArch64 materializes `%lv.a` as `add x9, sp, #72; str x9,
  [sp, #16]`, so the runtime pointer value consumed by `printf`, `*a`, `*b`,
  and `*src` is `sp+72` instead of `.str0`.
- Runtime evidence remains `[RUNTIME_NONZERO] exit=Segmentation fault` with
  empty stdout/stderr, consistent with the first `printf("%s\n", a)` reading
  through the invalid stack-spill address before the loop/copy output appears.

Classification: the fixed-global-byte semantic bug remains repaired, and the
new owner is not dynamic pointer-derived load preservation. It is a downstream
address-materialization/publication residual for string literal pointer values:
the address of `.str0` is not published for the pointer carrier `%t2`, so the
now-dynamic pointer-value loads correctly follow a bad pointer.

## Suggested Next

Open or activate a downstream AArch64/prepared-addressing packet for string
literal pointer value publication. The narrow fix should make `%t2` / `char *a
= "hello"` materialize `.str0` as a pointer value before `%lv.a` is stored,
while preserving the repaired dynamic pointer-value loads for `*b` and `*src`.

## Watchouts

- Do not special-case `00173`, one string literal, one global symbol, or one
  loop body.
- Keep direct fixed global/string byte loads distinct from dynamic pointer
  loads.
- Preserve existing dynamic-global-scalar array coverage that intentionally
  materializes global array element candidates with `LoadGlobalInst`; the
  failing owner is pointer-carrier dereference provenance, not every global
  scalar-array access.
- The semantic fixed-global-byte overfit is repaired: prepared addressing for
  `*b`, `*src`, and the destination copy uses `base=pointer_value`, not
  fixed `.str0` global-byte loads.
- The invalid dynamic address is `sp+72`, produced by treating `%t2` as a
  frame-slot value instead of the `.str0` string address; consumers include
  the first `printf("%s\n", a)`, the initial `*a`, the `b` loop loads, and the
  `src` copy loop.
- Do not reopen AArch64 address-valued memory, recursive `00181` publication,
  runner behavior, expectations, timeout policy, or CTest registration.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00173_c)$' | tee test_after.log`

Result: build completed with no work to do; `backend_lir_to_bir_notes`,
`backend_aarch64_instruction_dispatch`, and
`backend_aarch64_memory_operand_contract` passed, confirming the
fixed-global-byte semantic repair still holds. The delegated CTest subset still
reports `c_testsuite_aarch64_backend_src_00173_c` failed with
`[RUNTIME_NONZERO] exit=Segmentation fault` and empty stdout/stderr, matching
the localized bad `.str0` pointer-value publication residual. Proof log:
`test_after.log`.
