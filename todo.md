Status: Active
Source Idea Path: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Compare Caller Publication Against Callee Consumption

# Current Packet

## Just Finished

Post-repair Step 2 compared caller publication against callee consumption and
classified the remaining `va-arg-13.c` abort as a caller
publication/materialization boundary.

The explicit prepared call-argument value-publication facts are present and
specific:

- call inst 9 publishes payload `%t7.memcpy.copy.0` into argument object `%t7`
  at object slot `#6`, stack offset `24`
- call inst 16 publishes payload `%t14.memcpy.copy.0` into argument object
  `%t14` at object slot `#7`, stack offset `32`

Both payloads come from `source_load_local=yes` store-source records. The
problem is the effective source being loaded: prepared memory access records
materialize `%t7.memcpy.copy.0` / `%t14.memcpy.copy.0` from frame slot `#4`
for `%lv.state.8`, while the RV64 `va_start` helper publishes the active
va_list word to helper destination slot `#13`, stack offset `128`. Runtime then
shows the first argument object contains `0x0000ffff9eb4d860` instead of the
saved variadic argument area pointer, so `dummy` loads the int from
`0x0000ffff9eb4d860` and aborts.

For the current C4C same-module route, `dummy` consumes `a0` as a pointer to a
copied va_list object (`ld s2,0(a0)`, then `lw s1,0(s2)`). Therefore the
caller should pass the copied object address, but that object's first word must
be the save-area pointer payload. Clang's RV64 lowering passes the save-area
pointer directly as scalar `va_list`; that broader ABI difference remains a
watchout, but it is not the immediate same-C4C abort owner.

Analysis log:
`build/agent_state/392_postrepair_step2_analysis.log`.

## Suggested Next

Execute Step 3 from `plan.md`: trace and repair the prepared/RV64 object
materialization owner for `load_local %lv.state.8` after `llvm.va_start`.
The value used for `%t7.memcpy.copy.0` and `%t14.memcpy.copy.0` must come from
the active va_start-published va_list word, not the stale/local frame slot that
currently yields the caller stack pointer.

## Watchouts

- Keep the caller argument object address separate from its contents. The
  object-address route is selected; the bad value is the word stored inside the
  object.
- Do not regress the Step 3 equality guard for explicit call-argument
  value-publication facts. The next repair should not infer payload authority
  from generic `StoreLocalPublication` alone.
- The immediate owner is not the old `mv t1,s1` overwrite; that pattern is gone.
- Keep the broader clang/RV64 scalar-`va_list` ABI difference visible, but do
  not broaden Step 3 unless the materialization repair proves insufficient.

## Proof

Delegated proof/evidence command run:
`mkdir -p build/agent_state && { echo 'Post-repair Step 2 caller/callee comparison for idea 392'; cmake --build --preset default --target c4cll >/dev/null; build/c4cll --target riscv64-linux-gnu --dump-prepared-bir tests/c/external/gcc_torture/src/va-arg-13.c > build/agent_state/392_postrepair_step2_prepared.log 2>&1; echo 'prepared_dump_exit='$?; } > test_after.log 2>&1`.

Result: passed for evidence capture; `prepared_dump_exit=0`. Logs:
`test_after.log`,
`build/agent_state/392_postrepair_step2_prepared.log`, and
`build/agent_state/392_postrepair_step2_analysis.log`.
