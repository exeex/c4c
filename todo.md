Status: Active
Source Idea Path: ideas/open/212_bir_mir_allocation_contract.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Add Focused Proof

# Current Packet

## Just Finished

Step 6 of `plan.md` ran fresh focused proof for the allocation contract and
implemented-surface alignment without changing implementation files, tests,
test expectations, or roadmap markdown.

The delegated proof command rebuilt the default preset and ran the focused
backend CTest subset. `test_after.log` contains the full command output and
records `100% tests passed, 0 tests failed out of 131`.

## Suggested Next

Execute Step 7: final consistency review against the source idea acceptance
criteria and active plan, including checks that allocation authority remains
structured and separate from core BIR, assembly text, rendered physical names,
and final stack offsets.

## Watchouts

- This proof-only packet intentionally did not add tests or touch
  implementation files because the delegated backend bucket already covered the
  existing AArch64 surfaces.
- Final review should still verify that GPR and FPR/SIMD pools remain separate,
  reserved MIR scratch registers are excluded from long-lived homes, and
  x86/x86-64 fixed operand/flag/subregister/ABI-stack carriers remain
  representable without implementing x86.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed. Proof output is preserved in `test_after.log`, including
`100% tests passed, 0 tests failed out of 131`.
