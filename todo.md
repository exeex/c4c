# Current Packet

Status: Active
Source Idea Path: ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extract direct frontend-LIR producer probes

## Just Finished

- Lifecycle switch: 769's bounded structured global-initializer contract is
  complete; resume 768 at its preserved Step 3.

## Suggested Next

- Begin Step 3 from the accepted static structured initializer representation,
  then extract the remaining focused producer probes. Do not modify carrier or
  Raw-BIR/importer routes.

## Watchouts

- The four-case missing-`addr_value` family predates 769 and is 768 producer
  evidence, not a global-initializer regression or a Raw-BIR/importer boundary.

## Proof

- Preserved acceptance evidence: `56d86556a` focused `frontend_lir` 5/5;
  fresh exact four-case reproduction remains 4/4 failing only at missing
  `LirIndirectBrOp.addr_value`, as recorded before 769 in 768 and 764.
