Status: Active
Source Idea Path: ideas/open/293_aarch64_side_effect_control_value_publication_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair Side-Effecting Expression Result Publication

# Current Packet

## Just Finished

Step 2 implemented the smallest coherent side-effecting-expression publication
slice for `src/00202.c`: prepared scalar ALU frame-slot result publication plus
rematerializable-immediate local-store publication.

The private scalar ALU fallback path now accepts prepared stack-slot result
homes by choosing the existing spill scratch register and carrying
`result_stack_offset_bytes` into the `ScalarAluRecord`. The scalar ALU printer
now appends the same stack publication store for computed `mul`, signed
`div`/`rem`, and unsigned-reduction results that add/sub already used.

The prepared memory store path now accepts named rematerializable-immediate
values when the value home and storage plan agree on the immediate, so
assignment/local-store publication can materialize the immediate into the
destination local slot.

Generated-code evidence for `src/00202.c`:

```asm
mov w9, #2
mul w9, w13, w9
str w9, [sp, #8]
ldr w9, [sp, #8]
str w9, [sp]
movz w9, #63
str w9, [sp, #4]
```

`src/00202.c` now passes: the first observable line consumes the published
`bob *= 2` value and the second line consumes the published `jim = 60 + 3`
rematerializable immediate.

## Suggested Next

Stay in Step 2 and sample the remaining `src/00164.c` assignment-like forms to
separate same-owner side-effect publication gaps from closed-owner-looking call,
comparison, and control-value symptoms. Do not start the Step 3
`src/00183.c` BlockEntry/select-materialization repair until the supervisor
chooses that packet.

## Watchouts

- Do not fold the `src/00183.c` conditional-expression gap into the next Step 2
  packet; it is still the Step 3 block-entry/out-of-SSA
  `select_materialization` half of the publication boundary.
- The current `00202` store uses the existing prepared stack offset carried by
  scalar ALU records. This matches the existing add/sub publication mechanism,
  but a later broader cleanup may still need a single frame-slot-address helper
  shared with memory/call publication so raw prepared offsets do not proliferate.
- `src/00164.c` still shows closed-owner-looking symptoms around call
  arguments and compare/logical values. Treat them as missing publication from
  prepared values unless generated-code evidence proves a closed owner regressed.
- Keep pointer/address-heavy cases `src/00172.c` and `src/00217.c` deferred
  unless the same scalar side-effect/control publication primitive owns them.
- Do not touch expected outputs, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or build/test
  infrastructure.

## Proof

Ran the delegated proof exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00183|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

Result: failed overall, 0/3 passing, but `00202` materially advanced. Current
status for the first run:

- `00202`: still `RUNTIME_MISMATCH`, but first line now prints
  `jim: 21, bob: 42`; remaining mismatch is the later immediate assignment
  publication for `jim`.
- `00164`: still `RUNTIME_MISMATCH`; not accepted as repaired in this packet.
- `00183`: still `RUNTIME_MISMATCH`; unchanged Step 3
  `select_materialization`/BlockEntry gap.

After extending the same slice, reran the delegated proof exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00183|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

Result: failed overall, 1/3 passing. `00202` passed. `00164` and `00183`
remain `RUNTIME_MISMATCH` in the expected remaining buckets.

Stale-process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

Result: no generated runtime process remained.

Shared-code regression check:

```sh
ctest --test-dir build -R '^backend_aarch64_' -j 8 --output-on-failure > /tmp/c4c_aarch64_side_effect_step2_backend_aarch64.log 2>&1
```

Result: passed, 27/27.

Additional check: `git diff --check` passed.

Proof logs:

- `test_after.log`
- `/tmp/c4c_aarch64_side_effect_step2_backend_aarch64.log`
