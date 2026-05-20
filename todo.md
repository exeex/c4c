Status: Active
Source Idea Path: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Relocalize The Current Byval Lane First Bad Fact

# Current Packet

## Just Finished

Step 1 relocalized the current `00204` byval lane evidence for
`fa_s1(s1)` against fresh generated artifacts.

Concrete source bytes: `struct s1 { char x[1]; } s1 = { "0" };` and
`fa_s1(s1)` in `tests/c/external/c-testsuite/src/00204.c` require the single
payload byte `0x30` (`'0'`) to reach the callee's first integer ABI lane. The
prepared BIR for `arg` still builds `%t2` as the byval object:
`%t2.global.aggregate.load.0 = bir.load_local i8 %t2.0, addr s1`,
`bir.store_local %t2.0, i8 %t2.global.aggregate.load.0`, then
`bir.call void fa_s1(ptr byval(size=1, align=1) %t2)`.

Destination ABI lane: AAPCS64 register argument lane `x0`/low `w0`; `fa_s1`
entry consumes it with `strb w0, [sp]`, so the caller must publish the payload
byte into low `w0` before `bl fa_s1`.

Current emitted callsite fact: the prepared callsite says
`arg0 bank=gpr from=register:x21 to=x0`, and the generated assembly still
materializes the temporary address with `add x0, sp, #928; bl fa_s1` after
storing the payload at `[sp, #928]`. That is address publication, not payload
lane publication.

Owner classification: this remains caller-side byval aggregate lane
publication because the semantic/prepared byval object has the right one-byte
payload, and the callee consumes low `w0`; the bad fact is at the call
boundary where the caller chooses the prepared object address as `x0` instead
of packing byte `0x30` into low `w0`.

## Suggested Next

Execute Step 2 from `plan.md`: repair the general small byval aggregate
payload publication path so register-passed byval objects publish payload
bytes into the selected AAPCS64 integer lane instead of forwarding the
prepared object address.

## Watchouts

- `fa_s2` is already the nearby positive guard: source bytes `"12"` are stored
  at `[sp, #929]`/`[sp, #930]`, then emitted as `ldrb w0`, `ldrb w9`, and
  `orr x0, x0, x9, lsl #8` before `bl fa_s2`; do not regress that payload
  packing while fixing size 1.
- Prior idea 328 repairs are still visible and must stay in bounds:
  `fa_s9`/`fa_s10` publish split payloads through `x0/w2[x0,x1]`, `fa1`/`fa2`
  preserve rounded register-to-stack placement, and `fa_s17` remains
  stack-passed with `add x0, sp, #1064; bl fa_s17`.
- Do not implement a named `00204` / `fa_s1` shortcut; the repair should make
  the one-byte case use the same semantic small-byval payload publication rule
  as the existing multi-byte register-lane cases.
- Do not reopen HFA/floating, stdarg cursor, MOVI, fixed-formal, or
  local/value-home owners without a new first-bad-fact record.
- The delegated proof passed in this environment, but the generated callsite
  evidence above still shows the stale address-passing fact. Treat the green
  runtime result as insufficient to retire this caller-lane issue.

## Proof

Ran the delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00204_c$' | tee test_after.log`

Result: build was up to date and `c_testsuite_aarch64_backend_src_00204_c`
reported pass. Proof log preserved at `test_after.log`.
