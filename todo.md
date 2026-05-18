# AArch64 C-Testsuite Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select the Next Focused Repair Idea

# Current Packet

## Just Finished

Step 3 selected the next focused repair family from the refreshed AArch64
backend c-testsuite inventory evidence:
`/tmp/c4c_aarch64_backend_scan_212.log` and
`/tmp/c4c_aarch64_backend_scan_212.classified`.

Selected family: AArch64 backend string/global address materialization and
call-argument lowering for externally visible calls, especially `printf`.

Why this family is ready:

- It has a minimal runtime-visible repro: `src/00125.c` is only
  `printf("hello world\n")`, but generated assembly contains `mov x0, x13; bl
  printf` and no visible string-literal data/address materialization. This is a
  semantic backend defect, not a parser, runner, expectation, allowlist, or
  timeout-policy issue.
- The same failure shape repeats across multiple current cases instead of one
  named testcase: `src/00131.c` repeated string prints pass an uninitialized
  `x19`-shaped value, `src/00154.c` formats loaded struct fields with a bad
  format pointer, `src/00161.c` formats looped integers with a bad format
  pointer, `src/00211.c` formats macro integer expressions, and related
  mismatch cases include `src/00156.c`, `src/00158.c`, `src/00159.c`,
  `src/00160.c`, `src/00166.c`, `src/00168.c`, `src/00169.c`, `src/00183.c`,
  `src/00184.c`, `src/00190.c`, `src/00191.c`, `src/00192.c`, `src/00197.c`,
  `src/00201.c`, and `src/00206.c`.
- It is narrower and safer than the timeout family. `src/00132.c` is the only
  current timeout and is compounded by loop/local-store problems, but its
  generated code also shows bad string/call materialization (`mov x0, x13`,
  `mov x0, x19`, `mov x0, x20`). The focused idea should use `00132.c` only as
  supporting overlap evidence, not as the first acceptance proof.
- It is more tractable now than local-memory GEP/load/store because the first
  repro does not require arrays, pointer increments, Duff-device control flow,
  aggregates, or broad stack-slot semantics.
- It is more semantically valuable than machine-printer compare-branch/bitwise
  coverage because it repairs successfully assembled programs that currently
  produce missing or garbage output, while still leaving printer-only compile
  gates as a separate focused route.

Likely owner boundary for the follow-on idea:

- Own AArch64 backend lowering/emission needed to materialize string literals,
  global/static data addresses, and pointer-valued call arguments into the
  proper ABI argument registers before external calls.
- Include direct-call argument setup where the broken value is an address or
  formatted scalar argument flowing to `printf`/stdio.
- Exclude runner behavior, CTest registration, expectation rewrites,
  allowlists, timeout settings, parser/sema changes, and named-case shortcuts.
- Defer full aggregate ABI/HFA returns, variadic ABI completeness beyond
  ordinary integer/pointer argument placement, `_Generic`, wide characters,
  function-pointer casts, and unaligned/global-string crash cases unless they
  reduce to the same address-materialization owner after the basic family is
  repaired.

Deferred families and ambiguity:

- Machine-printer compare-branch and bitwise scalar opcode coverage remains
  tractable and well represented (`src/00030.c`, `src/00076.c`, `src/00101.c`,
  `src/00214.c` for compare-branch; `src/00027.c`, `src/00028.c`,
  `src/00029.c`, `src/00151.c`, `src/00208.c` for bitwise), but it is a
  compile-time printability route and should not be mixed with runtime data
  semantics.
- Local-memory GEP/load/store admission remains important
  (`src/00143.c`, `src/00157.c`, `src/00176.c`, `src/00181.c`, `src/00182.c`,
  `src/00205.c`, `src/00209.c`, plus store/load cases `src/00046.c`,
  `src/00140.c`, `src/00216.c`, `src/00218.c`), but the family is broader and
  more entangled with arrays, aggregates, pointer increments, and control flow.
- Plain scalar arithmetic/control-flow `RUNTIME_NONZERO` exits are numerous
  but not yet one clean owner; keep them deferred until a tighter family split
  is available.
- The timeout case `src/00132.c` should remain deferred from first proof
  selection because it combines bad string/call materialization with loop
  induction/local-store issues and can produce very large output.

## Suggested Next

Ask the plan owner to create and activate a focused idea for AArch64 backend
string/global address materialization plus call-argument lowering for external
calls, using `src/00125.c`, `src/00131.c`, `src/00154.c`, `src/00161.c`, and
`src/00211.c` as representative starter cases. The first implementation packet
should prove a tiny non-timeout subset before expanding to related stdio/data
mismatch cases.

## Watchouts

- This umbrella route is planning and classification only; do not implement
  compiler, runner, or test expectation changes here.
- `FRONTEND_FAIL` is a runner bucket for compiler nonzero. Most sampled cases
  are backend route failures after the AArch64 assembly path starts.
- `src/00132.c.bin.run-output` is very large because the timeout generated
  repeated garbage output; avoid opening it without size-limited tools.
- Numeric `RUNTIME_NONZERO` exits often mean assertion-return miscompiles; bus
  error and segmentation fault exits point at invalid address/data/ABI lowering.
- Split and switch to a focused idea before implementation work begins.
- Do not use `src/00132.c` as the initial proof case; it is useful overlap
  evidence but remains timeout-sensitive and loop/local-store-compounded.
- Reject fixes that only special-case `printf`, one string literal spelling, or
  one c-testsuite filename. The repair needs to materialize address-valued data
  and place call arguments according to the backend ABI path.

## Proof

Evidence used:

- `/tmp/c4c_aarch64_backend_scan_212.log`
- `/tmp/c4c_aarch64_backend_scan_212.classified`
- representative sources under `tests/c/external/c-testsuite/src/`
- representative generated assembly/run-output under
  `build-aarch64-scan/c_testsuite_aarch64_backend/src/`

No broad scan was rerun. No root-level proof log was created or modified for
this evidence-only route-selection packet.
