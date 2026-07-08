Status: Active
Source Idea Path: ideas/open/620_prepared_mixed_object_data_slots.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory mixed object-data fact gap

# Current Packet

## Just Finished

- Completed Step 1 inventory for the mixed object-data fact gap.
- Evidence files captured:
  - `build/agent_state/620_step1_20010924_bir.txt`
  - `build/agent_state/620_step1_20010924_prepared_bir.txt`
  - `build/agent_state/620_step1_20010924_hir.txt`
  - `build/agent_state/620_step1_pr61517_bir.txt`
  - `build/agent_state/620_step1_pr61517_prepared_bir.txt`
  - `build/agent_state/620_step1_pr57877_bir.txt`
  - `build/agent_state/620_step1_pr57877_prepared_bir.txt`
- Representative `src/20010924-1.c` has a mixed selected object-data shape:
  `a1` is a 16-byte object with ordinary byte data for the leading `char` and
  a pointer slot at byte offset `8` initialized to string object `.str0`.
  Current prepared access evidence proves loads from `a1` offset `0` and
  pointer loads from `a1` offset `8`, but object-data publication still falls
  back to `unsupported_but_coherent`.
- Neighboring rows `src/pr61517.c` and `src/pr57877.c` show scalar global
  object-data shapes with scalar layout authority in prepared memory facts, but
  the selected object-data contract still fails closed for rows whose
  initializer evidence cannot be represented by the current object-data facts.
- First missing prepared fact: an explicit mixed object-data relocation slot
  representation. `PreparedGlobalObjectData` currently has whole-object
  `emitted_bytes`, `zero_fill_byte_count`, and relocation booleans, but no list
  of relocation slots carrying byte offset, byte size, and target identity.

## Suggested Next

Execute Step 2 from `plan.md`: add explicit relocation-slot prepared facts.
Start with schema and verifier coverage before producer population. The first
code packet should add a relocation-slot record to `PreparedGlobalObjectData`
with byte offset, byte size, and target identity, then teach the selected
object-data verifier to reject missing, duplicate, overlapping, out-of-range,
or targetless slots while preserving current byte-only, zero-fill,
unsupported, and relocation-only behavior.

## Watchouts

- Do not route RV64 relocation-record emission, byte emission, symbol
  materialization, or access-width policy into this plan.
- Do not mark mixed aggregate object data coherent without prepared emitted
  byte spans plus relocation slot offsets and target identity.
- Preserve `608` parked evidence for prepared global memory facts and direct
  global-symbol base-plus-offset authority; do not repeat helper-only
  `ByteStorageAggregate` publication as progress.
- Preserve relocation-only object-data progress from `608`, including the
  `src/921110-1.c` move to the RV64 relocation-record consumer stop.
- Treat `src/20010924-1.c` as a representative, not a named-case shortcut.
- Do not populate mixed rows as coherent in Step 2; schema/verifier support
  should land before producer population.

## Proof

- Inventory-only packet; no code proof required.
- Inspection commands run:
  - `./build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20010924-1.c`
  - `./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20010924-1.c`
  - same BIR/prepared-BIR dumps for `src/pr61517.c` and `src/pr57877.c`
  - `./build/c4cll --dump-hir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20010924-1.c`
  - source/code inspection of `src/backend/prealloc/object_data.cpp`,
    `src/backend/prealloc/prepared_contract_verifier.hpp`, and
    `src/backend/prealloc/prepared_contract_verifier.cpp`
- Recommended Step 2 proof: focused object-data verifier/unit coverage plus
  `ctest --test-dir build -R '^backend_riscv_object_emission$' --output-on-failure`.
