Status: Active
Source Idea Path: ideas/open/375_aarch64_local_aggregate_bitfield_layout_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Local Aggregate Bit-Field Layout Gap

# Current Packet

## Just Finished

Step 1 localized the `00218` local aggregate bit-field mismatch. Frontend HIR
and Clang agree that `union tree_node.common.code` is stored in the
`tree_common` bit-field storage unit at byte `16`, bit `0`; `convert_like_real`
semantic/prepared BIR also loads `addr %p.convs+16`. The first bad fact is
already present in `main` semantic BIR: `convs.common.code = AMBIG_CONV`
lowers to `bir.store_local %lv.convs.0, ... addr %lv.convs.0+2`, and prepared
addressing preserves that bad `frame_slot #1 offset=2` store through AArch64
publication (`str w9, [sp,#2]`).

## Suggested Next

Execute Step 2 by adding focused backend coverage for semantic-BIR local
aggregate member/bit-field GEP lowering: a local aggregate with an initial
pointer-sized member followed by an unsigned 8-bit bit-field is written, its
address is passed to a scalar pointer consumer, and the dump/assembly proof
requires the write and later load to use the ABI/layout byte offset (`16` here),
not the logical field index (`2`) or a testcase-specific spelling.

## Watchouts

Likely owning boundary is semantic layout publication in the local aggregate
member/bit-field GEP path before prepared handoff, not frame-slot allocation,
bit-field mask/store lowering, or AArch64 store publication. Evidence:
`--dump-hir` reports `tree_common.code offset=16 size=4 bitfield(width=8,
bit_offset=0, storage_bits=32)`, while `--dump-bir --mir-focus-function main`
has `addr %lv.convs.0+2`; prepared stack layout and AArch64 merely preserve
that bad offset. Do not reopen idea 374's local aggregate address call
publication boundary; the call now passes `&convs` correctly.

## Proof

Ran delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00218_c$'; } > test_after.log 2>&1`.
Result: build was up to date, focused CTest failed with the known runtime
mismatch (`actual: unsigned enum bit-fields broken`). Proof log:
`test_after.log`.
