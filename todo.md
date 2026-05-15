Status: Active
Source Idea Path: ideas/open/233_aarch64_global_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 9
Current Step Title: Validate Semantic Coverage

# Current Packet

## Just Finished

Step 9 validated semantic coverage for idea 233 and recorded the final handoff.

- Direct global and string-constant address materialization are covered by
  structured prepared carriers, selected `DirectPageLow12`/string records, and
  terminal ADRP plus low-12 ADD printer output.
- Label address materialization is covered by structured target-label identity,
  selected `LabelPageLow12` records, and terminal ADRP plus low-12 ADD output
  from the selected record's target-label text.
- GOT-backed global materialization is covered by explicit target/BIR/prepared
  `GotRequired` policy, prepared `GotGlobal`, selected `GotPageLow12`, and
  terminal `:got:`/`:got_lo12:` load output. Missing GOT policy remains
  fail-closed.
- TLS materialization is covered by explicit prepared/AArch64 local-exec
  thread-pointer-relative facts (`tpidr_el0`, `tprel_hi12`,
  `tprel_lo12_nc`) and terminal TLS output from the selected result register.
- Global load/store memory addressing remains separate from result-producing
  address materialization carriers; no load/store path was repurposed as an
  address-materialization shortcut.
- Selection and printing use structured symbol/text/label/policy/TLS fields;
  the route does not infer behavior from rendered names, `is_extern`, storage
  class text, or the archived implicit `x0` scratch convention.

## Suggested Next

Idea 233 is ready for plan-owner close review.

## Watchouts

- The full-suite before/after baseline is now available as `test_before.log`
  and `test_after.log`, both passing 3167/3167 tests.
- Future expansion beyond the current TLS local-exec model should add explicit
  policy/fact fields before selecting or printing new TLS forms.
- Keep global load/store memory operands and result-producing address
  materialization records separate in future backend work.

## Proof

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1`

Result: passed, 3167/3167 full-suite tests green. Proof log: `test_after.log`.
