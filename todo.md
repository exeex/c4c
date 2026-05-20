Status: Active
Source Idea Path: ideas/open/338_aarch64_scalar_cast_register_source_operand_facts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Scalar Cast Operand Fact Loss

# Current Packet

## Just Finished

Lifecycle switched from umbrella idea 295 to focused idea 338 for the
post-337 AArch64 scalar cast machine-printer operand-fact bucket.

## Suggested Next

Delegate Step 1 to localize where selected `sign_extend` and `zero_extend`
scalar cast nodes lose or fail to publish the structured register source
operand required by AArch64 printing.

## Watchouts

- Do not reopen closed owners 334 through 337 from counts alone.
- Keep the scope on scalar cast register-source operand facts, not generic
  scalar arithmetic operand printing.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.
- If the old printer diagnostic disappears but representatives still fail,
  classify the new first bad fact before claiming closure.

## Proof

Lifecycle-only split/switch. No implementation validation was run.
