Status: Active
Source Idea Path: ideas/open/366_aarch64_string_literal_pointer_null_comparison.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Pointer Comparison Publication Gap

# Current Packet

## Just Finished

Step 1 localized the `00112` runtime failure.

`tests/c/external/c-testsuite/src/00112.c` is:

```c
int
main()
{
    return "abc" == (void *)0;
}
```

The generated AArch64 file `build/c_testsuite_aarch64_backend/src/00112.c.s`
contains only the stale return publication plus the rodata object:

```asm
main:
    mov x0, x13
    ret
...
.str0:
    .byte 97, 98, 99, 0
```

There is no `adrp`/`add` materialization of `.str0`, no `cmp`, and no `cset`.

Semantic BIR still has the intended comparison shape:

```text
bir.func @main() -> i32 {
entry:
  %t2 = bir.eq ptr %t0, 0
  bir.ret i32 %t2
}
```

Prepared BIR preserves the same shape and assigns homes, but has no prepared
addressing record for the string-literal pointer:

```text
storage %t2 register bank=gpr placement=gpr:caller_saved#0/w1 reg=x13
storage %t0 register bank=gpr placement=gpr:callee_saved#0/w1 reg=x20
...
prepared-addressing:
prepared.func @main frame_size=0 frame_alignment=1
```

The first missing backend boundary is therefore direct string-literal pointer
address materialization/publication for a scalar comparison operand. `.str0`
rodata emission is present, but no prepared materialization publishes the
address into `%t0`/`x20`, so AArch64 compare lowering cannot materialize the
left-hand side of `bir.eq ptr %t0, 0`.

The null-comparison and boolean-result publication helpers are downstream
rather than the first bad boundary: `lower_scalar_compare_publication` already
has pointer register views, maps equality to `eq`, can use the immediate null
right-hand side, and would emit `cmp` plus `cset` once the named pointer operand
is available. Return publication is also downstream: the before-return move
publishes stale `%t2`/`x13` to `x0` because `%t2` was never defined by compare
publication.

Idea 356 is unrelated. That idea owns dynamic pointer-derived byte loads from
string storage; `00112` has no byte load and no pointer-derived memory access,
only a direct string-literal pointer value compared with null.

## Suggested Next

Repair should focus on the general path that makes direct string-literal
pointer values usable as scalar comparison operands, most likely by producing a
prepared AArch64 string-constant address materialization for the named pointer
value or by extending the scalar compare operand materialization path to request
that record.

Focused coverage should use a semantic shape like direct string-literal pointer
compared against null and assert a defined boolean result path without relying
on exact `.str0`, `x13`, or `x20` spellings. Useful checks are: a prepared
addressing/materialization record exists for the string literal pointer operand
or equivalent compare-local materialization is emitted; generated AArch64
contains address materialization for the string constant, a pointer compare
against null, and a boolean result publication; the final return consumes that
defined boolean value rather than an uninitialized home.

## Watchouts

Keep this distinct from idea 356 dynamic pointer-derived byte loads. The owner
must be direct string-literal pointer/null comparison result publication, not
a testcase-shaped `.str0` or `x13` patch.

## Proof

Ran:

```sh
ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00112_c$' > test_after.log 2>&1
```

Result: failed as expected for localization. `test_after.log` reports
`RUNTIME_NONZERO` for `tests/c/external/c-testsuite/src/00112.c` with
`exit=32`.
