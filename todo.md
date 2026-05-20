Status: Active
Source Idea Path: ideas/open/346_aarch64_direct_call_argument_formal_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Narrow Publication Boundary

# Current Packet

## Just Finished

Step 1 localized the current representative AArch64 direct-call ABI
publication/consumption first bad facts using existing generated assembly under
`build/c_testsuite_aarch64_backend/src/*.c.s` plus focused
`--dump-prepared-bir` snapshots written under `/tmp`.

- `00140`: caller-side address or aggregate publication. Prepared `main`
  records direct same-module `f1` calls with `%lv.f` in `x20` and ABI moves to
  `x0`/`x1`/`x4`/`x5`, but emitted `main` never initializes `%lv.f` into
  `x20`; it calls `f1` with stale `x20`, causing the aggregate/address
  argument path to fault. Callee `f1` also consumes the byval aggregate from
  incoming stack slots and pointer formal from `x1`, so this case covers both
  caller aggregate/address publication and callee byval formal consumption.
- `00159`: callee-side scalar formal consumption. Prepared `myfunc` says `%p.x`
  is in incoming `x0`, but emitted `myfunc` computes `mul w0, w20, w20`.
  Caller `main` publishes immediates `3` and `4` into `w0` before `bl myfunc`,
  so the first bad fact is the direct callee reading stale `w20` instead of its
  incoming scalar formal.
- `00170`: caller-side overflow/stack publication. Prepared `printf` call
  records args 0-7 in `x0`-`x7` and arg8 literal `75` at outgoing
  `stack+0`; emitted code subtracts outgoing stack space but never stores
  `75` to that slot before `bl printf`, so the eighth variadic integer prints
  as `0`.
- `00175`: caller-side scalar and FP register publication. Prepared same-module
  calls require converted scalar temps in `x13` to move to `x0`, and FP temps
  in `d13` to move to `s0`; emitted `main` calls `charfunc`/`intfunc` with
  stale `w13` for the `99.0` conversions and calls `floatfunc` with stale
  `s13` for all three FP cases. The direct callees themselves consume incoming
  `w0`/`s0` normally, so the first bad fact is caller publication.
- `00218`: caller-side address publication. Prepared `main` records
  `%lv.convs` in `x21` and an ABI move to `x0` for `convert_like_real`; emitted
  `main` uses `mov x0, x21` before `bl convert_like_real`, but never publishes
  `&convs` into `x21`. `convert_like_real` then correctly reads from incoming
  `x0`, so the stale address comes from the caller.

Closed-owner comparison: these facts do not reopen 309 because the failing
calls are direct, not indirect `blr` preservation; do not reopen 311 because
the selected call-boundary printer and aggregate stack-copy diagnostics are
past assembly emission; do not reopen 336 because no correct return value is
being overwritten in an epilogue; do not reopen 337 because the first bad facts
exist before call entry/argument use rather than after a live callee-saved home
is clobbered across a call; and do not reopen 345 because none of the observed
failures is a `csel` result-publication consumer reload.

First implementation target: repair AArch64 direct-call consumption of
prepared `CallArgumentAbi` move/binding records in
`src/backend/mir/aarch64/codegen/calls.cpp`, starting with register
publication from prepared source homes to ABI argument registers and overflow
stack-slot publication before the branch. Keep the callee-side fixed-formal
entry consumption rule for incoming ABI registers as the paired target because
`00159` proves that caller publication alone will not cover the family.

## Suggested Next

Execute `plan.md` Step 2 against the AArch64 direct-call `CallArgumentAbi`
publication path: add focused coverage for prepared register/immediate/frame or
address source moves into `x0`-`x7`/`s0` and overflow stack slots, then repair
`calls.cpp` so the prepared ABI move/binding records are emitted before `bl`.
Include one focused callee-formal entry check for an incoming scalar formal
home that must be consumed from `x0`.

## Watchouts

- This owner is direct-call ABI argument/formal publication only.
- Do not special-case `x20`, `x21`, `w13`, `d13`, argument 8, or the five
  representative filenames; the repair must consume the prepared move/binding
  contracts generally.
- `00140` has an aggregate/byval dimension and should not be reduced to only
  a scalar register shuffle.
- `00170` needs outgoing stack-slot publication, not only register moves.
- `00159` is the explicit callee-side scalar formal consumption guardrail.
- Do not absorb pointer/null result publication, FP comparison
  materialization, aggregate/table memory corruption, libc/file/string
  residuals, semantic `lir_to_bir` admission, or timeout/output-storm buckets
  without fresh first-bad-fact evidence.
- Do not reopen closed ideas 309, 311, 336, 337, or 345 from counts or
  filename recurrence alone.
- Reject filename-specific, register-specific, expectation, unsupported,
  runner, timeout, proof-log, or CTest-registration changes.

## Proof

No build, CTest, or implementation proof was required for this
localization-only packet. Existing generated artifacts under
`build/c_testsuite_aarch64_backend/src/00140.c.s`,
`00159.c.s`, `00170.c.s`, `00175.c.s`, and `00218.c.s` were used. Focused
prepared dumps were generated with `./build/c4cll --dump-prepared-bir --target
aarch64-linux-gnu --mir-focus-function ...` into `/tmp/c4c_*_*.prepared.txt`;
no root-level proof logs were created or modified.
