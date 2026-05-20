Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select The Next Focused Owner

# Current Packet

## Just Finished

Step 3: Select The Next Focused Owner gathered evidence for the proposed
scalar integer/FP runtime-value group from the existing post-338
`test_after.log`, generated AArch64 assembly, and prepared-BIR dumps. No
implementation, test, expectation, plan, idea, review, runner, registration,
or proof-log files were changed.

Conclusion: reject a combined scalar integer/FP runtime-value owner as too
broad for the next focused split. The representative cases are all runtime
residuals, but the first bad facts do not collapse to one focused owner:

- Local scalar storage/writeback defect: `00086` prepared BIR contains
  `store_local x = 0`, `load_local`, `add`, `trunc`, `store_local`, then
  `load_local` and compare, but `/tmp/c4c_00086.s` allocates frame size 0,
  emits `ldrh w13, [sp]`, computes `add w20, w13, #1`, never stores the
  incremented short, then reloads `[sp]` for the compare. Prepared addressing
  also reports the `%lv.x` frame-slot accesses with `size=0 align=1` even
  though the object is `i16 size=0 align=2`, which points at stack/local
  object sizing and writeback lowering rather than scalar arithmetic alone.
- Compound/local scalar assignment defect: `00111` exits 1 and
  `/tmp/c4c_00111.s` stores only the `long l = 1`, reads `short s` from
  `[sp,#8]` before initializing it, computes `sub x21, x20, x13`, never writes
  the result back to `s`, then returns a reload of the original short slot.
  This is adjacent to the local writeback owner above, but still integer
  storage/materialization rather than FP.
- FP comparison lowering defect: `00119` and `00123` prepared BIR both contain
  `load_global double @x`, `sitofp i32 1 to double`, `slt double`, and
  `ret i32 %t3`. Their assembly loads `d13` from global `x` and then returns
  `mov x0, x13` without materializing the `x < 1` comparison result. The
  prepared value locations show suspicious pre-comparison moves from both FP
  operands into the GPR result value `%t3`, so this is an FPR-to-GPR compare
  result/return-lowering problem, not the same first bad fact as `00086`.
- Conditional pointer/null value defect: `00112` lowers to just
  `mov x0, x13; ret` for `"abc" == (void *)0`, leaving the result
  unmaterialized. `00144` exits 64 and its assembly repeatedly reads
  uninitialized local `i`/pointer slots, carries values through `x13`/`x20`
  without stable stores, and returns `x20`. This overlaps conditionals and
  pointer-null conversions, not only scalar integer arithmetic.
- FP expression/call defect: `00174` prepared BIR contains the expected double
  `printf` calls, FP comparisons, `sitofp`, and `sin`. The assembly has no
  visible `fadd`, `fsub`, `fmul`, `fdiv`, or `fcmp` in the probed output;
  it repeatedly passes stale `d13`/stack values to `printf`, and the runtime
  output is zeroes plus corrupt comparison rows. This is a separate FP
  arithmetic/comparison/call-argument materialization owner.
- Return-value materialization/spill defect: `00200` prints `0 test(s)
  failed` but exits 122. Prepared BIR ends with `nfailed != 0` and
  `ret i32 %t501`; prepared lowering records `return_stack_to_register`.
  The assembly computes `cset w9, ne`, then ignores that result and loads
  `x0` from `[sp,#908]` before returning. This is a scalar return-spill/ABI
  materialization problem and can be proven without taking all integer/FP
  expression lowering with it.

Focused split recommendation: do not create one owner covering
`00086`, `00111`, `00112`, `00119`, `00123`, `00144`, `00174`, and `00200`.
The best next owner candidate from this evidence is narrower:
`AArch64 scalar local storage/writeback sizing for non-address-exposed scalar
locals`, initially scoped to `00086` and `00111`.

Proposed scope if the supervisor asks plan-owner to split:

- Own fixing prepared stack-object/access sizes and machine lowering for
  scalar local `store_local`/`load_local` writeback where frame size is
  currently 0 or an uninitialized stack slot is read.
- Include `i16` initialization, promotion/truncation, and compound assignment
  writeback through local slots.
- Non-goals: FP arithmetic/comparisons/call args (`00119`, `00123`, `00174`),
  pointer-null conditional conversions (`00112`, `00144`), broad return-spill
  ABI (`00200`), aggregate/address owners, timeouts, and expectation changes.
- Proof ladder: first generated assembly for `00086` must show an initialized
  short slot or register-resident equivalent with the incremented value used
  by the compare; then `00111` must show initialized `s`, `s -= l` writeback,
  and return of the updated short; then run the narrow matching
  `c_testsuite_aarch64_backend_src_00086_c|00111_c` subset only after code
  changes.

## Suggested Next

Supervisor should review this evidence and, if accepted, route plan-owner to
create a narrower split for scalar local storage/writeback around `00086` and
`00111`. Keep the FP compare/expression cases and return-spill case parked
for separate owner selection.

## Watchouts

- Do not implement under the umbrella inventory plan.
- Do not combine FP compare/expression lowering with integer local writeback
  just because the failures share runtime surfaces.
- Do not treat `00200` as proof for local storage/writeback: it has a cleaner
  return-spill/materialization signature after a correct printed nfailed
  value.
- Do not treat `00112`/`00144` as the first local-scalar owner unless fresh
  evidence narrows the conditionals and pointer-null conversions to the same
  storage-size/writeback defect.
- `00086` prepared addressing reports `size=0` for `i16` local accesses, so
  any implementation packet should check whether zero-sized scalar local
  objects are the root cause before patching individual machine op emissions.

## Proof

Used the existing Step 1 `test_after.log` and focused non-mutating probes only;
no broad CTest was rerun and `test_after.log` was not rewritten.

```sh
rg -n "(00086|00111|00112|00119|00123|00144|00174|00200)" test_after.log
./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00086.c -o /tmp/c4c_00086.s
./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00111.c -o /tmp/c4c_00111.s
./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00112.c -o /tmp/c4c_00112.s
./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00119.c -o /tmp/c4c_00119.s
./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00123.c -o /tmp/c4c_00123.s
./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00144.c -o /tmp/c4c_00144.s
./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00174.c -o /tmp/c4c_00174.s
./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00200.c -o /tmp/c4c_00200.s
./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00086.c > /tmp/c4c_00086.prepared.txt
./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00119.c > /tmp/c4c_00119.prepared.txt
./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00174.c > /tmp/c4c_00174.prepared.txt
./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00200.c > /tmp/c4c_00200.prepared.txt
```

Existing proof log path: `test_after.log`. Probe artifacts are under `/tmp`
and are non-canonical evidence only.
