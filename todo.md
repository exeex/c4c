Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Families

# Current Packet

## Just Finished

Completed plan Step 2: classified the 52 post-301 residual failures from
accepted `test_before.log` and c-testsuite source labels without rerunning
broad runtime tests or changing implementation/test contracts.

Residual family classification:

- Owner-ready scalar machine-node operand-form printer family (3):
  `00064`, `00139`, `00205`.
  Direct diagnostics show selected scalar nodes reaching the printer without
  the structured operands the AArch64 printer accepts: scalar `div`, scalar
  `mul`, and scalar `logical_shift_right` unsigned reduction. This is separate
  from closed fused compare-branch owner 296, scalar immediate owner 299,
  scalar-cast owner 300, and memory-store owner 301.
- Owner-ready assembly legality/materialization singletons (2): `00104`,
  `00182`.
  `00104` emits invalid `sxtw w20, w13`; `00182` emits invalid
  `mov x0, #1234567`. These are crisp compile/assembler failures, but they are
  narrower than the scalar operand-form family and should not be merged into
  299/300 without generated-code evidence.
- Owner-ready but low-cardinality call-boundary move printer gap (1): `00140`.
  The diagnostic is a selected-machine-node gap for a call-boundary move
  outside the supported register subset. It overlaps aggregate/variadic ABI
  subject matter but has a direct printer failure source.
- Parked `lir_to_bir` admission/local-memory residuals (2): `00204`, `00216`.
  Current diagnostics name `gep` local-memory and load local-memory failures,
  but these cases sit near closed admission/projection owners 297 and 298.
  They need a narrow diagnostic/generated-code probe before reopening or
  splitting a semantic admission owner.
- Parked scalar value, promotion, and control-value runtime family (14):
  `00035`, `00066`, `00086`, `00102`, `00111`, `00113`, `00119`, `00121`,
  `00123`, `00144`, `00164`, `00168`, `00200`, `00218`.
  Source labels cover unary/logical value production, short/char/integer
  promotions, shifts and shift-result types, int/float/double conversion,
  recursion-multiply return flow, conditional pointer/null conversions, and
  enum bit-field zero-extension. These are not owner-ready from counts alone;
  at least one generated-code probe must decide whether the common root is
  value publication, extension width, call return, branch/control authority, or
  multiple separate owners.
- Parked aggregate, array, initializer, and indexed-storage runtime family (6):
  `00050`, `00151`, `00157`, `00176`, `00185`, `00195`.
  The failures involve nested global aggregate initialization, multidimensional
  array/range initializers, local/global indexed array stores, quicksort over a
  global array, and global array-of-struct member access. This is adjacent to
  closed projection/store owners 298 and 301, so it needs generated-code
  evidence before an owner is selected.
- Parked pointer, string, address, and indirect-value runtime/crash family (11):
  `00089`, `00112`, `00137`, `00138`, `00170`, `00172`, `00173`, `00179`,
  `00189`, `00208`, `00214`.
  The set includes function-pointer values, string-literal address flow,
  pointer equality, enum/pointer-adjacent crashes, string buffer/libc paths,
  local array/static aggregate lifetime, and builtin/statement-expression
  address flow. This overlaps closed owners 287, 289, 294, and 301 at a high
  level, so runtime status alone is insufficient for ownership.
- Parked ABI, libc, float/conversion, and formatted-output runtime family (5):
  `00159`, `00174`, `00175`, `00186`, `00207`.
  The visible symptoms include stale scalar call arguments, all-zero or junk
  floating output, char/int/float argument conversion failures, short `sprintf`
  loop output, and VLA/goto/short-circuit crash behavior. This needs a
  generated-code probe before deciding whether it belongs to call ABI, floating
  lowering, conversion lowering, or control-flow publication.
- Parked complex control-flow, side-effect, and timeout family (8): `00143`,
  `00169`, `00181`, `00187`, `00193`, `00196`, `00215`, `00220`.
  Duff's device, nested loops, recursive Hanoi, file I/O timeout, switch
  return/break flow, short-circuit side effects, dead infinite-loop suppression,
  and wide-character timeout/output behavior should remain parked until narrow
  probes distinguish bad branch/control lowering from output storm, libc, or
  frontend/runtime-library behavior.

Best next focused owner candidate:

- Recommend activating an AArch64 scalar machine-node operand-form owner for
  `00064`, `00139`, and `00205`.
  It has multiple direct compile-stage diagnostics, does not depend on runtime
  output interpretation, and is clearly outside the recently closed owner
  boundaries: not fused compare-branch operands (296), not non-encodable
  add/sub/bitwise immediates (299), not zero/sign-extension cast forms (300),
  and not store operand materialization (301). The owner should repair scalar
  `mul`/`div`/reduction operand publication or selection semantically and prove
  the full three-case focused subset, not just one diagnostic.

## Suggested Next

Create or activate a focused owner for AArch64 scalar machine-node operand
forms covering `00064`, `00139`, and `00205`; keep runtime, timeout, and
`lir_to_bir` residuals parked until narrow generated-code probes assign them.

## Watchouts

- This umbrella plan is classification-only; switch to a focused owner before
  implementation.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, CTest registration, or test contracts.
- Do not reopen ideas 285 through 301 from counts alone. The parked runtime
  buckets overlap several closed owner themes at a high level but lack the
  generated-code evidence needed to contradict closure boundaries.
- Keep `00104` and `00182` out of the recommended scalar operand-form owner
  unless a probe proves they share the same operand-publication path; they are
  compile-stage singletons and risk reopening 299/300 too broadly.
- Keep `00204` and `00216` separate from 297/298 until a narrow diagnostic
  probe explains why local-memory admission diagnostics remain after those
  owners closed.
- Keep timeout or output-storm cases separate unless evidence supports a
  hang-specific owner.
- The broad proof shows all 52 residual failures are `c_testsuite` AArch64
  backend failures; local backend/unit tests selected by the broad regex passed.
- `test_before.log` contains a very large output region before the final CTest
  summary; avoid rerunning broad runtime tests unless the supervisor explicitly
  requests a fresh proof.

## Proof

Used existing accepted broad proof `test_before.log`; no tests were rerun per
packet instruction. The accepted proof records 352 selected, 300 passed, and
52 failed after idea 301. No `test_after.log` update was required for this
classification-only packet.
