Status: Active
Source Idea Path: ideas/open/564_bir_scalar_local_memory_semantic_admission_followup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Decide Unordered Floating Compare Representation

# Current Packet

## Just Finished

Step 4 - Prove IEEE Representatives And Route Downstream Failures completed as
a diagnostic/proof packet. Direct RV64 backend probes show that all four target
IEEE representatives advanced beyond the umbrella `scalar/local-memory semantic
family` diagnostic and now stop at the narrower fail-closed unordered floating
compare boundary:

- `tests/c/external/gcc_torture/src/ieee/fp-cmp-8.c`: latest function failure
  `test_isunordered` failed in `unordered-float-compare scalar/local-memory
  semantic family`; reached narrower unordered-float compare boundary: yes.
- `tests/c/external/gcc_torture/src/ieee/fp-cmp-8f.c`: latest function failure
  `test_isunordered` failed in `unordered-float-compare scalar/local-memory
  semantic family`; reached narrower unordered-float compare boundary: yes.
- `tests/c/external/gcc_torture/src/ieee/fp-cmp-8l.c`: latest function failure
  `test_isunordered` failed in `unordered-float-compare scalar/local-memory
  semantic family`; reached narrower unordered-float compare boundary: yes.
- `tests/c/external/gcc_torture/src/ieee/pr38016.c`: latest function failure
  `test_isunordered` failed in `unordered-float-compare scalar/local-memory
  semantic family`; reached narrower unordered-float compare boundary: yes.

The remaining downstream work is still scalar/local-memory semantic
representation for unordered floating compare predicates: BIR needs an explicit
semantic representation/lowering decision for unordered predicates such as
`fcmp uno`. No evidence from this packet routes the blocker to another owner.

## Suggested Next

Plan-owner lifecycle review decision: continue within this source idea rather
than move to closure. Step 4 proved that the representatives advanced to a
narrower same-owner boundary, but the source idea still calls for repairing the
semantic producer or lowering rule responsible for the scalar/local-memory
diagnostics.

Next executable packet is Step 5 - Decide Unordered Floating Compare
Representation. The packet should make an owner-level decision for representing
or lowering `fcmp uno` rather than mapping it onto an existing ordered
comparison opcode.

## Watchouts

- This packet intentionally made no source, test, expectation, unsupported
  marker, allowlist, or lowering-behavior changes.
- The observed diagnostic is function-level `test_isunordered` for all four
  representatives, so the live blocker remains the `fcmp uno` unordered
  predicate path after any earlier casts or setup have lowered far enough.
- `ord` and `ueq` remain nearby unordered/ordered predicate-family questions;
  do not fold them into the next slice without focused coverage and a semantic
  representation decision.

## Proof

Direct diagnostics:

`build/c4cll --codegen asm --target riscv64-linux-gnu tests/c/external/gcc_torture/src/ieee/fp-cmp-8.c`

`build/c4cll --codegen asm --target riscv64-linux-gnu tests/c/external/gcc_torture/src/ieee/fp-cmp-8f.c`

`build/c4cll --codegen asm --target riscv64-linux-gnu tests/c/external/gcc_torture/src/ieee/fp-cmp-8l.c`

`build/c4cll --codegen asm --target riscv64-linux-gnu tests/c/external/gcc_torture/src/ieee/pr38016.c`

Each direct diagnostic exited 1 at the expected fail-closed boundary:
`latest function failure: semantic lir_to_bir function 'test_isunordered'
failed in unordered-float-compare scalar/local-memory semantic family`.

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: pass. CTest reported `100% tests passed, 0 tests failed out of 346`.

Proof log: `test_after.log`.
