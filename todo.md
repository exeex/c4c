Status: Active
Source Idea Path: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Classified Stdarg Owner

# Current Packet

## Just Finished

Plan step 2 repaired the localized materialized-bool branch clobber for spilled
same-block compare conditions. AArch64 conditional block lowering now recognizes
when the terminator condition is a prepared same-block compare whose materialized
boolean lives in a stack slot, and emits a fresh compare-and-branch at the
terminator instead of trusting a stale cached boolean register after intervening
lowered instructions.

For `myprintf` block `vaarg.regtry.37`, the generated `%9s` transition no
longer branches on the clobbered stride constant. The emitted block now
recomputes the cursor advance and branches with `cmp w9, #0; b.le ...; b ...`
after the intervening add, so the fourth `%9s` aggregate switches from
register-save to overflow. The first `stdarg:` payload line now prints all six
`ABCDEFGHI` fields.

The delegated CTest proof is not green because the first bad fact advanced to
the next stdarg payload line: expected
`lmnopqr ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI`, observed
`lmnopqr ABCDEFGH ABCDEFGH ABCDEFGH ABCDEFGH ABCDEFGH`. That is a later
aggregate-width/copy issue after the repaired register-to-overflow transition,
not the original always-register-path branch clobber.

## Suggested Next

Localize and repair the advanced stdarg mismatch on the second payload line,
where the `lmnopqr`/narrow aggregate path now prints `ABCDEFGH` fields instead
of `ABCDEFGHI` fields after the Step 2 branch repair.

## Watchouts

The Step 2 repair intentionally applies only when the prepared condition value
is stack-backed; ordinary register-backed fused compare branches should continue
through the existing compare-branch path. The focused regression
`backend_aarch64_branch_control_lowering` covers a same-block compare whose
materialized bool register is clobbered before the terminator.

Do not reopen byval rounded-slot placement, outgoing stack argument base,
HFA/floating publication, non-HFA aggregate materialization, post-`va_arg` call
setup, or the `%9s` register-to-overflow decision unless generated-code
evidence moves the first bad fact back there.

## Proof

Built `c4cll` and `backend_aarch64_branch_control_lowering_test`.

Focused backend regression passed:
`ctest --test-dir build --output-on-failure -R '^backend_aarch64_branch_control_lowering$'`.

Delegated proof command was run exactly with output captured to `test_after.log`:
`ctest --test-dir build --output-on-failure -R '^(backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1`.
The prepared-BIR publication test passed; `c_testsuite_aarch64_backend_src_00204_c`
failed at the advanced runtime mismatch recorded above.

`git diff --check` passed.
