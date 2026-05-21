Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prepare Lifecycle Handoff

# Current Packet

## Just Finished

Step 3 - Select the Next Focused Owner selected exactly one next lifecycle
owner from the Step 2 post-369 residual classification:
`ideas/open/326_aarch64_variadic_hfa_floating_residual.md`.

Selection evidence from Step 2:

- The selected bucket is AArch64 aggregate/varargs ABI call-boundary lowering,
  represented by `c_testsuite_aarch64_backend_src_00140_c` and
  `c_testsuite_aarch64_backend_src_00204_c`.
- `00140.c` currently segfaults in calls involving a struct argument plus
  variadic extras.
- `00204.c` now fails in the machine printer with
  `deferred_unsupported: call-boundary move node requires prepared GPR
  registers, scalar FPR registers, or structured f128 q-register authority`.
- This is not a source unsupported-classification issue: Step 2 observed that
  the backend already has a prepared-BIR publication path for `00204`, and the
  first bad fact is an explicit call-boundary move gap.
- Existing idea 326 is activatable because its current source intent covers
  AArch64 variadic HFA/floating call-boundary argument transport, and the fresh
  `00204` diagnostic supplies direct f128/composite call-boundary evidence
  rather than reopening that route from failing counts alone.

Rejected owner choices:

- Do not create a new composite ABI idea yet; idea 326 already carries the
  open variadic HFA/floating residual contract and can absorb this fresh
  f128/composite call-boundary evidence with a narrow lifecycle handoff note.
- Do not select the indexed array/writeback bucket (`00157`, `00176`,
  `00182`) because the closest existing indexed/writeback ideas are parked
  after different first-bad facts, and the bucket needs a later split if it is
  chosen.
- Do not select the scalar comparison/value materialization bucket (`00112`,
  `00119`, `00123`) or the timeout bucket (`00200`, `00207`) before the
  existing activatable ABI/HFA/f128 owner is handled.

Step 2 - Classify Residuals by Semantic Owner completed a read-only
classification of the 18 external AArch64 c-testsuite residuals captured in
`test_after.log`. Evidence used: per-test diagnostics in `test_after.log`,
source files under `tests/c/external/c-testsuite/src/`, and generated assembly
under `build/c_testsuite_aarch64_backend/src/`.

Classification by first bad boundary or actionable owner:

- Scalar comparison/value materialization:
  `c_testsuite_aarch64_backend_src_00112_c`,
  `c_testsuite_aarch64_backend_src_00119_c`,
  `c_testsuite_aarch64_backend_src_00123_c`.
  Evidence: `00112.c` is only `"abc" == (void *)0` but generated `main`
  returns uninitialized `x13`; `00119.c` and `00123.c` are `double x` less-than
  checks that should return zero, but generated code loads the double into
  `d13` and returns `x13` without producing a comparison boolean. Rejected
  adjacent owner: runner/runtime, because the wrong exit codes are explained by
  generated return-value materialization.
- Floating-point expression and vararg call lowering:
  `c_testsuite_aarch64_backend_src_00174_c`.
  Evidence: expected float arithmetic/comparison output is replaced by zeros
  and junk integers; generated assembly uses uninitialized FP values before
  multiple `printf` calls, while the later `sin(2)` library result is correct.
  Rejected adjacent owner: libc/math, because the external call result is the
  one reliable value.
- Aggregate/member lvalue address lowering:
  `c_testsuite_aarch64_backend_src_00163_c`.
  Evidence: all direct global struct field prints are correct, but after
  `b = &(bolshevic.b)`, `*b` prints `42` instead of `34`; generated assembly
  reloads the earlier stack `a` slot for the final pointer dereference. Rejected
  adjacent owner: global data emission, because direct `bolshevic.a/b/c`
  accesses match expected output.
- Indexed array lvalue and select-matrix lowering:
  `c_testsuite_aarch64_backend_src_00157_c`,
  `c_testsuite_aarch64_backend_src_00176_c`,
  `c_testsuite_aarch64_backend_src_00182_c`.
  Evidence: `00157.c` fills and prints `Array[Count-1]` but actual output is
  patterned uninitialized data; `00176.c` initializes a global array correctly
  but quicksort's indexed swaps produce a permutation instead of sorted output;
  `00182.c` emits LED zeroes where switch-driven digit writers should populate
  a character buffer. Rejected adjacent owner: printf/output comparison,
  because the first printed unsorted line in `00176.c` is correct and failures
  start after indexed writes.
- Switch/select-matrix label ownership:
  `c_testsuite_aarch64_backend_src_00143_c`.
  Evidence: backend assembly fails before runtime because
  `.Lselect_mat_1_24_164_*` labels are defined twice in the generated assembly
  for Duff's-device switch/fallthrough control. Rejected adjacent owner: CTest
  contract, because the assembler reports duplicate symbols in the generated
  `.s` file.
- Conditional operator lowering:
  `c_testsuite_aarch64_backend_src_00183_c`.
  Evidence: `(Count < 5) ? Count*Count : Count*3` prints `0..9`, showing the
  conditional result collapses to `Count` instead of selecting either arm.
  Rejected adjacent owner: multiplication, because both expected arms require
  multiplication and neither is selected.
- File API call-boundary argument/result lowering:
  `c_testsuite_aarch64_backend_src_00187_c`.
  Evidence: file creation plus later `fgetc`, `getc`, and `fgets` output match,
  but the first `fread(freddy, 1, 6, f)` returns a non-6 result and emits only
  the extra "couldn't read fred.txt" line. Rejected adjacent owner: filesystem
  or runner cwd, because the same file is subsequently read correctly.
- AArch64 aggregate/varargs ABI call-boundary lowering:
  `c_testsuite_aarch64_backend_src_00140_c`,
  `c_testsuite_aarch64_backend_src_00204_c`.
  Evidence: `00140.c` segfaults in calls involving a struct argument plus
  variadic extras; `00204.c` fails in the machine printer with
  `deferred_unsupported: call-boundary move node requires prepared GPR
  registers, scalar FPR registers, or structured f128 q-register authority`.
  Rejected adjacent owner: source unsupported classification, because the
  backend already has a prepared-BIR publication path for `00204` and the
  failure is an explicit call-boundary move gap.
- Constant `sizeof`/compile-time value materialization in generated control:
  `c_testsuite_aarch64_backend_src_00205_c`.
  Evidence: global `cases` data is emitted with the expected `.xword` values,
  but `main` produces blank output; generated loop bounds for
  `sizeof(cases)/sizeof(cases[0])` and `sizeof(cases->c)/sizeof(cases->c[0])`
  are loaded from uninitialized stack slots. Rejected adjacent owner: global
  initializer lowering, because the `.data cases:` payload matches the expected
  constants.
- Complex aggregate initializer/object layout lowering:
  `c_testsuite_aarch64_backend_src_00216_c`.
  Evidence: the source stresses nested structs, compound literals, flexible
  arrays, anonymous unions, range initializers, and function-pointer relocs;
  runtime segfaults, and generated assembly shows many stack loads from
  uninitialized offsets while building aggregate copies. Rejected adjacent
  owner: one specific printed output check, because the program crashes before
  producing comparison text.
- Enum bit-field layout/load/store handling:
  `c_testsuite_aarch64_backend_src_00218_c`.
  Evidence: source assigns `AMBIG_CONV` to an 8-bit enum bit-field and expects
  no output; actual output is `unsigned enum bit-fields broken`. Generated
  `convert_like_real` masks the loaded field with `255` and compares against
  the right enum value, but it reads the wrong stored value/layout slot.
  Rejected adjacent owner: switch dispatch generally, because the failure is
  specifically the enum bit-field value reaching the switch.
- Timeout-only residuals kept separate:
  `c_testsuite_aarch64_backend_src_00200_c` and
  `c_testsuite_aarch64_backend_src_00207_c`.
  Evidence: both hit the 5 second CTest timeout with no runtime diagnostic.
  `00200.c` is a dense left-shift/integer-promotion/`sizeof` macro test;
  `00207.c` combines VLA stack adjustment, goto into a block, and short-circuit
  conditionals. These are not accepted as owned by adjacent pass-count movement
  alone; they need a timeout-focused probe before being merged into any Step 3
  owner bucket.

## Suggested Next

Perform lifecycle handoff away from the umbrella inventory to selected owner
`ideas/open/326_aarch64_variadic_hfa_floating_residual.md` before any
implementation edits. The next focused plan should narrow execution to the
current AArch64 aggregate/varargs ABI call-boundary evidence for `00140` and
`00204`, not to unrelated historical `00204` residuals already split or
parked.

## Watchouts

Do not weaken expectations, mark supported cases unsupported, or treat filename
clusters/pass-count movement as owner evidence. The two timeout cases remain
separate until a focused probe shows whether they share a concrete first bad
boundary with one of the non-timeout groups.

For the selected idea 326 handoff, do not reopen prior local/value-home,
fixed-formal entry, stdarg cursor, byval lane, MOVI zero-extension, or old
HFA-output repairs unless fresh generated-code evidence ties the current first
bad fact back to that exact owner. The selected route is the fresh composite
call-boundary move gap, especially the structured f128/q-register authority
diagnostic in `00204`.

## Proof

Ran classification proof:
`test -s test_after.log && rg '^The following tests FAILED:|c_testsuite_aarch64_backend_src_' test_after.log >/dev/null`

Result: proof passed. Existing proof log remains `test_after.log`; this packet
did not modify it.
