Status: Active
Source Idea Path: ideas/open/604_bir_local_memory_gep_address_semantics.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Local-Memory GEP Address Production

# Current Packet

## Just Finished

Partially completed Step 3 from `plan.md`: repaired direct local-object GEP/address production for two selected representative shapes and preserved the delegated guard subset.

Implementation:
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`: dynamic GEP from a derived local scalar-array base now keeps the static local base index as a byte offset instead of rejecting all nonzero bases. This covers derived in-object bases such as `c + 390` and in-object negative subscripts from a known local array element base.
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`: dynamic local aggregate GEP production now publishes explicit pointer-address facts from the local object/slot provenance before prepared/RV64 consumption, and nested dynamic aggregate GEP dispatch lets an existing explicit pointer-address fact take the relative-GEP path instead of forcing the aggregate side table first.

Representative movement:
- `src/pr24851.c`: `--dump-bir --target riscv64-linux-gnu` now succeeds; the local array base `&a[1]` plus `&q[-1]` shape is no longer stopped at the old GEP producer boundary.
- `src/pr80421.c`: the selected row now moves past the old GEP producer boundary and stops later in `load local-memory semantic family`, which is a downstream local-memory consumer boundary rather than the Step 3 GEP producer stop.
- `src/930614-2.c`: still stops in `gep local-memory semantic family` on the nested multidimensional dynamic aggregate shape. The remaining shape is `x[i][k][j][l]`, specifically the second dynamic aggregate projection after the first dynamic local aggregate address is produced.

Guard result:
- The delegated pointer/formal, global/static, runtime/string, aggregate/member/flexible/alias, variadic, and pointer local-memory consumer guard rows stayed green under the selected CTest subset.

## Suggested Next

Continue Step 3 with the remaining nested multidimensional dynamic aggregate producer model for `src/930614-2.c`.

This slice is acceptance-ready as a partial semantic Step 3 repair because more than one selected direct local-object GEP row progressed beyond the old producer stop and the delegated guard subset stayed green. The next subcase should combine multiple loop-carried local aggregate indices into one local address/provenance fact without a testcase-shaped shortcut.

## Watchouts

- Keep BIR local-memory GEP/address production separate from RV64 frame-slot consumption and target-side pointer authority.
- Do not broaden unsupported pointer arithmetic policy under this idea.
- The remaining `930614-2.c` shape needs a general way to combine multiple loop-carried local aggregate indices into one local address/provenance fact. Avoid a row-specific shortcut.
- `src/pr80421.c` is no longer a GEP producer stop; its current `load local-memory semantic family` stop should not be folded back into this GEP producer packet without supervisor approval.

## Proof

Ran the supervisor-delegated proof command:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(llvm_gcc_c_torture_src_pr80421_c|llvm_gcc_c_torture_src_930614_2_c|llvm_gcc_c_torture_src_pr24851_c|llvm_gcc_c_torture_src_pr44468_c|llvm_gcc_c_torture_src_pr65956_c|llvm_gcc_c_torture_src_pr58209_c|llvm_gcc_c_torture_src_20000717_4_c|llvm_gcc_c_torture_src_20031214_1_c|llvm_gcc_c_torture_src_20080424_1_c|llvm_gcc_c_torture_src_memcpy_2_c|llvm_gcc_c_torture_src_strlen_1_c|llvm_gcc_c_torture_src_20051113_1_c|llvm_gcc_c_torture_src_20100430_1_c|llvm_gcc_c_torture_src_pta_field_1_c|llvm_gcc_c_torture_src_va_arg_22_c|llvm_gcc_c_torture_src_20000722_1_c)$' >> test_after.log 2>&1
```

Result: build passed; CTest passed `16/16`; proof log is `test_after.log`.

Supervisor follow-up verification:
- Fresh BIR dump for `src/pr24851.c` succeeded.
- Fresh BIR dump for `src/pr80421.c` moved to `load local-memory semantic family`.
- Fresh BIR dump for `src/930614-2.c` still reports `gep local-memory semantic family`.
- Added `backend_lir_to_bir_notes` to the focused CTest subset; result passed `17/17`.
- Broader backend bucket passed: `ctest --test-dir build -j --output-on-failure -R '^backend_'` reported `346/346` passing.
