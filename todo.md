# Current Packet

Status: Active
Source Idea Path: ideas/open/726_rv64_explicit_register_inline_asm_bir_syntax.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Add structured BIR syntax classification

## Just Finished

- Plan Step 1: added BIR-owned structured RV64 explicit-GPR identities (bank, x-register index, and canonical spelling), target-gated canonical `{xN}`, `={xN}`, and `+{xN}` classification, and direct LIR-to-BIR role/identity plus fail-closed negative coverage.

## Suggested Next

- Execute Plan Step 2 by proving the source-backed RV64 route reaches the structured BIR identity while preserving compatibility behavior; do not enter prepared allocation.

## Watchouts

- Clobber handling remains separate from explicit-register operand identity. The focused prealloc subset still reports its pre-existing prepared-only block-entry printer failure; Step 1 does not touch that out-of-scope prepared path.

## Proof

- Direct `backend_lir_to_bir_notes_test` execution passed, including canonical role/index/identity cases and wrong-target plus malformed/unsupported fail-closed cases.
- Supervisor-selected proof ran exactly as `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prealloc_inline_asm$' > test_after.log 2>&1`; the build passed and the subset retained the known prepared-only block-entry printer failure in `test_after.log`.
