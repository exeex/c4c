Status: Active
Source Idea Path: ideas/open/842_lir_restricted_first_class_value_unions.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate named boundary consumers

# Current Packet

## Just Finished

Completed plan.md Step 2 for the selected PHI value boundary.

Added `LirPhiBoundaryValueType` as a PHI-local restricted carrier with explicit
scalar, vector, aggregate, and pointer alternatives. Attached it only to
`LirPhiOp` as `boundary_value_type`, initialized from `type_str` as a
compatibility mirror. The verifier now treats the carrier as selected PHI
boundary authority, requires it to mirror `LirPhiOp.type_str`, and rejects
function, void, opaque, and runtime-text-only/wrong-family carriers.

Focused tests prove admitted integer scalar, floating scalar, vector,
anonymous aggregate, and pointer PHI alternatives; reject function, void,
opaque, runtime-text, and stale `type_str` cases; and preserve printer parity
through `LirPhiOp.type_str`.

## Suggested Next

Execute plan.md Step 3 for PHI only: migrate the selected PHI verifier/printer
consumer path to read the restricted `LirPhiBoundaryValueType` authority
directly while keeping `LirPhiOp.type_str` as parity text.

## Watchouts

PHI now has a selected carrier, but `LirPhiOp.type_str` still owns rendering
parity. Step 3 should stay PHI-only and avoid call, select, return, Raw-BIR,
universal value bags, generic IDs, RTTI/vtables, and deletion of
`LirPhiOp.type_str`.

The Step 2 wrong-kind test covers runtime-text-only refs as the feasible local
surface for metadata-like/unbounded payload rejection. Partially parsed call
signature text and raw `args_str` payloads remain call-boundary concerns and
were not introduced into PHI.

## Proof

Step 2 proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'; } > test_after.log 2>&1`

Result: passed, 1/1 focused test passing.

Additional proof: `git diff --check` passed.

Proof log: `test_after.log`.
