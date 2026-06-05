Status: Active
Source Idea Path: ideas/open/112_aarch64_00216_00204_post_closure_regression.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Compare Against The Baseline-Clean Closure Point

# Current Packet

## Just Finished

Step 2 `Compare Against The Baseline-Clean Closure Point` narrowed the
post-`23213dbe4` suspect range using the named commit diffs plus current HEAD
AArch64 dumps.

History evidence:

- `da0e90842` is the likely implementation commit to investigate next. It
  tightened call aggregate source selection so frame-slot address arguments
  depend on explicit prepared address materialization, replacing the broader
  local aggregate address compatibility path.
- `044513d31` only added/strengthened tests for that contract.
- `fa3826e14` added prepared store-source publication records and printer
  output; current dumps show those records are present and available, but the
  change is mostly observation/visibility unless later code consumes the new
  publication facts.
- `9855aee9a` only added printer coverage for the new dump fields.

Dump evidence:

- `00204/stdarg` contains repeated byval aggregate calls to `myprintf` and
  prepared call arguments sourced from frame slots, including stack-passed
  variadic aggregate traffic. It has `frame_alignment=16`, so the failure is
  not primarily explained by whole-frame alignment.
- `00216/foo` and `00216/main` contain many address-like calls through local
  and global aggregate pointers. `foo` reports `frame_alignment=8`, but the
  emitted frame size is rounded to a 16-byte stack adjustment; this may be a
  secondary watchout rather than the shared root.
- `00216/main` call planning uses prior-preservation source selections for
  repeated global pointer arguments and stack-slot preservation for `@phdr`.
- Store-source rows are available in both dumps; many local byte stores in
  `00216/foo` have `source=<none>` because they originate from immediates, not
  from missing publication authority.

Likely next subsystem/semantic rule: `src/backend/prealloc/call_plans.cpp`
call argument source selection after `da0e90842`, specifically the rule that
maps frame-slot or computed-address aggregate arguments to
`FrameSlotAddress`, `LocalFrameAddressMaterialization`, or byval register-lane
sources from explicit prepared address materialization. The two failures appear
related through call aggregate address planning, with `00204` additionally
covering variadic/byval aggregate handling. Publication authority looks
instrumental evidence, not the primary root, and no separate root is proven yet.

## Suggested Next

Execute a narrow Step 3 code-changing packet in `call_plans.cpp`: audit and
repair the `da0e90842` frame-materialization-required path so call aggregate
address planning still accepts valid AArch64 local/global aggregate arguments
and byval aggregate lanes only when structured authority is present.

## Watchouts

- Do not weaken, skip, or reclassify `00216` or `00204`.
- Do not add filename-shaped or expected-output-shaped special cases.
- Keep `fa3826e14` printer/store-source differences out of the repair path
  unless they prove a construction-time side effect.
- Preserve the fail-closed contract from `044513d31`; do not restore the old
  broad compatibility search unconditionally.
- `00204` has extensive byval and variadic aggregate traffic, so the repair
  must cover semantic byval lane source selection, not just plain pointer
  arguments.
- `00216/foo` reports `frame_alignment=8` in prepared output even though the
  emitted stack adjustment is 16-byte rounded; keep this as a follow-up signal
  if the call-planning repair does not explain `00216`.

## Proof

Ran:

```sh
git show --stat --oneline --decorate --name-only da0e90842
git show --stat --oneline --decorate --name-only 044513d31
git show --stat --oneline --decorate --name-only fa3826e14
git show --stat --oneline --decorate --name-only 9855aee9a
git show --no-ext-diff --unified=80 da0e90842 -- src/backend/prealloc/call_plans.cpp src/backend/prealloc/stack_layout/analysis.cpp src/backend/prealloc/stack_layout/coordinator.cpp
git show --no-ext-diff --unified=80 044513d31 -- tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp tests/backend/bir/backend_prepared_lookup_helper_test.cpp
git show --no-ext-diff --unified=80 fa3826e14 -- src/backend/prealloc/module.hpp src/backend/prealloc/prealloc.cpp src/backend/prealloc/prepared_printer.cpp src/backend/prealloc/prepared_printer/private.hpp src/backend/prealloc/prepared_printer/select_chains.cpp src/backend/prealloc/publication_plans.cpp src/backend/prealloc/publication_plans.hpp
git show --no-ext-diff --unified=80 9855aee9a -- tests/backend/bir/backend_prepared_printer_test.cpp
./build/c4cll --dump-bir --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00204.c > /tmp/c4c_00204_dump_bir.txt 2>&1
./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function stdarg tests/c/external/c-testsuite/src/00204.c > /tmp/c4c_00204_stdarg_prepared_bir.txt 2>&1
./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function foo tests/c/external/c-testsuite/src/00216.c > /tmp/c4c_00216_foo_prepared_bir.txt 2>&1
./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function main tests/c/external/c-testsuite/src/00216.c > /tmp/c4c_00216_main_prepared_bir.txt 2>&1
./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00204.c -o /tmp/c4c_00204.s > /tmp/c4c_00204_asm_cmd.txt 2>&1
./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00216.c -o /tmp/c4c_00216.s > /tmp/c4c_00216_asm_cmd.txt 2>&1
```

Result: observation-only packet completed. Large scratch evidence is under
`/tmp`; no root-level proof log was written or modified for this packet.
