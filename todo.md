Status: Active
Source Idea Path: ideas/open/163_backend_semantic_bir_route_alignment.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and Classify the 31 Baseline Failures

# Current Packet

## Just Finished

Step 1 inventory/classification is complete for the 31 backend semantic BIR route failures from `test_baseline.log`, using `test_after.log` for detailed failure context. All failures are `BACKEND_ROUTE_SNIPPET_MISSING`; classification below separates route/harness LLVM output mismatches from actual BIR snippet drift and inline asm constraint-shape drift.

### Failure Inventory

#### Aggregate/byval/function pointer routes

| Test | Name | Observed output | Classification | Notes |
| --- | --- | --- | --- | --- |
| 93 | `backend_codegen_route_x86_64_byval_member_array_params_observe_semantic_bir` | BIR | Stale BIR numbering | Expected `bir.ret i32 %t9`; actual returns `%t3` after byval/local copy numbering changed. |
| 96 | `backend_codegen_route_x86_64_indirect_aggregate_param_return_pair_observe_semantic_bir` | LLVM IR | LLVM-vs-BIR route mismatch | Semantic-BIR test received LLVM IR for aggregate return/indirect function pointer path. |
| 100 | `backend_codegen_route_x86_64_global_aggregate_array_function_pointer_call_observe_semantic_bir` | LLVM IR | LLVM-vs-BIR route mismatch | Semantic-BIR test received LLVM IR for global array function pointer call. |
| 102 | `backend_codegen_route_x86_64_global_nested_aggregate_array_function_pointer_call_observe_semantic_bir` | BIR | Stale BIR numbering | Expected `%t5 = bir.load_global ...`; actual equivalent load is `%t4`, with call/return renumbered. |
| 161 | `backend_codegen_route_x86_64_aggregate_param_return_pair_fn_param_observe_semantic_bir` | LLVM IR | LLVM-vs-BIR route mismatch | Semantic-BIR test received LLVM IR for function-param aggregate return path. |

#### Global struct/array read/write routes

| Test | Name | Observed output | Classification | Notes |
| --- | --- | --- | --- | --- |
| 105 | `backend_codegen_route_x86_64_global_struct_array_read_observe_semantic_bir` | LLVM IR | LLVM-vs-BIR route mismatch | Semantic-BIR test received LLVM IR for global struct-array read. |
| 106 | `backend_codegen_route_x86_64_global_struct_array_store_observe_semantic_bir` | LLVM IR | LLVM-vs-BIR route mismatch | Semantic-BIR test received LLVM IR for global struct-array store. |
| 107 | `backend_codegen_route_x86_64_nested_global_struct_array_read_observe_semantic_bir` | BIR | Stale BIR numbering | Expected `%t5 = bir.load_global ...`; actual equivalent load is `%t4`. |
| 108 | `backend_codegen_route_x86_64_nested_global_struct_array_store_observe_semantic_bir` | BIR | Stale BIR numbering | Expected `%t10 = bir.load_global ...`; actual equivalent load is `%t8` after store. |

#### Inline asm readwrite routes

| Test | Name | Observed output | Classification | Notes |
| --- | --- | --- | --- | --- |
| 134 | `backend_codegen_route_x86_64_inline_asm_output_readwrite_i32_observe_semantic_bir` | BIR | Changed inline asm constraint shape | Expected `constraints="+r"`; actual BIR prints tied readwrite as `constraints="=r,0"`. |
| 136 | `backend_codegen_route_x86_64_inline_asm_output_readwrite_ptr_observe_semantic_bir` | BIR | Changed inline asm constraint shape | Same `+r` to `=r,0` constraint-shape drift for pointer readwrite output. |

#### Builtin memcpy/memset aggregate and array routes

| Test | Name | Observed output | Classification | Notes |
| --- | --- | --- | --- | --- |
| 139 | `backend_codegen_route_x86_64_builtin_memcpy_local_i32_array_observe_semantic_bir` | LLVM IR | LLVM-vs-BIR route mismatch | Semantic-BIR test received LLVM IR containing `declare ptr @memcpy(...)`. |
| 140 | `backend_codegen_route_x86_64_builtin_memcpy_nested_i32_array_field_observe_semantic_bir` | BIR | Stale BIR numbering | Actual lowered copy sequence is BIR; expected copy temp prefix `%t26...`, actual `%t20...`. |
| 141 | `backend_codegen_route_x86_64_builtin_memcpy_local_i32_array_to_pair_observe_semantic_bir` | LLVM IR | LLVM-vs-BIR route mismatch | Semantic-BIR test received LLVM IR for array-to-pair memcpy. |
| 142 | `backend_codegen_route_x86_64_builtin_memcpy_local_pair_to_i32_array_observe_semantic_bir` | LLVM IR | LLVM-vs-BIR route mismatch | Semantic-BIR test received LLVM IR for pair-to-array memcpy. |
| 143 | `backend_codegen_route_x86_64_builtin_memcpy_prefix_local_i32_array_to_pair_observe_semantic_bir` | BIR | Stale BIR numbering | Actual copy and return are BIR; expected return temp `%t31`, actual `%t27`. |
| 145 | `backend_codegen_route_x86_64_builtin_memcpy_prefix_local_i32_subarray_to_nested_pair_field_observe_semantic_bir` | BIR | Stale BIR numbering | Expected copy temp prefix `%t30...`, actual `%t24...`; copy offsets appear preserved. |
| 147 | `backend_codegen_route_x86_64_builtin_memset_local_i32_array_observe_semantic_bir` | LLVM IR | LLVM-vs-BIR route mismatch | Semantic-BIR test received LLVM IR containing `declare ptr @memset(...)`. |
| 149 | `backend_codegen_route_x86_64_builtin_memset_nonzero_local_i32_array_observe_semantic_bir` | LLVM IR | LLVM-vs-BIR route mismatch | Semantic-BIR test received LLVM IR for nonzero local array memset. |
| 152 | `backend_codegen_route_x86_64_builtin_memset_nested_i32_array_field_observe_semantic_bir` | BIR | Stale BIR numbering | Actual BIR stores zeroes to nested array field and returns `%t29`; expected `%t35`. |
| 153 | `backend_codegen_route_x86_64_builtin_memset_nonzero_nested_i32_array_field_observe_semantic_bir` | BIR | Stale BIR numbering | Actual BIR stores `-1` lane values and returns `%t68`; expected `%t74`. |
| 154 | `backend_codegen_route_x86_64_builtin_memset_nonzero_prefix_nested_i32_subarray_field_observe_semantic_bir` | BIR | Stale BIR numbering | Expected `%t38 = bir.load_local ...`; actual equivalent load is `%t32`, with control-flow temps shifted. |
| 155 | `backend_codegen_route_x86_64_builtin_memset_nonzero_prefix_local_i32_subarray_observe_semantic_bir` | LLVM IR | LLVM-vs-BIR route mismatch | Semantic-BIR test received LLVM IR for nonzero prefix subarray memset. |

#### Nested pointer/member/dynamic array routes

| Test | Name | Observed output | Classification | Notes |
| --- | --- | --- | --- | --- |
| 162 | `backend_codegen_route_x86_64_nested_member_pointer_array_observe_semantic_bir` | LLVM IR | LLVM-vs-BIR route mismatch | Dual route semantic-BIR test received LLVM IR for nested member pointer array. |
| 163 | `backend_codegen_route_x86_64_nested_pointer_param_dynamic_member_array_load_observe_semantic_bir` | BIR | Stale BIR numbering | Expected dynamic lane prefix `%t6...`; actual equivalent select/load sequence uses `%t5...`. |
| 164 | `backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir` | BIR | Stale BIR numbering | Expected dynamic lane prefix `%t4...`; actual equivalent select/load sequence uses `%t3...`. |
| 165 | `backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir` | BIR | Stale BIR numbering | Expected `%t2 = bir.sext ...`; actual is `%t1`, with dynamic store temps shifted. |
| 166 | `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir` | BIR | Stale BIR numbering | Expected lane load `%t16.elt0`; actual equivalent lane load is `%t12.elt0`. |
| 167 | `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir` | BIR | Stale BIR numbering | Expected `%t16 = bir.load_local ...`; actual equivalent local index load is `%t12`. |
| 168 | `backend_codegen_route_x86_64_packed_local_member_offsets_observe_semantic_bir` | BIR | Stale BIR numbering | Expected packed lane load `%t19.elt0`; actual equivalent is `%t15.elt0` at offset `%lv.p.1`. |
| 169 | `backend_codegen_route_x86_64_local_direct_dynamic_struct_array_call_observe_semantic_bir` | LLVM IR | LLVM-vs-BIR route mismatch | Semantic-BIR test received LLVM IR for dynamic struct-array element call. |

### First Narrow Implementation Target

Repair the semantic-BIR route selection/harness path for tests that currently receive LLVM IR. Start with the aggregate/byval/function-pointer subset (`96`, `100`, `161`) because it is the earliest source-idea family with multiple LLVM-vs-BIR mismatches and should establish whether the same route-selection defect also covers global, memcpy/memset, and dynamic-member cases.

## Suggested Next

Implement Step 2 for the first narrow target: trace why semantic-BIR route tests `96`, `100`, and `161` receive LLVM IR, then fix route/harness selection without changing expected snippets.

## Watchouts

- Do not mark supported route cases unsupported or weaken semantic BIR expectations.
- Do not update snippets until the actual BIR is classified as semantically correct.
- A semantic BIR observation that receives LLVM IR should be fixed at the route or harness layer.
- Do not add named-test shortcuts; prove repairs across nearby same-feature cases.
- Route mismatch tests block semantic classification of their actual BIR; inspect regenerated BIR after route selection is repaired before touching snippets.
- BIR-output failures mostly look like stale temp numbering or inline asm constraint-shape drift, but semantic correctness still needs a per-family check before any expectation update.

## Proof

No build/test required for this inventory-only packet. Used existing `test_baseline.log` for the 31-test list and existing `test_after.log` for failure-output classification.
