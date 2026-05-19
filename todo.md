Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify 00189 Runtime/Call-Argument Crash

# Current Packet

## Just Finished

Step 1 from `plan.md` classified the post-308 `00189.c`
`RUNTIME_NONZERO exit=Segmentation fault` from existing evidence only. The old
extern-data-symbol PIC failure remains absent: `test_before.log` no longer
shows `R_AARCH64_ADR_PREL_PG_HI21` or `stdout@@GLIBC_2.17`, and current
`00189.c.s` uses `:got:stdout` / `:got_lo12:stdout`.

The generated runtime call sequence is a call-boundary corruption shape, not a
reopened idea 308 relocation shape. In `fred`, `printf("yo %d\n", p)` loads
`.str0` into `x0` but then copies `w0` into `w1`, so the integer vararg is the
format-string address low bits instead of parameter `p`. In `main`, the
compiler loads `fprintfptr` into `x20` and materializes `stdout`, but then
overwrites the `stdout` value while preparing `f`, calls `(*f)(24)`, and does
not preserve the returned `42` for the later vararg. The final indirect
`fprintfptr(stdout, "%d\n", ...)` setup is badly scrambled: `stdout` is loaded
into `x9` instead of `x0`, `.str1` is never moved into `x1`, `x1` receives the
function pointer from `x20`, `w2` is loaded from the saved `x21` stack slot,
and the call branches through `x21` even though the callee was held in `x20`.

Against the parked runtime/call-boundary buckets, `00189.c` is a strong
focused owner candidate for indirect-call argument/callee preservation across
nested call setup, with a varargs boundary symptom. It is not ready to split
from this single artifact alone unless the supervisor accepts a singleton
owner; otherwise it needs Step 2 comparison against neighboring runtime
nonzero/mismatch/crash cases to prove whether this is a broader call-boundary
lowering bucket.

## Suggested Next

Execute Step 2/3 by comparing `00189.c` against likely call-boundary runtime
neighbors before splitting: inspect generated assembly for parked
runtime-nonzero, runtime-mismatch, and crash residuals that involve indirect
calls, varargs, nested calls used as call arguments, or function-pointer
callees. If no neighbor shares the same callee/argument preservation failure,
record the missing evidence and either keep `00189.c` parked as a singleton
candidate or ask the supervisor whether a focused singleton owner is acceptable.

## Watchouts

- This umbrella packet is classification only; do not implement repairs here.
- Do not reopen closed owners 285 through 308 from counts alone.
- Do not change expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or proof-log policy.
- Treat idea 308 as still closed unless generated assembly returns to direct
  non-PIC relocation forms for externally binding data symbols.
- The likely crash site is the `blr x21` in `main`, because `x21` is not the
  loaded `fprintfptr` callee. The argument setup is also invalid, so a repair
  owner should be framed around general call lowering/preservation, not only
  the final branch register.
- `fred` also demonstrates bad argument forwarding into `printf`; nearby
  probes should distinguish direct-call vararg argument placement from the
  nested indirect-call/callee preservation bug in `main`.
- Do not run a fresh broad backend regex command unless the supervisor
  delegates that proof command with timeout and stale-process guidance.

## Proof

No build or tests were run, per packet instruction. Classification used current
`test_before.log`, `tests/c/external/c-testsuite/src/00189.c`, and
`build/c_testsuite_aarch64_backend/src/00189.c.s`; no proof logs were modified.
