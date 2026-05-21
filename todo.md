Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Buckets

# Current Packet

## Just Finished

Executed Step 2 residual-bucket classification after idea 371. No
implementation files were modified and no new proof run was required; the
classification used the existing backend-regex `test_after.log`, generated
AArch64 under `build/c_testsuite_aarch64_backend/src/`, external C sources
under `tests/c/external/c-testsuite/src/`, and open/closed idea scope names.

Classified residual buckets:

- `00163`: pointer-valued local assignment/publication. First bad fact is the
  final `b = &(bolshevic.b)` path: generated AArch64 keeps using the old local
  pointer home for `&a`, so the final `*b` prints `42` instead of the global
  field value `34`. Owner candidate is address-valued pointer publication into
  scalar local homes, not struct field writeback.
- `00174`: scalar FP constant/arithmetic publication. The first `%f` values are
  emitted from stale/zero FP carriers before constants are materialized, while
  later comparisons materialize their FP constants. Owner candidate is scalar
  FP expression/constant result publication to consumers and varargs, not HFA
  aggregate stdarg.
- `00182`: static local/global digit-array selected store/readback. Current
  AArch64 no longer shows the old `% 10` quotient/divisor clobber as the first
  bad fact; the generated `udiv`/`msub` shape is present. Runtime output is an
  all-zero LED display, pointing at static local `d[]` lane store/readback or
  selected global/static aggregate publication.
- `00187`: external call return publication. The first bad fact is after
  `fread`: the return value in `x0` is not published to the compare home, and
  generated AArch64 compares a stale stack slot against `6`, taking the
  "couldn't read fred.txt" branch. Owner candidate is libc/external call scalar
  return publication before compare/control consumers, not file I/O semantics.
- `00200` timeout: left-shift result type/promotion family. Source is the
  shift-type probe; generated code reaches a large check cascade and the
  backend-regex run times out. Treat as a timeout classification bucket until a
  focused run identifies the first failing `check`.
- `00205`: global aggregate initializer plus dynamic selected global loads.
  Source is a large initialized `cases[]` struct array; generated/BIR shape
  contains dynamic selected loads from `@cases` lanes and the runtime mismatch
  occurs in the case-printing loop. Owner candidate is global aggregate
  initializer/layout and selected aggregate load publication, not scalar
  division.
- `00207` timeout: dynamic stack/VLA alloca with goto/stackrestore. Source
  contains `llvm.stacksave`/dynamic alloca/stackrestore-like control in `f1`
  plus goto-heavy paths; generated AArch64 shows dynamic stack adjustment and
  label flow before the timeout. Keep separate from short-circuit idea 370
  unless a focused run proves the `f3` short-circuit path is first bad.
- `00216`: aggregate initializer/compound literal/local copy publication.
  Runtime segfaults before useful output; generated `foo` contains many local
  aggregate initializer/copy paths with reloads from stack homes that appear
  unmaterialized for the current aggregate state. Owner candidate is aggregate
  initializer/layout/copy and compound-literal publication, not selected
  snapshot writeback from idea 371.
- `00218`: unsigned enum bit-field layout/address publication. Runtime prints
  `unsigned enum bit-fields broken`; generated setup stores through a small
  stack offset and then passes an uninitialized-looking local address carrier
  into `convert_like_real`. Owner candidate is bit-field storage layout,
  address publication, and unsigned/zero-extension for enum bit-fields.

## Suggested Next

For Step 3, pick one focused semantic owner rather than repairing inside this
umbrella. Suggested ranking:

1. `00163` pointer-valued address publication: smallest concrete first bad
   fact, likely a tight focused packet around address-of global/member values
   assigned to scalar pointer locals.
2. `00187` external call return publication: also concrete, with a clear
   missing `x0` return-to-compare publication boundary after `fread`.
3. `00182` static local selected array store/readback: good representative for
   static/global aggregate lane publication after the old udiv/rem issue was
   removed.
4. Aggregate initializer/layout family: `00205` and `00216` are high-impact but
   broader and should likely become a separate source idea before repair.
5. Remaining singleton buckets: `00174` scalar FP publication, `00218` enum
   bit-fields, then timeout-only `00200` and `00207` after focused timeout
   localization.

## Watchouts

This umbrella is for classification and focused-owner selection only. Do not
implement fixes under idea 295. Do not reopen closed owners from counts alone;
require generated-code, diagnostic, timeout, or proof evidence that contradicts
their closure boundary. The current classifications treat idea 371 as complete:
`00157`, `00176`, and `00183` are no longer residuals, and `00216`/`00205` are
not the same selected-snapshot writeback shape without fresh evidence. Do not
route `00182` back to the old unsigned div/rem bucket unless a new first bad
fact reintroduces quotient/divisor clobbering. Do not classify `00174` as HFA
or variadic aggregate ABI until scalar FP publication is disproved.

## Proof

No new CTest proof was requested for Step 2 and none was run. Classification
used the Step 1 backend-regex proof log:

```sh
cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1
```

Existing result: build completed; CTest selected 356 tests, 347 passed, 9
failed. Failures were `00163`, `00174`, `00182`, `00187`, `00200` timeout,
`00205`, `00207` timeout, `00216`, and `00218`. Proof log is
`test_after.log`. Inspection commands read `test_after.log`, generated
`build/c_testsuite_aarch64_backend/src/*.c.s` for the residuals, the matching
external C sources, and open/closed idea scope names; no implementation files
were edited.
