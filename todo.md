Status: Active
Source Idea Path: ideas/open/517_residual_scalar_f32_f64_cast_object_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Residual Cast Row Evidence

# Current Packet

## Just Finished

Step 1 - Rebuild Residual Cast Row Evidence completed for the three fresh
`unsupported_floating_cast` representatives.

Evidence sources used:

- `build/rv64_gcc_c_torture_backend/src_cvt-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_920618-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_pr66233.c/case.log`
- `./build/c4cll --target riscv64-unknown-linux-gnu --dump-prepared-bir
  tests/c/external/gcc_torture/src/cvt-1.c`
- `./build/c4cll --target riscv64-unknown-linux-gnu --dump-prepared-bir
  tests/c/external/gcc_torture/src/920618-1.c`
- `./build/c4cll --target riscv64-unknown-linux-gnu --dump-prepared-bir
  tests/c/external/gcc_torture/src/pr66233.c`
- `rg -n "unsupported_floating_cast|supports only prepared FPR width casts"
  src/backend/mir/riscv/codegen/object_emission.cpp
  tests/backend/mir/backend_riscv_object_emission_test.cpp`

Current row diagnostic for all three case logs:
`unsupported_floating_cast: RV64 object route supports only prepared FPR width
casts and I32/I64-to-F32/F64 integer-to-floating casts`.

Row evidence:

| Row | Current cast evidence | Source -> destination | Banks and homes | Materialization need | Current rejection site | Classification |
| --- | --- | --- | --- | --- | --- | --- |
| `tests/c/external/gcc_torture/src/cvt-1.c` | Prepared BIR has `bir.fptosi double %p.x to i64` in `g1`, `bir.sitofp i64 %t0 to double`, another `bir.fptosi double %t1 to i64`, `bir.fptosi double %p.f to i64` in `g2`, and several `bir.sitofp i64 ... to double` sites in `f`/`main`. | First unsupported direction is F64 -> I64 signed (`fptosi double` to `i64`). Adjacent accepted-looking direction is I64 -> F64 (`sitofp`). | First `g1` source `%p.x` is FPR register/home `a0`; destination `%t0` is GPR register/home `t0`. Later `g1` `%t1` is FPR `ft0` -> `%t2` GPR `t0`; `g2` `%p.f` FPR `a0` -> `%t0` GPR `t0`. | Register-to-register for the first F64 -> I64 casts. The file also has immediate I64 -> F64 constants in `main`, but the unsupported first owner is FP-to-int, not stack/local-memory movement. | Object emission floating-cast gate in `src/backend/mir/riscv/codegen/object_emission.cpp` reports this diagnostic when neither `fragment_for_prepared_cast(...)` nor `prepared_floating_cast_publication_is_coherent(...)` accepts the cast shape; the diagnostic text is built near the `unsupported_floating_cast` rejection helper. | RV64 object-lowering-owned. Prepared facts provide scalar types, banks, and homes for FP-to-GPR signed conversion; no producer fact gap is visible. |
| `tests/c/external/gcc_torture/src/920618-1.c` | Prepared BIR has `%t0 = bir.fptrunc double 0x3FF199999999999A to float`, then `%t1 = bir.sle float 0x00000000, %t0`. | F64 immediate -> F32 (`fptrunc double` to `float`). | Source is a typed floating immediate with no value home. Destination `%t0` is FPR register/home `ft0`; compare result `%t1` is GPR register/home `t0`. | Needs immediate floating-constant materialization before or as part of the width cast. This is not a raw stack slot or local-memory case. | Same object emission floating-cast gate and `unsupported_floating_cast` diagnostic as above; existing text allows prepared FPR width casts but this row lacks a register source home because the source is an immediate. | RV64 object-lowering-owned if Step 2 explicitly includes typed FP immediate materialization in the scalar-width cast contract; otherwise producer/prepared-fact-owned. Do not infer a hidden source home. |
| `tests/c/external/gcc_torture/src/pr66233.c` | Prepared BIR in `foo` has `%t7 = bir.sitofp i32 %t6 to float` followed by `%t11 = bir.fptoui float %t7 to i32`; `main` has no floating casts. | Accepted-looking I32 -> F32 (`sitofp`) followed by first unsupported F32 -> U32 unsigned (`fptoui float` to `i32`). | `%t6` is GPR register/home `t0`; `%t7` is FPR register/home `ft0`; `%t11` is GPR register/home `s1`. | Register-to-register FPR -> GPR conversion. Surrounding global-store/select values use GPR/frame slots after the cast, but the cast itself does not require stack-frame ownership. | Same object emission floating-cast gate and `unsupported_floating_cast` diagnostic as above; current route supports I32/I64-to-F32/F64 int-to-floating but not FPR-to-integer conversion. | RV64 object-lowering-owned. Prepared facts provide scalar types, banks, and homes for FP-to-GPR unsigned conversion; no producer fact gap is visible. |

One RV64 scalar cast contract can cover the accepted rows if it is stated as a
single scalar F32/F64 object-lowering contract with explicit subcases for
prepared FPR-width casts, integer-to-FPR casts that already exist, and the new
FPR-to-I32/I64 signed/unsigned conversions. `920618-1.c` should be included in
that same contract only if Step 2 makes typed FP-immediate materialization an
explicit authority; otherwise it should split to producer/prepared-fact work
instead of being inferred in RV64 object emission.

## Suggested Next

Delegate Step 2 - Define The Scalar Cast Contract. Use the row table above to
accept only casts with explicit prepared facts or an explicitly authorized FP
immediate materialization rule.

## Watchouts

Keep `920618-1.c` separate during contract definition until immediate FP
materialization authority is explicit. The two FP-to-integer rows have concrete
source/destination homes and banks and should not require producer changes.
Keep F128/helper ABI, local-memory, aggregate/byval, stack-frame,
branch/select, call/return, and `conversion.c` work out of this route.

## Proof

`git diff --check -- todo.md`
