Status: Active
Source Idea Path: ideas/open/347_aarch64_local_conversion_store_load_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize the Residual First Bad Fact

# Current Packet

## Just Finished

Step 1 localized the first residual bad fact in
`c_testsuite_aarch64_backend_src_00175_c`: the local `char c = 97.0`
conversion is stored from stale/unpublished `w13` instead of a materialized
`97` byte. In generated
`build/c_testsuite_aarch64_backend/src/00175.c.s`, the direct-call conversion
series correctly materializes ABI arguments through `w0`/`s0` and prints the
first nine expected lines, including `char: c`, `int: 99`, and
`float: 99.000000`. Immediately after that series, lines 100-102 emit
`movz w9, #97`, `strb w9, [sp, #16]`, then `strb w13, [sp, #17]` for
`char b = 97; char c = 97.0;` with no intervening conversion or constant
publication into `w13`. The later reload at lines 116-117 reads `[sp, #17]`
for the second `%d`, producing the runtime first mismatch `97 17` instead of
`97 97`.

This is not a direct-call ABI argument/formal publication regression: the
callsite conversions before the local declarations pass through `w0`/`s0`,
the callee formals print correctly, and the first mismatch occurs only after
those calls, when a local conversion result is stored to its stack home from a
stale caller-side temporary.

## Suggested Next

Execute Step 2: repair the general local conversion result publication rule so
converted scalar and FP values are materialized before stores to their local
homes instead of reusing stale temporaries such as `w13`/`s13`.

## Watchouts

- The adjacent local conversions show the same publication shape after the
  first bad fact: `int e = 97.0` stores `w13` to `[sp, #4]`, and
  `float f = 'a'; float g = 97;` stores `s13` to `[sp, #8]` and `[sp, #12]`.
  These are corroborating local store facts, not earlier first-bad facts.
- Do not special-case `00175`, named functions, specific registers, stack
  offsets, or one instruction sequence.
- Do not edit expectations, unsupported classifications, allowlists, CTest
  registration, timeout policy, proof-log policy, runner behavior, or root
  logs.

## Proof

Ran the delegated proof command exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00175_c$' | tee test_after.log`.
The build was up to date; the representative test still fails with
`[RUNTIME_MISMATCH]`, preserving the localized evidence in `test_after.log`
(`97 17`, `97 -628715856`, `0.000000 0.000000` actual after the nine
correct direct-call output lines). The proof is sufficient for Step 1
localization and intentionally not a passing acceptance proof for Step 2.
