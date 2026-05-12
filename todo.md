Status: Active
Source Idea Path: ideas/open/174_aggregate_abi_classification_structured_facts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select the Aggregate ABI Boundary

# Current Packet

## Just Finished

Activation created the runbook for Step 1 from
`ideas/open/174_aggregate_abi_classification_structured_facts.md`.

## Suggested Next

Inspect BIR aggregate call classification and the AArch64 direct LIR aggregate
route. Select one bounded aggregate ABI boundary where structured layout or ABI
facts are already available and can replace or guard spelling-derived
classification.

## Watchouts

Do not weaken aggregate ABI expectations or add new rendered-signature/type
parsing as structured classification. Metadata-rich inputs with missing or
mismatched structured facts need explicit compatibility, diagnostic, verifier,
assertion, or unsupported behavior rather than silent fallback.

## Proof

Lifecycle activation only. No code validation required yet.
