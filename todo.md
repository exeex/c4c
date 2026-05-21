Status: Active
Source Idea Path: ideas/open/364_aarch64_synthetic_select_label_uniqueness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Synthetic Label Collision

# Current Packet

## Just Finished

Step 1 localized the `00143` collision to duplicate AArch64 synthetic
select-materialization labels in `build/c_testsuite_aarch64_backend/src/00143.c.s`.
The duplicated definitions are the full label family
`.Lselect_mat_1_24_164_{0..37}_{true,end}`: 76 labels total, each defined
twice. The first definition group is at assembly lines 1604-1754, and the
second group is at lines 1911-2061; examples include
`.Lselect_mat_1_24_164_37_true` at 1604 and 1911,
`.Lselect_mat_1_24_164_37_end` at 1606 and 1913, and
`.Lselect_mat_1_24_164_0_true`/`_0_end` at 1752/1754 and 2059/2061.

The generated label format maps to function id `1`, AArch64 control-flow block
label `24` (`.LBB1_24` in the assembly), root materialization instruction index
`164`, recursive select label indices `0..37`, and suffix `true`/`end`. The two
definition groups sit in the same `.LBB1_24` block: the first materializes the
prepared-BIR `%t76` select chain from `%lv.a.*` into a temporary stack result,
and the second materializes the adjacent `%t80` select chain from `%lv.b.*`
before the final compare. Both use the same block label and consumer/root
instruction index, so `label_index` restarts from zero and reuses the same
synthetic namespace.

AST-backed lookup with `c4c-clang-tool-ccdb` found the owning backend boundary
in `src/backend/mir/aarch64/codegen/dispatch.cpp`: `select_chain_label` at line
4715 formats `.Lselect_mat_<function>_<block>_<instruction>_<label>_<suffix>`,
`emit_select_chain_value_to_register` at line 4731 allocates `true`/`end`
labels from a local `label_index`, and
`make_select_chain_materialization_instruction` at line 4855 publishes the
generated assembler lines. The direct caller path for this shape includes
`materialize_direct_global_select_chain_call_argument` at line 4986, but the
collision is in the shared synthetic label namespace, not in a test runner or
expectation.

Idea 352 is unrelated to this first bad fact: the failing symbols are duplicate
`.Lselect_mat_*` definitions emitted inside a valid `.LBB1_24` block, not basic
block label ordering, missing block labels, or branch target sequencing.

## Suggested Next

Proceed to focused coverage and repair. The coverage should exercise one
AArch64 function/block with two materialized select chains emitted for the same
consumer/root instruction index, then assert that no `.Lselect_mat_*` label is
defined more than once. The likely repair shape is to add a per-materialization
unique discriminator or otherwise make the shared select-materialization label
allocator span adjacent emissions in the same block/root instruction.

## Watchouts

Keep this distinct from idea 352 basic block label ordering and from the old
`00143` scalar-cast source-publication thread. Do not special-case `00143` or
only rename one suffix family; the backend boundary is synthetic
select/materialized-label uniqueness for repeated materialization emissions in
one AArch64 block.

## Proof

Ran:
`ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00143_c$' > test_after.log 2>&1`

Result: failed as expected for localization. `test_after.log` reports assembler
duplicate-symbol errors starting at
`build/c_testsuite_aarch64_backend/src/00143.c.s:1911:5` for
`.Lselect_mat_1_24_164_37_true`, followed by the rest of the duplicated
`.Lselect_mat_1_24_164_{37..0}_{true,end}` labels.
