Status: Active
Source Idea Path: ideas/open/580_rv64_scalar_compare_publication.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Implement Semantic Compare Publication

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by implementing semantic RV64 object-emission
publication for ordinary F32/F64 `eq`/`ne` compare results into prepared GPR
homes.

- Added an RV64 prepared object fragment that lowers F32/F64 compare operands
  from prepared FPR homes with `feq.s`/`feq.d` into the prepared GPR result
  home, including the `xori` inversion for `ne`.
- Wired that fragment into the non-terminator compare path after existing
  terminator and scalar-trunc compare handling, so terminator compare behavior
  and existing trunc publication behavior remain unchanged.
- Kept the `unsupported_scalar_compare_publication` diagnostic for missing
  homes and unsupported compare shapes.
- Strengthened the focused F32/F64 fixtures to assert the emitted compare
  publication instructions and the select-consumer local branch/jump
  materialization.

## Suggested Next

Run the representative scalar-compare publication route for the retained
ordinary rows and record whether the first owner advances beyond
`unsupported_scalar_compare_publication`; if green, follow with the supervisor
selected broader backend proof.

## Watchouts

- This implementation is intentionally limited to ordinary F32/F64 `eq`/`ne`
  compare result publication into prepared GPR homes from prepared FPR operand
  homes.
- Do not mix in floating casts, variadic helpers, F128, long-double, or helper
  implementation work.
- Do not claim progress through unsupported-marker changes, expectation
  rewrites, route allowlist edits, named-case checks, or fixed-register
  shortcuts.
- Ordered F32/F64 compare publication and immediate/stack operand
  materialization remain outside this packet unless the supervisor explicitly
  delegates that widening.

## Proof

Proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`

Result: passed. Focused `backend_riscv_object_emission` proof builds and the
new F32/F64 compare publication fixtures pass.
Log path: `test_after.log`.
