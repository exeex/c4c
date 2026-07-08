Status: Active
Source Idea Path: ideas/open/604_bir_local_memory_gep_address_semantics.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Proof Summary and Closure Readiness

# Current Packet

## Just Finished

Completed Step 5 from `plan.md`: summarized the accepted Step 3 implementation surface and Step 4 breadth proof for lifecycle handoff.

Implementation surface now committed:
- `5f01b8d1b lower direct local-object GEP address shapes`
- `f74dd307d lower nested local aggregate GEP address chains`

The implementation repaired BIR local-memory GEP/address production in `src/backend/bir/lir_to_bir/memory/addressing.cpp` and `src/backend/bir/lir_to_bir/memory/local_gep.cpp` by publishing address/provenance facts for ordinary local-object GEP shapes before prepared/RV64 handoff. It reused the existing pointer/address authority model rather than reconstructing target-side frame addresses or matching named cases.

Accepted proof basis:
- Step 4 proof passed and was rolled forward to `test_before.log`.
- The delegated 16-test RV64 gcc_torture subset passed `16/16`.
- Fresh Step 4 semantic BIR classification over the 62 Step 1 rows is recorded in `build/agent_state/604_step4_gep_breadth/current_rows.tsv` and `build/agent_state/604_step4_gep_breadth/current_counts.tsv`.
- Refreshed counts: `21/62` semantic BIR successes, `34/62` remaining `gep local-memory semantic family` stops, `2/62` downstream `load local-memory semantic family` stops, and `5/62` explicit runtime/intrinsic owner stops.

Acceptance criteria status:
- Satisfied for the active source idea's direct local-memory GEP/address producer route. Multiple selected GEP rows progressed beyond the old BIR producer stop: `src/pr24851.c` and `src/930614-2.c` now succeed through semantic BIR, while `src/pr80421.c` moved to downstream `load local-memory semantic family`.
- Guard rows that require pointer/address architecture discussion, global/static object policy, aggregate/member/flexible/alias handling, runtime/string support, variadic handling, or downstream local-memory load support remain separated by owner.
- The route did not downgrade expectations, broaden unsupported pointer arithmetic policy, infer target-side pointer authority, or add testcase-local shortcuts.

Remaining local-memory GEP/address limitation:
- No broad remaining direct local-object GEP producer subfamily is visible in the refreshed Step 4 evidence. The remaining `34/62` GEP-family rows are concentrated in Step 1 guard/prerequisite groups and should not be pulled into this direct-local route without plan review.

Adjacent owner groups still reporting GEP-family stops:
- Pointer/formal provenance boundary: `19` rows: `src/pr44468.c`, `src/pr48571-1.c`, `src/pr65956.c`, `src/20001027-1.c`, `src/20010116-1.c`, `src/20010910-1.c`, `src/20030717-1.c`, `src/20080604-1.c`, `src/20090113-1.c`, `src/20100708-1.c`, `src/920922-1.c`, `src/990128-1.c`, `src/pr38048-2.c`, `src/pr39339.c`, `src/pr49419.c`, `src/pr52760.c`, `src/pr65401.c`, `src/pr71083.c`, `src/pr86844.c`.
- Global/static object GEP boundary: `2` rows: `src/20080424-1.c`, `src/ieee/copysign2.c`.
- Aggregate/member/flexible/alias boundary: `10` rows: `src/20001203-2.c`, `src/20051113-1.c`, `src/20060412-1.c`, `src/20070824-1.c`, `src/20071120-1.c`, `src/20100430-1.c`, `src/mayalias-2.c`, `src/pr41395-1.c`, `src/pta-field-1.c`, `src/pta-field-2.c`.
- Runtime/string rows still reporting GEP-family stops: `2` rows: `src/20011121-1.c`, `src/strlen-5.c`.
- Variadic boundary: `1` row: `src/va-arg-22.c`.

Adjacent non-GEP handoffs:
- Downstream load-local owner: `2` rows: `src/pr80421.c`, `src/20030928-1.c`.
- Explicit runtime/intrinsic owners: `5` rows: `src/memcpy-2.c`, `src/memset-1.c`, `src/memset-2.c`, `src/memset-3.c`, `src/string-opt-5.c`.

## Suggested Next

Recommend closing idea `604` as acceptance-satisfied for the ordinary local-memory GEP/address producer route. Do not continue this runbook into the remaining `34/62` GEP stops; they are guard/prerequisite groups and need separate lifecycle ownership or plan review before implementation.

## Watchouts

- Treat the `21/62` success count as refreshed semantic BIR dump evidence only; it is not a full backend-object pass count.
- Keep `src/pr80421.c` and `src/20030928-1.c` with downstream `load local-memory` ownership unless the supervisor explicitly opens a consumer packet.
- Do not claim the remaining `34/62` GEP-family stops as direct-local misses. The refreshed evidence preserves pointer/formal, global/static, aggregate/member/flexible/alias, runtime/string, and variadic boundaries.
- A continuation packet against those remaining rows would risk testcase-overfit unless the lifecycle owner first narrows a new source intent around one of those adjacent owner groups.

## Proof

No validation run for this Step 5 packet; it is evidence-only/todo-only.

Accepted proof basis from Step 4 was already passed and rolled forward to `test_before.log`:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(llvm_gcc_c_torture_src_pr80421_c|llvm_gcc_c_torture_src_930614_2_c|llvm_gcc_c_torture_src_pr24851_c|llvm_gcc_c_torture_src_pr44468_c|llvm_gcc_c_torture_src_pr65956_c|llvm_gcc_c_torture_src_pr58209_c|llvm_gcc_c_torture_src_20000717_4_c|llvm_gcc_c_torture_src_20031214_1_c|llvm_gcc_c_torture_src_20080424_1_c|llvm_gcc_c_torture_src_memcpy_2_c|llvm_gcc_c_torture_src_strlen_1_c|llvm_gcc_c_torture_src_20051113_1_c|llvm_gcc_c_torture_src_20100430_1_c|llvm_gcc_c_torture_src_pta_field_1_c|llvm_gcc_c_torture_src_va_arg_22_c|llvm_gcc_c_torture_src_20000722_1_c)$' >> test_after.log 2>&1
```

Result: build passed; CTest passed `16/16`; proof log was rolled forward to `test_before.log`.

Step 4 also reran fresh semantic BIR dumps for all 62 Step 1 rows:

```sh
build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/<row>
```

Result: `21/62` success, `34/62` still `gep local-memory semantic family`, `2/62` downstream `load local-memory semantic family`, and `5/62` explicit runtime/intrinsic owner stops.
