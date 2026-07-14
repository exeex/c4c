# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Publish direct void scalar immediate argument authority (complete)

## Just Finished

- Completed Plan Step 4 for the direct void fixed integer-immediate argument.
- Replaced the string-only owned call-argument operand with `LirOperand`;
  formatting reads presentation while structured construction preserves native
  authority. ABI/aggregate paths retain mechanical monostate compatibility.
- Preserved immediate authority only through the representation-preserving
  fixed integer path, populated exact argument type refs, and added
  authority-first verification plus focused misleading/malformed coverage.
- Full-regression follow-up fixed `c_testsuite_src_00175_c`: integer-looking
  character-literal compatibility text was classified `Immediate` without
  native payload. The claim now also requires the producer's exact structured
  argument type markers, preserving malformed claimed-packet rejection.

## Suggested Next

- Execute Plan Step 5: publish direct void scalar SSA argument authority.

## Watchouts

- Step 5 must retain the existing source `LirValueId` through the same
  `OwnedLirTypedCallArg::operand`; do not add an SSA-only side carrier.
- Keep complete structured rows authority-first. Raw `args_str`, suffix, and
  operand/type spelling remain compatibility presentation only.
- Operand `kind()` alone is not a claim marker: raw character/numeric spelling
  may classify as immediate without native authority or exact typed facts.
- Keep ABI, variadic, aggregate, indirect-call, CFG/terminator, stack/local/
  object, and body-parameter work outside Step 5.
- Preserve the accepted Step-3 scalar call-result path unchanged.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-4 carrier,
  producer, verifier, and focused test changes.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1; canonical proof is in `test_after.log`.
- `ctest --test-dir build -R '^c_testsuite_src_00175_c$' --output-on-failure`
  passed 1/1 for the narrow compatibility regression.
- Supervisor acceptance guard used a clean-stashed full-suite baseline and
  after run: `test_before.log` passed 3033/3033 and `test_after.log` passed
  3033/3033.
- The monotonic regression delta was passed=0 and failed=0, with no new
  failures.
- `git diff --check` passed for the complete Step-4 slice.
