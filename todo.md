Status: Active
Source Idea Path: ideas/open/190_lir_call_argument_structured_payload_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Existing Call Argument Authority

# Current Packet

## Just Finished

Lifecycle switch reset the active plan from blocked closure gate idea 188 to
dependency idea 190.

## Suggested Next

Execute Step 1 from `plan.md`: inventory generated metadata-rich LIR call
argument authority, raw/no-metadata compatibility paths, and the first target
for the structured argument carrier.

## Watchouts

- Do not continue the idea 188 closure gate until ideas 190, 191, and 194 are
  closed or the closure-gate source idea is explicitly narrowed.
- Do not treat parser renames or printer-only tests as progress for idea 190.
- Do not let stale rendered `callee_type_suffix` or `args_str` override
  structured metadata when metadata is present.

## Proof

Lifecycle switch only. No implementation validation or proof logs were created
or modified.
