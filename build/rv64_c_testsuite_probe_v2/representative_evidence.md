# RV64 Undefined-Main Representative Evidence

Captured with:

```sh
cmake --build --preset default
./build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/c-testsuite/src/NNNNN.c
./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/c-testsuite/src/NNNNN.c
./build/c4cll --codegen asm --target riscv64-linux-gnu tests/c/external/c-testsuite/src/NNNNN.c -o build/rv64_c_testsuite_probe_v2/evidence/src_NNNNN_recheck.s
```

The generated artifacts live under `build/rv64_c_testsuite_probe_v2/evidence/`.
All three representatives preserve `main` through semantic BIR and prepared BIR,
but the emitted RV64 assembly file starts and ends with only:

```asm
    .text
```

That means the later c-testsuite linker diagnostic is an assembly-emission
failure mechanism, not a frontend or prepared-BIR loss of `main`.

## src/00024.c

Source shape: global aggregate storage plus direct field stores and loads:

```c
typedef struct { int x; int y; } s;
s v;
int main() { v.x = 1; v.y = 2; return 3 - v.x - v.y; }
```

Semantic BIR contains `main`:

```text
bir.func @main() -> i32 {
entry:
  bir.store_global @v, i32 1
  bir.store_global @v, offset 4, i32 2
  %t3 = bir.load_global i32 @v
  %t4 = bir.sub i32 3, %t3
  %t6 = bir.load_global i32 @v, offset 4
  %t7 = bir.sub i32 %t4, %t6
  bir.ret i32 %t7
}
```

Prepared BIR also contains `main`. The prepared addressing section records
global-symbol accesses to `v` at offsets `0` and `4` for both stores and loads:

```text
access block=entry inst_index=0 base=global_symbol symbol=v offset=0 size=4 align=4
access block=entry inst_index=1 base=global_symbol symbol=v offset=4 size=4 align=4
access block=entry inst_index=2 base=global_symbol result=%t3 symbol=v offset=0 size=4 align=4
access block=entry inst_index=4 base=global_symbol result=%t6 symbol=v offset=4 size=4 align=4
```

Observed reason this lands in the undefined-main bucket: the RV64 route has
prepared global storage/addressing authority for `v`, but the final assembly
printer emits no `.globl main`, label, body, or global-data materialization;
the rechecked asm head is only `.text`.

## src/00025.c

Source shape: local pointer initialized from a string literal, then passed to
an external `strlen` call:

```c
int strlen(char *);
int main() { char *p; p = "hello"; return strlen(p) - 5; }
```

Semantic BIR contains the external declaration and `main`:

```text
bir.func @strlen(ptr %p._anon_param_) -> i32

bir.func @main() -> i32 {
entry:
  bir.store_local %lv.p, ptr %t0
  @.str0 = bir.load_local ptr %lv.p, addr .str0
  bir.store_local %lv.p, ptr @.str0
  %t1 = bir.load_local ptr %lv.p
  %t2 = bir.call i32 strlen(ptr %t1)
  %t3 = bir.sub i32 %t2, 5
  bir.ret i32 %t3
}
```

Prepared BIR also contains `main`. It records the direct external call plan for
`strlen`, a local stack slot for `p`, and string-constant addressing for
`.str0`. The notes also flag the current string symbol metadata gap:

```text
[stack_layout] prepared address materialization for '@.str0' is missing a structured symbol LinkNameId
call block_index=0 inst_index=4 wrapper_kind=direct_extern_fixed_arity callee=strlen
access block=entry inst_index=1 base=string_constant result=@.str0 symbol=.str0 offset=0 size=8 align=8
address_materialization block=entry inst_index=1 kind=string_constant result=@.str0 text=.str0 policy=unspecified
```

Observed reason this lands in the undefined-main bucket: the route reaches
prepared call/storage/addressing metadata, including the string address
materialization warning, but RV64 assembly emission still emits only `.text`.
The missing `main` symbol is therefore downstream of prepared BIR and likely
coupled to incomplete RV64 function/global address materialization.

## src/00094.c

Source shape: an unused external global declaration plus a trivial `main`:

```c
extern int x;
int main() { return 0; }
```

Semantic BIR contains `main`:

```text
bir.func @main() -> i32 {
entry:
  bir.ret i32 0
}
```

Prepared BIR also contains `main`:

```text
prepared.summary @main stable_base=rsp frame_size=0 frame_alignment=1 has_dynamic_stack=no saved_regs=0 calls=0 dynamic_stack_ops=0 variadic_entry=no storage_values=0
prepared.func @main
  block entry terminator=return
```

The prepared storage and addressing sections are intentionally empty for this
case because the `extern int x` declaration is unused by `main`.

Observed reason this lands in the undefined-main bucket: even the no-storage,
no-addressing prepared function lowers to an assembly file containing only
`.text`. This makes `src/00094.c` the control case showing that the undefined
`main` failure is not restricted to global accesses or string constants; those
features expose additional missing metadata, but the RV64 asm route is already
dropping a prepared trivial function body.

## Summary

| Case | Source shape | Semantic BIR has `main` | Prepared BIR has `main` | Asm head | Relevant prepared evidence |
| --- | --- | --- | --- | --- | --- |
| `src/00024.c` | global struct `v` field stores/loads | yes | yes | `.text` only | `global_symbol` accesses to `v` offsets `0` and `4` |
| `src/00025.c` | string literal address plus `strlen` call | yes | yes | `.text` only | direct extern call plan, local slot, string address materialization warning |
| `src/00094.c` | unused extern plus trivial return | yes | yes | `.text` only | no storage/addressing, proving the body drop also hits trivial prepared functions |

Next implementation packet should start at RV64 asm emission from prepared BIR:
why selected prepared functions with valid control-flow summaries are not
printed as `.globl main`, `main:`, and executable body text. Global and string
address materialization should be treated as follow-up hazards, not as the sole
root cause, because `src/00094.c` fails without those features.
