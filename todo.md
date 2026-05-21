Status: Active
Source Idea Path: ideas/open/375_aarch64_local_aggregate_bitfield_layout_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Layout Publication Coverage

# Current Packet

## Just Finished

Step 2 added focused backend coverage in
`backend_codegen_route_x86_64_local_aggregate_bitfield_layout_observe_semantic_bir`.
The revised case `tests/backend/case/local_aggregate_bitfield_layout.c` uses a
local aggregate wrapper containing `struct NodeCommon common`; `NodeCommon` has
two pointer-sized fields followed by `unsigned code : 8`. The route test writes
`node.common.code`, passes `&node` to a consumer that reads `common.code`, and
checks semantic BIR stores and later loads the bit-field storage unit at
ABI/layout byte offset `16` instead of logical field index `2`.

## Suggested Next

Execute Step 3 by repairing the semantic-BIR local aggregate member/bit-field
GEP lowering so the failing `00218` AArch64 path publishes the layout byte
offset for the local store instead of preserving logical field index `2`.

## Watchouts

The focused x86_64 semantic-BIR route test is green before the repair with the
nested aggregate-wrapper shape. Keep the next slice in implementation code; do
not loosen this coverage or the external `00218` expectation.

## Proof

Ran delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_x86_64_local_aggregate_bitfield_layout_observe_semantic_bir|c_testsuite_aarch64_backend_src_00218_c)$'; } > test_after.log 2>&1`.
Result: build succeeded; the revised focused route test passed; the proof
command failed only because `c_testsuite_aarch64_backend_src_00218_c` still
reports the known runtime mismatch (`actual: unsigned enum bit-fields broken`).
Proof log: `test_after.log`.
