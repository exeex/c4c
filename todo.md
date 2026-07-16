Status: Active
Source Idea Path: ideas/open/844_lir_global_extern_initializer_family_facts.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire selected parameter legacy mirrors only with parity

# Current Packet

## Just Finished

Completed plan.md Step 4 for the selected link-backed extern fixed aggregate
byval parameter form as a bounded no-code retirement conclusion. No selected
semantic authority remains in final declaration text, `callee_type_suffix`, or
`args_str` when the valid extern signature-store carrier is present, but those
text mirrors still serve compatibility/output and fallback roles for
unselected declaration forms, nonaggregate parameters, retained call signature
parity, and legacy typed-call parsing.

## Suggested Next

Send the exhausted repaired runbook to plan-owner for semantic disposition of
idea 844: close if the selected global aggregate, extern aggregate return, and
extern aggregate byval parameter slices satisfy the durable source criteria, or
repair/switch if another in-scope global/extern type-fact target remains.

## Watchouts

Keep lifecycle disposition inside idea 844 global/extern type facts. Do not
reopen initializer payload semantics, global policy identity, collector-only
receiver work, Raw-BIR import work, varargs policy, nonaggregate declaration
rendering, or non-type string routing. The old
`ctest -R backend_lir_to_bir_notes_test` route matches zero tests and must not
be treated as proof.

## Proof

Step 4 was no-code. Prior Step 3 corrected proof remains the relevant accepted
code proof:
`{ cmake --build build && ctest --test-dir build -R '^backend_lir_to_bir_interface$|^frontend_lir_extern_decl_type_ref$|^frontend_lir_global_label_address_initializer$' --output-on-failure; } > test_after.log 2>&1`

Result: passed; `test_after.log` contains a successful build and 3/3 passing
tests. For this no-code conclusion, run `git diff --check` before handoff.
