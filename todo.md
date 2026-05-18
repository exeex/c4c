Status: Active
Source Idea Path: ideas/open/294_aarch64_pointer_derived_address_lvalue_lowering_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair Core Pointer-Derived Lvalue Address Publication

# Current Packet

## Just Finished

Step 2 follow-up probe for local subobject consumers and local address-valued
call arguments is incomplete. I traced `00130.c` and `00180.c` with the
AST-backed symbol inventory plus generated BIR/prepared-BIR/assembly, then ran
the delegated proof. No code change was kept because the attempted downstream
`addressing.cpp` publication did not change BIR or assembly and produced no
strict pass-count improvement.

Generated evidence: `00130.c` semantic BIR has the right local subobject load
shape, `%t32 = bir.load_local i8 %t32.addr, addr %lv.arr.4+3`, and prepared
addressing maps it to frame slot `#2` offset `0` (`arr.7` at final `sp + 2`),
but AArch64 assembly still emits `ldr w20, [sp, #2]` for an i8 load before the
sext/compare path, so the scalar subobject consumer reads a word-sized stale
value instead of the byte value `2`.

Generated evidence: `00180.c` LLVM has `%t5 = getelementptr i8, ptr %lv.a,
i64 %t4`, but semantic BIR still reaches the call as `%t6 = bir.call i32
printf(ptr @.str1, ptr %t5)` with no `%t5` address-producing definition.
Prepared call planning therefore treats `%t5` as `source_encoding=frame_slot`
at stack `+16`, and assembly emits `ldr w1, [sp, #16]` instead of publishing
`sp + 1` to `x1`.

## Suggested Next

Split the next packet at the missing facts: either extend the allowed files to
the direct call lowering surface that can publish nonzero local pointer
arguments before BIR call construction, or take a narrow AArch64 memory-printer
packet for i8 frame-slot loads so `00130.c` can consume the already-prepared
subobject address with `ldrb`/equivalent byte-width load semantics.

## Watchouts

- Do not repair this by naming the four tests or rewriting expectations. The
  address fact must become an authoritative value/materialization that ordinary
  load/store/call consumers can use.
- Do not reintroduce common semantic BIR pointer publications for x86; the
  regression subset is specifically guarding `%t0 = bir.load_local ptr %lv.p`
  and related load-local forms.
- `00130.c` is not blocked on address discovery anymore; it is blocked on the
  local i8 load consumer using a word load from the prepared frame-slot address.
- `00180.c` is blocked before AArch64 call-boundary consumption: `%t5` has no
  BIR definition, so prealloc/call lowering cannot distinguish it from a stale
  spill slot without an upstream call-argument address fact.
- Keep `00217.c` separated for a global/runtime pointer packet; it still needs
  `PointerAddress` propagation through load-compute-store and likely should not
  be widened into the local frame-slot patch.

## Proof

Ran the delegated proof:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00032|00130|00180|00217)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

`test_after.log` records no strict pass-count improvement over
`test_before.log`: `00032.c` still passes, while `00130.c`, `00180.c`, and
`00217.c` still fail. `00217.c` remains separated for the later global/runtime
pointer packet. `git diff --check` passed. Stale-process check found no stale
`ctest`, `c4cll`, `qemu`, or build/test process beyond the check command
itself.
