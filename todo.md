Status: Active
Source Idea Path: ideas/open/127_lir_structured_function_signature_metadata_boundary.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Signature Text Consumers

# Current Packet

## Just Finished

Lifecycle activation created this execution state for `plan.md` Step 1.

## Suggested Next

Delegate Step 1 to an executor: inventory `LirFunction::signature_text`
consumers, classify semantic versus final-text uses, and identify the
structured metadata needed by each semantic consumer.

## Watchouts

Do not treat `signature_text` removal as the goal. The intended change is to
stop semantic backend/BIR and verifier decisions from parsing final rendered
function header spelling.

## Proof

No code validation run during lifecycle activation.
