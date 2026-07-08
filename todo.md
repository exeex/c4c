Status: Active
Source Idea Path: ideas/open/605_bir_local_memory_alloca_and_scalar_semantics.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair In-Scope BIR Production

# Current Packet

## Just Finished

Step 3 from `plan.md`: repaired the selected alloca-local producer path without adding testcase-shaped matches.

Implementation surface:
- `src/backend/bir/lir_to_bir/aggregate.cpp`: local aggregate scalar leaves preserve legacy `<slot>.<byte_offset>` names when available, and use private `.field.<byte_offset>` names only when the legacy spelling would collide with an existing or hoisted source local object name such as `%lv.x.1`.
- `src/backend/bir/lir_to_bir/aggregate.cpp`: zero-size struct aggregate layouts are accepted as valid local aggregate facts with no scalar leaves; invalid layouts and zero-size scalar leaves still fail closed.
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`: fixed scalar-array alloca element leaves preserve legacy `<array>.<index>` names when available, and use private `.element.<index>` names only when the legacy spelling would collide with an existing or hoisted source local object name such as `%lv.a.1`.
- `src/backend/bir/lir_to_bir/lowering.hpp`: shared local carrier slot-name availability helpers keep aggregate and scalar-array naming on the same legacy-first policy.

Fresh selected-row BIR probes live under `build/agent_state/605_step3_alloca_repair/`:
- `src/20180921-1.c` function `aw`: moved beyond `alloca local-memory semantic family`; current BIR stop is `gep local-memory semantic family`.
- `src/pr38151.c` function `check2848va`: moved beyond `alloca local-memory semantic family`; current BIR stop is `load local-memory semantic family`.
- `src/931004-10.c` function `f`: moved beyond `alloca local-memory semantic family`; current BIR stop is `load local-memory semantic family`.

The in-scope sibling scalar/local reps and guard rows in the delegated proof subset still pass their existing CTest contracts. The repair did not edit expectations, unsupported markers, allowlists, accounting, load/store/GEP lowering, unordered float compare, scalar casts, vector binops, bootstrap/global, prepared/RV64, ABI, runtime, or timeout ownership.

## Suggested Next

Execute Step 4 as a same-family breadth proof for local alloca/aggregate carrier namespace behavior and record which remaining alloca/scalar-local rows are true downstream load/GEP/store/consumer owners versus still in-scope producer gaps.

## Watchouts

The selected alloca reps now expose downstream GEP/load owners. Do not pull those producers into the alloca-local slice. The sibling scalar/local rows were preserved by the delegated proof but were not fixed by this packet.

Keep watching generated local carrier slot names in BIR dumps: source alloca names and compiler-created array/aggregate leaf names now use private spellings only at real collisions, so existing layout-sensitive backend snippets should remain stable.

## Proof

Proof log: `test_after.log`.

Delegated proof command run successfully:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(llvm_gcc_c_torture_src_20180921_1_c|llvm_gcc_c_torture_src_pr38151_c|llvm_gcc_c_torture_src_931004_10_c|llvm_gcc_c_torture_src_20020411_1_c|llvm_gcc_c_torture_src_20041201_1_c|llvm_gcc_c_torture_src_ffs_2_c|llvm_gcc_c_torture_src_pr85169_c|llvm_gcc_c_torture_src_20050604_1_c|llvm_gcc_c_torture_src_20050607_1_c|llvm_gcc_c_torture_src_ieee_compare_fp_1_c|llvm_gcc_c_torture_src_20010605_2_c|llvm_gcc_c_torture_src_pr44468_c|llvm_gcc_c_torture_src_20040302_1_c|llvm_gcc_c_torture_src_20000722_1_c|llvm_gcc_c_torture_src_pr30778_c|llvm_gcc_c_torture_src_20020314_1_c)$' >> test_after.log 2>&1
```

Result: build passed; selected CTest subset passed `16/16`.

Revision-focused backend naming check also passed:

```sh
ctest --test-dir build --output-on-failure -R '^(backend_lir_to_bir_notes|backend_dump_riscv64_prepared_local_array_base_pointer|backend_dump_riscv64_prepared_local_array_subobject_pointer|backend_dump_riscv64_prepared_local_array_pointer_step|backend_dump_riscv64_prepared_local_array_i8_element_access|backend_dump_riscv64_i16_local_array_select_store|backend_dump_riscv64_aggregate_local_self_pointer_chain|backend_dump_riscv64_aggregate_local_anonymous_union_fields|backend_cli_dump_bir_layout_sensitive_aggregate)$'
```

Result: focused backend subset passed `9/9`.

Broader supervisor-style backend check passed:

```sh
ctest --test-dir build --output-on-failure -R '^backend_'
```

Result: backend subset passed `346/346`.
