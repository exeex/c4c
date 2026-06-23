Status: Active
Source Idea Path: ideas/open/318_rv64_byval_aggregate_call_abi.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Byval Aggregate ABI Evidence

# Current Packet

## Just Finished

Completed idea 318 Step 1 by normalizing fresh evidence for the
`src/00140.c` RV64 byval aggregate call ABI residual.

Probe artifacts:

- `build/rv64_c_testsuite_probe_latest/triage_318_step1/probe_results.tsv`
- `build/rv64_c_testsuite_probe_latest/triage_318_step1/summary.md`
- `build/rv64_c_testsuite_probe_latest/triage_318_step1/bir/src_00140_c.bir.txt`
- `build/rv64_c_testsuite_probe_latest/triage_318_step1/prepared_bir/src_00140_c.prepared_bir.txt`
- `build/rv64_c_testsuite_probe_latest/triage_318_step1/asm/src_00140_c.s`
- `build/rv64_c_testsuite_probe_latest/triage_318_step1/bin/src_00140_c.bin`
- `build/rv64_c_testsuite_probe_latest/triage_318_step1/logs/src_00140_c.*`

Probe status:

- BIR dump: 0
- prepared-BIR dump: 0
- RV64 asm emit: 0
- clang link: 0
- qemu: 132 (`Illegal instruction`)
- classification: `qemu-failure`

First bad mechanism: callee-side byval aggregate parameter access is prepared,
but RV64 emission has no working lowering for the byval parameter pointer/home
copy path. Prepared-BIR starts `f1` as
`bir.func @f1(ptr byval(size=32, align=8) %p.f, ptr %p.p, i32 %p.n) -> i32`
and immediately emits copy loads from `addr %p.f`, including:

- `%lv.param.p.f.aggregate.param.copy.0 = bir.load_local i32 ... addr %p.f`
- `%lv.param.p.f.aggregate.param.copy.16 = bir.load_local ptr ... addr %p.f+16`
- `%lv.param.p.f.aggregate.param.copy.24 = bir.load_local float ... addr %p.f+24`
- `%lv.param.f.byval.copy.0 = bir.load_local i32 ... addr %p.f`

Prepared locations show `%p.f` as `home %p.f value_id=0 kind=stack_slot
slot_id=0 offset=0`, with stack object `source_kind=byval_param type=ptr
size=32 align=8`. Prepared addressing records `base=pointer_value ...
pointer=%p.f` accesses, but emitted assembly truncates after:

```asm
.globl f1
f1:
.Lf1_entry:
    addi sp, sp, -160
.globl main
main:
.Lmain_entry:
    addi sp, sp, -48
    sd ra, 40(sp)
    li t1, 1
    sw t1, 4(sp)
    li t1, 1
    sw t1, 0(sp)
```

There is no emitted callee byval payload load, no `f1` branch/return body, and
no emitted call instruction in `main`. Caller-side prepared evidence still
matters: callsite arg0 is `bank=aggregate_address from=register:s1 to=none`
with before-call `destination_storage=stack_slot abi_index=0`, so Step 2
should cover caller payload transport as well as callee byval home loads.

## Suggested Next

Add focused expected-repair backend coverage for a fixed-arity byval aggregate
call that proves caller payload transport plus callee byval home loads. Include
a nearby integer/pointer/float-lane aggregate case, or split float-lane ABI
coverage if it proves to be a separate route. Keep the variadic byval call from
`src/00140.c` as optional follow-up until the fixed-arity byval path is
represented.

## Watchouts

- Do not special-case `src/00140.c`, `%p.f`, field names, or fixed stack-slot
  homes.
- Do not fake callee-local aggregate values; prove caller-side payload
  transport across the call boundary.
- Keep aggregate-local, function-pointer, external-call, and select/control
  repairs out of scope unless evidence proves a true dependency.
- The current failure is evidence-only; do not claim capability progress until
  focused Step 2 tests are added and a later implementation repairs the
  byval ABI path.

## Proof

Probe proof:
`src/00140.c` BIR dump, prepared-BIR dump, RV64 asm emission, clang link, and
qemu were captured under
`build/rv64_c_testsuite_probe_latest/triage_318_step1/`; see
`probe_results.tsv` and `summary.md`.

Delegated proof ran:
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The
fresh proof log is `test_after.log`.
