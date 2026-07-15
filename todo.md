# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.37
Current Step Title: Receive the one 824-authorized DirectScalar return-value parameter authority row

## Just Finished

- Step 7.37: received only the typed
  `LirRet.return_value_parameter_authority` DirectScalar ReturnValue row into
  the existing Raw-BIR parameter `ValueId` and `ReturnTerm`; importer
  validation now rejects missing, foreign, duplicate, owner/index/type/ABI/
  role/operand/signature-incoherent authority before publication, and nearby
  backend coverage verifies the retained exact parameter identity.

## Suggested Next

- Supervisor: reassess the source-idea completion gate after the completed
  Step 7.37 receiver slice.

## Watchouts

- The receiver derives admission only from the structured return tuple and its
  exact native definition; no other parameter form is admitted.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  passed 6/6; post-proof output: `test_after.log`.
