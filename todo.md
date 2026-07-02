# Current Packet

Status: Active
Source Idea Path: ideas/open/533_bir_route_index_standalone_prerequisites.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Route-Index Prerequisites

## Just Finished

Step 1 mapped why `src/backend/bir/bir_route_index.hpp` is not a standalone
header. A direct no-file probe:

```bash
printf '#include "src/backend/bir/bir_route_index.hpp"\n' | \
  c++ -x c++ -std=c++17 -I/workspaces/c4c -I/workspaces/c4c/src \
    -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser \
    -fsyntax-only -
```

fails before any implementation dependency is reached. The missing standalone
inputs are:

- header self-containment: no `namespace c4c::backend::bir` wrapper when parsed
  outside `bir.hpp`; no direct `<cstddef>` / `<string_view>` includes for
  `std::size_t` and `std::string_view`.
- core BIR names: `Function`, `Block`, `Value`, `BlockLabelId`,
  `kInvalidBlockLabel`.
- route1 identity: `Route1SourceValueIdentity`.
- route4 publication names: `Route4PublicationAvailabilityIndex`,
  `Route4PublicationAvailabilityStatus`,
  `Route4CurrentBlockPublicationRecord`,
  `Route4BlockEntryPublicationRecord`.
- route7 comparison names: `Route7ComparisonConditionIndex`,
  `Route7ComparisonStatus`, `Route7ComparisonOperandRole`,
  `Route7ComparisonInstructionRecord`, `Route7ComparisonOperandRecord`,
  `Route7BranchConditionRecord`.

Clang-backed header queries confirm `bir_route_index.hpp` currently declares
only the route-index enums/records/facades/functions, and the same missing
names make the tool report `ok: false`. When parsed through `bir.hpp`, clang
sees route-index types in `c4c::backend::bir`; when parsed alone, they are
global declarations, which proves the file is a namespace/order-dependent
fragment.

Prerequisite clusters currently supplied earlier by `bir.hpp`:

- ID/std cluster: `src/shared/text_id_table.hpp` provides `BlockLabelId`,
  `ValueNameId`, `kInvalidBlockLabel`, and `kInvalidValueName`; standalone use
  also needs `<cstddef>`, `<string_view>`, and the existing route1 cluster needs
  `<optional>` / `<cstdint>` through `bir.hpp`.
- core model cluster: `Function` and `Block` are forward-declared at
  `bir.hpp:18-19`; `Value` is defined at `bir.hpp:439`. The route-index header
  itself only stores pointers/references to `Function`, `Block`, and `Value`,
  so it does not need complete `Function` or `Block`, but
  `Route1SourceValueIdentity` uses `Value::Kind` and therefore needs complete
  `Value`.
- route1 identity cluster: `Route1SourceValueIdentity` is defined at
  `bir.hpp:537` and is stored by value in `RouteIndexRecordReference`, so it
  must be complete before `bir_route_index.hpp` is included. It depends on
  `Value`, `TypeKind`, `ValueNameId`, `LinkNameId`, and standard optional /
  integer / string-view facilities.
- route4 cluster: `Route4PublicationAvailabilityStatus` is defined at
  `bir.hpp:4885`; route4 publication records and
  `Route4PublicationAvailabilityIndex` are defined at `bir.hpp:4940-4997`.
  `Route4IndexReferenceValidation` stores the status enum by value and uses
  default enumerators, so the enum must be complete. Record/index pointers and
  references in `bir_route_index.hpp` could be forward-declared, but moving the
  full route4 records would pull in `Route1SourceValueIdentity`, `TypeKind`,
  `ValueNameId`, `Inst`, `PhiInst`, and vectors.
- route7 cluster: `Route7ComparisonStatus` is defined at `bir.hpp:5497`;
  `Route7ComparisonOperandRole` at `bir.hpp:5540`; route7 records and
  `Route7ComparisonConditionIndex` at `bir.hpp:5553-5605`. The status and
  operand-role enums must be complete for by-value fields/default initializers.
  Record/index pointers and references could be forward-declared, but moving
  full route7 records would pull in `Route1SourceValueIdentity`,
  `ComparisonProducerKind`, `BinaryOpcode`, `TypeKind`, `Inst`, `BinaryInst`,
  optional integers, and vectors.

Complete-type risks:

- `RouteIndexRecordReference::value` makes `Route1SourceValueIdentity` the
  first hard complete-type blocker.
- `Route1SourceValueIdentity` makes `Value` complete-type-sensitive because it
  stores `Value::Kind`, not just `const Value*`.
- route4/route7 validation result structs only need complete status/role enums;
  their route record pointers can remain forward declarations at this boundary.
- moving full route4 or route7 record/index structs first is broad and risky
  because those records embed vectors and route1/core-model dependent fields.

Include-site constraints:

- No current file directly includes `bir_route_index.hpp` except the
  aggregator include inside `bir.hpp`.
- Broad `bir.hpp` users still include route implementation files, MIR/prealloc
  consumers, and direct-source BIR tests. Route-index API users found by `rg`
  include `bir_route_facade.cpp`, `bir_route4_publication.cpp`, `bir.cpp`,
  `src/backend/mir/query.cpp`,
  `src/backend/mir/aarch64/codegen/comparison.cpp`,
  `src/backend/mir/aarch64/codegen/calls.cpp`,
  `src/backend/prealloc/prepared_lookups.cpp`, and large focused BIR tests.
  These users also depend on broad BIR model and route surfaces, so direct
  include replacement remains unsafe until the header itself is standalone.

First safe boundary decision:

- Do not continue idea 530 direct include replacement yet.
- The first safe Step 2 boundary should be namespace/include hardening plus a
  narrow prerequisite declaration header, likely `bir_route_index_prereqs.hpp`
  or equivalent, that provides only:
  `src/shared/text_id_table.hpp`, standard includes, forward declarations for
  `Function`, `Block`, `Value`, route4/route7 record/index structs, and complete
  route-index-required enums/statuses where default enumerators are used.
- Leave full `Value`, `Route1SourceValueIdentity`, full route4 publication
  records/indexes, and full route7 comparison records/indexes in `bir.hpp` for
  now unless Step 2 intentionally chooses a smaller earlier cluster such as a
  route1 identity prerequisites split. A synthetic wrapped/stub probe passed,
  proving the route-index header can become standalone after those declaration
  prerequisites exist; it does not prove broad declaration movement is safe.

## Suggested Next

Step 2 should split only the first mapped prerequisite boundary. Recommended
packet: add namespace/include hardening for `bir_route_index.hpp` and introduce
a narrow route-index prerequisite declaration header with standard includes,
`src/shared/text_id_table.hpp`, forward declarations, and complete
route-index-required status/role enums. Keep `bir.hpp` as the compatibility
aggregator and do not replace any consumer includes in that packet.

## Watchouts

- Do not continue idea 530 Step 3-style direct include replacement attempts.
- Do not claim dependency reduction while `bir_route_index.hpp` still cannot
  compile as a top-level include.
- Keep `bir.hpp` as compatibility aggregator unless a consumer is proven not to
  need the broad model or route surface.
- Avoid broad declaration movement; full `Value`, `Route1SourceValueIdentity`,
  route4 records/indexes, and route7 records/indexes are not the first safe
  movement boundary.
- If Step 2 keeps `Route1SourceValueIdentity` by value in
  `RouteIndexRecordReference`, standalone success still requires a complete
  route1 identity declaration before `bir_route_index.hpp`; forward-declaring it
  is not enough.

## Proof

Mapping-only packet. No build/tests were required and `test_after.log` was not
created or updated. Evidence used:

- clang-backed `list-symbols` and `function-signatures` on
  `src/backend/bir/bir_route_index.hpp`, both failing with the missing-name map
  above while still listing the route-index declarations.
- clang-backed `type-refs` / definition-location queries over
  `bir_route_facade.cpp`, `bir_route4_publication.cpp`, and `bir.cpp` for
  route-index, route4, route7, and route1 dependency evidence.
- direct no-file top-level include probe for
  `#include "src/backend/bir/bir_route_index.hpp"`.
- synthetic wrapped/stub no-file probe proving namespace/include hardening plus
  prerequisite declarations is sufficient for this header's own declarations.

Focused Step 2 proof recommendation after header edits:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering)$') > test_after.log 2>&1
```

Also rerun the direct no-file top-level include probe for
`#include "src/backend/bir/bir_route_index.hpp"` and record whether the missing
declaration list shrank or the probe passed.
