Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.51
Current Step Title: Receive the one 864-authorized double(double) direct call-result authority row

# Current Packet

## Just Finished

Corrected `plan.md` Step 7.51 by removing the declaration-only callee
restriction from the one-double-argument direct floating call-result receiver.
The focused backend coverage now drives a module-owned defined direct
`double(double)` target and verifies Raw BIR plus Canonical BIR preserve the
same `DirectCallResult` tuple.

## Suggested Next

Supervisor should select the next bounded 734 receiver-completeness packet from
the active runbook.

## Watchouts

Do not edit LIR producer authority, repeat or broaden Step 7.50, reopen fixed
direct-call arguments 0/1, receive any body-parameter row, recover authority
from presentation text, or absorb memory/VA, aggregate/vector, module/type/
global/metadata, CFG/PHI, residual instruction/terminator, inline-assembly,
generic residual sweeps, or any other family. Step 7.51 deliberately differs
from Step 7.50 by accepting the direct `double(double)` callee whether it is a
module-owned declaration or definition when the structured signature tuple
matches.

## Proof

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$'; } > test_after.log 2>&1`
passed and wrote the focused receiver proof to root `test_after.log`.
