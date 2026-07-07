Status: Active
Source Idea Path: ideas/open/564_bir_scalar_local_memory_semantic_admission_followup.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Prove IEEE Representatives After Unordered Compare Work

# Current Packet

## Just Finished

Step 7 - Prove IEEE Representatives After Unordered Compare Work completed as a
no-code proof packet. Step 6 is complete by the Step 5 representation decision:
`fcmp uno` remains an explicit fail-closed scalar/local-memory owner-boundary
rejection through the current `unordered-float-compare scalar/local-memory
semantic family` diagnostic, rather than being lowered into an existing
integer-style BIR comparison opcode.

After that decision, all four affected IEEE representatives still advance to
the same narrow owner-boundary diagnostic:

- `tests/c/external/gcc_torture/src/ieee/fp-cmp-8.c`: latest function failure:
  semantic lir_to_bir function `test_isunordered` failed in
  `unordered-float-compare scalar/local-memory semantic family`.
- `tests/c/external/gcc_torture/src/ieee/fp-cmp-8f.c`: latest function failure:
  semantic lir_to_bir function `test_isunordered` failed in
  `unordered-float-compare scalar/local-memory semantic family`.
- `tests/c/external/gcc_torture/src/ieee/fp-cmp-8l.c`: latest function failure:
  semantic lir_to_bir function `test_isunordered` failed in
  `unordered-float-compare scalar/local-memory semantic family`.
- `tests/c/external/gcc_torture/src/ieee/pr38016.c`: latest function failure:
  semantic lir_to_bir function `test_isunordered` failed in
  `unordered-float-compare scalar/local-memory semantic family`.

No source, test, expectation, unsupported marker, allowlist, or lowering
behavior changes were made.

## Suggested Next

Next packet should execute Step 8 - Broader Validation And Closure Decision:
run the supervisor-selected broader validation, then route to plan-owner for the
closure/continue decision based on the active source idea and proof state.

## Watchouts

- This packet intentionally made no source, test, expectation, unsupported
  marker, allowlist, or lowering-behavior changes.
- Do not add a raw `Uno` `BinaryOpcode` without updating all comparison
  classifiers, route records, constant evaluators, and target emitters that
  currently assume compare opcodes are simple integer-style predicates.
- Do not map `fcmp uno` to `Ne`; that would erase NaN/unordered semantics and
  would be a semantic overfit.
- `ord` and `ueq` remain nearby predicate-family questions; keep any future
  decision predicate-family based and covered by focused tests.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: pass. CTest reported `100% tests passed, 0 tests failed out of 346`.

Proof log: `test_after.log`.

Direct diagnostics also run with:

`build/c4cll --codegen asm --target riscv64-linux-gnu <representative>`

Each representative failed at `test_isunordered` with
`unordered-float-compare scalar/local-memory semantic family`, matching the
selected Step 6 fail-closed owner-boundary rejection.
