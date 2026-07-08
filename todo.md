Status: Active
Source Idea Path: ideas/open/604_bir_local_memory_gep_address_semantics.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Local-Memory GEP Address Production

# Current Packet

## Just Finished

Completed the remaining Step 3 subcase from `plan.md`: repaired nested multidimensional dynamic local aggregate GEP production for the selected `x[i][k][j][l]` shape without adding a row-specific shortcut.

Implementation:
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`: when a local-slot-backed pointer-address fact names an array whose elements are aggregate types, a dynamic GEP now publishes the next aggregate pointer-address fact by scaling the lowered index by the aggregate element size and preserving the established local-slot provenance.
- This combines successive dynamic local aggregate indices through BIR pointer-address facts, so the chain can progress from `x[i]` to `x[i][k]` to `x[i][k][j]` before the existing scalar dynamic store/load handling owns `x[i][k][j][l]`.

Representative movement:
- `src/930614-2.c`: `--dump-bir --target riscv64-linux-gnu` now succeeds through semantic BIR for the nested multidimensional dynamic local aggregate shape.
- `src/pr24851.c`: remains successful through semantic BIR from the accepted direct local-object repair.
- `src/pr80421.c`: remains past the GEP producer boundary and at its downstream `load local-memory semantic family` owner.

Guard result:
- The delegated pointer/formal, global/static, runtime/string, aggregate/member/flexible/alias, variadic, and pointer local-memory consumer guard rows stayed green under the selected CTest subset.

## Suggested Next

Move to Step 4 breadth proof for the local-memory GEP/address family using the supervisor-selected broader same-family subset.

## Watchouts

- Keep BIR local-memory GEP/address production separate from RV64 frame-slot consumption and target-side pointer authority.
- Do not broaden unsupported pointer arithmetic policy under this idea.
- `src/pr80421.c` is no longer a GEP producer stop; its current `load local-memory semantic family` stop should not be folded back into this GEP producer packet without supervisor approval.
- The new aggregate-array continuation is intentionally constrained to local-slot-backed pointer-address provenance; pointer/formal, global/static, and broad runtime pointer arithmetic remain outside this Step 3 repair.

## Proof

Ran the supervisor-delegated proof command:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(llvm_gcc_c_torture_src_pr80421_c|llvm_gcc_c_torture_src_930614_2_c|llvm_gcc_c_torture_src_pr24851_c|llvm_gcc_c_torture_src_pr44468_c|llvm_gcc_c_torture_src_pr65956_c|llvm_gcc_c_torture_src_pr58209_c|llvm_gcc_c_torture_src_20000717_4_c|llvm_gcc_c_torture_src_20031214_1_c|llvm_gcc_c_torture_src_20080424_1_c|llvm_gcc_c_torture_src_memcpy_2_c|llvm_gcc_c_torture_src_strlen_1_c|llvm_gcc_c_torture_src_20051113_1_c|llvm_gcc_c_torture_src_20100430_1_c|llvm_gcc_c_torture_src_pta_field_1_c|llvm_gcc_c_torture_src_va_arg_22_c|llvm_gcc_c_torture_src_20000722_1_c)$' >> test_after.log 2>&1
```

Result: build passed; CTest passed `16/16`; proof log is `test_after.log`.

Additional local observation before the delegated proof:
- Fresh BIR dump for `src/930614-2.c` succeeded and shows successive dynamic pointer-address adds for the nested `i`, `k`, `j`, and `l` projections.

Supervisor follow-up verification:
- Added `backend_lir_to_bir_notes` to the focused CTest subset; result passed `17/17`.
- Broader backend bucket passed: `ctest --test-dir build -j --output-on-failure -R '^backend_'` reported `346/346` passing.
