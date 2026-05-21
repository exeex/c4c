Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize the Current Call-Boundary Gap

# Current Packet

## Just Finished

Step 1 localized the current `00204` residual without code, test,
expectation, runner, CTest contract, `plan.md`, or source-idea edits.

First bad fact: `tests/c/external/c-testsuite/src/00204.c:220` expects
`printf("%.1f %.1f\n", fr_hfa12().a, fr_hfa12().b)` to print `12.1 12.2`,
but the fresh AArch64 backend run prints `0.0 12.2` in the `Return values:`
block before the later HFA/stdarg corruption and segmentation fault.

Owning backend boundary: this is between prealloc/regalloc's prepared
call-boundary plan and AArch64 lowering/publication, not frontend HFA return
scalarization and not the old compile-time call-boundary authority stop.
Focused prepared-BIR for `ret` records `%t99 = bir.fpext float %t98 to double`
as the first `fr_hfa12().a` lane, assigns `%t99` to callee-saved FPR `d20`,
preserves `%t99` across the second `fr_hfa12`, and records the `printf`
before-call move as `from_value_id=1915` to ABI arg1 `d0`. The same dump
records `%t104` for the second lane in `d21` with the before-call move to
`d1`.

Generated AArch64 contradicts that prepared plan. In `ret`, the first
`fr_hfa12` returns lanes in `s0/s1`; lane 0 is saved and reloaded, then
converted with `fcvt d8, s13`. After the second `fr_hfa12`, lane 1 is
converted with `fcvt d9, s13`. Immediately before `printf`, the generated
publication is `fmov d0, d20` and `fmov d1, d9`. Since no emitted instruction
has populated `d20` with the converted `%t99` value, `d20` is stale preserved
FPR state while the live converted lane is in `d8`; this explains why the
first printed lane is `0.0` while the second lane is `12.2`.

AST-backed lookup points the next repair at AArch64 call/cast lowering around
`lower_scalar_call_argument_producers`,
`emit_scalar_conversion_cast_to_register`, and call-boundary move publication
(`make_prior_preserved_call_argument_source` / `print_call_boundary_move`).
The prepared move facts are internally coherent; the stale publication appears
when AArch64 lowering materializes the FPExt producer into a scratch register
instead of the prepared home that the later call-argument move uses.

## Suggested Next

Repair the AArch64 FPExt-to-variadic-call argument publication gap for values
that cross an intervening call in callee-saved FPR homes: either make the
producer materialization write the prepared home (`d20` for `%t99`) or make
the call-boundary publication source the actual materialized register when
that is the authoritative live value.

The next packet can repair using existing coverage: the delegated `00204`
AArch64 backend CTest reproduces the first bad fact, and the existing
prepared-BIR focus/debug coverage already exposes the intended `%t99`/`d20`
and `%t104`/`d21` call-boundary facts. Focused unit coverage would be useful
as a regression guard if the repair touches generic scalar conversion
publication, but it is not required before attempting the repair.

## Watchouts

Do not classify the current `00204` failure as the old call-boundary authority
diagnostic: assembly generation succeeds, links, runs, and fails at runtime.

Do not chase the later HFA long-double/double/float stdarg corruption before
repairing or reclassifying the first bad fact. The first actionable mismatch
is the stale `d20` publication for `fr_hfa12().a`.

The stale `d20` is not evidence that prealloc failed to preserve `%t99`; the
prepared callsite explicitly says `%t99` is preserved in `d20`. The mismatch is
that AArch64 final lowering emitted the FPExt into `d8`, then trusted the
prepared `d20` home for the later call argument.

## Proof

Ran the delegated Step 1 proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00204_c$' > test_after.log 2>&1
```

Result: build succeeded (`ninja: no work to do`) and the selected CTest failed
as expected: `c_testsuite_aarch64_backend_src_00204_c` is `RUNTIME_NONZERO`
with `exit=Segmentation fault`. `test_after.log` is the preserved proof log
and records the first runtime mismatch as `0.0 12.2` in the HFA return-value
block.
