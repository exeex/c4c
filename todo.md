Status: Active
Source Idea Path: ideas/open/643_rv64_scalar_call_boundary_freshness_after_call.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace The Freshness Boundary

# Current Packet

## Just Finished

Step 2 traced the `src/ipa-sra-2.c` scalar freshness boundary from the refreshed
object/runtime artifacts without changing implementation files.

Artifacts:
- case log: `build/rv64_gcc_c_torture_backend/src_ipa-sra-2.c/case.log`
- work dir: `build/rv64_gcc_c_torture_backend/src_ipa-sra-2.c`
- C4C object: `build/rv64_gcc_c_torture_backend/src_ipa-sra-2.c/c4c.o`
- C4C binary: `build/rv64_gcc_c_torture_backend/src_ipa-sra-2.c/c4c.bin`
- clang control binary:
  `build/rv64_gcc_c_torture_backend/src_ipa-sra-2.c/clang.bin`
- prior prepared dump:
  `build/agent_state/634_step2_ipa-sra-2.prepared.txt`
- prior C4C/clang disassembly:
  `build/agent_state/634_step5_ipa_sra_2_c4c_bin_objdump.txt`,
  `build/agent_state/634_step5_ipa_sra_2_clang_bin_objdump.txt`

Freshness boundary:
- Stale/prepared scalar value: `%p.argc` in `main`, value id `6`, home
  `register a0`.
- Call boundary: `main` block `entry` instruction index `0`,
  `bir.call ptr calloc(i64 1, i64 40)`.
- Post-call consumed home/register: the compare for `%t2 = bir.sgt i32
  %p.argc, 2000` is lowered as C4C instructions `mv t3,a0; li t4,2000;
  slt s1,t4,t3` immediately after the `calloc` call, so it consumes post-call
  `a0`.
- Call-clobber facts: the prepared call plan for `calloc` records a result
  from `register:a0` to `%t1`/`s1` and caller-clobber entries for scratch
  homes such as `t0`, but `%p.argc` remains recorded as direct home `a0` with
  no preservation, republication, or rematerialization authority across the
  call.
- Expected fresh source: preserve or republish the incoming scalar parameter
  before the call, then use that fresh preserved value for `%t2`; clang's
  control binary stores incoming `argc` at `-24(s0)` and reloads it after
  `calloc` before comparing against `2000`.
- Runtime consequence: C4C uses the `calloc` return pointer as the first
  argument to `foo`; the pointer is almost always greater than `2000`, so
  `foo` takes the large-offset `agg->big.data[999999]` path against a 40-byte
  allocation and segfaults.

Candidate-owner classification:
- object relocation: ruled out; `readelf` shows ordinary `R_RISCV_CALL_PLT`
  relocations for `calloc`, `foo`, and `free`, and link succeeds.
- large-offset local memory: not first owner; idea 634 already made the
  3999996-byte access materializable, and the crash is reached only after the
  wrong `foo` flag is computed.
- stack layout: not first owner; local homes for `agg` and `r` are in-bounds
  frame slots, and the incorrect compare happens before those post-call local
  reloads matter.
- branch/control-flow: not first owner; `foo` branches consistently on its
  first argument, but `main` supplies the wrong boolean because `%p.argc` was
  not freshened after `calloc`.
- runtime-library behavior: ruled out as first owner; clang's binary exits `0`
  with the same libc calls, while C4C passes a stale-derived flag to `foo`.
- scalar call-boundary freshness: active owner. The complete-authority family
  is scalar parameter/register-home use after an intervening call, where the
  value must be preserved, republished, or rematerialized before a post-call
  consumer may read it.

## Suggested Next

Execute Step 3: implement one complete-authority scalar call-boundary
freshness path for register-homed scalar parameters or values used after an
intervening call. The repair should publish a fresh source by preservation,
republication, or rematerialization before the post-call consumer, and must keep
fail-closed behavior for values that lack that authority.

## Watchouts

- Reject named-case fixes for `src/ipa-sra-2.c`, `calloc`, `foo`, `argc`, or
  physical register `a0`; the repair family is generic scalar freshness across
  call boundaries.
- Do not treat a pre-call register home as fresh after a call without explicit
  preservation, republication, or rematerialization authority.
- Do not weaken call-clobber, preservation, republication, rematerialization,
  expectation, unsupported-marker, allowlist, timeout, or pass/fail accounting
  behavior.
- The current trace proves the stale `%p.argc` consumer is before the `foo`
  call. The later `free` call is not the first owner for this row because the
  C4C binary already passes the wrong flag to `foo`.

## Proof

No new root proof command was run for this evidence-only packet, per delegated
instructions. The existing Step 1 proof log remains `test_after.log`; Step 2
used the existing object, binary, case-log, prepared-dump, and disassembly
artifacts listed above.
