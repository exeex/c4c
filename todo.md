Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Reconcile With Remaining Runtime Buckets

# Current Packet

## Just Finished

Step 2 from `plan.md` compared the post-308 `00189.c` runtime segfault against
likely runtime call-boundary neighbors from the accepted umbrella history and
generated AArch64 artifacts. No fresh tests were run.

Inspected as runtime nonzero/crash neighbors:

- `00208.c`: chosen because idea 300 records it as a current
  `RUNTIME_NONZERO exit=Segmentation fault` residual and the source has direct
  calls feeding short-circuit control flow plus `printf`. The generated
  `main` uses direct `bl f1char`, direct `bl f1int`, and direct `bl printf`;
  it does show bad value retention after the second call (`f1int` result is
  copied to `x20` while the later branch still tests stale `x13`), but it has
  no indirect callee setup, no nested call result used as a later vararg, and
  no function-pointer callee preservation like `00189.c`.
- `00181.c`: chosen because idea 301 records it as a current runtime segfault
  residual and the source is call-heavy recursive Hanoi code. The generated
  `Hanoi` path shows direct-call argument shuffle damage around `Move` and
  recursive `Hanoi` calls (`mov x1, x3` followed by `mov x3, x1` style
  sequential copies), so it is a plausible direct multi-argument call shuffle
  bucket. It does not share `00189.c`'s indirect function-pointer callee plus
  vararg/nested-return setup.
- `00207.c`: chosen because idea 299 records it as a segfault residual and the
  source combines VLA/goto control flow with nested `printf` expressions.
  Generated `f1` calls `printf` through direct `bl printf` after `mov x0,
  x21`, where `x21` is not a materialized string pointer; the likely owner is
  dynamic-stack/goto local value materialization, not the `00189.c` indirect
  call boundary.
- `00215.c`: chosen because idea 299 records it as a segfault residual and the
  source is a direct-call/goto/switch stressor with many `printf` calls.
  Generated `main` and the `kb_wait_*` callees use direct calls and ordinary
  string arguments; no matching indirect callee or nested return-as-vararg
  corruption was found.
- `00173.c` and `00214.c`: chosen because idea 301 records them as runtime
  nonzero/segfault residuals and they contain `printf` around memory/control
  constructs. Their generated snippets point at string/pointer/store/control
  materialization issues and direct calls, not function-pointer call lowering.

Inspected as runtime mismatch neighbors:

- `00182.c`: chosen because idea 307 parks it as a runtime mismatch and the
  source is direct-call heavy (`print_led`, line printers, `printf`/`fprintf`).
  The generated code still has direct-call argument corruption such as passing
  `x1` through `mov x1, x1` before `bl topline`, so it fits a direct pointer
  argument preservation/move bucket. It does not use an indirect callee or a
  nested call result as a vararg.
- `00200.c`: chosen because idea 296 parks it as a runtime mismatch and the
  source is a `printf`/helper-call shift-type checker. Generated `check`
  directly calls `printf` and visibly aliases varargs (`mov x1, x0`, then
  `mov w2, w1`, `mov w3, w2`), which is a direct-call vararg argument
  materialization bug. It is related to `00189.c` only at the broad vararg
  boundary level and lacks the indirect callee/nested-call preservation shape.
- `00218.c`: chosen because idea 299 parks it as a runtime mismatch and the
  source calls `convert_like_real(&convs)` before a possible `printf`.
  Generated `main` passes `x21` to `convert_like_real` instead of the local
  aggregate address, so it is a direct-call address-of-local argument
  preparation bucket, not the `00189.c` function-pointer callee case.
- `00035.c`, `00151.c`, and `00176.c`: checked as parked runtime
  nonzero/mismatch names from the idea history. They do not contain the
  relevant indirect-call/vararg/nested-call shape; their generated snippets
  point at scalar boolean/global initializer/output semantics instead.

Result: a shared semantic owner is not ready from Step 2 evidence. The nearest
neighbors (`00181.c`, `00182.c`, `00200.c`, and `00218.c`) prove there are
other call-boundary bugs, but grouping them with `00189.c` would produce an
overbroad "all call lowering" owner. `00189.c` remains a singleton focused
candidate for indirect function-pointer callee and argument preservation
across nested call setup, with direct-call vararg forwarding in `fred` as a
secondary symptom.

## Suggested Next

Step 3 Suggested Next: choose between a narrow singleton split for `00189.c`
or one more classification probe over direct-call argument buckets. If the
supervisor accepts a singleton owner, split `00189.c` around indirect
function-pointer callee/argument preservation across nested call setup, with
boundaries excluding direct multi-argument shuffle (`00181.c`/`00182.c`),
direct vararg aliasing (`00200.c`), and address-of-local direct-call argument
preparation (`00218.c`). If a non-singleton owner is required, classify those
direct-call neighbors separately before splitting.

## Watchouts

- This umbrella packet is classification only; do not implement repairs here.
- Do not reopen closed owners 285 through 308 from counts alone.
- Do not change expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or proof-log policy.
- Treat idea 308 as still closed unless generated assembly returns to direct
  non-PIC relocation forms for externally binding data symbols.
- The likely `00189.c` crash site remains `blr x21` in `main`, because `x21`
  is not the loaded `fprintfptr` callee. The argument setup is also invalid:
  `stdout` is not placed in `x0`, `.str1` is not placed in `x1`, and the
  nested `(*f)(24)` result is not preserved as the later integer vararg.
- Direct-call neighbors should not be folded into a `00189.c` split unless the
  owner is intentionally broadened to call-argument parallel-copy/value
  preservation. Current evidence says that would mix several subfamilies.
- Do not run a fresh broad backend regex command unless the supervisor
  delegates that proof command with timeout and stale-process guidance.

## Proof

No build or tests were run, per packet instruction. Classification used current
focused `test_before.log`, accepted broad inventory history recorded in
`ideas/open/295_backend_regex_failure_family_inventory.md`, generated assembly
under `build/c_testsuite_aarch64_backend/src/`, and source files under
`tests/c/external/c-testsuite/src/`. No proof logs were modified.
