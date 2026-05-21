Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair General Call-Boundary Preparation

# Current Packet

## Just Finished

Step 3 attempted a generalized AArch64 variadic small byval aggregate GPR-lane
publication repair for the `00204` second `stdarg` call. AST-backed lookup was
run before editing against the call-plan and AArch64 call-boundary files.

The useful confirmed facts are:
the prepared call plan and grouped trace already know the `.str50` `%9s`
arguments consume two contiguous GPR lanes each (`x2,x3`, `x4,x5`, `x6,x7`),
but emitted AArch64 publishes only the first lanes (`x2`, `x4`, `x6`) before
the call. The source BIR is byval size 16 for these `%9s` aggregates, so the
missing lanes are a call-boundary publication/preparation gap rather than a
format-string or `va_arg` cursor issue.

An attempted synthetic lane fallback in the AArch64 call-boundary path was
reverted because proof showed it selected the wrong aggregate lane storage for
nearby same-feature cases, regressed `backend_aarch64_instruction_dispatch`,
and still did not advance `00204` past the second `stdarg` first bad fact. No
implementation changes from that failed route remain.

## Suggested Next

Repair the authoritative call-boundary move/source-selection layer that should
consume the existing prepared call-plan width and aggregate lane provenance.
The next route should explain why the existing aggregate register-lane
publication path is not used for the `.str50` register-passed byval arguments
even though the prepared call plan has width 2 spans.

## Watchouts

Do not synthesize residual lanes by slot-name suffix or callsite-local stack
offset guessing. The reverted fallback loaded `x3`/`x5`/`x7` from concrete
stack offsets that looked plausible for `.str50` but corrupted neighboring
same-feature calls. Direct `ldr xN, [sp,#offset]` synthesis also needs the
existing address-materialization rules for large or unaligned offsets.

Keep `backend_aarch64_instruction_dispatch` in the focused subset; it caught
the size-1 byval aggregate publication regression introduced by the attempted
fallback.

## Proof

Ran the delegated Step 3 proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00140_c|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1
```

Result: build succeeded; selected `backend_.*` tests and `00140` passed, but
`c_testsuite_aarch64_backend_src_00204_c` still failed with `RUNTIME_NONZERO`.
The preserved failing output still reaches the first `stdarg` line, then the
second line prints only `lmnopqr ABCDEFGHI ABCDEFGHI` before flowing directly
into `HFA long double:`. `test_after.log` is the preserved proof log.
