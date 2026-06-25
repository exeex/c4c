Status: Active
Source Idea Path: ideas/open/363_rv64_object_parameter_home_coverage.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Unsupported Parameter Homes

# Current Packet

## Just Finished

Activation created this executor-compatible scratchpad for `plan.md` Step 1.
No implementation packet has run yet.

## Suggested Next

Start Step 1 by auditing the prepared parameter homes behind the
`unsupported_param_home` diagnostic in the `src/20030914-2.c` representative
case. Record the unsupported home class, producer facts, RV64 consumer check,
and narrow proof command here before changing code.

## Watchouts

- Treat `src/20030914-2.c` as a representative signal, not an implementation
  key.
- Do not reopen scalar vararg, `va_arg`, `va_copy`, or variadic helper work
  from idea 362 and its follow-ups.
- Do not broaden into vector ABI, unrelated FPR ABI, globals, data sections, or
  broad object-route rewrites.
- Progress must be based on structured parameter-home classes; diagnostic
  renames, expectation rewrites, skips, and testcase-shaped matching are not
  enough.

## Proof

No proof run for activation-only lifecycle work.
