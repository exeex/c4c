# RV64 Ordinary Control And Expression Completion

## Closure Summary

Closed after the ordinary RV64 prepared-backend control/expression family was
repaired through semantic lowering rules rather than c-testsuite-specific
matching:

- fused compare branch emission now completes prepared conditional branches
- prepared loop successor bodies continue through prepared block-label lookup
- compare results can materialize as values when used outside fused branches
- integer `and`/`or`/`xor` and i32 signed `sdiv`/`srem` tails are emitted for
  ordinary return/control paths

Focused backend coverage for fused-compare, loop-successor, and signed
division/remainder cases passed. The targeted RV64 c-testsuite acceptance sweep
passed `src/00006.c`, `src/00007.c`, `src/00008.c`, `src/00009.c`,
`src/00027.c`, `src/00028.c`, `src/00029.c`, `src/00030.c`, `src/00031.c`,
and `src/00105.c` through emit, clang, and QEMU.

The remaining probed residual, `src/00143.c`, is intentionally left out of
this idea: its failure involves expanded local array indexed updates/reads,
large `i16` select/update chains, pointer/address-offset local access forms,
and later switch-shaped flow rather than the ordinary fused-compare,
successor-body, bitwise boolean-tail, or signed div/rem return-tail mechanisms
owned here.

## Goal

Complete ordinary RV64 control-flow and expression-result emission so compiled
C functions do not fall through into missing branch, return-value, epilogue, or
`ret` tails after otherwise valid expression lowering has started.

## Why This Exists

The RV64 c-testsuite runtime triage in
`ideas/open/310_rv64_c_testsuite_qemu_nonzero_runtime_triage.md` classified 23
runtime failures as `incomplete_control_or_expression_emission`.

Step 4 evidence in
`build/rv64_c_testsuite_probe_latest/triage_step4/summary.md` describes this
family as assembly that enters an expression, condition, call-result, branch, or
loop block and then stops before publishing the final branch, return value,
epilogue, or `ret`. The ranked triage recommends this as the first backend
repair because it is ordinary RV64 lowering, independent of external libc
policy, and has clear assembly tails.

## Owned Feature Family

RV64 backend emission for ordinary local expression completion, conditional
control flow, loop condition publication, call-result use, return-value
finalization, function epilogues, and final `ret` emission.

## Candidate Cases

Primary runtime candidates from the Step 4 classification:

- `src/00006.c`
- `src/00007.c`
- `src/00008.c`
- `src/00009.c`
- `src/00027.c`
- `src/00028.c`
- `src/00029.c`
- `src/00030.c`
- `src/00031.c`
- `src/00035.c`
- `src/00036.c`
- `src/00041.c`
- `src/00044.c`
- `src/00053.c`
- `src/00079.c`
- `src/00083.c`
- `src/00084.c`
- `src/00102.c`
- `src/00105.c`
- `src/00109.c`
- `src/00126.c`
- `src/00133.c`
- `src/00143.c`

Representative evidence called out by triage:

- `src/00008.c`: do-while condition tail reaches a load and does not finish
  the condition branch/continuation.
- `src/00030.c`: call result reaches `mv t0, a0` and does not complete the
  value/control tail.
- `src/00105.c`: for-loop condition reaches a stack load and does not finish
  the branch or loop transition.

## In Scope

- Identify the RV64 emission point that should complete expression values into
  branch predicates, call-result uses, returns, and epilogues.
- Add semantic lowering rules that generalize across ordinary conditions,
  loops, arithmetic expressions, and call-result values.
- Add focused tests around representative local expression/control patterns
  instead of relying only on the full c-testsuite sweep.
- Re-run a small c-testsuite subset including representative candidates to
  confirm the family moved for semantic reasons.

## Out Of Scope

- Local stack slot or address-taken object materialization beyond what ordinary
  control/expression completion needs.
- External libc, libm, string, or bodyless-stub policy decisions.
- Global storage layout support and undefined temporary-label assembler issues.
- Marking runtime cases unsupported or weakening existing supported-path
  expectations.
- A broad "make all c-testsuite runtime cases pass" effort.

## Acceptance Criteria

- Representative ordinary control/expression cases compile, link, and run under
  the RV64 qemu probe without stopping at empty branch, epilogue, return, or
  call-result tails.
- The fix is expressed as a backend emission rule or rules that apply to a
  class of BIR/prepared-BIR operations, not to c-testsuite filenames.
- New or updated tests cover at least loop condition completion, call-result
  value use, and return/epilogue completion.
- A targeted c-testsuite subset includes at least `src/00008.c`, `src/00030.c`,
  and `src/00105.c`, plus nearby candidates from the same bucket.
- Any remaining failing candidates are documented by a different mechanism,
  such as stack/local address materialization or external empty-stub policy,
  instead of being silently counted as fixed.

## Reviewer Reject Signals

- The patch checks for specific c-testsuite filenames, label names, or emitted
  assembly strings instead of repairing semantic RV64 lowering.
- Runtime progress is claimed mainly by changing expectations, adding
  unsupported diagnostics, or removing cases from the supported path.
- The implementation only appends a `ret` or generic epilogue in a way that
  leaves branch predicates, call-result values, or return values
  unmaterialized.
- The proof shows only one named candidate while adjacent loop, call-result,
  and return-tail cases remain unexamined.
- The route drifts into libc stubs, global storage, or stack address
  materialization and claims this idea complete without proving ordinary
  control/expression completion.
- A helper rename or abstraction hides the exact old fall-through/missing-tail
  failure mode without changing emitted behavior.
