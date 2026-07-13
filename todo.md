# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 weak-external global packet now admits the exact existing LIR
  producer row with `linkage_vis == "extern_weak "`, `global ` qualifier, empty
  initializer evidence, `is_extern_decl=true`, and `is_internal=false`.
- Import preserves both typed Raw-BIR facts: weak linkage and external
  declaration state. Ordinary `external ` declarations remain non-weak, while
  definition-shaped, constant-qualified, and initializer-bearing extern-weak
  near-neighbors reject the whole module transactionally.

## Suggested Next

- Execute one bounded remaining Step 3 global, external-symbol, or initializer
  completeness packet selected from the runbook, keeping aggregate and
  flexible-special-type global support separate.

## Watchouts

- Weak external admission is exact to `extern_weak ` with no visibility suffix,
  a `global ` qualifier, declaration state, no internal flag, and no initializer
  payload or initializer links. Visibility-qualified variants and definition
  shapes remain fail-closed.
- Raw-BIR represents weak external declarations through the existing
  independent `is_weak` and `is_extern_declaration` fields. The verifier still
  rejects weak+internal and all declaration/initializer state mismatches.
- Aggregate and flexible-special-type globals remain unsupported and were not
  widened by this packet.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof covers exact importer admission, typed weak and
  external-declaration views, verifier publication, and transactional rejection
  of malformed near-neighbor rows.
