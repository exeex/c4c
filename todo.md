Status: Active
Source Idea Path: ideas/open/293_aarch64_side_effect_control_value_publication_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair Control-Selected Expression Publication

# Current Packet

## Just Finished

Step 3 materially advanced the next `00164` stale publication after `%t106` by
repairing scalar value-home freshness across the eighth-line expression
`a | b ^ c & d`. The AArch64 scalar state now evicts older value names when a
new value is published into the same physical register, clears caller-saved
scalar publications after ordinary calls, and records after-call prepared move
destinations back into the scalar state. The scalar fallback also treats an
un-emitted register-home value as reloadable when prepared memory facts identify
the defining load source, and its memory-source scratch choice avoids clobbering
a still-needed register operand.

Generated-code evidence for `00164`: after the `%t106` print, the eighth output
expression now reloads the source locals and produces the expected `46` instead
of OR-ing stale `x13`:

```asm
ldr w9, [sp, #8]
ldr w10, [sp, #12]
and w9, w9, w10
str w9, [sp, #160]
ldr w10, [sp, #4]
eor w9, w10, w9
str w9, [sp, #164]
ldr w10, [sp]
orr w9, w10, w9
str w9, [sp, #168]
```

The focused subset still fails overall with known remaining `00164` and `00183`
runtime mismatches, while `00202` passes. `00164` now preserves and advances the
front of the output to `134`, `134`, `0`, `1`, `1`, `1`, `1`, `46`; the compare
pair remains `1, 0` and `0, 1`. The next `00164` stale value is now the
eleventh line, expected `1`.

## Suggested Next

Continue Step 3 at the next `00164` stale select-chain use after the eighth-line
repair. The next packet should target the eleventh output line, expected `1`,
where `a != b && c != d` still prints a stale call/result register value. Keep
the fix tied to prepared join-transfer, parallel-copy, or scalar publication
authority rather than enabling all `BeforeInstruction` moves globally.

Proposed proof command:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00183|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

## Watchouts

- A broad attempt to emit every `PreparedMovePhase::BeforeInstruction` bundle
  regressed the first-six-line `00164` contract. Keep the next slice tied to
  explicit predecessor parallel-copy or join-transfer authority.
- The implemented edge hook currently handles the bounded shape needed by
  `%t106`: an out-of-SSA predecessor block-entry bundle with a named scalar
  compare source defined in the successor join prefix, and only when source and
  destination prepared homes name the same register. If the next stale value
  needs non-coalesced edge publication, add an explicit prepared-register move
  after source lowering rather than assuming the source home is the destination.
- Prepared memory access instruction indexes can lag the compacted prepared BIR
  block indexes; the scalar fallback now reloads by prepared result value name
  when ordinary memory lowering misses an un-emitted register-home load.
- `00183` is still unchanged by this slice and remains the broader
  conditional-expression target for Step 3 after the `00164` select boundary is
  owned.
- Do not touch expected outputs, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or build/test
  infrastructure.

## Proof

Ran the delegated proof exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00183|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

Result: failed overall at the focused scan subset. The scan subset remains 1/3
passing: `00202` passed; `00164` and `00183` remain `RUNTIME_MISMATCH`. `00164`
advanced to actual output starting `134`, `134`, `0`, `1`, `1`, `1`, `1`, `46`;
the compare pair lines remain `1, 0` and `0, 1`. The next stale `00164` line is
the eleventh line, expected `1`, actual stale register value.

Stale-process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

Result: no generated runtime process remained.

Additional check: `git diff --check` passed.

Log paths:

- `test_after.log`
