Status: Active
Source Idea Path: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Format-Byte NUL-Test Materialization

# Current Packet

## Just Finished

Step 1 localized the `00204.c` `myprintf` format-loop overread to AArch64
scalar control-value materialization, not to the semantic BIR or prepared
memory-access facts.

AST-backed lookup covered the AArch64 dispatch/operand/control publication
helpers before reading the implementation. Generated evidence for `myprintf`
shows semantic and prepared BIR preserve the correct byte contract:

```text
%t1 = bir.load_local i8 %t1.addr, addr %t0
%t2 = bir.ne i8 %t1, 0
branch_condition for.cond.70 kind=fused_compare condition=%t2 compare=ne i8 %t1, 0
home %t1 value_id=2046 kind=stack_slot slot_id=2911 offset=880
slot #2911 object_id=2881 func=myprintf offset=880 size=1 align=1
access block=for.cond.70 inst_index=1 base=pointer_value result=%t1 pointer=%t0 offset=0 size=1 align=1
```

The first bad final instruction sequence is in
`build/c_testsuite_aarch64_backend/src/00204.c.s` at `myprintf`'s
`for.cond.70` lowering:

```asm
ldr x13, [sp]
ldrb w9, [x13]
strb w9, [sp, #880]
ldr x13, [sp, #880]
cmp w13, #0
```

The byte load and byte spill are correct through `strb`, but the NUL test
materializes `%t1` from its one-byte prepared stack home with a wider `ldr`.
That consumes stale high bytes from the stack slot neighborhood, so the loop
does not stop at `.str50`'s NUL and continues into adjacent `.str51` /
`.str52` text.

Likely backend boundary: `src/backend/mir/aarch64/codegen/alu.cpp`
`make_control_publication_operand` / `materialize_control_register_source`
uses `make_prepared_scalar_load_source` for the stack-homed `%t1`, but
`append_scalar_frame_slot_load` emits a hard-coded `ldr` and ignores the
prepared memory operand's `size_bytes=1` / i8 type. The same helper is used by
control compare publication, so the repair should teach this load helper to
select `ldrb`/`ldrh`/`ldr` consistently with the prepared operand width or
value type instead of relying only on the target register spelling.

## Suggested Next

Add focused AArch64 backend coverage for stack-homed narrow scalar values used
as control/compare operands. The smallest useful case is an i8 load from a
pointer, spill/home to a one-byte prepared frame slot, then a compare against
zero that must materialize with `ldrb` rather than `ldr` before branching.
After that, repair `append_scalar_frame_slot_load` or its call sites so
prepared scalar frame-slot loads preserve byte/halfword width.

## Watchouts

Do not route this packet to HFA, byval aggregate, or aggregate `va_arg`
owners. The failing proof now reaches all six payloads in the second `%7s %9s
...` call:

```text
lmnopqr ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHIHFA long double:
```

That shows the aggregate vararg publication/cursor path has advanced past the
prior residual. The first bad fact is the missing newline/terminator handling
between `.str50` and `.str51`, before the standalone HFA stdarg format is
legitimately reached.

Avoid a testcase-shaped `.str50` or `myprintf` special case. The defect is a
general narrow stack-home reload in AArch64 control publication; similar i1/i8
and i16 stack-homed compare operands should be covered.

## Proof

Ran the delegated Step 1 proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00204_c$' > test_after.log 2>&1
```

Result: build succeeded; `c_testsuite_aarch64_backend_src_00204_c` still
failed. `test_after.log` is the preserved proof log. No implementation files,
expectations, unsupported classifications, runners, `plan.md`, or idea files
were changed.
