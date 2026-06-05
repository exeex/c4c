Status: Active
Source Idea Path: ideas/open/112_aarch64_00216_00204_post_closure_regression.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair The Shared Semantic Defect

# Current Packet

## Just Finished

Step 3 `Repair The Shared Semantic Defect` repaired the remaining AArch64
callee-side aggregate `va_arg` va-list lifetime/source-home rule for HFA
traffic.

Semantic rule repaired: inline aggregate `va_arg` lowering now carries the
stable va-list object home from the matching `va_start` helper and
rematerializes the va-list base register from that home before reading or
updating AAPCS64 va-list fields. This preserves the va-list home even when the
generic call-address materialization for the helper's hidden sret argument uses
the same physical register immediately before the inline helper body. Ordinary
scalar call argument producers are also skipped for inline variadic entry
helpers, because structured `va_start`/`va_arg`/`va_copy` lowering owns those
pseudo-call operand effects.

Evidence: regenerated `/tmp/c4c_00204_after_va_list_home_repair.s` shows the
first aggregate `va_arg` block still receiving the generic `add x21, sp, #24`
address materialization, but the inline variadic lowering immediately
rematerializes `x21` as `sp + 1264` before loading `fp_offset` and the register
save area fields. The focused runtime no longer segfaults.

## Suggested Next

Supervisor review/commit of the combined Step 3 code slice and `todo.md`
progress is the next coherent packet. If more validation is desired before
commit, rerun the same two-test focused proof or escalate to the supervisor's
chosen broader AArch64 backend subset.

## Watchouts

- Do not weaken, skip, or reclassify `00216` or `00204`.
- Do not add filename-shaped or expected-output-shaped special cases.
- The repair is semantic, not testcase-shaped: it links aggregate `va_arg`
  source homes to the matching prepared `va_start` va-list object home and
  does not force HFA variadic arguments into memory.
- The generic hidden-sret address materialization for inline aggregate
  `va_arg` still exists; the variadic helper now treats that as clobber-prone
  and restores the va-list base from the durable va-list object home before
  field access.
- Preserve the earlier outgoing stack argument boundary repair in
  `calls.cpp`/`machine_printer.cpp`; `00204` depends on both the caller-side
  overflow staging order and this callee-side va-list home repair.
- Scratch evidence from this packet is under `/tmp`:
  `/tmp/c4c_00204_myprintf_prepared_after_skip.txt`,
  `/tmp/c4c_00204_after_variadic_producer_skip2.s`, and
  `/tmp/c4c_00204_after_va_list_home_repair.s`.

## Proof

Ran delegated proof:

```sh
cmake --build --preset default && ctest --test-dir build -j1 --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00(216|204)_c)$' > test_after.log 2>&1
```

Result: build passed; focused CTest passed both selected tests:
`c_testsuite_aarch64_backend_src_00216_c` and
`c_testsuite_aarch64_backend_src_00204_c`. Proof log path: `test_after.log`.
