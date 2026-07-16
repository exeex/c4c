#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/lir_to_bir/index_adapter.hpp"
#include "src/codegen/lir/ir.hpp"

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace bir = c4c::backend::bir;
namespace lir = c4c::codegen::lir;

namespace {

[[noreturn]] void fail(const std::string& message) {
  std::cerr << "FAIL: " << message << '\n';
  std::exit(1);
}

void expect(bool condition, const std::string& message) {
  if (!condition) fail(message);
}

void test_structured_gep_index_adapter() {
  namespace detail = c4c::backend::lir_to_bir_detail;

  const lir::LirGepIndex authoritative_immediate = lir::LirGepIndex::typed(
      lir::LirTypeRef::integer(64), lir::LirOperand::integer("999", 0));
  const auto facts =
      detail::authoritative_gep_index_facts(authoritative_immediate);
  expect(facts && facts->type_text == "i64" &&
             facts->operand.integer_immediate() &&
             facts->operand.integer_immediate()->value == 0,
         "structured GEP adaptation should preserve native immediate authority");
  const auto native_value = detail::resolve_index_operand_authority_first(
      facts->operand,
      [](std::string_view) -> std::optional<std::int64_t> { return 999; },
      [](std::string_view) -> std::optional<std::int64_t> {
        return std::nullopt;
      });
  expect(native_value && *native_value == 0,
         "native GEP immediate must override misleading display");

  const auto raw_parts =
      detail::parse_raw_typed_operand_parts("i64 5");
  expect(raw_parts && raw_parts->type_text == "i64" &&
             raw_parts->value_text == "5",
         "raw GEP compatibility should retain typed-text splitting");
  const auto raw_value = detail::resolve_index_operand_authority_first(
      lir::LirOperand(raw_parts->value_text),
      [](std::string_view display) -> std::optional<std::int64_t> {
        return display == "5" ? std::optional<std::int64_t>{5}
                              : std::nullopt;
      },
      [](std::string_view) -> std::optional<std::int64_t> {
        return std::nullopt;
      });
  expect(raw_value && *raw_value == 5,
         "raw GEP immediate should retain presentation parsing");

  bool authoritative_alias_lookup = false;
  const auto authoritative_ssa_value =
      detail::resolve_index_operand_authority_first(
          lir::LirOperand::ssa("%legacy", lir::LirValueId{7}),
          [](std::string_view) -> std::optional<std::int64_t> { return 41; },
          [&](std::string_view) -> std::optional<std::int64_t> {
            authoritative_alias_lookup = true;
            return 41;
          });
  expect(!authoritative_ssa_value && !authoritative_alias_lookup,
         "authoritative SSA index must not consult display aliases");

  bool raw_alias_lookup = false;
  const auto raw_ssa_value = detail::resolve_index_operand_authority_first(
      lir::LirOperand("%legacy"),
      [](std::string_view) -> std::optional<std::int64_t> { return 41; },
      [&](std::string_view display) -> std::optional<std::int64_t> {
        raw_alias_lookup = true;
        return display == "%legacy" ? std::optional<std::int64_t>{41}
                                      : std::nullopt;
      });
  expect(raw_ssa_value && *raw_ssa_value == 41 && raw_alias_lookup,
         "raw SSA index should retain display-keyed compatibility lookup");
}

void test_void_return_rejects_authoritative_value() {
  lir::LirBlock block;
  block.id = lir::LirBlockId{0};
  block.label = "entry";
  block.terminator = lir::LirRet{
      lir::LirOperand::integer("void", 7), lir::LirTypeRef::integer(32)};

  lir::LirFunction function;
  function.name = "authoritative_return";
  function.signature_return_type_ref = lir::LirTypeRef("void");
  function.blocks.push_back(std::move(block));
  function.entry = lir::LirBlockId{0};

  lir::LirModule module;
  module.functions.push_back(std::move(function));

  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(!raw.has_value() &&
             raw.error().code == bir::ImportErrorCode::InvalidVoidReturn,
         "new-BIR must reject authoritative scalar return before display interpretation");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(!canonical.has_value() &&
             canonical.error().code == bir::ImportErrorCode::InvalidVoidReturn,
         "canonical new-BIR boundary must preserve InvalidVoidReturn");
}

lir::LirFunction void_declaration(std::string name) {
  lir::LirFunction function;
  function.name = std::move(name);
  function.is_declaration = true;
  function.signature_return_type_ref = lir::LirTypeRef("void");
  return function;
}

lir::LirFunction void_definition(std::string name,
                                 std::vector<lir::LirBlock> blocks) {
  lir::LirFunction function;
  function.name = std::move(name);
  function.signature_return_type_ref = lir::LirTypeRef("void");
  function.blocks = std::move(blocks);
  function.entry = function.blocks.front().id;
  return function;
}

lir::LirBlock return_block(std::uint32_t id, std::string label) {
  lir::LirBlock block;
  block.id = lir::LirBlockId{id};
  block.label = std::move(label);
  block.terminator = lir::LirRet{std::nullopt, "void"};
  return block;
}

lir::LirBlock unreachable_block(std::uint32_t id, std::string label) {
  lir::LirBlock block;
  block.id = lir::LirBlockId{id};
  block.label = std::move(label);
  block.terminator = lir::LirUnreachable{};
  return block;
}

c4c::TypeSpec scalar_type(c4c::TypeBase base) {
  c4c::TypeSpec type{};
  type.base = base;
  return type;
}

lir::LirTypeRef stable_parameter_mirror(c4c::TypeBase base,
                                        c4c::TargetArch arch = c4c::TargetArch::X86_64) {
  switch (base) {
    case c4c::TB_INT:
    case c4c::TB_UINT: return lir::LirTypeRef::integer(32);
    case c4c::TB_LONG:
    case c4c::TB_ULONG:
      return lir::LirTypeRef::integer(
          c4c::long_width_bits(c4c::default_target_profile(arch)));
    case c4c::TB_LONGLONG:
    case c4c::TB_ULONGLONG: return lir::LirTypeRef::integer(64);
    case c4c::TB_FLOAT: return lir::LirTypeRef("float");
    case c4c::TB_DOUBLE: return lir::LirTypeRef("double");
    default: return lir::LirTypeRef("unsupported");
  }
}

lir::LirFunction stable_parameter_function(
    std::string name, bool declaration,
    const std::vector<c4c::TypeBase>& bases,
    c4c::TargetArch arch = c4c::TargetArch::X86_64) {
  lir::LirFunction function =
      declaration ? void_declaration(std::move(name))
                  : void_definition(std::move(name), {return_block(0, "entry")});
  // This syntactically valid but conflicting legacy shadow must not affect the
  // complete structured parameter facts below.
  function.signature_text = "define void @stale_signature_shadow(i64 %shadow)";
  for (std::size_t index = 0; index < bases.size(); ++index) {
    auto logical = scalar_type(bases[index]);
    logical.inner_rank = -1;
    auto signature = logical;
    function.params.emplace_back(
        "%logical-display-" + std::to_string(index), logical);
    function.signature_params.push_back(
        {"%signature-display-" + std::to_string(index), signature, false});
    function.signature_param_type_refs.push_back(
        stable_parameter_mirror(bases[index], arch));
  }
  return function;
}

lir::LirFunction explicit_void_parameter_function(std::string name,
                                                   bool declaration) {
  lir::LirFunction function =
      declaration ? void_declaration(std::move(name))
                  : void_definition(std::move(name), {return_block(0, "entry")});
  auto sentinel = scalar_type(c4c::TB_VOID);
  sentinel.inner_rank = -1;
  function.params.emplace_back("%void-display", sentinel);
  function.signature_has_void_param_list = true;
  return function;
}

double double_from_bits(std::uint64_t bits) {
  double value = 0.0;
  static_assert(sizeof(value) == sizeof(bits));
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

lir::LirInlineAsmOp void_inline_asm(std::string asm_text,
                                    std::string constraints) {
  lir::LirInlineAsmOp op;
  op.ret_type = lir::LirTypeRef("void");
  op.original_asm_text = asm_text;
  op.original_constraint_text = constraints;
  op.asm_text = std::move(asm_text);
  op.constraints = std::move(constraints);
  op.side_effects = true;
  op.clobbers = {"memory", "cc"};
  return op;
}

void test_supported_import_and_views() {
  lir::LirModule module;
  module.functions.push_back(void_declaration("decl"));
  module.functions.push_back(
      void_definition("returns", {return_block(0, "entry")}));

  lir::LirBlock branch_entry;
  branch_entry.id = lir::LirBlockId{0};
  branch_entry.label = "entry";
  branch_entry.terminator = lir::LirBr{"exit", lir::LirBlockId{1}};
  module.functions.push_back(void_definition(
      "branches", {std::move(branch_entry), return_block(1, "exit")}));

  std::string asm_bytes = "add %0, %1";
  asm_bytes.push_back('\0');
  asm_bytes += "#opaque";
  std::string constraint_bytes = "=r,r,VR,VRM2,~{memory}";
  constraint_bytes.push_back('\0');
  constraint_bytes += "tail";
  auto asm_block = return_block(0, "entry");
  auto opaque_asm = void_inline_asm(asm_bytes, constraint_bytes);
  opaque_asm.asm_text = "llvm compatibility template";
  opaque_asm.constraints = "~{compatibility-only}";
  asm_block.insts.push_back(std::move(opaque_asm));
  module.functions.push_back(
      void_definition("inline_asm", {std::move(asm_block)}));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(), "supported LIR subset should publish RawBir");

  const bir::ModuleView module_view = imported.value().view();
  const auto functions = module_view.functions();
  expect(functions.size() == 4, "module view should preserve all functions");

  auto declaration = module_view.function(functions[0]);
  expect(declaration.has_value() && declaration.value().is_declaration(),
         "declaration should remain a declaration");
  expect(declaration.value().link_name() == "decl",
         "declaration should preserve its link name");
  expect(declaration.value().blocks().empty(),
         "declaration should not acquire a body");

  auto returns = module_view.function(functions[1]);
  expect(returns.has_value(), "returning definition should resolve");
  const auto return_blocks = returns.value().blocks();
  expect(return_blocks.size() == 1, "returning definition should have one block");
  auto return_term = returns.value().terminator(return_blocks[0]);
  expect(return_term.has_value() &&
             std::holds_alternative<bir::ReturnTerm>(return_term.value()),
         "void LIR return should become a BIR ReturnTerm");

  auto branches = module_view.function(functions[2]);
  expect(branches.has_value(), "branching definition should resolve");
  const auto branch_blocks = branches.value().blocks();
  expect(branch_blocks.size() == 2, "branching definition should preserve block order");
  auto successors = branches.value().successors(branch_blocks[0]);
  expect(successors.has_value() && successors.value().size() == 1 &&
             successors.value()[0] == branch_blocks[1],
         "CFG successor view should resolve the imported branch target");

  auto inline_asm_function = module_view.function(functions[3]);
  expect(inline_asm_function.has_value(), "inline-asm function should resolve");
  const auto asm_blocks = inline_asm_function.value().blocks();
  auto asm_insts = inline_asm_function.value().instructions(asm_blocks[0]);
  expect(asm_insts.has_value() && asm_insts.value().size() == 1,
         "inline asm should publish one stable instruction identity");
  auto asm_inst = inline_asm_function.value().instruction(asm_insts.value()[0]);
  expect(asm_inst.has_value() &&
             asm_inst.value().opcode() == bir::Opcode::InlineAsm,
         "instruction view should expose the closed InlineAsm opcode");
  const auto* payload =
      std::get_if<bir::InlineAsmNode>(&asm_inst.value().payload());
  expect(payload != nullptr, "InlineAsm opcode should carry InlineAsmNode");
  expect(payload->asm_text == asm_bytes,
         "inline asm template bytes must remain byte-exact and opaque");
  expect(payload->constraint_text == constraint_bytes,
         "aggregate constraint bytes must remain byte-exact and opaque");
  expect(payload->side_effects &&
             payload->clobbers == std::vector<std::string>({"memory", "cc"}),
         "side-effect and ordered clobber fields must remain separate");
  expect(asm_inst.value().operands().empty() &&
             asm_inst.value().results().empty(),
         "void/no-argument LIR asm must not acquire invented value identities");

  auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "the same target-independent storage should pass canonical verification");
  expect(canonical.value().view().functions().size() == functions.size(),
         "canonical publication should preserve the verified graph");
}

lir::LirModule direct_branch_module() {
  lir::LirBlock entry;
  entry.id = lir::LirBlockId{10};
  entry.label = "entry";
  entry.terminator = lir::LirBr{"exit", lir::LirBlockId{11}};
  auto exit = return_block(11, "exit");

  lir::LirModule module;
  module.functions.push_back(
      void_definition("direct_branch", {std::move(entry), std::move(exit)}));
  return module;
}

void test_direct_branch_successor_receipt_and_rejections() {
  const auto module = direct_branch_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "direct LirBr successor must publish verified Raw BIR");
  const auto function = raw.value().view().function(raw.value().view().functions()[0]).value();
  const auto blocks = function.blocks();
  const auto terminator = function.terminator(blocks[0]);
  expect(terminator.has_value() &&
             std::holds_alternative<bir::JumpTerm>(terminator.value()) &&
             std::get<bir::JumpTerm>(terminator.value()).target == blocks[1],
         "direct LirBr must retain its structural successor as the typed JumpTerm target");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(), "direct LirBr successor must canonicalize");

  const auto rejected = [](auto mutate, bir::ImportErrorCode expected,
                           const std::string& message) {
    auto candidate = direct_branch_module();
    mutate(candidate);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() && raw_rejected.error().code == expected,
           message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value() &&
               canonical_rejected.error().code == expected,
           message + " (canonical rollback)");
  };
  rejected(
      [](lir::LirModule& candidate) {
        std::get<lir::LirBr>(candidate.functions[0].blocks[0].terminator)
            .successor = lir::LirBlockId::invalid();
      },
      bir::ImportErrorCode::MissingBranchTarget,
      "missing direct successor authority must reject transactionally");
  rejected(
      [](lir::LirModule& candidate) {
        std::get<lir::LirBr>(candidate.functions[0].blocks[0].terminator)
            .successor = lir::LirBlockId{99};
      },
      bir::ImportErrorCode::MissingBranchTarget,
      "unresolved direct successor authority must reject transactionally");
  rejected(
      [](lir::LirModule& candidate) {
        std::get<lir::LirBr>(candidate.functions[0].blocks[0].terminator)
            .target_label = "misleading display";
      },
      bir::ImportErrorCode::MissingBranchTarget,
      "incoherent direct successor display shadow must reject without label recovery");
  rejected(
      [](lir::LirModule& candidate) {
        candidate.functions[0].blocks[1].id = lir::LirBlockId{10};
      },
      bir::ImportErrorCode::DuplicateBlockId,
      "duplicate current-function successor ownership must reject transactionally");

  lir::LirBlock foreign_entry;
  foreign_entry.id = lir::LirBlockId{20};
  foreign_entry.label = "foreign_entry";
  foreign_entry.terminator = lir::LirBr{"foreign_exit", lir::LirBlockId{21}};
  lir::LirModule cross_owner;
  cross_owner.functions.push_back(
      void_definition("owner", {std::move(foreign_entry)}));
  cross_owner.functions.push_back(
      void_definition("other_owner", {return_block(21, "foreign_exit")}));
  const auto cross_raw = bir::lower_lir_to_raw_bir(cross_owner);
  expect(!cross_raw.has_value() &&
             cross_raw.error().code == bir::ImportErrorCode::MissingBranchTarget,
         "cross-function direct successor authority must publish no Raw BIR");
  const auto cross_canonical = bir::lower_lir_to_canonical_bir(cross_owner);
  expect(!cross_canonical.has_value() &&
             cross_canonical.error().code == bir::ImportErrorCode::MissingBranchTarget,
         "cross-function direct successor authority must publish no Canonical BIR");
}

void test_generic_inline_asm_ssa_edges() {
  bir::ModuleBuilder builder;
  bir::FunctionSignature signature;
  signature.return_type = bir::Type{bir::TypeKind::I64};
  signature.parameter_types = {bir::Type{bir::TypeKind::I64}};
  auto function = builder.create_function(std::move(signature), "ssa_asm", false);
  expect(function.has_value(), "SSA asm test function should be constructible");

  bir::BlockId block_id{};
  bir::InstId instruction_id{};
  bir::ValueId parameter_id{};
  bir::ValueId result_id{};
  auto edited = builder.with_function(
      function.value(), [&](bir::FunctionBuilder& function_builder) {
        auto block = function_builder.create_block("entry");
        if (!block) return bir::Result<void, bir::BuildError>::failure(block.error());
        block_id = block.value();
        auto parameter = function_builder.parameter(0);
        if (!parameter)
          return bir::Result<void, bir::BuildError>::failure(parameter.error());
        parameter_id = parameter.value();

        bir::InlineAsmSpec malformed;
        malformed.asm_text = "opaque";
        malformed.constraint_text = "+r";
        malformed.inputs = {
            bir::ValueId{function.value(), bir::ValueKind::Parameter, 999, 1}};
        auto rejected = function_builder.append(block_id, std::move(malformed));
        expect(!rejected.has_value() &&
                   rejected.error() == bir::BuildError::InvalidValue,
               "missing SSA input must be rejected before instruction publication");

        bir::InlineAsmSpec spec;
        spec.asm_text = "# uses abstract SSA only";
        spec.constraint_text = "+r";
        spec.clobbers = {"memory"};
        spec.side_effects = true;
        spec.inputs = {parameter_id};
        spec.result_types = {bir::Type{bir::TypeKind::I64}};
        auto appended = function_builder.append(block_id, std::move(spec));
        if (!appended)
          return bir::Result<void, bir::BuildError>::failure(appended.error());
        instruction_id = appended.value().instruction;
        result_id = appended.value().results[0];
        return function_builder.set_terminator(
            block_id, bir::ReturnTerm{result_id});
      });
  expect(edited.has_value(), "generic SSA asm construction should succeed");

  auto published = std::move(builder).publish();
  expect(published.has_value(),
         "generic inline-asm use/def edges should pass Raw BIR verification");
  auto function_view = published.value().view().function(function.value());
  expect(function_view.has_value(), "published SSA asm function should resolve");
  auto instructions = function_view.value().instructions(block_id);
  expect(instructions.has_value() && instructions.value().size() == 1 &&
             instructions.value()[0] == instruction_id,
         "failed append must leave no partial instruction in block order");
  auto instruction = function_view.value().instruction(instruction_id);
  expect(instruction.has_value() &&
             instruction.value().operands() ==
                 std::vector<bir::ValueId>({parameter_id}) &&
             instruction.value().results() ==
                 std::vector<bir::ValueId>({result_id}),
         "InlineAsm must expose generic ordered SSA input and output edges");
  auto result = function_view.value().value(result_id);
  const auto* definition = result.has_value()
                               ? std::get_if<bir::InstResultDef>(
                                     &result.value().definition)
                               : nullptr;
  expect(definition && definition->instruction == instruction_id &&
             definition->result_index == 0,
         "inline-asm output must own a coherent InstResultDef coordinate");
  expect(parameter_id != result_id,
         "read/write asm must keep incoming use and produced result distinct");
}

void test_structured_lir_import_ssa_chain() {
  const auto binding = [](std::string value, std::string type,
                          lir::LirInlineAsmValueRole role,
                          std::size_t constraint_index) {
    return lir::LirInlineAsmValueBinding{
        lir::LirOperand(std::move(value)), lir::LirTypeRef(std::move(type)),
        role, constraint_index};
  };

  std::string semantic_asm = "semantic read/write";
  semantic_asm.push_back('\0');
  semantic_asm += "opaque tail";
  std::string semantic_constraints = "+r";
  semantic_constraints.push_back('\0');
  semantic_constraints += "opaque tail";

  auto producer = void_inline_asm("semantic producer", "=r");
  producer.asm_text = "llvm producer";
  producer.constraints = "=compat";
  producer.args_str = "compatibility producer args";
  producer.ordinary_results = {
      binding("%seed", "i64", lir::LirInlineAsmValueRole::Output, 0)};

  auto read_write = void_inline_asm(semantic_asm, semantic_constraints);
  read_write.asm_text = "llvm read/write";
  read_write.constraints = "+compat";
  read_write.args_str = "compatibility read/write args";
  read_write.clobbers = {"cc", "memory", "x7"};
  read_write.ordinary_inputs = {
      binding("%seed", "i64", lir::LirInlineAsmValueRole::ReadWrite, 0)};
  read_write.ordinary_results = {
      binding("%next", "i64", lir::LirInlineAsmValueRole::ReadWrite, 0)};

  auto consumer = void_inline_asm("semantic consumer", "r");
  consumer.asm_text = "llvm consumer";
  consumer.constraints = "compat";
  consumer.args_str = "compatibility consumer args";
  consumer.ordinary_inputs = {
      binding("%next", "i64", lir::LirInlineAsmValueRole::Input, 0)};

  lir::LirModule module;
  auto block = return_block(0, "entry");
  block.insts = {std::move(producer), std::move(read_write),
                 std::move(consumer)};
  module.functions.push_back(
      void_definition("structured_chain", {std::move(block)}));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "structured LIR inline-asm chain should publish RawBir");
  const auto module_view = imported.value().view();
  const auto function_ids = module_view.functions();
  expect(function_ids.size() == 1,
         "structured chain should preserve one function identity");
  auto function = module_view.function(function_ids[0]);
  expect(function.has_value(), "structured chain function should resolve");
  const auto blocks = function.value().blocks();
  auto instructions = function.value().instructions(blocks[0]);
  expect(instructions.has_value() && instructions.value().size() == 3,
         "structured chain should preserve stable instruction order");

  auto first = function.value().instruction(instructions.value()[0]);
  auto second = function.value().instruction(instructions.value()[1]);
  auto third = function.value().instruction(instructions.value()[2]);
  expect(first.has_value() && second.has_value() && third.has_value() &&
             first.value().results().size() == 1 &&
             second.value().operands() == first.value().results() &&
             second.value().results().size() == 1 &&
             third.value().operands() == second.value().results(),
         "ordinary result map should connect producer, read/write, and consumer");
  expect(second.value().operands()[0] != second.value().results()[0],
         "read/write import must retain distinct old and new SSA identities");
  const auto* payload =
      std::get_if<bir::InlineAsmNode>(&second.value().payload());
  expect(payload && payload->asm_text == semantic_asm &&
             payload->constraint_text == semantic_constraints &&
             payload->clobbers ==
                 std::vector<std::string>({"cc", "memory", "x7"}) &&
             payload->side_effects,
         "BIR payload must preserve only original semantic authority");
  auto result = function.value().value(second.value().results()[0]);
  expect(result.has_value() && result.value().type == bir::Type{bir::TypeKind::I64},
         "structured result type should survive through the stable value view");

  auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "the same structured chain should publish CanonicalBir transactionally");
}

void test_structured_lir_import_rejections() {
  const auto binding = [](std::string value, std::string type,
                          lir::LirInlineAsmValueRole role,
                          std::size_t constraint_index) {
    return lir::LirInlineAsmValueBinding{
        lir::LirOperand(std::move(value)), lir::LirTypeRef(std::move(type)),
        role, constraint_index};
  };
  const auto expect_rejected = [](lir::LirModule module,
                                  const std::string& message) {
    auto raw = bir::lower_lir_to_raw_bir(module);
    expect(!raw.has_value() &&
               raw.error().code == bir::ImportErrorCode::UnsupportedInlineAsmShape,
           message + " (RawBir)");
    auto canonical = bir::lower_lir_to_canonical_bir(module);
    expect(!canonical.has_value() &&
               canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedInlineAsmShape,
           message + " (CanonicalBir)");
  };
  const auto one_function_module = [](std::string name,
                                      std::vector<lir::LirInst> instructions) {
    lir::LirModule module;
    auto block = return_block(0, "entry");
    block.insts = std::move(instructions);
    module.functions.push_back(
        void_definition(std::move(name), {std::move(block)}));
    return module;
  };

  auto missing = void_inline_asm("missing", "r");
  missing.ordinary_inputs = {
      binding("%missing", "i64", lir::LirInlineAsmValueRole::Input, 0)};
  expect_rejected(one_function_module("missing", {missing}),
                  "missing structured input must fail without publication");

  auto first = void_inline_asm("first", "=r");
  first.ordinary_results = {
      binding("%dup", "i64", lir::LirInlineAsmValueRole::Output, 0)};
  auto duplicate = void_inline_asm("duplicate", "=r");
  duplicate.ordinary_results = {
      binding("%dup", "i64", lir::LirInlineAsmValueRole::Output, 0)};
  expect_rejected(one_function_module("duplicate", {first, duplicate}),
                  "duplicate structured result must fail without publication");

  auto mistyped = void_inline_asm("mistyped", "r");
  mistyped.ordinary_inputs = {
      binding("%dup", "i32", lir::LirInlineAsmValueRole::Input, 0)};
  expect_rejected(one_function_module("mistyped", {first, mistyped}),
                  "mistyped structured input must fail without publication");

  auto inconsistent = void_inline_asm("inconsistent", "+r");
  inconsistent.ordinary_inputs = {
      binding("%dup", "i64", lir::LirInlineAsmValueRole::ReadWrite, 0)};
  expect_rejected(one_function_module("inconsistent", {first, inconsistent}),
                  "unpaired read/write input must fail without publication");

  lir::LirModule foreign;
  auto foreign_def = return_block(0, "entry");
  foreign_def.insts = {first};
  foreign.functions.push_back(
      void_definition("foreign_def", {std::move(foreign_def)}));
  auto foreign_use = return_block(0, "entry");
  auto use = void_inline_asm("foreign use", "r");
  use.ordinary_inputs = {
      binding("%dup", "i64", lir::LirInlineAsmValueRole::Input, 0)};
  foreign_use.insts = {std::move(use)};
  foreign.functions.push_back(
      void_definition("foreign_use", {std::move(foreign_use)}));
  expect_rejected(std::move(foreign),
                  "foreign-function identity must fail without publication");
}

void test_lir_inline_asm_structured_value_contract() {
  static_assert(std::is_same_v<
                decltype(lir::LirInlineAsmValueBinding::value),
                lir::LirOperand>);
  const auto binding = [](std::string value, std::string type,
                          lir::LirInlineAsmValueRole role,
                          std::size_t constraint_index) {
    return lir::LirInlineAsmValueBinding{
        lir::LirOperand(std::move(value)), lir::LirTypeRef(std::move(type)), role,
        constraint_index};
  };
  const auto verify_op = [](lir::LirInlineAsmOp op) {
    lir::LirModule module;
    auto block = return_block(0, "entry");
    block.insts.push_back(std::move(op));
    auto function =
        void_definition("structured_lir_asm", {std::move(block)});
    function.signature_text = "define void @structured_lir_asm()";
    module.functions.push_back(std::move(function));
    lir::verify_module(module);
  };
  const auto expect_rejected = [&](lir::LirInlineAsmOp op,
                                   const std::string& message) {
    try {
      verify_op(std::move(op));
    } catch (const lir::LirVerifyError&) {
      return;
    }
    fail(message);
  };

  auto input = void_inline_asm("llvm input rendering", "r");
  input.original_asm_text = "opaque input %0";
  input.original_constraint_text = "r";
  input.ordinary_inputs = {
      binding("%input", "i64", lir::LirInlineAsmValueRole::Input, 0)};
  verify_op(input);
  expect(input.original_asm_text == "opaque input %0" &&
             input.asm_text == "llvm input rendering" &&
             input.side_effects &&
             input.clobbers == std::vector<std::string>({"memory", "cc"}),
         "semantic text, ordered clobbers, and side effects must remain "
         "distinct from LLVM compatibility rendering");

  auto output = void_inline_asm("llvm output rendering", "=r");
  output.original_asm_text = "opaque output %0";
  output.original_constraint_text = "=r";
  output.ordinary_results = {
      binding("%output", "i64", lir::LirInlineAsmValueRole::Output, 0)};
  verify_op(output);

  auto read_write = void_inline_asm("llvm read/write rendering", "+r,r");
  read_write.original_asm_text = "opaque read/write %0, %1";
  read_write.original_constraint_text = "+r,r";
  read_write.ordinary_inputs = {
      binding("%old", "i64", lir::LirInlineAsmValueRole::ReadWrite, 0),
      binding("%input", "i64", lir::LirInlineAsmValueRole::Input, 1)};
  read_write.ordinary_results = {
      binding("%new", "i64", lir::LirInlineAsmValueRole::ReadWrite, 0)};
  verify_op(read_write);
  expect(read_write.ordinary_inputs[0].value !=
             read_write.ordinary_results[0].value,
         "read/write LIR asm must use distinct ordinary input/result IDs");

  auto invalid_identity = input;
  invalid_identity.ordinary_inputs[0].value = lir::LirOperand();
  expect_rejected(std::move(invalid_identity),
                  "invalid ordinary input identity must be rejected");

  auto invalid_type = output;
  invalid_type.ordinary_results[0].type = lir::LirTypeRef("void");
  expect_rejected(std::move(invalid_type),
                  "void structured result type must be rejected");

  auto invalid_role = input;
  invalid_role.ordinary_inputs[0].role = lir::LirInlineAsmValueRole::Output;
  expect_rejected(std::move(invalid_role),
                  "output role in the ordered input list must be rejected");

  auto invalid_order = read_write;
  invalid_order.ordinary_inputs[0].constraint_index = 1;
  invalid_order.ordinary_inputs[1].constraint_index = 0;
  expect_rejected(std::move(invalid_order),
                  "structured bindings outside constraint order must be rejected");

  auto reused_identity = read_write;
  reused_identity.ordinary_results[0].value =
      reused_identity.ordinary_inputs[0].value;
  expect_rejected(std::move(reused_identity),
                  "read/write input/result identity reuse must be rejected");

  auto mismatched_read_write_type = read_write;
  mismatched_read_write_type.ordinary_results[0].type = lir::LirTypeRef("i32");
  expect_rejected(std::move(mismatched_read_write_type),
                  "read/write input/result type mismatch must be rejected");
}

void test_closed_typed_lir_type_receipt() {
  lir::LirModule module;
  auto texts = std::make_shared<c4c::TextTable>();
  module.link_name_texts = texts;
  module.struct_names.attach_text_table(texts.get());
  const c4c::StructNameId pair_id = module.struct_names.intern("%struct.Pair");

  const std::vector<lir::LirTypeRef> value_types = {
      lir::LirTypeRef::integer(257),
      lir::LirTypeRef("x86_fp80"),
      lir::LirTypeRef("ptr"),
      lir::LirTypeRef("<4 x i32>"),
      lir::LirTypeRef::vrm_register(8),
      lir::LirTypeRef("[3 x i16]"),
      lir::LirTypeRef::struct_type("%struct.Pair", pair_id),
      lir::LirTypeRef("{ i32, ptr }"),
      lir::LirTypeRef("i32 (i8)", lir::LirTypeKind::Function),
      lir::LirTypeRef("%opaque.Payload", lir::LirTypeKind::Opaque),
  };
  const std::vector<bir::TypeKind> expected_kinds = {
      bir::TypeKind::Integer,     bir::TypeKind::Floating,
      bir::TypeKind::Pointer,     bir::TypeKind::Vector,
      bir::TypeKind::VrmRegister, bir::TypeKind::Array,
      bir::TypeKind::Struct,      bir::TypeKind::Struct,
      bir::TypeKind::Function,    bir::TypeKind::Opaque,
  };
  auto producer = void_inline_asm(
      "typed producers", "=r,=r,=r,=r,=r,=r,=r,=r,=r,=r");
  for (std::size_t index = 0; index < value_types.size(); ++index)
    producer.ordinary_results.push_back(lir::LirInlineAsmValueBinding{
        lir::LirOperand("%typed." + std::to_string(index)), value_types[index],
        lir::LirInlineAsmValueRole::Output, index});
  auto block = return_block(0, "entry");
  block.insts.push_back(std::move(producer));
  module.functions.push_back(void_definition("typed_asm", {std::move(block)}));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(), "all value-capable LirTypeKind alternatives and "
                               "the void signature should publish typed RawBir");
  const auto view = imported.value().view();
  const auto functions = view.functions();
  expect(functions.size() == 1, "typed asm definition should survive");
  auto function = view.function(functions.front());
  expect(function.has_value() &&
             function.value().signature().return_type.kind == bir::TypeKind::Void,
         "function signature receipt should use the shared conversion for void");
  const auto typed_block = function.value().blocks().front();
  const auto typed_inst = function.value().instructions(typed_block).value().front();
  const auto results = function.value().instruction(typed_inst).value().results();
  expect(results.size() == value_types.size(),
         "each closed value type should retain an ordinary result identity");
  for (std::size_t index = 0; index < results.size(); ++index) {
    const bir::Type type = function.value().value(results[index]).value().type;
    expect(type.kind == expected_kinds[index] && bir::is_well_formed(type),
           "Raw value view should expose a well-formed closed type kind");
  }
  expect(function.value().value(results[0]).value().type.bit_width == 257,
         "integer widths beyond the legacy fixed enum must remain exact");
  const auto fp80 = function.value().value(results[1]).value().type;
  expect(fp80.bit_width == 80 && fp80.spelling == "x86_fp80",
         "floating spelling and typed width must remain exact");
  expect(function.value().value(results[6]).value().type.struct_name_id == pair_id,
         "struct receipt must preserve StructNameId identity");
}

void test_structured_type_spec_signature_receipt() {
  lir::LirModule module;
  auto mirrored = void_declaration("mirrored_void");
  mirrored.return_type.inner_rank = -1;
  module.functions.push_back(std::move(mirrored));
  auto structured_only = void_declaration("structured_only_void");
  structured_only.signature_return_type_ref.reset();
  structured_only.return_type.inner_rank = -1;
  module.functions.push_back(std::move(structured_only));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "valid structured void TypeSpec should reconcile with or without a mirror");
  const auto functions = imported.value().view().functions();
  for (const auto function_id : functions) {
    const auto function = imported.value().view().function(function_id);
    const auto type = function.value().signature().return_type;
    expect(type.kind == bir::TypeKind::Void && type.structured_spec.has_value(),
           "Raw signature view must expose structured TypeSpec authority");
    const auto& facts = *type.structured_spec;
    expect(facts.base == bir::StructuredTypeBase::Void &&
               facts.pointer_level == 0 && !facts.is_lvalue_reference &&
               !facts.is_rvalue_reference && facts.array_rank == 0 &&
               !facts.is_pointer_to_array && facts.inner_array_rank == 0 &&
               !facts.is_function_pointer,
           "valid structured void facts must survive without invented shape");
  }
}

void test_direct_scalar_signature_receipt() {
  lir::LirModule module;
  module.target_profile.arch = c4c::TargetArch::I686;
  module.target_profile.os = c4c::TargetOs::Linux;

  const auto declaration = [](std::string name, c4c::TypeSpec type,
                              std::optional<lir::LirTypeRef> mirror) {
    lir::LirFunction function;
    function.name = std::move(name);
    function.is_declaration = true;
    function.return_type = type;
    function.signature_return_type_ref = std::move(mirror);
    return function;
  };

  auto integer = scalar_type(c4c::TB_INT);
  integer.inner_rank = -1;
  module.functions.push_back(declaration(
      "integer_mirror", integer, lir::LirTypeRef::integer(32)));

  auto floating = scalar_type(c4c::TB_DOUBLE);
  floating.inner_rank = 0;
  module.functions.push_back(
      declaration("floating_without_mirror", floating, std::nullopt));

  auto target_long = scalar_type(c4c::TB_LONG);
  target_long.inner_rank = -1;
  module.functions.push_back(declaration(
      "target_long", target_long, lir::LirTypeRef::integer(32)));

  auto target_long_double = scalar_type(c4c::TB_LONGDOUBLE);
  target_long_double.inner_rank = -1;
  module.functions.push_back(declaration(
      "target_long_double", target_long_double, lir::LirTypeRef("x86_fp80")));

  auto normalized_enum = scalar_type(c4c::TB_ENUM);
  normalized_enum.enum_underlying_base = c4c::TB_USHORT;
  normalized_enum.inner_rank = -1;
  module.functions.push_back(declaration(
      "normalized_enum", normalized_enum, lir::LirTypeRef::integer(16)));

  lir::LirFunction definition;
  definition.name = "nonvoid_unreachable";
  definition.return_type = integer;
  definition.signature_return_type_ref.reset();
  definition.blocks.push_back(unreachable_block(0, "entry"));
  definition.entry = definition.blocks.front().id;
  module.functions.push_back(std::move(definition));

  const auto add_stale_signature_shadow = [&](lir::LirFunction* function) {
    function->params.emplace_back("%logical", integer);
    function->signature_params.push_back({"%structured", integer, false});
    function->signature_param_type_refs.push_back(lir::LirTypeRef::integer(32));
    function->signature_return_type_ref = lir::LirTypeRef::integer(32);
  };
  auto stale_declaration = declaration(
      "stale_scalar_declaration", integer, lir::LirTypeRef::integer(32));
  add_stale_signature_shadow(&stale_declaration);
  stale_declaration.signature_text =
      "declare i64 @stale_scalar_declaration(double %legacy)";
  module.functions.push_back(std::move(stale_declaration));

  lir::LirFunction stale_definition;
  stale_definition.name = "stale_scalar_definition";
  stale_definition.return_type = integer;
  stale_definition.blocks.push_back(unreachable_block(0, "entry"));
  stale_definition.entry = stale_definition.blocks.front().id;
  add_stale_signature_shadow(&stale_definition);
  stale_definition.signature_text =
      "define i64 @stale_scalar_definition(double %legacy) {";
  module.functions.push_back(std::move(stale_definition));

  const std::vector<bir::Type> expected = {
      {bir::TypeKind::Integer, 32, "i32"},
      {bir::TypeKind::Floating, 64, "double"},
      {bir::TypeKind::Integer, 32, "i32"},
      {bir::TypeKind::Floating, 80, "x86_fp80"},
      {bir::TypeKind::Integer, 16, "i16"},
      {bir::TypeKind::Integer, 32, "i32"},
      {bir::TypeKind::Integer, 32, "i32"},
      {bir::TypeKind::Integer, 32, "i32"},
  };
  const auto expect_signatures = [&](const auto& view,
                                     const std::string& layer) {
    const auto functions = view.functions();
    expect(functions.size() == expected.size(),
           layer + " must retain every scalar signature");
    for (std::size_t index = 0; index < functions.size(); ++index) {
      const auto function = view.function(functions[index]);
      expect(function.has_value() &&
                 function.value().signature().return_type == expected[index],
             layer + " must retain exact scalar return kind, width, and spelling");
    }
    for (const std::size_t index : {std::size_t{6}, std::size_t{7}}) {
      const auto function = view.function(functions[index]).value();
      expect(function.signature().parameter_types.size() == 1 &&
                 function.signature().parameter_types.front() ==
                     bir::Type{bir::TypeKind::Integer, 32, "i32"},
             layer +
                 " must lower complete scalar parameter facts before stale signature text");
    }
  };

  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value(),
         "producer-valid direct scalar signatures should publish RawBir");
  expect_signatures(raw.value().view(), "RawBir");
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "foundation verification must accept direct scalar signatures");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "producer-valid direct scalar signatures should publish CanonicalBir");
  expect_signatures(canonical.value().view(), "CanonicalBir");

  lir::LirModule windows;
  windows.target_profile.arch = c4c::TargetArch::X86_64;
  windows.target_profile.os = c4c::TargetOs::Windows;
  windows.functions.push_back(declaration(
      "windows_long_double", target_long_double, lir::LirTypeRef("double")));
  const auto windows_raw = bir::lower_lir_to_raw_bir(windows);
  expect(windows_raw.has_value() &&
             windows_raw.value()
                     .view()
                     .function(windows_raw.value().view().functions().front())
                     .value()
                     .signature()
                     .return_type ==
                 bir::Type{bir::TypeKind::Floating, 64, "double"},
         "Windows long double signatures must use the target-shaped double carrier");

  lir::LirModule lp64;
  lp64.target_profile = c4c::default_target_profile(c4c::TargetArch::X86_64);
  lp64.functions.push_back(declaration(
      "lp64_long", target_long, lir::LirTypeRef::integer(64)));
  const auto lp64_raw = bir::lower_lir_to_raw_bir(lp64);
  expect(lp64_raw.has_value() &&
             lp64_raw.value()
                     .view()
                     .function(lp64_raw.value().view().functions().front())
                     .value()
                     .signature()
                     .return_type == bir::Type{bir::TypeKind::Integer, 64, "i64"},
         "LP64 long signatures must retain i64 structured mirrors");
}

void test_direct_scalar_signature_rejections_and_transactionality() {
  const auto rejected = [](auto mutate, const std::string& message) {
    lir::LirModule module;
    module.target_profile = c4c::default_target_profile(c4c::TargetArch::I686);
    auto accepted = void_declaration("accepted_before_failure");
    accepted.return_type.inner_rank = -1;
    module.functions.push_back(std::move(accepted));

    lir::LirFunction candidate;
    candidate.name = "rejected_scalar";
    candidate.is_declaration = true;
    candidate.return_type = scalar_type(c4c::TB_INT);
    candidate.return_type.inner_rank = -1;
    candidate.signature_return_type_ref = lir::LirTypeRef::integer(32);
    mutate(candidate);
    module.functions.push_back(std::move(candidate));

    const auto raw = bir::lower_lir_to_raw_bir(module);
    expect(!raw.has_value() &&
               raw.error().code == bir::ImportErrorCode::UnsupportedReturnType,
           message + " (Raw rollback)");
    const auto canonical = bir::lower_lir_to_canonical_bir(module);
    expect(!canonical.has_value() &&
               canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedReturnType,
           message + " (Canonical rollback)");
  };

  rejected(
      [](lir::LirFunction& function) {
        function.signature_return_type_ref = lir::LirTypeRef::integer(64);
      },
      "conflicting integer mirrors must reject the complete module transactionally");
  rejected(
      [](lir::LirFunction& function) {
        function.return_type.base = c4c::TB_LONG;
        function.signature_return_type_ref = lir::LirTypeRef::integer(64);
      },
      "I686 long i64 return mirrors must reject target-policy conflicts");
  rejected(
      [](lir::LirFunction& function) {
        function.signature_return_type_ref = lir::LirTypeRef("float");
      },
      "wrong-kind floating mirrors must reject integer structured authority");
  rejected(
      [](lir::LirFunction& function) {
        function.return_type.base = c4c::TB_DOUBLE;
        function.signature_return_type_ref = lir::LirTypeRef("float");
      },
      "wrong-width floating mirrors must reject structured floating authority");
  for (const int invalid_inner_rank : {-2, 1}) {
    rejected(
        [invalid_inner_rank](lir::LirFunction& function) {
          function.return_type.inner_rank = invalid_inner_rank;
        },
        "invalid scalar inner-rank residue must remain closed");
  }
  rejected(
      [](lir::LirFunction& function) { function.return_type.ptr_level = 1; },
      "pointer returns remain outside direct scalar signature receipt");
  rejected(
      [](lir::LirFunction& function) {
        function.return_type.is_lvalue_ref = true;
      },
      "lvalue-reference returns remain outside direct scalar signature receipt");
  rejected(
      [](lir::LirFunction& function) {
        function.return_type.is_rvalue_ref = true;
      },
      "rvalue-reference returns remain outside direct scalar signature receipt");
  rejected(
      [](lir::LirFunction& function) {
        function.return_type.array_rank = 1;
      },
      "array returns remain outside direct scalar signature receipt");
  rejected(
      [](lir::LirFunction& function) {
        function.return_type.is_ptr_to_array = true;
      },
      "pointer-to-array residue remains outside direct scalar signature receipt");
  rejected(
      [](lir::LirFunction& function) {
        function.return_type.is_fn_ptr = true;
      },
      "function-pointer returns remain outside direct scalar signature receipt");
  rejected(
      [](lir::LirFunction& function) {
        function.return_type.is_vector = true;
        function.return_type.vector_lanes = 4;
        function.return_type.vector_bytes = 16;
      },
      "vector returns remain outside direct scalar signature receipt");
  rejected(
      [](lir::LirFunction& function) {
        function.return_type.vector_lanes = 4;
      },
      "residual vector facts must not be discarded by scalar receipt");
  rejected(
      [](lir::LirFunction& function) {
        function.return_type.array_size = 4;
      },
      "positive residual array size must not be discarded by scalar receipt");
  rejected(
      [](lir::LirFunction& function) {
        function.return_type.array_dims[0] = 4;
      },
      "positive residual array dimensions must not be discarded by scalar receipt");
  rejected(
      [](lir::LirFunction& function) {
        function.return_type.array_size_expr =
            reinterpret_cast<c4c::Node*>(1);
      },
      "unevaluated array-size residue must not be discarded by scalar receipt");
  rejected(
      [](lir::LirFunction& function) {
        function.return_type.enum_underlying_base = c4c::TB_LONG;
      },
      "non-enum returns must reject residual enum storage authority");
  rejected(
      [](lir::LirFunction& function) {
        function.return_type.base = c4c::TB_ENUM;
        function.return_type.enum_underlying_base = c4c::TB_FLOAT;
      },
      "enum returns must reject non-integer underlying storage");
}

void test_plain_parameter_signature_receipt() {
  lir::LirModule module;
  module.target_profile.arch = c4c::TargetArch::X86_64;
  module.target_profile.os = c4c::TargetOs::Linux;
  module.functions.push_back(void_declaration("empty_params"));
  module.functions.push_back(
      void_definition("empty_params", {return_block(0, "entry")}));
  module.functions.push_back(
      explicit_void_parameter_function("void_params", true));
  module.functions.push_back(
      explicit_void_parameter_function("void_params", false));
  const std::vector<c4c::TypeBase> bases = {
      c4c::TB_INT,       c4c::TB_UINT,  c4c::TB_LONG,
      c4c::TB_ULONG,     c4c::TB_LONGLONG, c4c::TB_ULONGLONG,
      c4c::TB_FLOAT,     c4c::TB_DOUBLE};
  module.functions.push_back(
      stable_parameter_function("stable_params", true, bases));
  module.functions.push_back(
      stable_parameter_function("stable_params", false, bases));

  const std::vector<bir::Type> expected_types = {
      {bir::TypeKind::Integer, 32, "i32"},
      {bir::TypeKind::Integer, 32, "i32"},
      {bir::TypeKind::Integer, 64, "i64"},
      {bir::TypeKind::Integer, 64, "i64"},
      {bir::TypeKind::Integer, 64, "i64"},
      {bir::TypeKind::Integer, 64, "i64"},
      {bir::TypeKind::Floating, 32, "float"},
      {bir::TypeKind::Floating, 64, "double"},
  };
  const auto expect_graph = [&](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto functions = view.functions();
    expect(functions.size() == 3,
           layer + " should merge matching declarations and definitions");
    for (std::size_t index = 0; index < 2; ++index) {
      const auto function = view.function(functions[index]).value();
      expect(function.signature().parameter_types.empty() &&
                 function.parameters().empty() && !function.is_declaration(),
             layer + " should preserve empty and explicit-void as zero ABI parameters");
    }
    const auto function = view.function(functions[2]).value();
    expect(function.signature().parameter_types == expected_types &&
               function.parameters().size() == expected_types.size() &&
               !function.signature().is_variadic && !function.is_declaration(),
           layer + " should retain exact stable scalar signature order and types");
    const auto parameters = function.parameters();
    for (std::size_t ordinal = 0; ordinal < parameters.size(); ++ordinal) {
      const auto value = function.value(parameters[ordinal]).value();
      const auto* definition = std::get_if<bir::ParameterDef>(&value.definition);
      expect(parameters[ordinal].kind == bir::ValueKind::Parameter &&
                 value.kind == bir::ValueKind::Parameter && definition &&
                 definition->ordinal == ordinal &&
                 value.type == expected_types[ordinal],
             layer + " should use typed ordinal-only ParameterDef storage");
    }
  };

  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value(),
         "exact zero, void-list, and stable scalar signatures should publish RawBir");
  expect_graph(raw.value(), "RawBir");
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "foundation verification should reach imported parameter definitions");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "the same parameter graph should publish CanonicalBir");
  expect_graph(canonical.value(), "CanonicalBir");

  lir::LirModule i686;
  i686.target_profile = c4c::default_target_profile(c4c::TargetArch::I686);
  const std::vector<c4c::TypeBase> i686_bases = {c4c::TB_LONG, c4c::TB_ULONG};
  i686.functions.push_back(stable_parameter_function(
      "i686_long_params", true, i686_bases, c4c::TargetArch::I686));
  i686.functions.push_back(stable_parameter_function(
      "i686_long_params", false, i686_bases, c4c::TargetArch::I686));
  const auto i686_raw = bir::lower_lir_to_raw_bir(i686);
  expect(i686_raw.has_value() &&
             i686_raw.value().view().function(i686_raw.value().view().functions().front())
                     .value().signature().parameter_types ==
                 std::vector<bir::Type>{{bir::TypeKind::Integer, 32, "i32"},
                                        {bir::TypeKind::Integer, 32, "i32"}},
         "I686 long and unsigned-long parameters must receive i32 signatures");
}

void test_plain_parameter_signature_rejections_and_transactionality() {
  const std::vector<c4c::TypeBase> bases = {c4c::TB_INT, c4c::TB_DOUBLE};
  const auto rejected = [&](auto mutate, bir::ImportErrorCode expected,
                            const std::string& message,
                            c4c::TargetArch arch = c4c::TargetArch::X86_64) {
    lir::LirModule module;
    module.target_profile.arch = arch;
    module.target_profile.os = c4c::TargetOs::Linux;
    module.functions.push_back(void_declaration("accepted_before_parameters"));
    auto candidate = stable_parameter_function("rejected_parameters", true, bases);
    mutate(candidate);
    module.functions.push_back(std::move(candidate));
    const auto raw = bir::lower_lir_to_raw_bir(module);
    expect(!raw.has_value() && raw.error().code == expected,
           message + " (Raw rollback)");
    const auto canonical = bir::lower_lir_to_canonical_bir(module);
    expect(!canonical.has_value() && canonical.error().code == expected,
           message + " (Canonical rollback)");
  };

  rejected([](auto& function) { function.params.pop_back(); },
           bir::ImportErrorCode::UnsupportedFunctionParameters,
           "missing logical parameter authority must reject transactionally");
  rejected([](auto& function) { function.signature_params.pop_back(); },
           bir::ImportErrorCode::UnsupportedFunctionParameters,
           "missing structured ABI parameter authority must reject transactionally");
  rejected(
      [](auto& function) { function.signature_param_type_refs.pop_back(); },
      bir::ImportErrorCode::UnsupportedFunctionParameters,
      "missing typed mirror authority must reject transactionally");
  rejected([](auto& function) { std::swap(function.params[0], function.params[1]); },
           bir::ImportErrorCode::UnsupportedFunctionParameters,
           "reordered logical authority must reject transactionally");
  rejected(
      [](auto& function) { function.signature_params[0].type.base = c4c::TB_UINT; },
      bir::ImportErrorCode::UnsupportedFunctionParameters,
      "conflicting structured scalar types must reject transactionally");
  rejected(
      [](auto& function) {
        function.signature_param_type_refs[0] =
            lir::LirTypeRef("i32", lir::LirTypeKind::RawText);
      },
      bir::ImportErrorCode::UnsupportedFunctionParameters,
      "raw-text mirrors must reject transactionally");
  rejected([](auto& function) { function.params[0].second.align_bytes = 16; },
           bir::ImportErrorCode::UnsupportedFunctionParameters,
           "alignment residue must reject transactionally");
  rejected([](auto& function) { function.signature_params[0].type.is_const = true; },
           bir::ImportErrorCode::UnsupportedFunctionParameters,
           "cv residue must reject transactionally");
  rejected(
      [](auto& function) {
        function.params[0].second.template_param_owner_namespace_context_id = 7;
      },
      bir::ImportErrorCode::UnsupportedFunctionParameters,
      "template metadata residue must reject transactionally");
  rejected([](auto& function) { function.params[0].second.tag_text_id = 1; },
           bir::ImportErrorCode::UnsupportedFunctionParameters,
           "tag metadata residue must reject transactionally");
  rejected(
      [](auto& function) {
        function.params[0].second.deferred_member_type_name = "display-only";
      },
      bir::ImportErrorCode::UnsupportedFunctionParameters,
      "deferred metadata residue must reject transactionally");
  rejected(
      [](auto& function) {
        function.params[0].second.ptr_level = 1;
        function.signature_params[0].type.ptr_level = 1;
        function.signature_param_type_refs[0] = lir::LirTypeRef("ptr");
      },
      bir::ImportErrorCode::UnsupportedFunctionParameters,
      "pointer parameters must remain fail-closed");
  rejected(
      [](auto& function) {
        function.params[0].second.base = c4c::TB_CHAR;
        function.signature_params[0].type.base = c4c::TB_CHAR;
        function.signature_param_type_refs[0] = lir::LirTypeRef::integer(8);
      },
      bir::ImportErrorCode::UnsupportedFunctionParameters,
      "narrow parameters must remain fail-closed");
  rejected(
      [](auto& function) {
        function.params[0].second.base = c4c::TB_ULONG;
        function.signature_params[0].type.base = c4c::TB_ULONG;
        function.signature_param_type_refs[0] = lir::LirTypeRef::integer(64);
      },
      bir::ImportErrorCode::UnsupportedFunctionParameters,
      "I686 unsigned-long i64 mirrors must reject target-policy conflicts",
      c4c::TargetArch::I686);
  rejected(
      [](auto& function) {
        function.params[0].second.base = c4c::TB_STRUCT;
        function.signature_params[0].type.base = c4c::TB_STRUCT;
        function.signature_params[0].is_byval = true;
        function.signature_param_type_refs[0] = lir::LirTypeRef("ptr");
      },
      bir::ImportErrorCode::UnsupportedFunctionParameters,
      "aggregate byval parameters must remain fail-closed");
  rejected(
      [](auto& function) {
        function.params[0].second.base = c4c::TB_STRUCT;
        function.signature_params[0].type.base = c4c::TB_STRUCT;
        function.signature_param_type_refs[0] =
            lir::LirTypeRef("%struct.Direct", lir::LirTypeKind::Struct);
      },
      bir::ImportErrorCode::UnsupportedFunctionParameters,
      "direct aggregate parameters must remain fail-closed");
  rejected(
      [](auto& function) {
        function.params.resize(1);
        function.params[0].second.base = c4c::TB_STRUCT;
        for (auto& signature : function.signature_params)
          signature.type = scalar_type(c4c::TB_FLOAT);
        for (auto& mirror : function.signature_param_type_refs)
          mirror = lir::LirTypeRef("float");
      },
      bir::ImportErrorCode::UnsupportedFunctionParameters,
      "one-logical-to-many-ABI HFA parameters must remain fail-closed");
  rejected(
      [](auto& function) {
        function.params[0].second.is_vector = true;
        function.params[0].second.vector_lanes = 4;
        function.params[0].second.vector_bytes = 16;
        function.signature_params[0].type = function.params[0].second;
        function.signature_param_type_refs[0] = lir::LirTypeRef("<4 x i32>");
      },
      bir::ImportErrorCode::UnsupportedFunctionParameters,
      "vector parameter transformations must remain fail-closed");
  rejected(
      [](auto& function) {
        function.params[0].second.is_fn_ptr = true;
        function.signature_params[0].type.is_fn_ptr = true;
        function.signature_param_type_refs[0] = lir::LirTypeRef("ptr");
      },
      bir::ImportErrorCode::UnsupportedFunctionParameters,
      "function-pointer parameters must remain fail-closed");
  rejected(
      [](auto& function) {
        function.params[0].second.base = c4c::TB_VA_LIST;
        function.signature_params[0].type.base = c4c::TB_VA_LIST;
        function.signature_param_type_refs[0] = lir::LirTypeRef("ptr");
      },
      bir::ImportErrorCode::UnsupportedFunctionParameters,
      "va-list parameters must remain fail-closed");
  rejected([](auto& function) { function.signature_is_variadic = true; },
           bir::ImportErrorCode::UnsupportedVariadicFunction,
           "variadic fixed prefixes must remain fail-closed");

  const auto rejected_void = [&](auto mutate, const std::string& message) {
    lir::LirModule module;
    module.functions.push_back(void_declaration("accepted_before_void"));
    auto candidate = explicit_void_parameter_function("rejected_void", true);
    mutate(candidate);
    module.functions.push_back(std::move(candidate));
    const auto raw = bir::lower_lir_to_raw_bir(module);
    expect(!raw.has_value() && raw.error().code ==
                                   bir::ImportErrorCode::UnsupportedFunctionParameters,
           message + " (Raw rollback)");
    const auto canonical = bir::lower_lir_to_canonical_bir(module);
    expect(!canonical.has_value() && canonical.error().code ==
                                         bir::ImportErrorCode::UnsupportedFunctionParameters,
           message + " (Canonical rollback)");
  };
  rejected_void([](auto& function) { function.params.clear(); },
                "void-list flag without its logical sentinel must reject");
  rejected_void(
      [](auto& function) {
        function.signature_params.push_back({"%invented", scalar_type(c4c::TB_INT), false});
        function.signature_param_type_refs.push_back(lir::LirTypeRef::integer(32));
      },
      "void-list signatures must reject invented ABI parameters");
}

void test_function_metadata_builder_contract() {
  bir::ModuleBuilder builder;
  const bir::FunctionSignature signature{
      bir::Type{bir::TypeKind::Void}, {}, false};
  const auto declaration =
      builder.create_function(signature, "definition_looking_declaration", true);
  const auto external =
      builder.create_function(signature, "static_looking_external", false);
  const auto helper = builder.create_function(
      signature, "ordinary_looking_helper", false,
      bir::FunctionMetadata{false, true});
  const auto internal = builder.create_function(
      signature, "extern_looking_internal", false,
      bir::FunctionMetadata{true, true});
  expect(declaration.has_value() && external.has_value() && helper.has_value() &&
             internal.has_value(),
         "every producer-valid function metadata combination should stage");
  expect(builder
             .create_function(signature, "invalid_internal", false,
                              bir::FunctionMetadata{true, false})
             .error() == bir::BuildError::InvalidFunctionMetadata,
         "internal/non-elidable metadata must reject at the builder boundary");
  expect(builder
             .create_function(signature, "invalid_helper_declaration", true,
                              bir::FunctionMetadata{false, true})
             .error() == bir::BuildError::InvalidFunctionMetadata,
         "declarations cannot carry helper elision metadata");
  expect(builder
             .create_function(signature, "invalid_internal_declaration", true,
                              bir::FunctionMetadata{true, true})
             .error() == bir::BuildError::InvalidFunctionMetadata,
         "declarations cannot carry internal metadata");

  const auto merge = builder.create_function(signature, "metadata_merge", true);
  expect(merge.has_value(), "metadata merge fixture declaration should stage");
  expect(builder
             .create_function(signature, "metadata_merge", false,
                              bir::FunctionMetadata{false, true})
             .error() == bir::BuildError::ConflictingDeclaration,
         "metadata conflicts must reject before declaration mutation");
  const auto coherent_merge =
      builder.create_function(signature, "metadata_merge", false);
  expect(coherent_merge.has_value() && coherent_merge.value() == merge.value(),
         "a coherent merge after rejection should reuse the untouched declaration");

  const auto add_body = [&](bir::FunctionId function_id) {
    return builder.with_function(
        function_id, [](bir::FunctionBuilder& function) {
          const auto block = function.create_block("entry");
          if (!block)
            return bir::Result<void, bir::BuildError>::failure(block.error());
          return function.set_terminator(block.value(), bir::ReturnTerm{});
        });
  };
  expect(add_body(external.value()).has_value() &&
             add_body(helper.value()).has_value() &&
             add_body(internal.value()).has_value() &&
             add_body(coherent_merge.value()).has_value(),
         "valid metadata definitions should accept ordinary bodies");

  auto raw = std::move(builder).publish();
  expect(raw.has_value(),
         "valid metadata and conflict rollback should publish verified RawBir");
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "FoundationVerifier should reach valid function metadata");
  const auto view = raw.value().view();
  const auto functions = view.functions();
  expect(functions.size() == 5,
         "rejected metadata staging must append no partial function");
  const auto decl_view = view.function(functions[0]).value();
  const auto external_view = view.function(functions[1]).value();
  const auto helper_view = view.function(functions[2]).value();
  const auto internal_view = view.function(functions[3]).value();
  const auto merge_view = view.function(functions[4]).value();
  expect(decl_view.is_declaration() && !decl_view.is_internal() &&
             !decl_view.can_elide_if_unreferenced() &&
             !external_view.is_declaration() && !external_view.is_internal() &&
             !external_view.can_elide_if_unreferenced() &&
             !helper_view.is_internal() &&
             helper_view.can_elide_if_unreferenced() &&
             internal_view.is_internal() &&
             internal_view.can_elide_if_unreferenced() &&
             !merge_view.is_declaration() && !merge_view.is_internal() &&
             !merge_view.can_elide_if_unreferenced(),
         "immutable views must preserve metadata facts independent of names/order");
}

lir::LirModule function_metadata_module() {
  lir::LirModule module;
  auto declaration = void_declaration("definition_looking_declaration");
  declaration.signature_text = "define internal void @wrong() {";
  module.functions.push_back(std::move(declaration));

  auto external =
      void_definition("static_looking_external", {return_block(0, "entry")});
  external.signature_text = "define internal void @wrong() {";
  module.functions.push_back(std::move(external));

  auto helper =
      void_definition("ordinary_looking_helper", {return_block(0, "entry")});
  helper.can_elide_if_unreferenced = true;
  helper.signature_text = "define void @wrong() {";
  module.functions.push_back(std::move(helper));

  auto internal =
      void_definition("extern_looking_internal", {return_block(0, "entry")});
  internal.is_internal = true;
  internal.can_elide_if_unreferenced = true;
  internal.signature_text = "declare void @wrong()";
  module.functions.push_back(std::move(internal));

  module.functions.push_back(void_declaration("merged_external"));
  module.functions.push_back(
      void_definition("merged_external", {return_block(0, "entry")}));
  return module;
}

void test_function_metadata_import_receipt() {
  const auto expect_graph = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto functions = view.functions();
    expect(functions.size() == 5,
           layer + " should preserve source order and coherent merges");
    const auto declaration = view.function(functions[0]).value();
    const auto external = view.function(functions[1]).value();
    const auto helper = view.function(functions[2]).value();
    const auto internal = view.function(functions[3]).value();
    const auto merged = view.function(functions[4]).value();
    expect(declaration.is_declaration() && !declaration.is_internal() &&
               !declaration.can_elide_if_unreferenced() &&
               !external.is_declaration() && !external.is_internal() &&
               !external.can_elide_if_unreferenced() && !helper.is_internal() &&
               helper.can_elide_if_unreferenced() && internal.is_internal() &&
               internal.can_elide_if_unreferenced() &&
               !merged.is_declaration() && !merged.is_internal() &&
               !merged.can_elide_if_unreferenced(),
           layer + " must retain native LIR metadata without display inference");
  };

  const auto module = function_metadata_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value(),
         "producer-valid linkage/elision metadata should publish RawBir");
  expect_graph(raw.value(), "RawBir");
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "FoundationVerifier must accept imported function metadata");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "producer-valid linkage/elision metadata should publish CanonicalBir");
  expect_graph(canonical.value(), "CanonicalBir");
}

void test_function_metadata_import_rejections_and_transactionality() {
  const auto rejected = [](auto mutate, const std::string& message) {
    auto module = function_metadata_module();
    auto candidate =
        void_definition("invalid_metadata", {return_block(0, "entry")});
    mutate(candidate);
    module.functions.push_back(std::move(candidate));
    const auto raw = bir::lower_lir_to_raw_bir(module);
    expect(!raw.has_value() && raw.error().code ==
                                   bir::ImportErrorCode::UnsupportedFunctionMetadata,
           message + " (Raw rollback)");
    const auto canonical = bir::lower_lir_to_canonical_bir(module);
    expect(!canonical.has_value() && canonical.error().code ==
                                         bir::ImportErrorCode::UnsupportedFunctionMetadata,
           message + " (Canonical rollback)");
  };
  rejected(
      [](auto& function) {
        function.is_internal = true;
        function.can_elide_if_unreferenced = false;
      },
      "internal/non-elidable LIR metadata must reject transactionally");
  rejected(
      [](auto& function) {
        function.is_declaration = true;
        function.blocks.clear();
        function.can_elide_if_unreferenced = true;
      },
      "discardable declaration metadata must reject transactionally");
  rejected(
      [](auto& function) {
        function.is_declaration = true;
        function.blocks.clear();
        function.is_internal = true;
        function.can_elide_if_unreferenced = true;
      },
      "internal declaration metadata must reject transactionally");

  auto conflicting = function_metadata_module();
  conflicting.functions.push_back(void_declaration("conflicting_merge"));
  auto definition =
      void_definition("conflicting_merge", {return_block(0, "entry")});
  definition.can_elide_if_unreferenced = true;
  conflicting.functions.push_back(std::move(definition));
  const auto raw = bir::lower_lir_to_raw_bir(conflicting);
  expect(!raw.has_value() && raw.error().code == bir::ImportErrorCode::BuilderFailure &&
             raw.error().build_error == bir::BuildError::ConflictingDeclaration,
         "metadata merge conflicts must reject Raw publication before mutation");
  const auto canonical = bir::lower_lir_to_canonical_bir(conflicting);
  expect(!canonical.has_value() &&
             canonical.error().code == bir::ImportErrorCode::BuilderFailure &&
             canonical.error().build_error ==
                 bir::BuildError::ConflictingDeclaration,
         "metadata merge conflicts must reject Canonical publication transactionally");
}

void test_typed_lir_type_rejections() {
  const auto expect_value_rejected = [](lir::LirTypeRef type,
                                        const std::string& message) {
    lir::LirModule module;
    auto producer = void_inline_asm("bad type", "=r");
    producer.ordinary_results.push_back(lir::LirInlineAsmValueBinding{
        lir::LirOperand("%bad"), std::move(type),
        lir::LirInlineAsmValueRole::Output, 0});
    auto block = return_block(0, "entry");
    block.insts.push_back(std::move(producer));
    module.functions.push_back(void_definition("bad_type", {std::move(block)}));
    auto imported = bir::lower_lir_to_raw_bir(module);
    expect(!imported.has_value() &&
               imported.error().code ==
                   bir::ImportErrorCode::UnsupportedInlineAsmShape,
           message);
  };
  expect_value_rejected(
      lir::LirTypeRef("not-an-integer", lir::LirTypeKind::Integer),
      "integer receipt must reject a missing typed width");
  expect_value_rejected(
      lir::LirTypeRef("c4c.vrm", lir::LirTypeKind::VrmRegister),
      "VRM receipt must reject a missing typed width");
  expect_value_rejected(lir::LirTypeRef("semantic raw text"),
                        "semantic RawText must be rejected");
  expect_value_rejected(
      lir::LirTypeRef("<4 x i32", lir::LirTypeKind::Vector),
      "malformed vector shape must be rejected");
  expect_value_rejected(
      lir::LirTypeRef("i32 (", lir::LirTypeKind::Function),
      "malformed function shape must be rejected");

  lir::LirModule conflicting_type_spec;
  auto conflicting_void = void_declaration("conflicting_type_spec");
  conflicting_void.return_type.base = c4c::TB_INT;
  conflicting_type_spec.functions.push_back(std::move(conflicting_void));
  auto type_spec_conflict = bir::lower_lir_to_raw_bir(conflicting_type_spec);
  expect(!type_spec_conflict.has_value() &&
             type_spec_conflict.error().code ==
                 bir::ImportErrorCode::UnsupportedReturnType,
         "structured TypeSpec must not be overridden by a void type mirror");

  const auto expect_signature_shape_rejected = [](auto mutate,
                                                   const std::string& message) {
    lir::LirModule module;
    auto declaration = void_declaration("bad_signature_shape");
    mutate(declaration);
    module.functions.push_back(std::move(declaration));
    auto imported = bir::lower_lir_to_raw_bir(module);
    expect(!imported.has_value() &&
               imported.error().code == bir::ImportErrorCode::UnsupportedReturnType,
           message);
  };
  expect_signature_shape_rejected(
      [](lir::LirFunction& function) { function.return_type.ptr_level = 1; },
      "structured pointer shape must conflict with void receipt");
  expect_signature_shape_rejected(
      [](lir::LirFunction& function) {
        function.return_type.is_lvalue_ref = true;
      },
      "structured lvalue-reference shape must conflict with void receipt");
  expect_signature_shape_rejected(
      [](lir::LirFunction& function) {
        function.return_type.is_rvalue_ref = true;
      },
      "structured rvalue-reference shape must conflict with void receipt");
  expect_signature_shape_rejected(
      [](lir::LirFunction& function) { function.return_type.array_rank = 1; },
      "structured array shape must conflict with void receipt");
  expect_signature_shape_rejected(
      [](lir::LirFunction& function) { function.return_type.is_fn_ptr = true; },
      "structured function-pointer shape must conflict with void receipt");
  expect_signature_shape_rejected(
      [](lir::LirFunction& function) {
        function.signature_return_type_ref = lir::LirTypeRef::integer(32);
      },
      "non-void mirror must conflict with structured void TypeSpec");

  lir::LirModule invalid_struct;
  auto texts = std::make_shared<c4c::TextTable>();
  invalid_struct.link_name_texts = texts;
  invalid_struct.struct_names.attach_text_table(texts.get());
  const c4c::StructNameId pair_id =
      invalid_struct.struct_names.intern("%struct.Pair");
  auto bad_struct_producer = void_inline_asm("bad struct", "=r");
  bad_struct_producer.ordinary_results.push_back(lir::LirInlineAsmValueBinding{
      lir::LirOperand("%bad"),
      lir::LirTypeRef::struct_type("%struct.Other", pair_id),
      lir::LirInlineAsmValueRole::Output, 0});
  auto bad_struct_block = return_block(0, "entry");
  bad_struct_block.insts.push_back(std::move(bad_struct_producer));
  invalid_struct.functions.push_back(
      void_definition("bad_struct", {std::move(bad_struct_block)}));
  auto bad_struct = bir::lower_lir_to_raw_bir(invalid_struct);
  expect(!bad_struct.has_value() &&
             bad_struct.error().code ==
                 bir::ImportErrorCode::UnsupportedInlineAsmShape,
         "conflicting StructNameId/spelling identity must be rejected");
}

void test_anonymous_aggregate_layout_verifier_rejections() {
  const auto module_with_layout = [](const lir::LirTypeRef& layout) {
    lir::LirModule module;
    auto texts = std::make_shared<c4c::TextTable>();
    module.link_name_texts = texts;
    module.link_names.attach_text_table(texts.get());
    module.struct_names.attach_text_table(texts.get());
    const c4c::LinkNameId callee_id = module.link_names.intern("anon_pair_source");

    lir::LirCallSignature signature;
    signature.return_type_ref = layout;
    signature.fixed_param_types = {"i32"};
    signature.fixed_param_type_refs = {lir::LirTypeRef::integer(32)};

    lir::LirBlock block;
    block.id = lir::LirBlockId{0};
    block.label = "entry";
    block.insts.push_back(lir::LirCallOp{
        .result = lir::LirOperand::ssa("%pair", lir::LirValueId{1}),
        .return_type = layout,
        .callee = lir::LirOperand("@anon_pair_source"),
        .direct_callee_link_name_id = callee_id,
        .callee_type_suffix = "(i32)",
        .args_str = "i32 5",
        .arg_type_refs = {lir::LirTypeRef::integer(32)},
        .callee_signature = std::move(signature),
        .structured_args = {{"i32", lir::LirOperand::integer("5", 5),
                             lir::LirTypeRef::integer(32)}},
    });
    block.insts.push_back(lir::LirExtractValueOp{
        .result = lir::LirOperand::ssa("%field", lir::LirValueId{2}),
        .agg_type = layout,
        .agg = lir::LirOperand::ssa("%pair", lir::LirValueId{1}),
        .index = 0,
        .requires_native_result_authority = true,
        .result_element_type = lir::LirTypeRef("float"),
    });
    block.terminator = lir::LirRet{std::nullopt, lir::LirTypeRef("void")};
    auto function = void_definition("anonymous_layout_verifier", {std::move(block)});
    function.signature_text = "define void @anonymous_layout_verifier()";
    module.functions.push_back(std::move(function));
    return module;
  };

  const lir::LirTypeRef valid_layout = lir::LirTypeRef::anonymous_struct(
      {lir::LirTypeRef("float"), lir::LirTypeRef("float")});
  lir::verify_module(module_with_layout(valid_layout));

  const auto expect_rejected = [&](const lir::LirTypeRef& layout,
                                   const std::string& message) {
    try {
      lir::verify_module(module_with_layout(layout));
      fail(message);
    } catch (const lir::LirVerifyError&) {
    }
  };

  auto stale_layout = valid_layout;
  stale_layout.str() = "{ double, double }";
  expect_rejected(stale_layout,
                  "anonymous aggregate layout must reject a stale display mirror");
  expect_rejected(lir::LirTypeRef::anonymous_struct({}),
                  "anonymous aggregate layout must reject missing ordered fields");
  const lir::LirTypeRef foreign_field = lir::LirTypeRef::struct_type(
      "%struct.Foreign", c4c::StructNameId{77});
  expect_rejected(lir::LirTypeRef::anonymous_struct({foreign_field, foreign_field}),
                  "anonymous aggregate layout must reject foreign field types");
}

void test_verifier_rejects_malformed_raw_type() {
  bir::ModuleBuilder builder;
  bir::FunctionSignature malformed;
  malformed.return_type = bir::Type{bir::TypeKind::Void, 0, "void"};
  bir::StructuredTypeSpecFacts malformed_facts;
  malformed_facts.pointer_level = 1;
  malformed.return_type.structured_spec = malformed_facts;
  auto function = builder.create_function(std::move(malformed),
                                          "malformed_raw_type", true);
  expect(function.has_value(),
         "builder should retain malformed staged state for verifier diagnosis");
  auto published = std::move(builder).publish();
  expect(!published.has_value() &&
             published.error().reason == bir::PublishError::VerificationFailed &&
             !published.error().verification.errors.empty(),
         "Raw publication verifier must reject malformed typed state");

  bir::ModuleBuilder array_builder;
  bir::Type malformed_array{bir::TypeKind::Array, 0, "[5 x i16]"};
  malformed_array.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Integer, 16, 0, {5, 2}};
  expect(array_builder
             .add_global_object("malformed_array", std::move(malformed_array),
                                4, false, false, false, true)
             .has_value(),
         "builder should retain incoherent typed array facts for verifier diagnosis");
  auto rejected_array = std::move(array_builder).publish();
  expect(!rejected_array.has_value() &&
             rejected_array.error().reason ==
                 bir::PublishError::VerificationFailed &&
             !rejected_array.error().verification.errors.empty(),
         "Raw publication verifier must reject array dimensions that disagree with typed spelling");

  bir::ModuleBuilder empty_array_builder;
  bir::Type empty_array{bir::TypeKind::Array, 0, "[5 x i16]"};
  empty_array.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Integer, 16, 0, {}};
  expect(empty_array_builder
             .add_global_object("empty_array", std::move(empty_array), 4,
                                false, false, false, true)
             .has_value(),
         "builder should retain empty typed array dimensions for verifier diagnosis");
  auto rejected_empty_array = std::move(empty_array_builder).publish();
  expect(!rejected_empty_array.has_value() &&
             rejected_empty_array.error().reason ==
                 bir::PublishError::VerificationFailed &&
             !rejected_empty_array.error().verification.errors.empty(),
         "Raw publication verifier must reject empty typed array dimensions");

  bir::ModuleBuilder zero_array_builder;
  bir::Type zero_array{bir::TypeKind::Array, 0, "[0 x i16]"};
  zero_array.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Integer, 16, 0, {0}};
  expect(zero_array_builder
             .add_global_object("zero_array", std::move(zero_array), 4,
                                false, false, false, true)
             .has_value(),
         "builder should retain producer-valid zero-length array facts");
  const auto accepted_zero_array = std::move(zero_array_builder).publish();
  expect(accepted_zero_array.has_value(),
         "Raw publication verifier must accept exact zero-length array facts");

  const auto malformed_array_facts_reject = [](std::string name,
                                                bir::Type type) {
    bir::ModuleBuilder malformed_builder;
    if (!malformed_builder
             .add_global_object(std::move(name), std::move(type), 4, false,
                                false, false, true)
             .has_value())
      return false;
    const auto published = std::move(malformed_builder).publish();
    return !published.has_value() &&
           published.error().reason ==
               bir::PublishError::VerificationFailed &&
           !published.error().verification.errors.empty();
  };

  bir::Type negative_pointer_depth{bir::TypeKind::Array, 0, "[5 x ptr]"};
  negative_pointer_depth.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Integer, 16, -1, {5}};
  expect(malformed_array_facts_reject("negative_pointer_depth",
                                      std::move(negative_pointer_depth)),
         "Raw publication verifier must reject negative array element pointer depth");

  bir::Type negative_outer_dimension{bir::TypeKind::Array, 0, "[-1 x i16]"};
  negative_outer_dimension.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Integer, 16, 0, {-1}};
  expect(malformed_array_facts_reject("negative_outer_dimension",
                                      std::move(negative_outer_dimension)),
         "Raw publication verifier must reject negative outer array dimensions");

  bir::Type negative_inner_dimension{bir::TypeKind::Array, 0,
                                     "[5 x [-2 x i16]]"};
  negative_inner_dimension.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Integer, 16, 0, {5, -2}};
  expect(malformed_array_facts_reject("negative_inner_dimension",
                                      std::move(negative_inner_dimension)),
         "Raw publication verifier must reject negative inner array dimensions");

  bir::Type nonscalar_pointer_base{bir::TypeKind::Array, 0, "[5 x ptr]"};
  nonscalar_pointer_base.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Struct, 0, 1, {5}};
  expect(malformed_array_facts_reject("nonscalar_pointer_base",
                                      std::move(nonscalar_pointer_base)),
         "Raw publication verifier must reject nonscalar pointer-element base facts");

  bir::Type pointer_spelling_conflict{bir::TypeKind::Array, 0, "[5 x i16]"};
  pointer_spelling_conflict.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Integer, 16, 1, {5}};
  expect(malformed_array_facts_reject("pointer_spelling_conflict",
                                      std::move(pointer_spelling_conflict)),
         "Raw publication verifier must reject pointer depth that disagrees with array spelling");

  bir::Type nested_array_without_pointer_depth{bir::TypeKind::Array, 0,
                                                "[5 x i16]"};
  nested_array_without_pointer_depth.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::Integer, 16, 0, {5}, std::nullopt,
      bir::PointerArrayTypeFacts{{3}, 1}};
  expect(malformed_array_facts_reject(
             "nested_array_without_pointer_depth",
             std::move(nested_array_without_pointer_depth)),
         "Raw publication verifier must require pointer depth for nested array pointee facts");

  bir::Type empty_nested_array{bir::TypeKind::Array, 0, "[5 x ptr]"};
  empty_nested_array.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::Integer, 16, 1, {5}, std::nullopt,
      bir::PointerArrayTypeFacts{{}, 0}};
  expect(malformed_array_facts_reject("empty_nested_array",
                                      std::move(empty_nested_array)),
         "Raw publication verifier must reject empty nested pointee dimensions");

  bir::Type negative_nested_array{bir::TypeKind::Array, 0, "[5 x ptr]"};
  negative_nested_array.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::Integer, 16, 1, {5}, std::nullopt,
      bir::PointerArrayTypeFacts{{3, -1}, 2}};
  expect(malformed_array_facts_reject("negative_nested_array",
                                      std::move(negative_nested_array)),
         "Raw publication verifier must reject negative nested pointee dimensions");

  bir::Type mismatched_nested_rank{bir::TypeKind::Array, 0, "[5 x ptr]"};
  mismatched_nested_rank.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::Integer, 16, 1, {5}, std::nullopt,
      bir::PointerArrayTypeFacts{{3, 2}, 1}};
  expect(malformed_array_facts_reject("mismatched_nested_rank",
                                      std::move(mismatched_nested_rank)),
         "Raw publication verifier must reject mismatched nested pointee rank");

  bir::Type incompatible_nested_base{bir::TypeKind::Array, 0, "[5 x ptr]"};
  incompatible_nested_base.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::Struct, 0, 1, {5}, std::nullopt,
      bir::PointerArrayTypeFacts{{3}, 1}};
  expect(malformed_array_facts_reject("incompatible_nested_base",
                                      std::move(incompatible_nested_base)),
         "Raw publication verifier must reject nested pointee dimensions on incompatible base facts");

  bir::Type hidden_dimensions_in_spelling{bir::TypeKind::Array, 0,
                                           "[5 x [3 x ptr]]"};
  hidden_dimensions_in_spelling.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::Integer, 16, 1, {5}, std::nullopt,
      bir::PointerArrayTypeFacts{{3}, 1}};
  expect(malformed_array_facts_reject(
             "hidden_dimensions_in_spelling",
             std::move(hidden_dimensions_in_spelling)),
         "Raw publication verifier must keep nested pointee dimensions out of opaque pointer storage spelling");

  bir::Type facts_on_scalar{bir::TypeKind::Integer, 16, "i16"};
  facts_on_scalar.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Integer, 16, 0, {5}};
  expect(malformed_array_facts_reject("facts_on_scalar",
                                      std::move(facts_on_scalar)),
         "Raw publication verifier must reject typed array facts on non-array types");

  const auto malformed_pointer_facts_reject = [](std::string name,
                                                  bir::Type type) {
    bir::ModuleBuilder malformed_builder;
    if (!malformed_builder
             .add_global_object(std::move(name), std::move(type), 8, false,
                                false, false, true)
             .has_value())
      return false;
    const auto published = std::move(malformed_builder).publish();
    return !published.has_value() &&
           published.error().reason ==
               bir::PublishError::VerificationFailed &&
           !published.error().verification.errors.empty();
  };

  bir::Type pointer_facts_on_scalar{bir::TypeKind::Integer, 32, "i32"};
  pointer_facts_on_scalar.pointer_facts =
      bir::PointerTypeFacts{bir::TypeKind::Integer, 32, 1};
  expect(malformed_pointer_facts_reject("pointer_facts_on_scalar",
                                        std::move(pointer_facts_on_scalar)),
         "Raw publication verifier must reject pointer facts on non-pointer types");

  bir::Type zero_pointer_depth{bir::TypeKind::Pointer};
  zero_pointer_depth.pointer_facts =
      bir::PointerTypeFacts{bir::TypeKind::Integer, 32, 0};
  expect(malformed_pointer_facts_reject("zero_pointer_depth",
                                        std::move(zero_pointer_depth)),
         "Raw publication verifier must reject zero typed pointer depth");

  bir::Type negative_direct_pointer_depth{bir::TypeKind::Pointer};
  negative_direct_pointer_depth.pointer_facts =
      bir::PointerTypeFacts{bir::TypeKind::Floating, 64, -1};
  expect(malformed_pointer_facts_reject(
             "negative_direct_pointer_depth",
             std::move(negative_direct_pointer_depth)),
         "Raw publication verifier must reject negative typed pointer depth");

  bir::Type invalid_pointer_base{bir::TypeKind::Pointer};
  invalid_pointer_base.pointer_facts =
      bir::PointerTypeFacts{bir::TypeKind::Struct, 0, 1};
  expect(malformed_pointer_facts_reject("invalid_pointer_base",
                                        std::move(invalid_pointer_base)),
         "Raw publication verifier must reject nonscalar pointer pointee facts");

  bir::Type invalid_pointer_width{bir::TypeKind::Pointer};
  invalid_pointer_width.pointer_facts =
      bir::PointerTypeFacts{bir::TypeKind::Floating, 24, 1};
  expect(malformed_pointer_facts_reject("invalid_pointer_width",
                                        std::move(invalid_pointer_width)),
         "Raw publication verifier must reject invalid floating pointee widths");

  bir::Type invalid_pointer_storage_width{bir::TypeKind::Pointer};
  invalid_pointer_storage_width.bit_width = 64;
  invalid_pointer_storage_width.pointer_facts =
      bir::PointerTypeFacts{bir::TypeKind::Integer, 64, 1};
  expect(malformed_pointer_facts_reject(
             "invalid_pointer_storage_width",
             std::move(invalid_pointer_storage_width)),
         "Raw publication verifier must reject storage width on typed opaque pointers");

  bir::Type facts_on_noncomplex{bir::TypeKind::Integer, 32, "i32"};
  facts_on_noncomplex.complex_facts =
      bir::ComplexTypeFacts{bir::TypeKind::Integer, 32};
  expect(malformed_pointer_facts_reject("facts_on_noncomplex",
                                        std::move(facts_on_noncomplex)),
         "Raw publication verifier must reject complex facts on non-complex types");

  bir::Type missing_complex_facts{bir::TypeKind::Complex, 32,
                                  "{ i32, i32 }"};
  expect(malformed_pointer_facts_reject("missing_complex_facts",
                                        std::move(missing_complex_facts)),
         "Raw publication verifier must require typed complex component facts");

  bir::Type invalid_complex_kind{bir::TypeKind::Complex, 32,
                                 "{ i32, i32 }"};
  invalid_complex_kind.complex_facts =
      bir::ComplexTypeFacts{bir::TypeKind::Struct, 32};
  expect(malformed_pointer_facts_reject("invalid_complex_kind",
                                        std::move(invalid_complex_kind)),
         "Raw publication verifier must reject non-scalar complex component kinds");

  bir::Type invalid_complex_width{bir::TypeKind::Complex, 24,
                                  "{ fp24, fp24 }"};
  invalid_complex_width.complex_facts =
      bir::ComplexTypeFacts{bir::TypeKind::Floating, 24};
  expect(malformed_pointer_facts_reject("invalid_complex_width",
                                        std::move(invalid_complex_width)),
         "Raw publication verifier must reject invalid complex component widths");

  bir::Type complex_spelling_conflict{bir::TypeKind::Complex, 32,
                                      "{ double, double }"};
  complex_spelling_conflict.complex_facts =
      bir::ComplexTypeFacts{bir::TypeKind::Floating, 32};
  expect(malformed_pointer_facts_reject("complex_spelling_conflict",
                                        std::move(complex_spelling_conflict)),
         "Raw publication verifier must reject complex spelling conflicts");

  bir::Type malformed_complex_pointer{bir::TypeKind::Pointer};
  malformed_complex_pointer.pointer_facts = bir::PointerTypeFacts{
      bir::TypeKind::Complex, 64, 2,
      bir::ComplexTypeFacts{bir::TypeKind::Floating, 32}};
  expect(malformed_pointer_facts_reject("malformed_complex_pointer",
                                        std::move(malformed_complex_pointer)),
         "Raw publication verifier must reject mismatched complex pointer facts");

  const auto pointer_array_facts = [](std::vector<std::int64_t> dimensions,
                                      int inner_rank) {
    return bir::PointerTypeFacts{
        bir::TypeKind::Integer, 32, 1, std::nullopt,
        bir::PointerArrayTypeFacts{std::move(dimensions), inner_rank}};
  };
  bir::Type empty_pointer_array{bir::TypeKind::Pointer};
  empty_pointer_array.pointer_facts = pointer_array_facts({}, -1);
  expect(malformed_pointer_facts_reject("empty_pointer_array",
                                        std::move(empty_pointer_array)),
         "Raw publication verifier must reject empty pointer-array dimensions");

  bir::Type negative_pointer_array{bir::TypeKind::Pointer};
  negative_pointer_array.pointer_facts = pointer_array_facts({3, -1}, -1);
  expect(malformed_pointer_facts_reject("negative_pointer_array",
                                        std::move(negative_pointer_array)),
         "Raw publication verifier must reject negative pointer-array dimensions");

  bir::Type split_pointer_array{bir::TypeKind::Pointer};
  split_pointer_array.pointer_facts = pointer_array_facts({3, 5}, 1);
  expect(malformed_pointer_facts_reject("split_pointer_array",
                                        std::move(split_pointer_array)),
         "Raw publication verifier must reject pointer-array facts with an outer array rank");

  bir::Type untyped_pointer_array{bir::TypeKind::Pointer};
  untyped_pointer_array.pointer_facts = bir::PointerTypeFacts{
      bir::TypeKind::Void, 0, 1, std::nullopt,
      bir::PointerArrayTypeFacts{{3}, -1}};
  expect(malformed_pointer_facts_reject("untyped_pointer_array",
                                        std::move(untyped_pointer_array)),
         "Raw publication verifier must reject pointer-array facts without typed pointee authority");

  bir::Type pointer_array_facts_on_scalar{bir::TypeKind::Integer, 32, "i32"};
  pointer_array_facts_on_scalar.pointer_facts = pointer_array_facts({3}, 1);
  expect(malformed_pointer_facts_reject(
             "pointer_array_facts_on_scalar",
             std::move(pointer_array_facts_on_scalar)),
         "Raw publication verifier must reject pointer-array facts on non-pointer types");

  bir::Type oversized_pointer_array{bir::TypeKind::Pointer};
  oversized_pointer_array.pointer_facts =
      pointer_array_facts({1, 2, 3, 4, 5, 6, 7, 8, 9}, 9);
  expect(malformed_pointer_facts_reject("oversized_pointer_array",
                                        std::move(oversized_pointer_array)),
         "Raw publication verifier must bound typed pointer-array dimensions");

  bir::Type incompatible_complex_pointer_array{bir::TypeKind::Pointer};
  incompatible_complex_pointer_array.pointer_facts = bir::PointerTypeFacts{
      bir::TypeKind::Complex, 64, 1,
      bir::ComplexTypeFacts{bir::TypeKind::Floating, 32},
      bir::PointerArrayTypeFacts{{3}, 1}};
  expect(malformed_pointer_facts_reject(
             "incompatible_complex_pointer_array",
             std::move(incompatible_complex_pointer_array)),
         "Raw publication verifier must reject incompatible nested complex pointer-array facts");

  bir::Type malformed_complex_array{bir::TypeKind::Array, 0,
                                    "[2 x { i32, i32 }]"};
  malformed_complex_array.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::Complex, 32, 0, {2}, std::nullopt};
  expect(malformed_pointer_facts_reject("malformed_complex_array",
                                        std::move(malformed_complex_array)),
         "Raw publication verifier must reject missing complex array element facts");

  bir::Type invalid_vrm_width{bir::TypeKind::VrmRegister, 3, "c4c.vrm3"};
  expect(malformed_pointer_facts_reject("invalid_vrm_width",
                                        std::move(invalid_vrm_width)),
         "Raw publication verifier must reject unsupported direct VRM widths");

  bir::Type invalid_vrm_spelling{bir::TypeKind::VrmRegister, 4, "c4c.vrm2"};
  expect(malformed_pointer_facts_reject("invalid_vrm_spelling",
                                        std::move(invalid_vrm_spelling)),
         "Raw publication verifier must reject direct VRM spelling conflicts");

  bir::Type invalid_vrm_pointer_width{bir::TypeKind::Pointer};
  invalid_vrm_pointer_width.pointer_facts =
      bir::PointerTypeFacts{bir::TypeKind::VrmRegister, 16, 2};
  expect(malformed_pointer_facts_reject(
             "invalid_vrm_pointer_width",
             std::move(invalid_vrm_pointer_width)),
         "Raw publication verifier must reject unsupported VRM pointer widths");

  bir::Type nested_vrm_pointer_facts{bir::TypeKind::Pointer};
  nested_vrm_pointer_facts.pointer_facts = bir::PointerTypeFacts{
      bir::TypeKind::VrmRegister, 4, 2,
      bir::ComplexTypeFacts{bir::TypeKind::Integer, 4}};
  expect(malformed_pointer_facts_reject(
             "nested_vrm_pointer_facts",
             std::move(nested_vrm_pointer_facts)),
         "Raw publication verifier must reject complex facts nested in VRM pointers");

  bir::Type invalid_vrm_array_width{bir::TypeKind::Array, 0,
                                    "[2 x c4c.vrm16]"};
  invalid_vrm_array_width.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::VrmRegister, 16, 0, {2}};
  expect(malformed_array_facts_reject("invalid_vrm_array_width",
                                      std::move(invalid_vrm_array_width)),
         "Raw publication verifier must reject unsupported VRM array widths");

  bir::Type nested_vrm_array_facts{bir::TypeKind::Array, 0,
                                   "[2 x c4c.vrm4]"};
  nested_vrm_array_facts.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::VrmRegister, 4, 0, {2},
      bir::ComplexTypeFacts{bir::TypeKind::Integer, 4}};
  expect(malformed_array_facts_reject("nested_vrm_array_facts",
                                      std::move(nested_vrm_array_facts)),
         "Raw publication verifier must reject complex facts nested in VRM arrays");

  const auto malformed_vector_facts_reject = [](std::string name,
                                                 bir::Type type) {
    bir::ModuleBuilder malformed_builder;
    if (!malformed_builder
             .add_global_object(std::move(name), std::move(type), 16, false,
                                false, false, true)
             .has_value())
      return false;
    const auto published = std::move(malformed_builder).publish();
    return !published.has_value() &&
           published.error().reason ==
               bir::PublishError::VerificationFailed &&
           !published.error().verification.errors.empty();
  };

  bir::Type vector_facts_on_scalar{bir::TypeKind::Integer, 32, "i32"};
  vector_facts_on_scalar.vector_facts =
      bir::VectorTypeFacts{bir::TypeKind::Integer, 32, 4, 16};
  expect(malformed_vector_facts_reject("vector_facts_on_scalar",
                                       std::move(vector_facts_on_scalar)),
         "Raw publication verifier must reject vector facts on non-vector types");

  bir::Type zero_vector_lanes{bir::TypeKind::Vector, 0, "<4 x i32>"};
  zero_vector_lanes.vector_facts =
      bir::VectorTypeFacts{bir::TypeKind::Integer, 32, 0, 16};
  expect(malformed_vector_facts_reject("zero_vector_lanes",
                                       std::move(zero_vector_lanes)),
         "Raw publication verifier must reject nonpositive vector lane facts");

  bir::Type negative_vector_storage{bir::TypeKind::Vector, 0, "<4 x i32>"};
  negative_vector_storage.vector_facts =
      bir::VectorTypeFacts{bir::TypeKind::Integer, 32, 4, -16};
  expect(malformed_vector_facts_reject("negative_vector_storage",
                                       std::move(negative_vector_storage)),
         "Raw publication verifier must reject nonpositive vector storage facts");

  bir::Type nonscalar_vector_base{bir::TypeKind::Vector, 0,
                                  "<4 x %struct.Payload>"};
  nonscalar_vector_base.vector_facts =
      bir::VectorTypeFacts{bir::TypeKind::Struct, 0, 4, 16};
  expect(malformed_vector_facts_reject("nonscalar_vector_base",
                                       std::move(nonscalar_vector_base)),
         "Raw publication verifier must reject nonscalar vector element facts");

  bir::Type invalid_vector_width{bir::TypeKind::Vector, 0, "<4 x float>"};
  invalid_vector_width.vector_facts =
      bir::VectorTypeFacts{bir::TypeKind::Floating, 24, 4, 16};
  expect(malformed_vector_facts_reject("invalid_vector_width",
                                       std::move(invalid_vector_width)),
         "Raw publication verifier must reject invalid floating vector widths");

  bir::Type vector_spelling_conflict{bir::TypeKind::Vector, 0,
                                     "<8 x i32>"};
  vector_spelling_conflict.vector_facts =
      bir::VectorTypeFacts{bir::TypeKind::Integer, 32, 4, 16};
  expect(malformed_vector_facts_reject("vector_spelling_conflict",
                                       std::move(vector_spelling_conflict)),
         "Raw publication verifier must reject vector spelling conflicts");

  bir::Type vector_facts_on_integer_pointer{bir::TypeKind::Pointer};
  vector_facts_on_integer_pointer.pointer_facts = bir::PointerTypeFacts{
      bir::TypeKind::Integer, 32, 1, std::nullopt, std::nullopt,
      bir::VectorTypeFacts{bir::TypeKind::Integer, 32, 4, 16}};
  expect(malformed_pointer_facts_reject(
             "vector_facts_on_integer_pointer",
             std::move(vector_facts_on_integer_pointer)),
         "Raw publication verifier must reject nested vector facts on non-vector pointees");

  bir::Type missing_vector_pointer_facts{bir::TypeKind::Pointer};
  missing_vector_pointer_facts.pointer_facts =
      bir::PointerTypeFacts{bir::TypeKind::Vector, 0, 2};
  expect(malformed_pointer_facts_reject(
             "missing_vector_pointer_facts",
             std::move(missing_vector_pointer_facts)),
         "Raw publication verifier must require typed vector pointee facts");

  bir::Type invalid_vector_pointer_facts{bir::TypeKind::Pointer};
  invalid_vector_pointer_facts.pointer_facts = bir::PointerTypeFacts{
      bir::TypeKind::Vector, 0, 2, std::nullopt, std::nullopt,
      bir::VectorTypeFacts{bir::TypeKind::Floating, 24, 4, 16}};
  expect(malformed_pointer_facts_reject(
             "invalid_vector_pointer_facts",
             std::move(invalid_vector_pointer_facts)),
         "Raw publication verifier must reject malformed vector pointee components");

  bir::Type nonpositive_vector_pointer_shape{bir::TypeKind::Pointer};
  nonpositive_vector_pointer_shape.pointer_facts = bir::PointerTypeFacts{
      bir::TypeKind::Vector, 0, 1, std::nullopt, std::nullopt,
      bir::VectorTypeFacts{bir::TypeKind::Integer, 32, 0, -16}};
  expect(malformed_pointer_facts_reject(
             "nonpositive_vector_pointer_shape",
             std::move(nonpositive_vector_pointer_shape)),
         "Raw publication verifier must reject nonpositive nested vector lane and storage facts");

  bir::Type incompatible_vector_pointer_facts{bir::TypeKind::Pointer};
  incompatible_vector_pointer_facts.pointer_facts = bir::PointerTypeFacts{
      bir::TypeKind::Vector, 0, 2,
      bir::ComplexTypeFacts{bir::TypeKind::Floating, 32},
      bir::PointerArrayTypeFacts{{3}, 1},
      bir::VectorTypeFacts{bir::TypeKind::Floating, 32, 4, 16}};
  expect(malformed_pointer_facts_reject(
             "incompatible_vector_pointer_facts",
             std::move(incompatible_vector_pointer_facts)),
         "Raw publication verifier must reject complex and pointer-array facts beside vector pointee facts");

  bir::Type vector_facts_on_integer_array{bir::TypeKind::Array, 0,
                                          "[2 x i32]"};
  vector_facts_on_integer_array.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::Integer, 32, 0, {2}, std::nullopt, std::nullopt,
      bir::VectorTypeFacts{bir::TypeKind::Integer, 32, 4, 16}};
  expect(malformed_array_facts_reject(
             "vector_facts_on_integer_array",
             std::move(vector_facts_on_integer_array)),
         "Raw publication verifier must reject nested vector facts on non-vector array elements");

  bir::Type missing_vector_array_facts{bir::TypeKind::Array, 0,
                                       "[2 x <4 x i32>]"};
  missing_vector_array_facts.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Vector, 0, 0, {2}};
  expect(malformed_array_facts_reject("missing_vector_array_facts",
                                      std::move(missing_vector_array_facts)),
         "Raw publication verifier must require typed vector element facts");

  bir::Type invalid_vector_array_shape{bir::TypeKind::Array, 0,
                                       "[2 x <4 x i32>]"};
  invalid_vector_array_shape.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::Vector, 0, 0, {2}, std::nullopt, std::nullopt,
      bir::VectorTypeFacts{bir::TypeKind::Integer, 32, 4, 0}};
  expect(malformed_array_facts_reject("invalid_vector_array_shape",
                                      std::move(invalid_vector_array_shape)),
         "Raw publication verifier must reject nonpositive nested vector array storage facts");

  bir::Type incompatible_vector_array_facts{bir::TypeKind::Array, 0,
                                            "[2 x <4 x i32>]"};
  incompatible_vector_array_facts.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::Vector, 0, 0, {2},
      bir::ComplexTypeFacts{bir::TypeKind::Integer, 32},
      bir::PointerArrayTypeFacts{{3}, 1},
      bir::VectorTypeFacts{bir::TypeKind::Integer, 32, 4, 16}};
  expect(malformed_array_facts_reject(
             "incompatible_vector_array_facts",
             std::move(incompatible_vector_array_facts)),
         "Raw publication verifier must reject complex and pointer-array facts beside vector element facts");

  bir::Type vector_array_spelling_conflict{bir::TypeKind::Array, 0,
                                           "[2 x <8 x i32>]"};
  vector_array_spelling_conflict.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::Vector, 0, 0, {2}, std::nullopt, std::nullopt,
      bir::VectorTypeFacts{bir::TypeKind::Integer, 32, 4, 16}};
  expect(malformed_array_facts_reject(
             "vector_array_spelling_conflict",
             std::move(vector_array_spelling_conflict)),
         "Raw publication verifier must reject visible vector array spelling conflicts");
}

void test_intrinsic_requirements_receipt() {
  const auto set_bit = [](lir::LirModule& module,
                          bir::IntrinsicRequirements& expected, int index) {
    switch (index) {
      case 0:
        module.need_va_start = true;
        expected.need_va_start = true;
        break;
      case 1:
        module.need_va_end = true;
        expected.need_va_end = true;
        break;
      case 2:
        module.need_va_copy = true;
        expected.need_va_copy = true;
        break;
      case 3:
        module.need_memcpy = true;
        expected.need_memcpy = true;
        break;
      case 4:
        module.need_memset = true;
        expected.need_memset = true;
        break;
      case 5:
        module.need_stacksave = true;
        expected.need_stacksave = true;
        break;
      case 6:
        module.need_stackrestore = true;
        expected.need_stackrestore = true;
        break;
      case 7:
        module.need_abs = true;
        expected.need_abs = true;
        break;
      case 8:
        module.need_ptrmask = true;
        expected.need_ptrmask = true;
        break;
      case 9:
        module.prefer_semantic_va_ops = true;
        expected.prefer_semantic_va_ops = true;
        break;
      default: fail("intrinsic requirement test bit is out of range");
    }
  };

  {
    lir::LirModule module;
    const bir::IntrinsicRequirements none;
    const auto raw = bir::lower_lir_to_raw_bir(module);
    const auto canonical = bir::lower_lir_to_canonical_bir(module);
    expect(raw.has_value() &&
               bir::FoundationVerifier::verify(raw.value()).ok() &&
               raw.value().view().intrinsic_requirements() == none &&
               canonical.has_value() &&
               canonical.value().view().intrinsic_requirements() == none,
           "all-false intrinsic requirements must survive Raw and Canonical BIR exactly");
  }

  for (int bit = 0; bit < 10; ++bit) {
    lir::LirModule module;
    bir::IntrinsicRequirements expected;
    set_bit(module, expected, bit);
    const auto raw = bir::lower_lir_to_raw_bir(module);
    const auto canonical = bir::lower_lir_to_canonical_bir(module);
    expect(raw.has_value() &&
               bir::FoundationVerifier::verify(raw.value()).ok() &&
               raw.value().view().intrinsic_requirements() == expected &&
               canonical.has_value() &&
               canonical.value().view().intrinsic_requirements() == expected,
           "each independent LIR intrinsic requirement bit must retain its exact Raw/Canonical position");
  }

  {
    lir::LirModule module;
    module.need_va_copy = true;
    module.need_memset = true;
    module.need_stackrestore = true;
    module.need_ptrmask = true;
    module.prefer_semantic_va_ops = true;
    bir::IntrinsicRequirements expected;
    expected.need_va_copy = true;
    expected.need_memset = true;
    expected.need_stackrestore = true;
    expected.need_ptrmask = true;
    expected.prefer_semantic_va_ops = true;
    const auto raw = bir::lower_lir_to_raw_bir(module);
    const auto canonical = bir::lower_lir_to_canonical_bir(module);
    expect(raw.has_value() &&
               raw.value().view().intrinsic_requirements() == expected &&
               canonical.has_value() &&
               canonical.value().view().intrinsic_requirements() == expected,
           "independent mixed intrinsic requirements must round-trip without invented implications");
  }

  bir::ModuleBuilder default_builder;
  const auto default_raw = std::move(default_builder).publish();
  expect(default_raw.has_value() &&
             default_raw.value().view().intrinsic_requirements() ==
                 bir::IntrinsicRequirements{},
         "builder default intrinsic requirements must be exactly all false");

  bir::ModuleBuilder assigned_builder;
  bir::IntrinsicRequirements assigned;
  assigned.need_memcpy = true;
  assigned.need_abs = true;
  expect(assigned_builder.set_intrinsic_requirements(assigned).has_value(),
         "builder must accept one exact intrinsic requirement assignment");
  bir::IntrinsicRequirements conflicting;
  conflicting.need_va_start = true;
  const auto duplicate =
      assigned_builder.set_intrinsic_requirements(conflicting);
  expect(!duplicate.has_value() &&
             duplicate.error() ==
                 bir::BuildError::DuplicateIntrinsicRequirements,
         "builder must reject duplicate or conflicting requirement assignment");
  const auto assigned_raw = std::move(assigned_builder).publish();
  expect(assigned_raw.has_value() &&
             assigned_raw.value().view().intrinsic_requirements() == assigned,
         "builder view must retain the first exact requirement assignment");
}

void test_module_name_and_struct_declaration_receipt() {
  lir::LirModule module;
  auto texts = std::make_shared<c4c::TextTable>();
  module.link_name_texts = texts;
  module.link_names.attach_text_table(texts.get());
  module.struct_names.attach_text_table(texts.get());

  const c4c::LinkNameId exported = module.link_names.intern("exported");
  const c4c::LinkNameId helper = module.link_names.intern("helper");
  const c4c::StructNameId outer = module.struct_names.intern("%struct.Outer");
  const c4c::StructNameId inner = module.struct_names.intern("%struct.Inner");

  lir::LirStructDecl outer_decl;
  outer_decl.name_id = outer;
  outer_decl.is_packed = true;
  outer_decl.fields = {
      {lir::LirTypeRef::struct_type("%struct.Inner", inner)},
      {lir::LirTypeRef("ptr")}};
  module.record_struct_decl(std::move(outer_decl));
  lir::LirStructDecl inner_decl;
  inner_decl.name_id = inner;
  inner_decl.fields = {{lir::LirTypeRef::integer(32)},
                       {lir::LirTypeRef::struct_type("%struct.Outer", outer)}};
  module.record_struct_decl(std::move(inner_decl));
  // Keep the legacy declarations as checked output shadows.  Their spelling is
  // derived from the structured declarations; neither the verifier nor the
  // Raw-BIR importer may recover struct identity or layout from these strings.
  module.type_decls = {
      lir::render_struct_decl_llvm(module, module.struct_decls[0]),
      lir::render_struct_decl_llvm(module, module.struct_decls[1]),
  };

  lir::verify_module(module);
  auto stale_shadow = module;
  stale_shadow.type_decls[0] = "%struct.Outer = type { i64 }";
  try {
    lir::verify_module(stale_shadow);
    fail("stale legacy struct declaration shadow must fail before BIR import");
  } catch (const lir::LirVerifyError&) {
  }

  auto declaration = void_declaration("structured-declaration-authority");
  declaration.link_name_id = exported;
  module.functions.push_back(std::move(declaration));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "structured declarations and semantic name tables should publish");
  const auto view = imported.value().view();
  const auto link_names = view.link_names();
  expect(link_names.size() == 2 &&
             view.spelling(link_names[0]).value() == "exported" &&
             view.spelling(link_names[1]).value() == "helper" &&
             view.source_id(link_names[0]).value() == exported,
         "link-name receipt must preserve source order, spelling, and source ID");
  const auto struct_names = view.struct_names();
  expect(struct_names.size() == 2 &&
             view.spelling(struct_names[0]).value() == "%struct.Outer" &&
             view.spelling(struct_names[1]).value() == "%struct.Inner" &&
             view.source_id(struct_names[1]).value() == inner,
         "struct-name receipt must preserve typed identity and spelling");
  const auto declarations = view.struct_declarations();
  expect(declarations.size() == 2,
         "struct declarations must remain in deterministic source order");
  const auto raw_outer = view.struct_declaration(declarations[0]).value();
  const auto raw_inner = view.struct_declaration(declarations[1]).value();
  expect(raw_outer.name == struct_names[0] && raw_outer.is_packed &&
             raw_outer.fields.size() == 2 &&
             raw_outer.fields[0].type.struct_name_id == inner &&
             raw_outer.fields[0].referenced_name == struct_names[1] &&
             raw_inner.name == struct_names[1] &&
             raw_inner.fields[1].type.struct_name_id == outer &&
             raw_inner.fields[1].referenced_name == struct_names[0],
         "shared and forward struct field types must retain structured references");
  expect(view.function(view.functions().front()).value().link_name() == "exported",
         "function identity must resolve from the semantic link-name table");
}

void test_module_name_and_struct_declaration_rejections() {
  const auto make_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    return module;
  };
  const auto rejected = [](lir::LirModule module, const std::string& message) {
    const auto imported = bir::lower_lir_to_raw_bir(module);
    expect(!imported.has_value() &&
               imported.error().code ==
                   bir::ImportErrorCode::UnsupportedTypeDeclarations,
           message);
  };

  auto missing_name = make_module();
  missing_name.struct_decls.push_back(lir::LirStructDecl{});
  missing_name.struct_decl_index.emplace(c4c::kInvalidStructName, 0);
  rejected(std::move(missing_name),
           "invalid declaration IDs must fail transactionally");

  auto duplicate = make_module();
  const auto duplicate_id = duplicate.struct_names.intern("%struct.Duplicate");
  duplicate.struct_decls = {{duplicate_id}, {duplicate_id}};
  duplicate.struct_decl_index.emplace(duplicate_id, 0);
  rejected(std::move(duplicate),
           "duplicate/conflicting declarations must fail transactionally");

  auto malformed = make_module();
  const auto malformed_id = malformed.struct_names.intern("%struct.Malformed");
  lir::LirStructDecl malformed_decl;
  malformed_decl.name_id = malformed_id;
  malformed_decl.fields = {{lir::LirTypeRef("semantic raw field")}};
  malformed.record_struct_decl(std::move(malformed_decl));
  rejected(std::move(malformed),
           "malformed structured field types must fail transactionally");

  auto bad_index = make_module();
  const auto indexed_id = bad_index.struct_names.intern("%struct.Indexed");
  bad_index.record_struct_decl(lir::LirStructDecl{indexed_id});
  bad_index.struct_decl_index[indexed_id] = 1;
  rejected(std::move(bad_index),
           "struct declaration cache mismatches must fail transactionally");

  auto bad_name_cache = make_module();
  bad_name_cache.struct_names.intern("%struct.Cache");
  bad_name_cache.struct_names.ids_.id_by_key_.clear();
  rejected(std::move(bad_name_cache),
           "semantic name-table cache mismatches must fail transactionally");

  bir::ModuleBuilder builder;
  expect(builder.add_struct_name(1, "%struct.BadRaw").has_value(),
         "builder should own staged struct-name state");
  expect(builder.add_struct_declaration(
                    1, {bir::StructField{bir::Type{bir::TypeKind::Void}}},
                    false, false)
             .has_value(),
         "builder should retain malformed staged declarations for diagnosis");
  const auto published = std::move(builder).publish();
  expect(!published.has_value() &&
             published.error().reason == bir::PublishError::VerificationFailed,
         "publication verifier must reject malformed staged declarations");
}

void test_structured_constant_value_receipt() {
  constexpr std::uint64_t negative_zero = UINT64_C(0x8000000000000000);
  constexpr std::uint64_t quiet_nan = UINT64_C(0x7ff8000000000042);

  lir::LirModule module;
  auto block = return_block(0, "entry");
  block.insts = {
      lir::LirConstInt{lir::LirValueId{0}, scalar_type(c4c::TB_LONGLONG),
                       std::numeric_limits<long long>::min()},
      lir::LirConstInt{lir::LirValueId{17}, scalar_type(c4c::TB_LONGLONG),
                       std::numeric_limits<long long>::max()},
      lir::LirConstFloat{lir::LirValueId{8}, scalar_type(c4c::TB_DOUBLE),
                         double_from_bits(negative_zero)},
      lir::LirConstFloat{lir::LirValueId{9}, scalar_type(c4c::TB_DOUBLE),
                         double_from_bits(quiet_nan)},
  };
  module.functions.push_back(
      void_definition("structured_constants", {std::move(block)}));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "typed integer and floating constants should publish RawBir");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "published constant state should be reachable by FoundationVerifier");
  const auto module_view = imported.value().view();
  const auto function_id = module_view.functions().front();
  const auto function = module_view.function(function_id).value();
  const auto values = function.values();
  expect(values.size() == 4,
         "constants must be ordinary values, not fabricated instructions");
  expect(function.instructions(function.blocks().front()).value().empty(),
         "constant definitions must not create runtime instruction nodes");

  const auto min_id =
      function.source_value(bir::SourceValueId{function_id, 0}).value();
  const auto max_id =
      function.source_value(bir::SourceValueId{function_id, 17}).value();
  const auto neg_zero_id =
      function.source_value(bir::SourceValueId{function_id, 8}).value();
  const auto nan_id =
      function.source_value(bir::SourceValueId{function_id, 9}).value();
  const auto min_def = function.value(min_id).value();
  const auto max_def = function.value(max_id).value();
  const auto neg_zero_def = function.value(neg_zero_id).value();
  const auto nan_def = function.value(nan_id).value();
  const auto min_constant = module_view
                                .constant(std::get<bir::ConstantDef>(
                                              min_def.definition)
                                              .constant)
                                .value();
  const auto max_constant = module_view
                                .constant(std::get<bir::ConstantDef>(
                                              max_def.definition)
                                              .constant)
                                .value();
  const auto neg_zero_constant =
      module_view
          .constant(std::get<bir::ConstantDef>(neg_zero_def.definition).constant)
          .value();
  const auto nan_constant =
      module_view.constant(std::get<bir::ConstantDef>(nan_def.definition).constant)
          .value();
  expect(std::get<bir::IntegerConstant>(min_constant.payload).value ==
             std::numeric_limits<std::int64_t>::min() &&
             std::get<bir::IntegerConstant>(max_constant.payload).value ==
                 std::numeric_limits<std::int64_t>::max(),
         "integer constants must preserve signed boundary values exactly");
  expect(std::get<bir::FloatingConstant>(neg_zero_constant.payload).bits ==
             negative_zero &&
             std::get<bir::FloatingConstant>(nan_constant.payload).bits == quiet_nan,
         "floating constants must preserve negative-zero and NaN payload bits");
  expect(min_def.type == bir::Type{bir::TypeKind::I64} &&
             neg_zero_def.type == bir::Type{bir::TypeKind::F64},
         "constant immutable views must expose their exact typed definitions");
}

void test_constant_value_rejections_and_forward_use() {
  const auto rejected = [](lir::LirModule module, const std::string& message) {
    auto imported = bir::lower_lir_to_raw_bir(module);
    expect(!imported.has_value() &&
               imported.error().code ==
                   bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message);
  };
  const auto module_with = [](std::string name,
                              std::vector<lir::LirInst> instructions) {
    lir::LirModule module;
    module.functions.push_back(void_declaration("valid_before_" + name));
    auto block = return_block(0, "entry");
    block.insts = std::move(instructions);
    module.functions.push_back(
        void_definition(std::move(name), {std::move(block)}));
    return module;
  };

  rejected(module_with(
               "duplicate_constant",
               {lir::LirConstInt{lir::LirValueId{3}, scalar_type(c4c::TB_INT), 1},
                lir::LirConstFloat{lir::LirValueId{3}, scalar_type(c4c::TB_DOUBLE),
                                   1.0}}),
           "duplicate LirValueId definitions must roll back the whole module");
  rejected(module_with(
               "missing_constant",
               {lir::LirConstInt{lir::LirValueId::invalid(),
                                 scalar_type(c4c::TB_INT), 1}}),
           "invalid source result identities must be rejected");
  rejected(module_with(
               "mistyped_constant",
               {lir::LirConstInt{lir::LirValueId{1}, scalar_type(c4c::TB_DOUBLE), 1}}),
           "integer payloads with floating types must be rejected");

  bir::ModuleBuilder builder;
  bir::FunctionSignature signature;
  signature.return_type = bir::Type{bir::TypeKind::Void};
  auto function = builder.create_function(signature, "forward_constant", false);
  expect(function.has_value(), "forward-use function should be constructible");
  bir::ValueId reserved{};
  bir::InstId use{};
  auto edited = builder.with_function(
      function.value(), [&](bir::FunctionBuilder& function_builder) {
        auto block = function_builder.create_block("entry");
        if (!block) return bir::Result<void, bir::BuildError>::failure(block.error());
        auto value = function_builder.reserve_source_value(
            41, bir::Type{bir::TypeKind::I64});
        if (!value) return bir::Result<void, bir::BuildError>::failure(value.error());
        reserved = value.value();

        bir::InlineAsmSpec spec;
        spec.asm_text = "forward use";
        spec.constraint_text = "r";
        spec.inputs = {reserved};
        auto appended = function_builder.append(block.value(), std::move(spec));
        if (!appended)
          return bir::Result<void, bir::BuildError>::failure(appended.error());
        use = appended.value().instruction;
        auto defined = function_builder.define_int_constant(reserved, -7);
        if (!defined) return defined;
        expect(!function_builder.define_int_constant(reserved, -8).has_value(),
               "conflicting second definitions must be rejected");
        return function_builder.set_terminator(block.value(), bir::ReturnTerm{});
      });
  expect(edited.has_value(),
         "a reserved identity should permit a use before its constant definition");
  auto published = std::move(builder).publish();
  expect(published.has_value(),
         "a resolved forward use should pass foundation verification");
  const auto published_view = published.value().view();
  const auto view = published_view.function(function.value()).value();
  const auto reserved_definition = view.value(reserved).value();
  const auto reserved_constant =
      published_view
          .constant(std::get<bir::ConstantDef>(reserved_definition.definition)
                        .constant)
          .value();
  expect(view.instruction(use).value().operands() ==
             std::vector<bir::ValueId>{reserved} &&
             std::get<bir::IntegerConstant>(reserved_constant.payload).value == -7,
         "forward uses and the later typed definition must share one ValueId");

  bir::ModuleBuilder unresolved_builder;
  auto unresolved_function = unresolved_builder.create_function(
      signature, "unresolved_constant", false);
  auto unresolved_edit = unresolved_builder.with_function(
      unresolved_function.value(), [&](bir::FunctionBuilder& function_builder) {
        auto block = function_builder.create_block("entry");
        if (!block) return bir::Result<void, bir::BuildError>::failure(block.error());
        auto value = function_builder.reserve_value(bir::Type{bir::TypeKind::I64});
        if (!value) return bir::Result<void, bir::BuildError>::failure(value.error());
        expect(!function_builder.define_float_constant_bits(value.value(), 0)
                    .has_value(),
               "mistyped definitions must be rejected without consuming a reservation");
        return function_builder.set_terminator(block.value(), bir::ReturnTerm{});
      });
  expect(unresolved_edit.has_value(), "unresolved staged state should remain inspectable");
  auto unresolved = std::move(unresolved_builder).publish();
  expect(!unresolved.has_value() &&
             unresolved.error().reason == bir::PublishError::VerificationFailed,
         "FoundationVerifier must reject unresolved reservations");

  bir::ModuleBuilder foreign_builder;
  auto owner = foreign_builder.create_function(signature, "owner", false);
  auto foreign = foreign_builder.create_function(signature, "foreign", false);
  bir::ValueId owner_value{};
  expect(foreign_builder
             .with_function(owner.value(), [&](bir::FunctionBuilder& fb) {
               auto block = fb.create_block("entry");
               owner_value = fb.reserve_value(bir::Type{bir::TypeKind::I64}).value();
               fb.define_int_constant(owner_value, 1);
               return fb.set_terminator(block.value(), bir::ReturnTerm{});
             })
             .has_value(),
         "owner value setup should succeed");
  expect(foreign_builder
             .with_function(foreign.value(), [&](bir::FunctionBuilder& fb) {
               expect(fb.define_int_constant(owner_value, 2).error() ==
                          bir::BuildError::ForeignOwner,
                      "foreign definitions must be rejected by exact owner");
               auto block = fb.create_block("entry");
               return fb.set_terminator(block.value(), bir::ReturnTerm{});
             })
             .has_value(),
         "foreign rejection should not poison the surrounding edit");
}

lir::LirModule direct_global_integer_store_module() {
  lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  const auto i32_link = module.link_names.intern("store_i32_global");
  const auto i64_link = module.link_names.intern("store_i64_global");

  const auto external_global = [](std::string name, c4c::LinkNameId link,
                                  c4c::TypeBase base, std::string type,
                                  int alignment) {
    lir::LirGlobal global;
    global.name = std::move(name);
    global.link_name_id = link;
    global.type = scalar_type(base);
    global.linkage_vis = "external ";
    global.qualifier = "global ";
    global.llvm_type = std::move(type);
    global.align_bytes = alignment;
    global.is_extern_decl = true;
    return global;
  };
  module.globals.push_back(external_global(
      "store_i32_global", i32_link, c4c::TB_INT, "i32", 4));
  module.globals.push_back(external_global(
      "store_i64_global", i64_link, c4c::TB_LONGLONG, "i64", 8));

  auto block = return_block(0, "entry");
  block.insts.push_back(lir::LirStoreOp{
      lir::LirTypeRef::integer(32),
      lir::LirOperand::integer("displayed-as-999", 7),
      lir::LirOperand::global("@misleading_destination", i32_link)});
  block.insts.push_back(lir::LirStoreOp{
      lir::LirTypeRef::integer(64),
      lir::LirOperand::integer("displayed-as-zero", -9),
      lir::LirOperand::global("@also_misleading", i64_link)});
  module.functions.push_back(
      void_definition("direct_global_integer_stores", {std::move(block)}));
  return module;
}

void test_direct_global_integer_store_receipt() {
  const auto module = direct_global_integer_store_module();
  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "two neighboring authoritative direct-global integer stores should publish Raw BIR");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "typed Store payloads and value uses should be verifier reachable");

  const auto module_view = imported.value().view();
  const auto globals = module_view.global_objects();
  const auto functions = module_view.functions();
  const auto function = module_view.function(functions[0]).value();
  const auto block = function.blocks()[0];
  const auto instructions = function.instructions(block).value();
  expect(globals.size() == 2 && instructions.size() == 2,
         "global and store source order must remain exact");
  for (std::size_t index = 0; index < instructions.size(); ++index) {
    const auto instruction = function.instruction(instructions[index]).value();
    const auto* store = instruction.store();
    expect(instruction.opcode() == bir::Opcode::Store && store &&
               store->destination == globals[index] &&
               instruction.operands().size() == 1 &&
               instruction.results().empty(),
           "immutable instruction view must expose one selected global and one value use");
    const auto value = function.value(instruction.operands()[0]).value();
    const auto constant_id = std::get<bir::ConstantDef>(value.definition).constant;
    const auto constant = module_view.constant(constant_id).value();
    const auto expected = index == 0 ? 7 : -9;
    expect(value.type == store->stored_type &&
               constant.type == store->stored_type &&
               std::get<bir::IntegerConstant>(constant.payload).value == expected,
           "store immediates must use the existing exact typed constant/value model");
  }

  auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "the admitted Store graph should pass canonical verification unchanged");
  const auto canonical_function =
      canonical.value().view().function(
          canonical.value().view().functions()[0]).value();
  expect(canonical_function.instructions(canonical_function.blocks()[0])
                 .value().size() == 2,
         "Canonical BIR must preserve both neighboring Store nodes");
}

void test_direct_global_integer_store_builder_contract() {
  bir::ModuleBuilder builder;
  const auto global = builder.add_global_object(
      "builder_store_global", bir::Type{bir::TypeKind::I32}, 4,
      false, false, false, true);
  bir::FunctionSignature signature;
  signature.return_type = bir::Type{bir::TypeKind::Void};
  const auto function =
      builder.create_function(signature, "builder_store", false);
  const auto foreign_function =
      builder.create_function(signature, "foreign_store_owner", true);
  expect(global.has_value() && function.has_value() &&
             foreign_function.has_value(),
         "Store builder fixture should create its typed owners");
  bir::InstId stored{};
  const auto edited = builder.with_function(
      function.value(), [&](bir::FunctionBuilder& function_builder) {
        const auto block = function_builder.create_block("entry");
        const auto value = function_builder.reserve_value(
            bir::Type{bir::TypeKind::I32});
        if (!block || !value)
          return bir::Result<void, bir::BuildError>::failure(
              bir::BuildError::StorageExhausted);
        auto defined = function_builder.define_int_constant(value.value(), 11);
        if (!defined) return defined;
        const auto foreign_global = function_builder.append(
            block.value(),
            bir::StoreSpec{bir::GlobalObjectId{999, 0},
                           bir::Type{bir::TypeKind::I32}, value.value()});
        expect(!foreign_global.has_value() &&
                   foreign_global.error() ==
                       bir::BuildError::InvalidGlobalObject,
               "Store builder must reject a foreign global identity without staging an instruction");
        const auto foreign_value = function_builder.append(
            block.value(),
            bir::StoreSpec{
                global.value(), bir::Type{bir::TypeKind::I32},
                bir::ValueId{foreign_function.value(),
                             bir::ValueKind::Ordinary, 0, 1}});
        expect(!foreign_value.has_value() &&
                   foreign_value.error() == bir::BuildError::ForeignOwner,
               "Store builder must reject a value use owned by another function");
        const auto wrong_type = function_builder.append(
            block.value(),
            bir::StoreSpec{global.value(), bir::Type{bir::TypeKind::I64},
                           value.value()});
        expect(!wrong_type.has_value() &&
                   wrong_type.error() == bir::BuildError::InvalidValueType,
               "Store builder must reject incoherent global/value/type facts");
        const auto appended = function_builder.append(
            block.value(),
            bir::StoreSpec{global.value(), bir::Type{bir::TypeKind::I32},
                           value.value()});
        if (!appended)
          return bir::Result<void, bir::BuildError>::failure(appended.error());
        stored = appended.value().instruction;
        return function_builder.set_terminator(block.value(),
                                               bir::ReturnTerm{});
      });
  expect(edited.has_value(), "one coherent Store should remain after rejected appends");
  auto raw = std::move(builder).publish();
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "a structurally built Store plus void return should publish Raw BIR");
  const auto view = raw.value().view().function(function.value()).value();
  expect(view.instructions(view.blocks()[0]).value() ==
                 std::vector<bir::InstId>{stored} &&
             view.instruction(stored).value().store() != nullptr,
         "rejected appends must leave only the structural Store reachable through the immutable view");
  expect(bir::canonicalize(std::move(raw).value()).has_value(),
         "the structural Store plus void return should publish Canonical BIR");

  bir::ModuleBuilder malformed_builder;
  const auto malformed_global = malformed_builder.add_global_object(
      "unresolved_store_global", bir::Type{bir::TypeKind::I32}, 4,
      false, false, false, true);
  const auto malformed_function = malformed_builder.create_function(
      signature, "unresolved_store", false);
  expect(malformed_builder
             .with_function(
                 malformed_function.value(),
                 [&](bir::FunctionBuilder& function_builder) {
                   const auto block = function_builder.create_block("entry");
                   const auto unresolved = function_builder.reserve_value(
                       bir::Type{bir::TypeKind::I32});
                   const auto appended = function_builder.append(
                       block.value(),
                       bir::StoreSpec{malformed_global.value(),
                                      bir::Type{bir::TypeKind::I32},
                                      unresolved.value()});
                   if (!appended)
                     return bir::Result<void, bir::BuildError>::failure(
                         appended.error());
                   return function_builder.set_terminator(block.value(),
                                                          bir::ReturnTerm{});
                 })
             .has_value(),
         "the public builder should stage a Store use before its reserved value is defined");
  const auto malformed = std::move(malformed_builder).publish();
  expect(!malformed.has_value() &&
             malformed.error().reason ==
                 bir::PublishError::VerificationFailed,
         "FoundationVerifier must prevent an unresolved Store operand graph from publishing Raw BIR");
}

void test_direct_global_integer_store_rejections() {
  const auto rejected = [](lir::LirModule candidate,
                           bir::ImportErrorCode expected,
                           const std::string& message) {
    const auto raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw.has_value() && raw.error().code == expected,
           message + " (Raw rollback)");
    const auto canonical = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical.has_value() && canonical.error().code == expected,
           message + " (Canonical rollback)");
  };

  auto raw_value = direct_global_integer_store_module();
  std::get<lir::LirStoreOp>(raw_value.functions[0].blocks[0].insts[0]).val =
      lir::LirOperand("7");
  rejected(std::move(raw_value),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a display-only integer value must remain unsupported");

  auto raw_pointer = direct_global_integer_store_module();
  std::get<lir::LirStoreOp>(raw_pointer.functions[0].blocks[0].insts[0]).ptr =
      lir::LirOperand("@store_i32_global");
  rejected(std::move(raw_pointer),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a display-only global pointer must remain unsupported");

  auto local_pointer = direct_global_integer_store_module();
  std::get<lir::LirStoreOp>(local_pointer.functions[0].blocks[0].insts[0]).ptr =
      lir::LirOperand::ssa("%local", lir::LirValueId{9});
  rejected(std::move(local_pointer),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "an SSA/local pointer store must remain outside direct-global receipt");

  auto noninteger = direct_global_integer_store_module();
  auto& noninteger_store =
      std::get<lir::LirStoreOp>(noninteger.functions[0].blocks[0].insts[0]);
  noninteger_store.type_str = lir::LirTypeRef("double");
  rejected(std::move(noninteger),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a noninteger Store type must remain unsupported");

  auto mismatched = direct_global_integer_store_module();
  std::get<lir::LirStoreOp>(mismatched.functions[0].blocks[0].insts[0])
      .type_str = lir::LirTypeRef::integer(64);
  rejected(std::move(mismatched),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "the Store type must exactly match its selected global object");

  auto out_of_range = direct_global_integer_store_module();
  std::get<lir::LirStoreOp>(out_of_range.functions[0].blocks[0].insts[0]).val =
      lir::LirOperand::integer("displayed-small", 1LL << 32);
  rejected(std::move(out_of_range),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "an out-of-range native immediate must reject without truncation");

  auto wrong_value_authority = direct_global_integer_store_module();
  auto& wrong_value = std::get<lir::LirStoreOp>(
      wrong_value_authority.functions[0].blocks[0].insts[0]);
  wrong_value.val = lir::LirOperand::global(
      "7", wrong_value_authority.globals[0].link_name_id);
  rejected(std::move(wrong_value_authority),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a non-immediate authority alternative must not be interpreted by display");

  auto wrong_pointer_authority = direct_global_integer_store_module();
  std::get<lir::LirStoreOp>(
      wrong_pointer_authority.functions[0].blocks[0].insts[0]).ptr =
      lir::LirOperand::integer("@store_i32_global", 1);
  rejected(std::move(wrong_pointer_authority),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a non-global pointer authority alternative must remain unsupported");

  auto unresolved = direct_global_integer_store_module();
  std::get<lir::LirStoreOp>(unresolved.functions[0].blocks[0].insts[0]).ptr =
      lir::LirOperand::global("@looks_valid",
                              static_cast<c4c::LinkNameId>(999));
  rejected(std::move(unresolved),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "an unresolved native LinkNameId must reject transactionally");

  auto ambiguous = direct_global_integer_store_module();
  ambiguous.globals[1].link_name_id = ambiguous.globals[0].link_name_id;
  ambiguous.globals[1].name = ambiguous.globals[0].name;
  rejected(std::move(ambiguous), bir::ImportErrorCode::UnsupportedGlobals,
           "ambiguous global ownership must reject before function publication");

  auto invalid_return = direct_global_integer_store_module();
  invalid_return.functions[0].blocks[0].terminator = lir::LirRet{
      lir::LirOperand::integer("misleading", 1),
      lir::LirTypeRef::integer(32)};
  rejected(std::move(invalid_return), bir::ImportErrorCode::InvalidVoidReturn,
           "accepted stores must advance diagnosis to the unsupported return boundary");

  auto later_load = direct_global_integer_store_module();
  later_load.functions[0].blocks[0].insts.push_back(lir::LirLoadOp{
      lir::LirOperand::ssa("%loaded", lir::LirValueId{77}),
      lir::LirTypeRef::integer(32),
      lir::LirOperand("@store_i32_global")});
  rejected(std::move(later_load),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a later raw load must roll back all earlier admitted stores");

  auto later_gep = direct_global_integer_store_module();
  later_gep.functions[0].blocks[0].insts.push_back(lir::LirGepOp{
      lir::LirOperand::ssa("%address", lir::LirValueId{78}),
      lir::LirTypeRef::integer(32),
      lir::LirOperand::global("@store_i32_global",
                              later_gep.globals[0].link_name_id),
      true,
      {lir::LirGepIndex::typed(lir::LirTypeRef::integer(64),
                               lir::LirOperand::integer("0", 0))}});
  rejected(std::move(later_gep),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a neighboring authoritative GEP must remain outside Store receipt");
}

lir::LirModule direct_global_integer_load_module() {
  lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  const auto i32_link = module.link_names.intern("load_i32_global");
  const auto i64_link = module.link_names.intern("load_i64_global");

  const auto external_global = [](std::string name, c4c::LinkNameId link,
                                  c4c::TypeBase base, std::string type,
                                  int alignment) {
    lir::LirGlobal global;
    global.name = std::move(name);
    global.link_name_id = link;
    global.type = scalar_type(base);
    global.linkage_vis = "external ";
    global.qualifier = "global ";
    global.llvm_type = std::move(type);
    global.align_bytes = alignment;
    global.is_extern_decl = true;
    return global;
  };
  module.globals.push_back(external_global(
      "load_i32_global", i32_link, c4c::TB_INT, "i32", 4));
  module.globals.push_back(external_global(
      "load_i64_global", i64_link, c4c::TB_LONGLONG, "i64", 8));

  auto block = return_block(0, "entry");
  block.insts.push_back(lir::LirLoadOp{
      lir::LirOperand::ssa("%display-result-999", lir::LirValueId{31}),
      lir::LirTypeRef::integer(32),
      lir::LirOperand::global("@misleading-load-source", i32_link)});
  block.insts.push_back(lir::LirLoadOp{
      lir::LirOperand::ssa("%display-result-zero", lir::LirValueId{32}),
      lir::LirTypeRef::integer(64),
      lir::LirOperand::global("@also-misleading-load-source", i64_link)});
  module.functions.push_back(
      void_definition("direct_global_integer_loads", {std::move(block)}));
  return module;
}

void test_direct_global_integer_load_receipt() {
  const auto module = direct_global_integer_load_module();
  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "two neighboring authoritative direct-global integer loads should publish Raw BIR");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "typed Load payloads and source-backed results should be verifier reachable");

  const auto module_view = imported.value().view();
  const auto globals = module_view.global_objects();
  const auto function_id = module_view.functions()[0];
  const auto function = module_view.function(function_id).value();
  const auto instructions =
      function.instructions(function.blocks()[0]).value();
  expect(globals.size() == 2 && instructions.size() == 2,
         "global and Load source order must remain exact");
  std::vector<bir::ValueId> result_order;
  for (std::size_t index = 0; index < instructions.size(); ++index) {
    const auto instruction = function.instruction(instructions[index]).value();
    const auto* load = instruction.load();
    const std::uint32_t source_id = static_cast<std::uint32_t>(31 + index);
    expect(instruction.opcode() == bir::Opcode::Load && load &&
               load->source == globals[index] &&
               instruction.operands().empty() &&
               instruction.results().size() == 1,
           "immutable Load view must expose one selected global and one result");
    const auto result_id = instruction.results()[0];
    result_order.push_back(result_id);
    const auto result = function.value(result_id).value();
    const auto* definition =
        std::get_if<bir::InstResultDef>(&result.definition);
    expect(result.type == load->loaded_type && result.source_id &&
               result.source_id->owner == function_id &&
               result.source_id->value == source_id && definition &&
               definition->instruction == instructions[index] &&
               definition->result_index == 0 &&
               function.source_value(
                           bir::SourceValueId{function_id, source_id})
                       .value() == result_id,
           "Load result must retain one native current-function source identity and coherent InstResultDef");
  }
  expect(function.values() == result_order,
         "Load results must preserve source instruction order in the sole value registry");

  auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "the admitted Load graph should pass canonical verification unchanged");
  const auto canonical_view = canonical.value().view();
  const auto canonical_function_id = canonical_view.functions()[0];
  const auto canonical_function =
      canonical_view.function(canonical_function_id).value();
  expect(canonical_function
                 .source_value(
                     bir::SourceValueId{canonical_function_id, 31})
                 .has_value() &&
             canonical_function
                     .instructions(canonical_function.blocks()[0])
                     .value().size() == 2,
         "Canonical BIR must preserve both Load nodes and source-value lookup");
}

void test_direct_global_integer_load_builder_contract() {
  bir::ModuleBuilder builder;
  const auto global = builder.add_global_object(
      "builder_load_global", bir::Type{bir::TypeKind::I32}, 4,
      false, false, false, true);
  bir::FunctionSignature signature;
  signature.return_type = bir::Type{bir::TypeKind::Void};
  const auto function =
      builder.create_function(signature, "builder_load", false);
  const auto foreign_function =
      builder.create_function(signature, "foreign_load_owner", true);
  expect(global.has_value() && function.has_value() &&
             foreign_function.has_value(),
         "Load builder fixture should create its typed owners");

  bir::InstId loaded{};
  bir::ValueId result{};
  const auto edited = builder.with_function(
      function.value(), [&](bir::FunctionBuilder& function_builder) {
        const auto block = function_builder.create_block("entry");
        if (!block)
          return bir::Result<void, bir::BuildError>::failure(block.error());
        const auto invalid_global = function_builder.append(
            block.value(),
            bir::LoadSpec{bir::GlobalObjectId{999, 0},
                          bir::Type{bir::TypeKind::I32}, 15});
        expect(!invalid_global.has_value() &&
                   invalid_global.error() ==
                       bir::BuildError::InvalidGlobalObject,
               "Load builder must reject a foreign global without staging state");
        const auto wrong_type = function_builder.append(
            block.value(),
            bir::LoadSpec{global.value(), bir::Type{bir::TypeKind::I64}, 15});
        expect(!wrong_type.has_value() &&
                   wrong_type.error() == bir::BuildError::InvalidValueType,
               "Load builder must reject incoherent global/result type facts");
        const auto invalid_source = function_builder.append(
            block.value(),
            bir::LoadSpec{global.value(), bir::Type{bir::TypeKind::I32},
                          std::numeric_limits<std::uint32_t>::max()});
        expect(!invalid_source.has_value() &&
                   invalid_source.error() ==
                       bir::BuildError::InvalidSourceValueId,
               "Load builder must reject the invalid native source identity");
        const auto appended = function_builder.append(
            block.value(),
            bir::LoadSpec{global.value(), bir::Type{bir::TypeKind::I32}, 15});
        if (!appended)
          return bir::Result<void, bir::BuildError>::failure(appended.error());
        loaded = appended.value().instruction;
        result = appended.value().results[0];
        const auto duplicate = function_builder.append(
            block.value(),
            bir::LoadSpec{global.value(), bir::Type{bir::TypeKind::I32}, 15});
        expect(!duplicate.has_value() &&
                   duplicate.error() ==
                       bir::BuildError::DuplicateSourceValue,
               "Load builder must reject duplicate current-function source identities atomically");
        return function_builder.set_terminator(block.value(),
                                               bir::ReturnTerm{});
      });
  expect(edited.has_value(), "one coherent Load should remain after rejected appends");
  auto raw = std::move(builder).publish();
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "a structurally built Load plus void return should publish Raw BIR");
  const auto module_view = raw.value().view();
  const auto view = module_view.function(function.value()).value();
  expect(view.instructions(view.blocks()[0]).value() ==
                 std::vector<bir::InstId>{loaded} &&
             view.instruction(loaded).value().load() != nullptr &&
             view.source_value(bir::SourceValueId{function.value(), 15})
                     .value() == result &&
             view.source_value(
                     bir::SourceValueId{foreign_function.value(), 15})
                     .error() == bir::ResolveError::WrongOwner,
         "Load construction must publish one source-backed result and isolate cross-function lookup");
  expect(bir::canonicalize(std::move(raw).value()).has_value(),
         "the structural Load plus void return should publish Canonical BIR");
}

void test_direct_global_integer_load_rejections() {
  const auto rejected = [](lir::LirModule candidate,
                           bir::ImportErrorCode expected,
                           const std::string& message) {
    const auto raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw.has_value() && raw.error().code == expected,
           message + " (Raw rollback)");
    const auto canonical = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical.has_value() && canonical.error().code == expected,
           message + " (Canonical rollback)");
  };

  auto raw_result = direct_global_integer_load_module();
  std::get<lir::LirLoadOp>(raw_result.functions[0].blocks[0].insts[0]).result =
      lir::LirOperand("%raw_result");
  rejected(std::move(raw_result),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a display-only Load result must remain unsupported");

  auto raw_pointer = direct_global_integer_load_module();
  std::get<lir::LirLoadOp>(raw_pointer.functions[0].blocks[0].insts[0]).ptr =
      lir::LirOperand("@load_i32_global");
  rejected(std::move(raw_pointer),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a display-only Load pointer must remain unsupported");

  auto local_pointer = direct_global_integer_load_module();
  std::get<lir::LirLoadOp>(local_pointer.functions[0].blocks[0].insts[0]).ptr =
      lir::LirOperand::ssa("%local", lir::LirValueId{9});
  rejected(std::move(local_pointer),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "an SSA/local Load pointer must remain outside direct-global receipt");

  auto wrong_result_authority = direct_global_integer_load_module();
  auto& wrong_result = std::get<lir::LirLoadOp>(
      wrong_result_authority.functions[0].blocks[0].insts[0]);
  wrong_result.result = lir::LirOperand::global(
      "%looks-like-result", wrong_result_authority.globals[0].link_name_id);
  rejected(std::move(wrong_result_authority),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a non-result authority alternative must not be interpreted by display");

  auto wrong_pointer_authority = direct_global_integer_load_module();
  std::get<lir::LirLoadOp>(
      wrong_pointer_authority.functions[0].blocks[0].insts[0]).ptr =
      lir::LirOperand::integer("@load_i32_global", 1);
  rejected(std::move(wrong_pointer_authority),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a non-global Load pointer authority must remain unsupported");

  auto invalid_result = direct_global_integer_load_module();
  std::get<lir::LirLoadOp>(invalid_result.functions[0].blocks[0].insts[0])
      .result = lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  rejected(std::move(invalid_result),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "an invalid native Load result identity must reject transactionally");

  auto noninteger = direct_global_integer_load_module();
  std::get<lir::LirLoadOp>(noninteger.functions[0].blocks[0].insts[0])
      .type_str = lir::LirTypeRef("double");
  rejected(std::move(noninteger),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a noninteger Load type must remain unsupported");

  auto mismatched = direct_global_integer_load_module();
  std::get<lir::LirLoadOp>(mismatched.functions[0].blocks[0].insts[0])
      .type_str = lir::LirTypeRef::integer(64);
  rejected(std::move(mismatched),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "the Load type must exactly match its selected global object");

  auto duplicate = direct_global_integer_load_module();
  std::get<lir::LirLoadOp>(duplicate.functions[0].blocks[0].insts[1]).result =
      lir::LirOperand::ssa("%different-display-same-id", lir::LirValueId{31});
  rejected(std::move(duplicate),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "duplicate native result identities must reject without display fallback");

  auto unresolved = direct_global_integer_load_module();
  std::get<lir::LirLoadOp>(unresolved.functions[0].blocks[0].insts[0]).ptr =
      lir::LirOperand::global("@looks-valid",
                              static_cast<c4c::LinkNameId>(999));
  rejected(std::move(unresolved),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "an unresolved native Load LinkNameId must reject transactionally");

  auto ambiguous = direct_global_integer_load_module();
  ambiguous.globals[1].link_name_id = ambiguous.globals[0].link_name_id;
  ambiguous.globals[1].name = ambiguous.globals[0].name;
  rejected(std::move(ambiguous), bir::ImportErrorCode::UnsupportedGlobals,
           "ambiguous Load global ownership must reject before function publication");

  auto invalid_return = direct_global_integer_load_module();
  invalid_return.functions[0].blocks[0].terminator = lir::LirRet{
      lir::LirOperand::integer("misleading", 1),
      lir::LirTypeRef::integer(32)};
  rejected(std::move(invalid_return), bir::ImportErrorCode::InvalidVoidReturn,
           "accepted loads must advance diagnosis to the unsupported return boundary");

  auto later_gep = direct_global_integer_load_module();
  later_gep.functions[0].blocks[0].insts.push_back(lir::LirGepOp{
      lir::LirOperand::ssa("%address", lir::LirValueId{78}),
      lir::LirTypeRef::integer(32),
      lir::LirOperand::global("@load_i32_global",
                              later_gep.globals[0].link_name_id),
      true,
      {lir::LirGepIndex::typed(lir::LirTypeRef::integer(64),
                               lir::LirOperand::integer("0", 0))}});
  rejected(std::move(later_gep),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a later unsupported GEP must roll back all earlier admitted loads");
}

lir::LirModule selected_global_array_gep_module() {
  lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  const auto i32_link = module.link_names.intern("gep_i32_array");
  const auto i64_link = module.link_names.intern("gep_i64_array");

  const auto external_array = [](std::string name, c4c::LinkNameId link,
                                 c4c::TypeBase base, int count,
                                 std::string llvm_type, int alignment) {
    lir::LirGlobal global;
    global.name = std::move(name);
    global.link_name_id = link;
    global.type = scalar_type(base);
    global.type.array_rank = 1;
    global.type.array_size = count;
    global.type.array_dims[0] = count;
    global.linkage_vis = "external ";
    global.qualifier = "global ";
    global.llvm_type = std::move(llvm_type);
    global.align_bytes = alignment;
    global.is_extern_decl = true;
    return global;
  };
  module.globals.push_back(external_array(
      "gep_i32_array", i32_link, c4c::TB_INT, 2, "[2 x i32]", 4));
  module.globals.push_back(external_array(
      "gep_i64_array", i64_link, c4c::TB_LONGLONG, 3, "[3 x i64]", 8));

  auto block = return_block(0, "entry");
  block.insts.push_back(lir::LirGepOp{
      lir::LirOperand::ssa("%misleading-gep-result", lir::LirValueId{61}),
      lir::LirTypeRef("[2 x i32]"),
      lir::LirOperand::global("@misleading-gep-base", i32_link), true,
      {lir::LirGepIndex::typed(
           lir::LirTypeRef::integer(64),
           lir::LirOperand::integer("displayed-nine", 0)),
       lir::LirGepIndex::typed(
           lir::LirTypeRef::integer(64),
           lir::LirOperand::integer("displayed-zero", 1))}});
  block.insts.push_back(lir::LirGepOp{
      lir::LirOperand::ssa("%also-misleading-result", lir::LirValueId{62}),
      lir::LirTypeRef("[3 x i64]"),
      lir::LirOperand::global("@also-misleading-base", i64_link), false,
      {lir::LirGepIndex::typed(
           lir::LirTypeRef::integer(32),
           lir::LirOperand::integer("displayed-seven", 0)),
       lir::LirGepIndex::typed(
           lir::LirTypeRef::integer(64),
           lir::LirOperand::integer("displayed-one", 2))}});
  module.functions.push_back(
      void_definition("selected_global_array_geps", {std::move(block)}));
  return module;
}

void test_selected_global_array_gep_receipt() {
  const auto module = selected_global_array_gep_module();
  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "two neighboring authoritative selected-global GEPs should publish Raw BIR");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "typed GEP payloads, ordered indices, and results should be verifier reachable");
  const auto module_view = imported.value().view();
  const auto globals = module_view.global_objects();
  const auto function_id = module_view.functions()[0];
  const auto function = module_view.function(function_id).value();
  const auto instructions =
      function.instructions(function.blocks()[0]).value();
  expect(globals.size() == 2 && instructions.size() == 2,
         "selected globals and GEP instructions must preserve source order");
  const std::vector<std::vector<std::int64_t>> expected_indices = {
      {0, 1}, {0, 2}};
  std::vector<bir::ValueId> expected_value_order;
  for (std::size_t instruction_index = 0;
       instruction_index < instructions.size(); ++instruction_index) {
    const auto instruction =
        function.instruction(instructions[instruction_index]).value();
    const auto* gep = instruction.get_element_ptr();
    const auto* global_base = gep
        ? std::get_if<bir::GlobalObjectId>(&gep->base.authority)
        : nullptr;
    expect(instruction.opcode() == bir::Opcode::GetElementPtr && gep &&
               global_base && *global_base == globals[instruction_index] &&
               gep->element_type ==
                   module_view.global_object(globals[instruction_index])
                       .value().object_type &&
               gep->inbounds == (instruction_index == 0) &&
               instruction.operands().size() == 2 &&
               instruction.results().size() == 1,
           "GEP view must expose exact structured global type, flag, ordered uses, and one result");
    for (std::size_t operand_index = 0;
         operand_index < instruction.operands().size(); ++operand_index) {
      const auto index_value =
          function.value(instruction.operands()[operand_index]).value();
      expected_value_order.push_back(instruction.operands()[operand_index]);
      const auto constant = module_view
                                .constant(std::get<bir::ConstantDef>(
                                              index_value.definition)
                                              .constant)
                                .value();
      const auto expected_index_type =
          instruction_index == 1 && operand_index == 0
              ? bir::Type{bir::TypeKind::Integer, 32, "i32"}
              : bir::Type{bir::TypeKind::Integer, 64, "i64"};
      expect(index_value.type == expected_index_type &&
                 !index_value.source_id &&
                 std::get<bir::IntegerConstant>(constant.payload).value ==
                     expected_indices[instruction_index][operand_index],
             "native immediate GEP indices must become ordered source-less exact constants");
    }
    const auto result_id = instruction.results()[0];
    expected_value_order.push_back(result_id);
    const auto result = function.value(result_id).value();
    const auto* definition =
        std::get_if<bir::InstResultDef>(&result.definition);
    const std::uint32_t source_id =
        static_cast<std::uint32_t>(61 + instruction_index);
    expect(result.type == bir::Type{bir::TypeKind::Pointer} &&
               result.source_id && result.source_id->value == source_id &&
               definition && definition->instruction == instructions[instruction_index] &&
               definition->result_index == 0 &&
               function.source_value(
                           bir::SourceValueId{function_id, source_id})
                       .value() == result_id,
           "GEP result must be one source-backed pointer InstResultDef");
  }
  expect(function.values() == expected_value_order,
         "GEP index constants and results must preserve source instruction order in the ordinary value registry");

  auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "the admitted selected-global GEP graph should publish Canonical BIR");
  const auto canonical_view = canonical.value().view();
  const auto canonical_function_id = canonical_view.functions()[0];
  const auto canonical_function =
      canonical_view.function(canonical_function_id).value();
  expect(canonical_function
                 .source_value(
                     bir::SourceValueId{canonical_function_id, 62})
                 .has_value() &&
             canonical_function
                     .instructions(canonical_function.blocks()[0])
                     .value().size() == 2,
         "Canonical BIR must preserve neighboring GEP results and source lookup");
}

void test_indirect_branch_terminator_receipt_and_rejections() {
  auto indirect_branch_module = [] {
    auto module = selected_global_array_gep_module();
    auto& function = module.functions.front();
    function.blocks[0].terminator = lir::LirIndirectBr{
        lir::LirValueId{61}, {lir::LirBlockId{2}, lir::LirBlockId{1}}};
    function.blocks.push_back(return_block(1, "first_target"));
    function.blocks.push_back(return_block(2, "second_target"));
    return module;
  };

  const auto module = indirect_branch_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "typed legacy indirect branch must publish verified Raw BIR");
  const auto function = raw.value().view().function(raw.value().view().functions()[0]).value();
  const auto blocks = function.blocks();
  const auto terminator = function.terminator(blocks[0]);
  const auto* indirect = terminator.has_value()
                             ? std::get_if<bir::IndirectJumpTerm>(&terminator.value())
                             : nullptr;
  expect(indirect && indirect->address ==
                         function.source_value(
                             bir::SourceValueId{raw.value().view().functions()[0], 61})
                             .value() &&
             indirect->targets == std::vector<bir::BlockId>({blocks[2], blocks[1]}) &&
             function.successors(blocks[0]).value() == indirect->targets,
         "indirect branch must preserve typed address and ordered target identities without labels");
  expect(bir::lower_lir_to_canonical_bir(module).has_value(),
         "typed legacy indirect branch must canonicalize");

  const auto rejected = [&](auto mutate, bir::ImportErrorCode expected,
                            const std::string& message) {
    auto candidate = indirect_branch_module();
    mutate(candidate);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() && raw_rejected.error().code == expected,
           message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value() && canonical_rejected.error().code == expected,
           message + " (Canonical rollback)");
  };
  rejected([](lir::LirModule& candidate) {
    std::get<lir::LirIndirectBr>(candidate.functions[0].blocks[0].terminator).addr =
        lir::LirValueId::invalid();
  }, bir::ImportErrorCode::UnsupportedTerminator,
  "missing indirect address authority must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    std::get<lir::LirIndirectBr>(candidate.functions[0].blocks[0].terminator).addr =
        lir::LirValueId{999};
  }, bir::ImportErrorCode::UnsupportedTerminator,
  "unresolved indirect address authority must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    std::get<lir::LirIndirectBr>(candidate.functions[0].blocks[0].terminator).targets.clear();
  }, bir::ImportErrorCode::MissingBranchTarget,
  "missing indirect target authority must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    std::get<lir::LirIndirectBr>(candidate.functions[0].blocks[0].terminator).targets =
        {lir::LirBlockId{2}, lir::LirBlockId{2}};
  }, bir::ImportErrorCode::MissingBranchTarget,
  "duplicate indirect target authority must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    std::get<lir::LirIndirectBr>(candidate.functions[0].blocks[0].terminator).targets =
        {lir::LirBlockId{999}};
  }, bir::ImportErrorCode::MissingBranchTarget,
  "invalid indirect target authority must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    candidate.functions[0].blocks[2].id = lir::LirBlockId{1};
  }, bir::ImportErrorCode::DuplicateBlockId,
  "ambiguous current-function indirect target ownership must reject transactionally");

  auto foreign = indirect_branch_module();
  foreign.functions[0].blocks.pop_back();
  foreign.functions.push_back(void_definition("foreign_owner", {return_block(2, "foreign_target")}));
  const auto foreign_raw = bir::lower_lir_to_raw_bir(foreign);
  expect(!foreign_raw.has_value() &&
             foreign_raw.error().code == bir::ImportErrorCode::MissingBranchTarget &&
             !bir::lower_lir_to_canonical_bir(foreign).has_value(),
         "foreign indirect target authority must publish neither Raw nor Canonical BIR");
}

void test_direct_label_address_constant_receipt_and_rejections() {
  auto direct_module = [] {
    auto module = selected_global_array_gep_module();
    auto& function = module.functions.front();
    function.link_name_id = module.link_names.intern("direct_label_owner");
    function.signature_text = "define void @direct_label_owner()";
    function.direct_label_address_constants.push_back(
        {function.link_name_id, lir::LirBlockId{1}, lir::LirTypeRef("ptr"),
         lir::LirValueId{63}});
    function.blocks[0].insts.push_back(lir::LirIndirectBrOp{
        lir::LirOperand::direct_constant(lir::LirValueId{63}), lir::LirValueId{63},
        {"target"}, {lir::LirBlockId{1}}});
    function.blocks[0].terminator = lir::LirUnreachable{};
    function.blocks.push_back(return_block(1, "target"));
    return module;
  };
  const auto module = direct_module();
  const auto store_module = [&] {
    auto candidate = direct_module();
    candidate.functions[0].blocks[0].insts.insert(
        candidate.functions[0].blocks[0].insts.begin(), lir::LirStoreOp{
            lir::LirTypeRef("ptr"), lir::LirOperand::direct_constant(lir::LirValueId{63}),
            lir::LirOperand::ssa("%label_address_slot", lir::LirValueId{61})});
    return candidate;
  };
  const auto direct_gep_module = [&] {
    auto candidate = direct_module();
    auto& gep = std::get<lir::LirGepOp>(candidate.functions[0].blocks[0].insts[0]);
    gep.ptr = lir::LirOperand::direct_constant(lir::LirValueId{63});
    return candidate;
  };
  lir::verify_module(store_module());
  lir::verify_module(direct_gep_module());
  const std::string printed = lir::print_llvm(store_module());
  expect(printed.find("store ptr blockaddress(@direct_label_owner, %target), ptr %label_address_slot") !=
             std::string::npos,
         "pointer store must render the structured direct label-address constant");
  const std::string direct_gep_printed = lir::print_llvm(direct_gep_module());
  expect(direct_gep_printed.find(
             "getelementptr inbounds [2 x i32], ptr blockaddress(@direct_label_owner, %target)") !=
             std::string::npos,
         "GEP must render its structured direct label-address base without display recovery");
  const auto direct_gep_raw = bir::lower_lir_to_raw_bir(direct_gep_module());
  expect(direct_gep_raw.has_value() &&
             bir::FoundationVerifier::verify(direct_gep_raw.value()).ok(),
         "direct label-address GEP must publish verified Raw BIR");
  const auto direct_gep_view = direct_gep_raw.value().view();
  const auto direct_gep_function =
      direct_gep_view.function(direct_gep_view.functions()[0]).value();
  const auto direct_gep_instructions =
      direct_gep_function.instructions(direct_gep_function.blocks()[0]).value();
  const auto* direct_gep = direct_gep_function
      .instruction(direct_gep_instructions[0]).value().get_element_ptr();
  const auto* direct_gep_base = direct_gep
      ? std::get_if<bir::LabelAddressGepBase>(&direct_gep->base.authority)
      : nullptr;
  expect(direct_gep_base &&
             direct_gep_base->value ==
                 direct_gep_function.source_value(
                     bir::SourceValueId{direct_gep_function.id(), 63}).value(),
         "GEP lowering must retain the exact typed direct label-address base identity");
  auto malformed_direct_gep = direct_gep_module();
  std::get<lir::LirGepOp>(malformed_direct_gep.functions[0].blocks[0].insts[0]).ptr =
      lir::LirOperand::direct_constant(lir::LirValueId{61});
  expect(!bir::lower_lir_to_raw_bir(malformed_direct_gep).has_value() &&
             !bir::lower_lir_to_canonical_bir(malformed_direct_gep).has_value(),
         "GEP lowering must reject a direct constant without the current-function label-address authority");
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "direct label-address constant must publish verified Raw BIR");
  const auto view = raw.value().view();
  const auto function_id = view.functions()[0];
  const auto function = view.function(function_id).value();
  const auto term = function.terminator(function.blocks()[0]);
  const auto* indirect = term ? std::get_if<bir::IndirectJumpTerm>(&term.value()) : nullptr;
  expect(indirect && indirect->address ==
                         function.source_value(bir::SourceValueId{function_id, 63}).value(),
         "existing indirect jump must consume the direct label-address source value");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = direct_module();
    mutate(candidate);
    expect(!bir::lower_lir_to_raw_bir(candidate).has_value() &&
               !bir::lower_lir_to_canonical_bir(candidate).has_value(),
           message);
  };
  rejected([](lir::LirModule& candidate) {
    candidate.functions[0].direct_label_address_constants[0].value = lir::LirValueId::invalid();
  }, "invalid direct label-address value must reject");
  rejected([](lir::LirModule& candidate) {
    candidate.functions[0].direct_label_address_constants.clear();
  }, "missing direct label-address definition must reject");
  rejected([](lir::LirModule& candidate) {
    candidate.functions[0].direct_label_address_constants.push_back(
        candidate.functions[0].direct_label_address_constants[0]);
  }, "duplicate direct label-address value must reject");
  rejected([](lir::LirModule& candidate) {
    candidate.functions[0].direct_label_address_constants[0].type = lir::LirTypeRef::integer(32);
  }, "nonpointer direct label-address type must reject");
  rejected([](lir::LirModule& candidate) {
    candidate.functions[0].direct_label_address_constants[0].owner = c4c::kInvalidLinkName;
  }, "invalid direct label-address owner must reject");
  rejected([](lir::LirModule& candidate) {
    candidate.functions.push_back(void_definition("foreign_label_owner",
                                                   {return_block(999, "foreign_target")}));
    candidate.functions[0].direct_label_address_constants[0].target = lir::LirBlockId{999};
  }, "foreign direct label-address target must reject");
  rejected([](lir::LirModule& candidate) {
    auto& op = std::get<lir::LirIndirectBrOp>(candidate.functions[0].blocks[0].insts.back());
    op.addr = lir::LirOperand::direct_constant(lir::LirValueId{61});
  }, "mismatched direct use and address identity must reject");

  const auto rejected_by_verifier = [&](auto mutate, const std::string& message) {
    auto candidate = store_module();
    mutate(candidate);
    try {
      lir::verify_module(candidate);
    } catch (const lir::LirVerifyError&) {
      return;
    }
    fail(message);
  };
  rejected_by_verifier([](lir::LirModule& candidate) {
    candidate.functions[0].direct_label_address_constants.clear();
  }, "store direct use without its definition must reject in the verifier");
  rejected_by_verifier([](lir::LirModule& candidate) {
    candidate.functions[0].direct_label_address_constants.push_back(
        candidate.functions[0].direct_label_address_constants[0]);
  }, "duplicate store direct definition must reject in the verifier");
  rejected_by_verifier([](lir::LirModule& candidate) {
    candidate.functions[0].direct_label_address_constants[0].owner =
        c4c::kInvalidLinkName;
  }, "invalid store direct owner must reject in the verifier");
  rejected_by_verifier([](lir::LirModule& candidate) {
    candidate.functions[0].direct_label_address_constants[0].owner =
        candidate.link_names.intern("foreign_store_direct_owner");
  }, "foreign store direct owner must reject in the verifier");
  rejected_by_verifier([](lir::LirModule& candidate) {
    candidate.functions[0].direct_label_address_constants[0].target =
        lir::LirBlockId{999};
  }, "invalid store direct target must reject in the verifier");
  rejected_by_verifier([](lir::LirModule& candidate) {
    auto foreign = void_definition("foreign_store_target",
                                   {return_block(999, "foreign_target")});
    foreign.signature_text = "define void @foreign_store_target()";
    candidate.functions.push_back(std::move(foreign));
    candidate.functions[0].direct_label_address_constants[0].target =
        lir::LirBlockId{999};
  }, "foreign store direct target must reject in the verifier");
  rejected_by_verifier([](lir::LirModule& candidate) {
    candidate.functions[0].direct_label_address_constants[0].type =
        lir::LirTypeRef::integer(32);
  }, "nonpointer store direct definition must reject in the verifier");
  rejected_by_verifier([](lir::LirModule& candidate) {
    auto& store = std::get<lir::LirStoreOp>(candidate.functions[0].blocks[0].insts[0]);
    store.type_str = lir::LirTypeRef::integer(32);
  }, "wrong store direct value type must reject in the verifier");
  rejected_by_verifier([](lir::LirModule& candidate) {
    auto& store = std::get<lir::LirStoreOp>(candidate.functions[0].blocks[0].insts[0]);
    store.val = lir::LirOperand::direct_constant(lir::LirValueId{61});
  }, "mismatched store direct use must reject in the verifier");

  const auto rejected_direct_gep_by_verifier = [&](auto mutate,
                                                    const std::string& message) {
    auto candidate = direct_gep_module();
    mutate(candidate);
    try {
      lir::verify_module(candidate);
    } catch (const lir::LirVerifyError&) {
      return;
    }
    fail(message);
  };
  rejected_direct_gep_by_verifier([](lir::LirModule& candidate) {
    candidate.functions[0].direct_label_address_constants.clear();
  }, "direct GEP use without its definition must reject in the verifier");
  rejected_direct_gep_by_verifier([](lir::LirModule& candidate) {
    auto& gep = std::get<lir::LirGepOp>(candidate.functions[0].blocks[0].insts[0]);
    gep.ptr = lir::LirOperand::direct_constant(lir::LirValueId{61});
  }, "arbitrary direct GEP identity must reject in the verifier");
  rejected_direct_gep_by_verifier([](lir::LirModule& candidate) {
    candidate.functions[0].direct_label_address_constants[0].type =
        lir::LirTypeRef::integer(32);
  }, "nonpointer direct GEP definition must reject in the verifier");
  rejected_direct_gep_by_verifier([](lir::LirModule& candidate) {
    auto& gep = std::get<lir::LirGepOp>(candidate.functions[0].blocks[0].insts[0]);
    gep.ptr.str() = "%misleading-direct-gep-base";
  }, "direct GEP display spelling must not override structured identity");
  rejected_direct_gep_by_verifier([](lir::LirModule& candidate) {
    auto& gep = std::get<lir::LirGepOp>(candidate.functions[0].blocks[0].insts[0]);
    gep.ptr = lir::LirOperand::integer("0", 0);
  }, "nonpointer direct GEP operand form must reject in the verifier");
}

void test_typed_computed_goto_receipt_and_rejections() {
  auto computed_goto_module = [] {
    auto module = selected_global_array_gep_module();
    auto& function = module.functions.front();
    auto& entry = function.blocks.front();
    entry.insts.push_back(lir::LirIndirectBrOp{
        lir::LirOperand::ssa("%misleading-address-display", lir::LirValueId{61}),
        lir::LirValueId{61}, {"misleading-second", "misleading-first"},
        {lir::LirBlockId{2}, lir::LirBlockId{1}}});
    entry.terminator = lir::LirUnreachable{};
    function.blocks.push_back(return_block(1, "first-target"));
    function.blocks.push_back(return_block(2, "second-target"));
    return module;
  };

  const auto module = computed_goto_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "typed computed-goto authority must publish verified Raw BIR");
  const auto view = raw.value().view();
  const auto function_id = view.functions()[0];
  const auto function = view.function(function_id).value();
  const auto blocks = function.blocks();
  const auto terminator = function.terminator(blocks[0]);
  const auto* indirect = terminator.has_value()
                             ? std::get_if<bir::IndirectJumpTerm>(&terminator.value())
                             : nullptr;
  expect(indirect && indirect->address ==
                         function.source_value(bir::SourceValueId{function_id, 61}).value() &&
             indirect->targets == std::vector<bir::BlockId>({blocks[2], blocks[1]}) &&
             function.successors(blocks[0]).value() == indirect->targets,
         "computed goto must consume only typed address and ordered successor authority");

  const auto rejected = [&](auto mutate, bir::ImportErrorCode expected,
                            const std::string& message) {
    auto candidate = computed_goto_module();
    mutate(candidate);
    const auto rejected_raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!rejected_raw.has_value() && rejected_raw.error().code == expected,
           message + " (Raw rollback)");
    expect(!bir::lower_lir_to_canonical_bir(candidate).has_value(),
           message + " (Canonical rollback)");
  };
  rejected([](lir::LirModule& candidate) {
    std::get<lir::LirIndirectBrOp>(candidate.functions[0].blocks[0].insts.back())
        .addr_value.reset();
  }, bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
  "missing computed-goto address authority must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    std::get<lir::LirIndirectBrOp>(candidate.functions[0].blocks[0].insts.back())
        .addr_value = lir::LirValueId{999};
  }, bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
  "foreign computed-goto address authority must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    std::get<lir::LirIndirectBrOp>(candidate.functions[0].blocks[0].insts.back())
        .addr_value = lir::LirValueId::invalid();
  }, bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
  "invalid computed-goto address authority must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    auto& entry = candidate.functions[0].blocks[0];
    entry.insts.insert(entry.insts.end() - 1,
                       lir::LirConstInt{lir::LirValueId{63}, scalar_type(c4c::TB_INT), 1});
    std::get<lir::LirIndirectBrOp>(entry.insts.back()).addr_value = lir::LirValueId{63};
  }, bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
  "non-pointer computed-goto address authority must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    std::get<lir::LirIndirectBrOp>(candidate.functions[0].blocks[0].insts.back())
        .successors.clear();
  }, bir::ImportErrorCode::MissingBranchTarget,
  "missing computed-goto successor authority must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    std::get<lir::LirIndirectBrOp>(candidate.functions[0].blocks[0].insts.back())
        .successors = {lir::LirBlockId{2}, lir::LirBlockId{2}};
  }, bir::ImportErrorCode::MissingBranchTarget,
  "duplicate computed-goto successors must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    std::get<lir::LirIndirectBrOp>(candidate.functions[0].blocks[0].insts.back())
        .successors = {lir::LirBlockId{999}};
  }, bir::ImportErrorCode::MissingBranchTarget,
  "invalid computed-goto successors must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    candidate.functions[0].blocks.pop_back();
    candidate.functions.push_back(
        void_definition("foreign-owner", {return_block(2, "foreign-target")}));
  }, bir::ImportErrorCode::MissingBranchTarget,
  "foreign computed-goto successors must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    candidate.functions[0].blocks[0].insts.push_back(
        lir::LirConstInt{lir::LirValueId{64}, scalar_type(c4c::TB_INT), 1});
  }, bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
  "computed-goto carrier with later instructions must reject transactionally");
}

lir::LirModule selected_global_i32_slt_compare_module();

void test_switch_selector_type_authority_and_display_mirror() {
  lir::LirModule module;
  lir::LirFunction function;
  function.name = "switch_selector_type_authority";
  function.signature_text = "define void @switch_selector_type_authority()";

  lir::LirBlock entry;
  entry.id = lir::LirBlockId{0};
  entry.label = "entry";
  const auto selector_id = lir::LirValueId{0};
  entry.insts.push_back(lir::LirBinOp{
      .result = lir::LirOperand::ssa("%selector", selector_id),
      .opcode = lir::LirBinaryOpcode::Add,
      .type_str = lir::LirTypeRef::integer(32),
      .lhs = lir::LirOperand::integer("7", 7),
      .rhs = lir::LirOperand::integer("0", 0),
  });
  entry.terminator = lir::LirSwitch{
      .selector_name = "%selector",
      .selector_type = "i64",
      .default_label = "default",
      .cases = {{7, "case"}},
      .default_successor = lir::LirBlockId{1},
      .case_successors = {lir::LirBlockId{2}},
      .selector = selector_id,
      .selector_type_ref = lir::LirTypeRef::integer(32),
  };
  lir::LirBlock default_block;
  default_block.id = lir::LirBlockId{1};
  default_block.label = "default";
  default_block.terminator = lir::LirRet{std::nullopt, lir::LirTypeRef("void")};
  lir::LirBlock case_block;
  case_block.id = lir::LirBlockId{2};
  case_block.label = "case";
  case_block.terminator = lir::LirRet{std::nullopt, lir::LirTypeRef("void")};
  function.blocks = {std::move(entry), std::move(default_block), std::move(case_block)};
  module.functions.push_back(std::move(function));

  try {
    lir::verify_module(module);
    fail("stale switch selector_type text must not override structured i32 authority");
  } catch (const lir::LirVerifyError&) {
  }

  auto& sw = std::get<lir::LirSwitch>(module.functions[0].blocks[0].terminator);
  sw.selector_type = "i32";
  try {
    lir::verify_module(module);
  } catch (const lir::LirVerifyError& error) {
    fail(std::string("structured i32 switch selector facts must verify: ") + error.what());
  }
  const std::string llvm = lir::print_llvm(module);
  expect(llvm.find("switch i32 %selector") != std::string::npos,
         "switch printer must render the verified structured i32 selector authority");
  sw.selector_type_ref = lir::LirTypeRef::integer(64);
  try {
    lir::verify_module(module);
    fail("structured switch selector type mismatch must fail verification");
  } catch (const lir::LirVerifyError&) {
  }
}

void test_switch_terminator_receipt_and_rejections() {
  auto switch_module = [] {
    auto module = selected_global_i32_slt_compare_module();
    auto& function = module.functions[0];
    function.blocks[0].terminator = lir::LirSwitch{
        "%presentation-only-slt-result", "i1",
        "default-display", {{7, "case-display"}},
        lir::LirBlockId{2}, {lir::LirBlockId{1}}, lir::LirValueId{33},
        lir::LirTypeRef::integer(1)};
    function.blocks.push_back(return_block(1, "case-display"));
    function.blocks.push_back(return_block(2, "default-display"));
    return module;
  };

  const auto module = switch_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "typed switch must publish verified Raw BIR");
  const auto view = raw.value().view();
  const auto function_id = view.functions()[0];
  const auto function = view.function(function_id).value();
  const auto blocks = function.blocks();
  const auto terminator = function.terminator(blocks[0]);
  const auto* sw = terminator.has_value()
                       ? std::get_if<bir::SwitchTerm>(&terminator.value())
                       : nullptr;
  expect(sw && sw->selector ==
                   function.source_value(bir::SourceValueId{function_id, 33}).value() &&
             sw->default_target == blocks[2] &&
             sw->case_targets == std::vector<bir::BlockId>({blocks[1]}) &&
             function.successors(blocks[0]).value() ==
                 std::vector<bir::BlockId>({blocks[2], blocks[1]}),
         "switch must retain typed selector/default/case order without display recovery");
  expect(bir::lower_lir_to_canonical_bir(module).has_value(),
         "typed switch must canonicalize");

  const auto rejected = [&](auto mutate, bir::ImportErrorCode expected,
                            const std::string& message) {
    auto candidate = switch_module();
    mutate(candidate);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() && raw_rejected.error().code == expected,
           message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value() && canonical_rejected.error().code == expected,
           message + " (Canonical rollback)");
  };
  rejected([](lir::LirModule& candidate) {
    std::get<lir::LirSwitch>(candidate.functions[0].blocks[0].terminator).selector =
        lir::LirValueId::invalid();
  }, bir::ImportErrorCode::UnsupportedTerminator,
  "missing switch selector authority must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    std::get<lir::LirSwitch>(candidate.functions[0].blocks[0].terminator).selector =
        lir::LirValueId{999};
  }, bir::ImportErrorCode::UnsupportedTerminator,
  "invalid switch selector authority must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    auto& sw = std::get<lir::LirSwitch>(candidate.functions[0].blocks[0].terminator);
    sw.default_successor = lir::LirBlockId::invalid();
  }, bir::ImportErrorCode::MissingBranchTarget,
  "missing switch default authority must reject transactionally");
  auto parallel_switch = switch_module();
  std::get<lir::LirSwitch>(parallel_switch.functions[0].blocks[0].terminator)
      .case_successors = {lir::LirBlockId{2}};
  const auto parallel_switch_raw = bir::lower_lir_to_raw_bir(parallel_switch);
  expect(parallel_switch_raw.has_value() &&
             parallel_switch_raw.value().view().function(
                 parallel_switch_raw.value().view().functions()[0]).value()
                 .successors(parallel_switch_raw.value().view().function(
                     parallel_switch_raw.value().view().functions()[0]).value().blocks()[0]).value().size() == 2,
         "parallel switch successor occurrences must remain distinct Raw-BIR CFG edges");
  rejected([](lir::LirModule& candidate) {
    auto& sw = std::get<lir::LirSwitch>(candidate.functions[0].blocks[0].terminator);
    sw.case_successors.clear();
  }, bir::ImportErrorCode::MissingBranchTarget,
  "incoherent switch case authority must reject transactionally");

  auto non_integer = selected_global_array_gep_module();
  non_integer.functions[0].blocks[0].terminator = lir::LirSwitch{
      "misleading-pointer-selector", "i32", "default", {},
      lir::LirBlockId{1}, {}, lir::LirValueId{61}};
  non_integer.functions[0].blocks.push_back(return_block(1, "default"));
  expect(!bir::lower_lir_to_raw_bir(non_integer).has_value() &&
             !bir::lower_lir_to_canonical_bir(non_integer).has_value(),
         "non-integer switch selector authority must reject transactionally");

  auto foreign = switch_module();
  foreign.functions.push_back(void_definition(
      "foreign_switch_owner", {return_block(3, "foreign-target")}));
  std::get<lir::LirSwitch>(foreign.functions[0].blocks[0].terminator)
      .default_successor = lir::LirBlockId{3};
  expect(!bir::lower_lir_to_raw_bir(foreign).has_value() &&
             !bir::lower_lir_to_canonical_bir(foreign).has_value(),
         "foreign switch target authority must publish neither Raw nor Canonical BIR");
}

void test_conditional_branch_terminator_receipt_and_rejections() {
  auto conditional_branch_module = [] {
    auto module = selected_global_i32_slt_compare_module();
    auto& function = module.functions[0];
    function.blocks[0].terminator = lir::LirCondBr{
        "%presentation-only-slt-result", "true-display", "false-display",
        lir::LirBlockId{1}, lir::LirBlockId{2}, lir::LirValueId{33}};
    function.blocks.push_back(return_block(1, "true-display"));
    function.blocks.push_back(return_block(2, "false-display"));
    return module;
  };

  const auto module = conditional_branch_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "typed conditional branch must publish verified Raw BIR");
  const auto view = raw.value().view();
  const auto function_id = view.functions()[0];
  const auto function = view.function(function_id).value();
  const auto blocks = function.blocks();
  const auto terminator = function.terminator(blocks[0]);
  const auto* conditional = terminator.has_value()
                                ? std::get_if<bir::CondJumpTerm>(&terminator.value())
                                : nullptr;
  expect(conditional && conditional->condition ==
                            function.source_value(bir::SourceValueId{function_id, 33}).value() &&
             conditional->true_target == blocks[1] &&
             conditional->false_target == blocks[2] &&
             function.successors(blocks[0]).value() ==
                 std::vector<bir::BlockId>({blocks[1], blocks[2]}),
         "conditional branch must preserve typed condition and ordered successors without display recovery");
  expect(bir::lower_lir_to_canonical_bir(module).has_value(),
         "typed conditional branch must canonicalize");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = conditional_branch_module();
    mutate(candidate);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value(), message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value(), message + " (Canonical rollback)");
  };
  rejected([](lir::LirModule& candidate) {
    std::get<lir::LirCondBr>(candidate.functions[0].blocks[0].terminator).condition =
        lir::LirValueId::invalid();
  }, "missing conditional condition authority must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    std::get<lir::LirCondBr>(candidate.functions[0].blocks[0].terminator).condition =
        lir::LirValueId{999};
  }, "invalid conditional condition authority must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    std::get<lir::LirCondBr>(candidate.functions[0].blocks[0].terminator).condition =
        lir::LirValueId{31};
  }, "non-boolean conditional condition authority must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    auto& branch = std::get<lir::LirCondBr>(candidate.functions[0].blocks[0].terminator);
    branch.true_successor = lir::LirBlockId::invalid();
  }, "missing conditional successor authority must reject transactionally");
  rejected([](lir::LirModule& candidate) {
    auto& branch = std::get<lir::LirCondBr>(candidate.functions[0].blocks[0].terminator);
    branch.false_successor = lir::LirBlockId{999};
  }, "invalid conditional successor authority must reject transactionally");
  auto parallel_conditional = conditional_branch_module();
  auto& parallel_branch = std::get<lir::LirCondBr>(
      parallel_conditional.functions[0].blocks[0].terminator);
  parallel_branch.false_successor = parallel_branch.true_successor;
  parallel_branch.false_label = parallel_branch.true_label;
  const auto parallel_conditional_raw = bir::lower_lir_to_raw_bir(parallel_conditional);
  expect(parallel_conditional_raw.has_value() &&
             parallel_conditional_raw.value().view().function(
                 parallel_conditional_raw.value().view().functions()[0]).value()
                 .successors(parallel_conditional_raw.value().view().function(
                     parallel_conditional_raw.value().view().functions()[0]).value().blocks()[0]).value().size() == 2,
         "parallel conditional successor occurrences must remain distinct Raw-BIR CFG edges");
  rejected([](lir::LirModule& candidate) {
    candidate.functions[0].blocks[2].id = lir::LirBlockId{1};
  }, "ambiguous conditional successor authority must reject transactionally");

  auto foreign = conditional_branch_module();
  foreign.functions.push_back(void_definition(
      "foreign_owner", {return_block(3, "foreign-display")}));
  std::get<lir::LirCondBr>(foreign.functions[0].blocks[0].terminator)
      .false_successor = lir::LirBlockId{3};
  expect(!bir::lower_lir_to_raw_bir(foreign).has_value() &&
             !bir::lower_lir_to_canonical_bir(foreign).has_value(),
         "foreign conditional successor authority must publish neither Raw nor Canonical BIR");

  auto presentation_incoherent = conditional_branch_module();
  auto& presentation_branch =
      std::get<lir::LirCondBr>(presentation_incoherent.functions[0].blocks[0].terminator);
  presentation_branch.cond_name = "%incoherent-display";
  presentation_branch.true_label = "incoherent-true-display";
  presentation_branch.false_label = "incoherent-false-display";
  expect(bir::lower_lir_to_raw_bir(presentation_incoherent).has_value() &&
             bir::lower_lir_to_canonical_bir(presentation_incoherent).has_value(),
         "conditional branch reception must ignore incoherent presentation shadows");
}

void test_selected_global_array_gep_ssa_index_receipt() {
  auto module = selected_global_array_gep_module();
  const auto index_link = module.link_names.intern("gep_index_global");
  lir::LirGlobal index_global;
  index_global.name = "gep_index_global";
  index_global.link_name_id = index_link;
  index_global.type = scalar_type(c4c::TB_LONGLONG);
  index_global.linkage_vis = "external ";
  index_global.qualifier = "global ";
  index_global.llvm_type = "i64";
  index_global.align_bytes = 8;
  index_global.is_extern_decl = true;
  module.globals.push_back(std::move(index_global));
  auto& instructions = module.functions[0].blocks[0].insts;
  instructions.insert(
      instructions.begin(),
      lir::LirLoadOp{
          lir::LirOperand::ssa("%misleading-index-load", lir::LirValueId{50}),
          lir::LirTypeRef::integer(64),
          lir::LirOperand::global("@misleading-index-global", index_link)});
  auto& gep = std::get<lir::LirGepOp>(instructions[1]);
  gep.indices[1] = lir::LirGepIndex::typed(
      lir::LirTypeRef::integer(64),
      lir::LirOperand::ssa("%wrong-index-display", lir::LirValueId{50}));

  auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value(),
         "a GEP SSA index should resolve through an earlier Load source identity");
  const auto module_view = raw.value().view();
  const auto function_id = module_view.functions()[0];
  const auto function = module_view.function(function_id).value();
  const auto ids = function.instructions(function.blocks()[0]).value();
  const auto load_result = function.instruction(ids[0]).value().results()[0];
  const auto indexed_gep = function.instruction(ids[1]).value();
  expect(indexed_gep.get_element_ptr() &&
             indexed_gep.operands()[1] == load_result &&
             function.source_value(bir::SourceValueId{function_id, 50})
                     .value() == load_result,
         "GEP must reuse the exact prior Load ValueId without display lookup or a parallel index registry");
}

bir::Type builder_i32_array_type(std::int64_t count) {
  bir::Type type{bir::TypeKind::Array, 0,
                 "[" + std::to_string(count) + " x i32]"};
  type.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Integer, 32, 0, {count}};
  return type;
}

void test_selected_global_array_gep_builder_contract() {
  bir::ModuleBuilder builder;
  const auto array_type = builder_i32_array_type(2);
  const auto global = builder.add_global_object(
      "builder_gep_array", array_type, 4, false, false, false, true);
  bir::FunctionSignature signature;
  signature.return_type = bir::Type{bir::TypeKind::Void};
  const auto function =
      builder.create_function(signature, "builder_gep", false);
  const auto foreign_function =
      builder.create_function(signature, "foreign_gep_owner", true);
  expect(global.has_value() && function.has_value() &&
             foreign_function.has_value(),
         "GEP builder fixture should create its typed owners");
  bir::InstId gep_id{};
  bir::ValueId result{};
  const auto edited = builder.with_function(
      function.value(), [&](bir::FunctionBuilder& function_builder) {
        const auto block = function_builder.create_block("entry");
        const auto index = function_builder.reserve_value(
            bir::Type{bir::TypeKind::I64});
        if (!block || !index)
          return bir::Result<void, bir::BuildError>::failure(
              bir::BuildError::StorageExhausted);
        auto defined = function_builder.define_int_constant(index.value(), 0);
        if (!defined) return defined;
        const auto invalid_global = function_builder.append(
            block.value(),
            bir::GetElementPtrSpec{
                bir::GlobalObjectId{999, 0}, array_type, true,
                {index.value()}, 70});
        expect(!invalid_global.has_value() &&
                   invalid_global.error() ==
                       bir::BuildError::InvalidGlobalObject,
               "GEP builder must reject a foreign global without staging state");
        const auto empty = function_builder.append(
            block.value(),
            bir::GetElementPtrSpec{global.value(), array_type, true, {}, 70});
        expect(!empty.has_value() &&
                   empty.error() == bir::BuildError::UnsupportedOpcode,
               "GEP builder must reject an empty index sequence");
        const auto foreign_index = function_builder.append(
            block.value(),
            bir::GetElementPtrSpec{
                global.value(), array_type, true,
                {bir::ValueId{foreign_function.value(),
                              bir::ValueKind::Ordinary, 0, 1}},
                70});
        expect(!foreign_index.has_value() &&
                   foreign_index.error() == bir::BuildError::ForeignOwner,
               "GEP builder must reject a cross-function index ValueId");
        const auto invalid_source = function_builder.append(
            block.value(),
            bir::GetElementPtrSpec{
                global.value(), array_type, true, {index.value()},
                std::numeric_limits<std::uint32_t>::max()});
        expect(!invalid_source.has_value() &&
                   invalid_source.error() ==
                       bir::BuildError::InvalidSourceValueId,
               "GEP builder must reject an invalid result source identity");
        auto appended = function_builder.append(
            block.value(),
            bir::GetElementPtrSpec{global.value(), array_type, true,
                                   {index.value()}, 70});
        if (!appended)
          return bir::Result<void, bir::BuildError>::failure(appended.error());
        gep_id = appended.value().instruction;
        result = appended.value().results[0];
        const auto duplicate = function_builder.append(
            block.value(),
            bir::GetElementPtrSpec{global.value(), array_type, true,
                                   {index.value()}, 70});
        expect(!duplicate.has_value() &&
                   duplicate.error() ==
                       bir::BuildError::DuplicateSourceValue,
               "GEP builder must reject duplicate result source identities atomically");
        return function_builder.set_terminator(block.value(),
                                               bir::ReturnTerm{});
      });
  expect(edited.has_value(), "one coherent GEP should remain after rejected appends");
  auto raw = std::move(builder).publish();
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "a structurally built GEP plus void return should publish Raw BIR");
  const auto view = raw.value().view().function(function.value()).value();
  expect(view.instruction(gep_id).value().get_element_ptr() &&
             view.source_value(bir::SourceValueId{function.value(), 70})
                     .value() == result,
         "structural GEP result must remain reachable through its native source identity");
  expect(bir::canonicalize(std::move(raw).value()).has_value(),
         "the structural GEP plus void return should publish Canonical BIR");

  bir::ModuleBuilder malformed_builder;
  const auto malformed_type = builder_i32_array_type(2);
  const auto malformed_global = malformed_builder.add_global_object(
      "unresolved_gep_array", malformed_type, 4, false, false, false, true);
  const auto malformed_function = malformed_builder.create_function(
      signature, "unresolved_gep", false);
  expect(malformed_builder
             .with_function(
                 malformed_function.value(),
                 [&](bir::FunctionBuilder& function_builder) {
                   const auto block = function_builder.create_block("entry");
                   const auto unresolved = function_builder.reserve_value(
                       bir::Type{bir::TypeKind::I64});
                   const auto appended = function_builder.append(
                       block.value(),
                       bir::GetElementPtrSpec{
                           malformed_global.value(), malformed_type, true,
                           {unresolved.value()}, 71});
                   if (!appended)
                     return bir::Result<void, bir::BuildError>::failure(
                         appended.error());
                   return function_builder.set_terminator(block.value(),
                                                          bir::ReturnTerm{});
                 })
             .has_value(),
         "the public builder should stage GEP use before an integer index reservation is defined");
  const auto malformed = std::move(malformed_builder).publish();
  expect(!malformed.has_value() &&
             malformed.error().reason ==
                 bir::PublishError::VerificationFailed,
         "FoundationVerifier must prevent an unresolved GEP index graph from publishing Raw BIR");
}

void test_label_address_gep_base_builder_contract() {
  bir::ModuleBuilder builder;
  const auto array_type = builder_i32_array_type(2);
  bir::FunctionSignature signature;
  signature.return_type = bir::Type{bir::TypeKind::Void};
  const auto function = builder.create_function(signature, "label_gep", false);
  const auto foreign_function =
      builder.create_function(signature, "foreign_label_gep", false);
  expect(function.has_value() && foreign_function.has_value(),
         "label-address GEP fixture should create its typed owners");

  bir::ValueId foreign_label{};
  const auto foreign_edited = builder.with_function(
      foreign_function.value(), [&](bir::FunctionBuilder& function_builder) {
        const auto target = function_builder.create_block("foreign_target");
        const auto label = function_builder.reserve_value(
            bir::Type{bir::TypeKind::Pointer});
        if (!target || !label)
          return bir::Result<void, bir::BuildError>::failure(
              bir::BuildError::StorageExhausted);
        const auto defined = function_builder.define_label_address_constant(
            label.value(), target.value());
        if (!defined) return defined;
        foreign_label = label.value();
        return function_builder.set_terminator(target.value(), bir::ReturnTerm{});
      });
  expect(foreign_edited.has_value(),
         "foreign label-address fixture should define its constant");

  bir::InstId gep_id{};
  bir::ValueId label_value{};
  const auto edited = builder.with_function(
      function.value(), [&](bir::FunctionBuilder& function_builder) {
        const auto target = function_builder.create_block("target");
        const auto entry = function_builder.create_block("entry");
        const auto label = function_builder.reserve_value(
            bir::Type{bir::TypeKind::Pointer});
        const auto index = function_builder.reserve_value(
            bir::Type{bir::TypeKind::I64});
        const auto non_label = function_builder.reserve_value(
            bir::Type{bir::TypeKind::I64});
        const auto missing_label = function_builder.reserve_value(
            bir::Type{bir::TypeKind::Pointer});
        if (!target || !entry || !label || !index || !non_label || !missing_label)
          return bir::Result<void, bir::BuildError>::failure(
              bir::BuildError::StorageExhausted);
        auto defined = function_builder.define_label_address_constant(
            label.value(), target.value());
        if (!defined) return defined;
        label_value = label.value();
        defined = function_builder.define_int_constant(index.value(), 0);
        if (!defined) return defined;
        defined = function_builder.define_int_constant(non_label.value(), 0);
        if (!defined) return defined;

        const bir::GetElementPtrBase structured_base{
            bir::LabelAddressGepBase{label.value()}};
        const auto* structured_label = std::get_if<bir::LabelAddressGepBase>(
            &structured_base.authority);
        expect(structured_label && structured_label->value == label.value(),
               "GEP schema must retain the exact label-address identity as its dedicated base alternative");

        const auto invalid = function_builder.append(
            entry.value(), bir::GetElementPtrSpec{
                               bir::LabelAddressGepBase{bir::ValueId{
                                   function.value(), bir::ValueKind::Ordinary,
                                   999, 0}},
                               array_type, true, {index.value()}, 80});
        expect(!invalid.has_value() &&
                   invalid.error() == bir::BuildError::InvalidValue,
               "GEP builder must reject a missing label-address identity before staging state");
        const auto foreign = function_builder.append(
            entry.value(), bir::GetElementPtrSpec{
                               bir::LabelAddressGepBase{foreign_label}, array_type,
                               true, {index.value()}, 80});
        expect(!foreign.has_value() &&
                   foreign.error() == bir::BuildError::ForeignOwner,
               "GEP builder must reject a foreign label-address identity before staging state");
        const auto non_label_value = function_builder.append(
            entry.value(), bir::GetElementPtrSpec{
                               bir::LabelAddressGepBase{non_label.value()}, array_type,
                               true, {index.value()}, 80});
        expect(!non_label_value.has_value() &&
                   non_label_value.error() == bir::BuildError::DefinitionTypeMismatch,
               "GEP builder must reject a non-label constant identity before staging state");
        const auto missing = function_builder.append(
            entry.value(), bir::GetElementPtrSpec{
                               bir::LabelAddressGepBase{missing_label.value()},
                               array_type, true, {index.value()}, 80});
        expect(!missing.has_value() &&
                   missing.error() == bir::BuildError::DefinitionTypeMismatch,
               "GEP builder must reject an undefined label-address identity before staging state");
        defined = function_builder.define_label_address_constant(
            missing_label.value(), target.value());
        if (!defined) return defined;
        const auto accepted = function_builder.append(
            entry.value(), bir::GetElementPtrSpec{
                               bir::LabelAddressGepBase{label.value()}, array_type,
                               true, {index.value()}, 80});
        expect(accepted.has_value(),
               "GEP builder must retain the exact current-function label-address identity");
        gep_id = accepted.value().instruction;
        const auto entry_terminated =
            function_builder.set_terminator(entry.value(), bir::ReturnTerm{});
        if (!entry_terminated) return entry_terminated;
        return function_builder.set_terminator(target.value(), bir::ReturnTerm{});
      });
  expect(edited.has_value(),
         "label-address GEP builder fixture should retain only the accepted structured base");
  auto raw = std::move(builder).publish();
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "a verified label-address GEP base should publish Raw BIR");
  const auto function_view = raw.value().view().function(function.value()).value();
  const auto* gep = function_view.instruction(gep_id).value().get_element_ptr();
  const auto* published_label = gep
      ? std::get_if<bir::LabelAddressGepBase>(&gep->base.authority)
      : nullptr;
  expect(published_label && published_label->value == label_value,
         "Raw-BIR verification must preserve the typed label-address GEP base without a global projection");
}

void test_selected_global_array_gep_rejections() {
  const auto rejected = [](lir::LirModule candidate,
                           bir::ImportErrorCode expected,
                           const std::string& message) {
    const auto raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw.has_value() && raw.error().code == expected,
           message + " (Raw rollback)");
    const auto canonical = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical.has_value() && canonical.error().code == expected,
           message + " (Canonical rollback)");
  };
  const auto first_gep = [](lir::LirModule& module) -> lir::LirGepOp& {
    return std::get<lir::LirGepOp>(module.functions[0].blocks[0].insts[0]);
  };

  auto raw = selected_global_array_gep_module();
  auto& raw_gep = first_gep(raw);
  raw_gep.result = lir::LirOperand("%raw");
  raw_gep.ptr = lir::LirOperand("@raw");
  raw_gep.indices = {lir::LirGepIndex::raw("i64 0")};
  rejected(std::move(raw), bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "an ordinary raw GEP producer must remain unsupported");

  auto mixed = selected_global_array_gep_module();
  first_gep(mixed).indices[1] = lir::LirGepIndex::raw("i64 1");
  rejected(std::move(mixed),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a mixed raw/authoritative GEP index sequence must reject");

  auto empty = selected_global_array_gep_module();
  first_gep(empty).indices.clear();
  rejected(std::move(empty),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "an empty authoritative GEP index sequence must reject");

  auto local_base = selected_global_array_gep_module();
  first_gep(local_base).ptr =
      lir::LirOperand::ssa("%local", lir::LirValueId{9});
  rejected(std::move(local_base),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "an SSA/local GEP base must remain outside selected-global receipt");

  auto invalid_result = selected_global_array_gep_module();
  first_gep(invalid_result).result =
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  rejected(std::move(invalid_result),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "an invalid native GEP result identity must reject transactionally");

  auto wrong_result_authority = selected_global_array_gep_module();
  first_gep(wrong_result_authority).result = lir::LirOperand::global(
      "%looks-like-result",
      wrong_result_authority.globals[0].link_name_id);
  rejected(std::move(wrong_result_authority),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a non-result GEP authority alternative must not use display fallback");

  auto wrong_base_authority = selected_global_array_gep_module();
  first_gep(wrong_base_authority).ptr =
      lir::LirOperand::integer("@looks-like-base", 0);
  rejected(std::move(wrong_base_authority),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a non-global GEP base authority alternative must remain unsupported");

  auto mismatched_array = selected_global_array_gep_module();
  first_gep(mismatched_array).element_type = lir::LirTypeRef("[3 x i32]");
  rejected(std::move(mismatched_array),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a distinct well-formed array spelling must reject parity instead of becoming BIR type authority");

  auto noninteger_index = selected_global_array_gep_module();
  first_gep(noninteger_index).indices[0] = lir::LirGepIndex::typed(
      lir::LirTypeRef("double"), lir::LirOperand::integer("0", 0));
  rejected(std::move(noninteger_index),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a noninteger typed GEP index must reject");

  auto wrong_index_authority = selected_global_array_gep_module();
  first_gep(wrong_index_authority).indices[0] = lir::LirGepIndex::typed(
      lir::LirTypeRef::integer(64),
      lir::LirOperand::global("0",
                              wrong_index_authority.globals[0].link_name_id));
  rejected(std::move(wrong_index_authority),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "an unsupported GEP index authority alternative must not use display fallback");

  auto out_of_range = selected_global_array_gep_module();
  first_gep(out_of_range).indices[0] = lir::LirGepIndex::typed(
      lir::LirTypeRef::integer(8),
      lir::LirOperand::integer("displayed-zero", 256));
  rejected(std::move(out_of_range),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "an out-of-range native GEP immediate must reject without truncation");

  auto unknown_index = selected_global_array_gep_module();
  first_gep(unknown_index).indices[1] = lir::LirGepIndex::typed(
      lir::LirTypeRef::integer(64),
      lir::LirOperand::ssa("%looks-known", lir::LirValueId{999}));
  rejected(std::move(unknown_index),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "an unknown current-function GEP index identity must reject");

  auto mismatched_ssa = selected_global_array_gep_module();
  const auto index_link = mismatched_ssa.link_names.intern("mismatch_index");
  lir::LirGlobal index_global;
  index_global.name = "mismatch_index";
  index_global.link_name_id = index_link;
  index_global.type = scalar_type(c4c::TB_INT);
  index_global.linkage_vis = "external ";
  index_global.qualifier = "global ";
  index_global.llvm_type = "i32";
  index_global.align_bytes = 4;
  index_global.is_extern_decl = true;
  mismatched_ssa.globals.push_back(std::move(index_global));
  mismatched_ssa.functions[0].blocks[0].insts.insert(
      mismatched_ssa.functions[0].blocks[0].insts.begin(),
      lir::LirLoadOp{
          lir::LirOperand::ssa("%index", lir::LirValueId{50}),
          lir::LirTypeRef::integer(32),
          lir::LirOperand::global("@index", index_link)});
  auto& shifted_gep = std::get<lir::LirGepOp>(
      mismatched_ssa.functions[0].blocks[0].insts[1]);
  shifted_gep.indices[1] = lir::LirGepIndex::typed(
      lir::LirTypeRef::integer(64),
      lir::LirOperand::ssa("%index", lir::LirValueId{50}));
  rejected(std::move(mismatched_ssa),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "an SSA GEP index type mismatch must reject exact source identity reuse");

  auto duplicate_result = selected_global_array_gep_module();
  std::get<lir::LirGepOp>(
      duplicate_result.functions[0].blocks[0].insts[1]).result =
      lir::LirOperand::ssa("%same-id-different-display", lir::LirValueId{61});
  rejected(std::move(duplicate_result),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "duplicate GEP result identities must reject transactionally");

  auto unresolved = selected_global_array_gep_module();
  first_gep(unresolved).ptr = lir::LirOperand::global(
      "@looks-valid", static_cast<c4c::LinkNameId>(999));
  rejected(std::move(unresolved),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "an unresolved native GEP LinkNameId must reject transactionally");

  auto ambiguous = selected_global_array_gep_module();
  ambiguous.globals[1].link_name_id = ambiguous.globals[0].link_name_id;
  ambiguous.globals[1].name = ambiguous.globals[0].name;
  rejected(std::move(ambiguous), bir::ImportErrorCode::UnsupportedGlobals,
           "ambiguous GEP global ownership must reject before function publication");

  auto later_raw = selected_global_array_gep_module();
  later_raw.functions[0].blocks[0].insts.push_back(lir::LirLoadOp{
      lir::LirOperand::ssa("%later", lir::LirValueId{90}),
      lir::LirTypeRef::integer(32), lir::LirOperand("@raw")});
  rejected(std::move(later_raw),
           bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           "a later raw instruction must roll back all earlier admitted GEP state");

  auto invalid_return = selected_global_array_gep_module();
  invalid_return.functions[0].blocks[0].terminator = lir::LirRet{
      lir::LirOperand::integer("misleading", 1),
      lir::LirTypeRef::integer(32)};
  rejected(std::move(invalid_return), bir::ImportErrorCode::InvalidVoidReturn,
           "admitted GEPs must advance diagnosis to the unsupported return boundary");
}

lir::LirModule scalar_integer_return_module(lir::LirOperand value,
                                             unsigned bit_width = 32) {
  lir::LirModule module;
  auto block = return_block(0, "entry");
  block.terminator = lir::LirRet{
      std::move(value), lir::LirTypeRef::integer(bit_width)};

  lir::LirFunction function;
  function.name = "scalar_integer_return";
  switch (bit_width) {
    case 8: function.return_type = scalar_type(c4c::TB_SCHAR); break;
    case 16: function.return_type = scalar_type(c4c::TB_SHORT); break;
    case 32: function.return_type = scalar_type(c4c::TB_INT); break;
    case 64: function.return_type = scalar_type(c4c::TB_LONGLONG); break;
    default: function.return_type = scalar_type(c4c::TB_INT); break;
  }
  function.return_type.inner_rank = -1;
  function.signature_return_type_ref =
      lir::LirTypeRef::integer(bit_width);
  function.blocks.push_back(std::move(block));
  function.entry = function.blocks.front().id;
  module.functions.push_back(std::move(function));
  return module;
}

void test_scalar_integer_return_receipt() {
  auto module = scalar_integer_return_module(
      lir::LirOperand::integer("%misleading-immediate-display", 37));
  module.functions.push_back(
      void_definition("void_return_retained", {return_block(0, "entry")}));

  const auto inspect = [](const auto& module_view,
                          const std::string& layer) {
    const auto functions = module_view.functions();
    expect(functions.size() == 2,
           layer + " must retain integer and void return functions");
    const auto integer_function = module_view.function(functions[0]).value();
    expect(integer_function.signature().return_type ==
               bir::Type{bir::TypeKind::I32},
           layer + " must retain the exact imported integer signature");
    const auto integer_terminator =
        integer_function.terminator(integer_function.blocks()[0]).value();
    const auto* integer_return =
        std::get_if<bir::ReturnTerm>(&integer_terminator);
    expect(integer_return && integer_return->value,
           layer + " must publish one scalar ReturnTerm value");
    const auto returned = integer_function.value(*integer_return->value).value();
    const auto* constant = std::get_if<bir::ConstantDef>(&returned.definition);
    expect(returned.type == bir::Type{bir::TypeKind::I32} &&
               !returned.source_id && constant,
           layer + " immediate return must be a source-less exact ordinary constant");
    const auto definition = module_view.constant(constant->constant).value();
    const auto* integer =
        std::get_if<bir::IntegerConstant>(&definition.payload);
    expect(definition.type == bir::Type{bir::TypeKind::I32} && integer &&
               integer->value == 37,
           layer + " must ignore return display and retain native immediate authority");

    const auto void_function = module_view.function(functions[1]).value();
    const auto void_terminator =
        void_function.terminator(void_function.blocks()[0]).value();
    const auto* void_return = std::get_if<bir::ReturnTerm>(&void_terminator);
    expect(void_function.signature().return_type.kind == bir::TypeKind::Void &&
               void_return && !void_return->value,
           layer + " must preserve existing valueless void ReturnTerm receipt");
  };

  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "authoritative immediate and void returns should publish verified Raw BIR");
  inspect(raw.value().view(), "Raw BIR");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "authoritative immediate and void returns should publish Canonical BIR");
  inspect(canonical.value().view(), "Canonical BIR");
}

void test_scalar_integer_ssa_return_receipt() {
  auto module = direct_global_integer_load_module();
  auto& function = module.functions[0];
  function.return_type = scalar_type(c4c::TB_INT);
  function.return_type.inner_rank = -1;
  function.signature_return_type_ref = lir::LirTypeRef::integer(32);
  function.blocks[0].terminator = lir::LirRet{
      lir::LirOperand::ssa("%display-points-at-wrong-value",
                           lir::LirValueId{31}),
      lir::LirTypeRef::integer(32)};

  const auto inspect = [](const auto& module_view,
                          const std::string& layer) {
    const auto function_id = module_view.functions()[0];
    const auto function = module_view.function(function_id).value();
    const auto block = function.blocks()[0];
    const auto instructions = function.instructions(block).value();
    const auto load_result =
        function.instruction(instructions[0]).value().results()[0];
    const auto terminator = function.terminator(block).value();
    const auto* returned = std::get_if<bir::ReturnTerm>(&terminator);
    expect(function.signature().return_type ==
               bir::Type{bir::TypeKind::I32} &&
               returned && returned->value && *returned->value == load_result &&
               function.source_value(
                           bir::SourceValueId{function_id, 31})
                       .value() == load_result,
           layer + " SSA return must reuse the exact prior Load result and native source identity");
  };

  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "a current-function authoritative Load result should be returnable");
  inspect(raw.value().view(), "Raw BIR");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "the same SSA ReturnTerm should publish Canonical BIR");
  inspect(canonical.value().view(), "Canonical BIR");
}

void test_scalar_integer_return_builder_contract() {
  bir::ModuleBuilder builder;
  bir::FunctionSignature signature;
  signature.return_type = bir::Type{bir::TypeKind::I32};
  const auto function =
      builder.create_function(signature, "builder_integer_return", false);
  const auto foreign =
      builder.create_function(signature, "foreign_integer_return", false);
  expect(function.has_value() && foreign.has_value(),
         "return builder fixture should create both typed owners");

  bir::ValueId returned{};
  bir::ValueId foreign_value{};
  const auto foreign_edit = builder.with_function(
      foreign.value(), [&](bir::FunctionBuilder& function_builder) {
        const auto block = function_builder.create_block("entry");
        if (!block)
          return bir::Result<void, bir::BuildError>::failure(block.error());
        const auto value =
            function_builder.reserve_value(bir::Type{bir::TypeKind::I32});
        if (!value)
          return bir::Result<void, bir::BuildError>::failure(value.error());
        foreign_value = value.value();
        const auto defined = function_builder.define_int_constant(value.value(), 1);
        if (!defined) return defined;
        return function_builder.set_terminator(
            block.value(), bir::ReturnTerm{value.value()});
      });
  expect(foreign_edit.has_value(),
         "foreign return owner fixture should be internally valid");

  const auto edited = builder.with_function(
      function.value(), [&](bir::FunctionBuilder& function_builder) {
        const auto block = function_builder.create_block("entry");
        if (!block)
          return bir::Result<void, bir::BuildError>::failure(block.error());
        expect(function_builder
                       .set_terminator(block.value(), bir::ReturnTerm{})
                       .error() == bir::BuildError::InvalidReturn,
               "nonvoid builder return must reject a missing value");
        expect(function_builder
                       .set_terminator(block.value(),
                                       bir::ReturnTerm{foreign_value})
                       .error() == bir::BuildError::ForeignOwner,
               "builder return must reject a cross-function ValueId");
        const auto wrong =
            function_builder.reserve_value(bir::Type{bir::TypeKind::I64});
        if (!wrong)
          return bir::Result<void, bir::BuildError>::failure(wrong.error());
        auto defined_wrong =
            function_builder.define_int_constant(wrong.value(), 2);
        if (!defined_wrong) return defined_wrong;
        expect(function_builder
                       .set_terminator(block.value(),
                                       bir::ReturnTerm{wrong.value()})
                       .error() == bir::BuildError::InvalidReturn,
               "builder return must reject a local value of the wrong type");
        const auto value =
            function_builder.reserve_value(bir::Type{bir::TypeKind::I32});
        if (!value)
          return bir::Result<void, bir::BuildError>::failure(value.error());
        returned = value.value();
        auto defined = function_builder.define_int_constant(returned, 3);
        if (!defined) return defined;
        return function_builder.set_terminator(
            block.value(), bir::ReturnTerm{returned});
      });
  expect(edited.has_value(),
         "valid return should remain set after rejected public-builder guards");
  const auto raw = std::move(builder).publish();
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "builder and FoundationVerifier must accept the exact typed ReturnTerm");
  const auto view = raw.value().view().function(function.value()).value();
  const auto terminator = view.terminator(view.blocks()[0]).value();
  const auto* return_term = std::get_if<bir::ReturnTerm>(&terminator);
  expect(return_term && return_term->value == returned,
         "published builder return must retain its exact local ValueId");
}

void test_scalar_integer_return_rejections() {
  const auto rejected = [](lir::LirModule candidate,
                           bir::ImportErrorCode expected,
                           const std::string& message) {
    candidate.functions.insert(candidate.functions.begin(),
                               void_definition("accepted_before_bad_return",
                                               {return_block(0, "entry")}));
    const auto raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw.has_value() && raw.error().code == expected,
           message + " (Raw rollback)");
    const auto canonical = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical.has_value() && canonical.error().code == expected,
           message + " (Canonical rollback)");
  };

  auto raw_value = scalar_integer_return_module(lir::LirOperand::raw("37"));
  rejected(std::move(raw_value), bir::ImportErrorCode::UnsupportedTerminator,
           "raw return text must not become scalar authority");

  auto monostate = scalar_integer_return_module(lir::LirOperand("%unknown"));
  rejected(std::move(monostate), bir::ImportErrorCode::UnsupportedTerminator,
           "classified return display without native authority must reject");

  auto wrong_authority = scalar_integer_return_module(
      lir::LirOperand::global("37", static_cast<c4c::LinkNameId>(1)));
  rejected(std::move(wrong_authority),
           bir::ImportErrorCode::UnsupportedTerminator,
           "LinkNameId return authority must remain unsupported");

  auto missing = scalar_integer_return_module(
      lir::LirOperand::integer("ignored", 1));
  std::get<lir::LirRet>(missing.functions[0].blocks[0].terminator).value_str.reset();
  rejected(std::move(missing), bir::ImportErrorCode::UnsupportedTerminator,
           "integer signatures must reject missing return values");

  auto mismatched_type = scalar_integer_return_module(
      lir::LirOperand::integer("ignored", 1));
  std::get<lir::LirRet>(mismatched_type.functions[0].blocks[0].terminator)
      .type_str = lir::LirTypeRef::integer(64);
  rejected(std::move(mismatched_type),
           bir::ImportErrorCode::UnsupportedTerminator,
           "return type must exactly match the imported function signature");

  auto raw_type = scalar_integer_return_module(
      lir::LirOperand::integer("ignored", 1));
  std::get<lir::LirRet>(raw_type.functions[0].blocks[0].terminator).type_str =
      lir::LirTypeRef("i32", lir::LirTypeKind::RawText);
  rejected(std::move(raw_type), bir::ImportErrorCode::UnsupportedTerminator,
           "raw return type text must not satisfy structured signature agreement");

  auto out_of_range = scalar_integer_return_module(
      lir::LirOperand::integer("looks-small", 256), 8);
  rejected(std::move(out_of_range),
           bir::ImportErrorCode::UnsupportedTerminator,
           "out-of-range return immediates must reject before constant creation");

  auto unknown_ssa = scalar_integer_return_module(
      lir::LirOperand::ssa("%looks-known", lir::LirValueId{999}));
  rejected(std::move(unknown_ssa),
           bir::ImportErrorCode::UnsupportedTerminator,
           "unknown current-function SSA returns must reject");

  auto invalid_ssa = scalar_integer_return_module(
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid()));
  rejected(std::move(invalid_ssa),
           bir::ImportErrorCode::UnsupportedTerminator,
           "invalid native SSA return identity must reject");

  auto cross_function_ssa = direct_global_integer_load_module();
  auto cross_return = scalar_integer_return_module(
      lir::LirOperand::ssa("%looks-shared", lir::LirValueId{31}));
  cross_function_ssa.functions.push_back(
      std::move(cross_return.functions.front()));
  rejected(std::move(cross_function_ssa),
           bir::ImportErrorCode::UnsupportedTerminator,
           "an SSA identity defined only in another function must not cross registries");

  auto mismatched_ssa = direct_global_integer_load_module();
  auto& mismatch_function = mismatched_ssa.functions[0];
  mismatch_function.return_type = scalar_type(c4c::TB_INT);
  mismatch_function.return_type.inner_rank = -1;
  mismatch_function.signature_return_type_ref = lir::LirTypeRef::integer(32);
  mismatch_function.blocks[0].terminator = lir::LirRet{
      lir::LirOperand::ssa("%looks-i32", lir::LirValueId{32}),
      lir::LirTypeRef::integer(32)};
  rejected(std::move(mismatched_ssa),
           bir::ImportErrorCode::UnsupportedTerminator,
           "SSA return identity must carry the exact signature type");

  auto noninteger = scalar_integer_return_module(
      lir::LirOperand::integer("ignored", 1));
  auto& noninteger_function = noninteger.functions[0];
  noninteger_function.return_type = scalar_type(c4c::TB_DOUBLE);
  noninteger_function.signature_return_type_ref = lir::LirTypeRef("double");
  std::get<lir::LirRet>(noninteger_function.blocks[0].terminator).type_str =
      lir::LirTypeRef("double");
  rejected(std::move(noninteger),
           bir::ImportErrorCode::UnsupportedTerminator,
           "noninteger scalar returns remain fail-closed");

  auto extra_void = scalar_integer_return_module(
      lir::LirOperand::integer("ignored", 1));
  auto& void_function = extra_void.functions[0];
  void_function.return_type = scalar_type(c4c::TB_VOID);
  void_function.signature_return_type_ref = lir::LirTypeRef("void");
  rejected(std::move(extra_void), bir::ImportErrorCode::InvalidVoidReturn,
           "void signatures must retain exact valueless return receipt");
}

void test_string_pool_receipt_and_views() {
  std::string embedded("A\0B", 3);
  embedded += "\\22\\5C";
  std::string unusual;
  unusual.push_back(static_cast<char>(0xff));
  unusual.push_back('\n');
  unusual += "opaque\\00tail";

  lir::LirModule module;
  module.string_pool = {
      lir::LirStringConst{"@.str0", embedded, 8},
      lir::LirStringConst{"@.str1", unusual, 3},
      lir::LirStringConst{"@.str2", "", 1},
      lir::LirStringConst{"@.str3", "[2 x i32] [i32 1, i32 0]", -1},
  };
  module.str_pool_map.emplace(std::string("source\0one", 10), "@.str0");
  module.str_pool_map.emplace("source-two", "@.str1");
  module.str_pool_map.emplace("", "@.str2");
  module.str_pool_idx = 4;

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "ordered string rows with coherent cache evidence should publish");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "published string data should be reachable by FoundationVerifier");
  const auto view = imported.value().view();
  const auto ids = view.string_data();
  expect(ids.size() == 4 && ids[0].slot == 0 && ids[1].slot == 1 &&
             ids[2].slot == 2 && ids[3].slot == 3,
         "string data IDs must preserve vector authority and order");
  const auto first = view.string_data(ids[0]).value();
  const auto second = view.string_data(ids[1]).value();
  const auto third = view.string_data(ids[2]).value();
  const auto wide = view.string_data(ids[3]).value();
  expect(first.pool_name == "@.str0" && first.raw_bytes == embedded &&
             first.byte_length == 8 && second.pool_name == "@.str1" &&
             second.raw_bytes == unusual && second.byte_length == 3 &&
             third.pool_name == "@.str2" && third.raw_bytes.empty() &&
             third.byte_length == 1 && wide.pool_name == "@.str3" &&
             wide.raw_bytes == "[2 x i32] [i32 1, i32 0]" &&
             wide.byte_length == -1,
         "immutable string views must preserve names, opaque bytes, and lengths exactly");
  expect(view.string_data("@.str1").value() == ids[1],
         "name lookup must resolve to the ordered typed identity");

  lir::LirModule empty;
  auto empty_import = bir::lower_lir_to_raw_bir(empty);
  expect(empty_import.has_value() &&
             empty_import.value().view().string_data().empty(),
         "an empty vector/cache with zero counter should publish an empty view");
}

void test_string_pool_rejections_and_transactionality() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.string_pool = {
        lir::LirStringConst{"@.str0", "opaque\\00", 4},
        lir::LirStringConst{"@.str1", "payload", 8},
    };
    module.str_pool_map.emplace("source-zero", "@.str0");
    module.str_pool_map.emplace("source-one", "@.str1");
    module.str_pool_idx = 2;
    module.functions.push_back(void_declaration("valid_before_bad_pool"));
    return module;
  };
  const auto rejected = [&](auto mutate, const std::string& message) {
    auto module = valid_module();
    mutate(module);
    auto imported = bir::lower_lir_to_raw_bir(module);
    expect(!imported.has_value() &&
               imported.error().code ==
                   bir::ImportErrorCode::UnsupportedStringPool,
           message);
  };

  rejected(
      [](lir::LirModule& module) { module.string_pool[0].pool_name.clear(); },
      "empty string names must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.string_pool[1].pool_name = module.string_pool[0].pool_name;
      },
      "duplicate ordered string names must reject the whole module");
  rejected(
      [](lir::LirModule& module) { module.string_pool[0].byte_length = -2; },
      "lengths below the evidenced sentinel must reject without parsing payloads");
  rejected(
      [](lir::LirModule& module) { module.string_pool[0].byte_length = -1; },
      "cache entries for cacheless sentinel rows must reject the whole module");
  rejected(
      [](lir::LirModule& module) { module.str_pool_map.erase("source-one"); },
      "missing cache entries must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.str_pool_map.emplace("extra-source", "@.str2");
      },
      "extra cache entries must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.str_pool_map["source-one"] = "@.str0";
      },
      "conflicting cache values must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.str_pool_map["source-one"] = "@.missing";
      },
      "cache values missing from ordered rows must reject the whole module");
  rejected(
      [](lir::LirModule& module) { module.str_pool_idx = 1; },
      "a stale string counter must reject the whole module");

  lir::LirModule malformed_empty;
  malformed_empty.str_pool_map.emplace("ghost", "@.str0");
  expect(!bir::lower_lir_to_raw_bir(malformed_empty).has_value(),
         "an empty pool cannot retain cache state");
  malformed_empty.str_pool_map.clear();
  malformed_empty.str_pool_idx = 1;
  expect(!bir::lower_lir_to_raw_bir(malformed_empty).has_value(),
         "an empty pool cannot retain a stale counter");

  bir::ModuleBuilder builder;
  expect(builder.add_string_data("", "payload", 1).error() ==
             bir::BuildError::EmptyStringDataName,
         "builder must reject an empty string-data name");
  expect(builder.add_string_data("@.str0", "opaque", 6).has_value(),
         "builder should stage a valid ordered string row");
  expect(builder.add_string_data("@.str0", "other", 5).error() ==
             bir::BuildError::DuplicateStringDataName,
         "builder must reject duplicate staged names transactionally");
  expect(builder.add_string_data("@.str1", "malformed opaque", -2).has_value(),
         "builder should retain malformed staged length for verifier diagnosis");
  auto published = std::move(builder).publish();
  expect(!published.has_value() &&
             published.error().reason == bir::PublishError::VerificationFailed &&
             !published.error().verification.errors.empty(),
         "FoundationVerifier must reject malformed staged string storage");
}

void test_external_declaration_receipt_and_views() {
  lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  const auto void_link = module.link_names.intern("linked_void");
  const auto struct_link = module.link_names.intern("linked_struct");
  const auto payload_name = module.struct_names.intern("%struct.Payload");
  module.record_struct_decl(lir::LirStructDecl{payload_name});

  const auto payload_type =
      lir::LirTypeRef::struct_type("%struct.Payload", payload_name);
  const lir::LirExternDecl fallback_sign{
      "fallback_sign", "i8", lir::LirTypeRef::integer(8),
      lir::LirExtAttr::SignExt, c4c::kInvalidLinkName};
  const lir::LirExternDecl linked_void{
      "linked_void", "void", lir::LirTypeRef("void"),
      lir::LirExtAttr::None, void_link};
  const lir::LirExternDecl linked_struct{
      "linked_struct", "%struct.Payload", payload_type,
      lir::LirExtAttr::None, struct_link};
  const lir::LirExternDecl fallback_zero{
      "fallback_zero", "i32", lir::LirTypeRef::integer(32),
      lir::LirExtAttr::ZeroExt, c4c::kInvalidLinkName};
  module.extern_decls = {fallback_sign, linked_void, linked_struct,
                         fallback_zero};
  module.extern_decl_link_name_map.emplace(
      void_link, lir::LirModule::ExternDeclInfo{
                     linked_void.name, linked_void.return_type_str,
                     linked_void.return_type, linked_void.return_ext_attr,
                     linked_void.link_name_id});
  module.extern_decl_link_name_map.emplace(
      struct_link, lir::LirModule::ExternDeclInfo{
                       linked_struct.name, linked_struct.return_type_str,
                       linked_struct.return_type,
                       linked_struct.return_ext_attr,
                       linked_struct.link_name_id});
  module.extern_decl_name_map.emplace(
      fallback_sign.name,
      lir::LirModule::ExternDeclInfo{
          fallback_sign.name, fallback_sign.return_type_str,
          fallback_sign.return_type, fallback_sign.return_ext_attr,
          fallback_sign.link_name_id});
  module.extern_decl_name_map.emplace(
      fallback_zero.name,
      lir::LirModule::ExternDeclInfo{
          fallback_zero.name, fallback_zero.return_type_str,
          fallback_zero.return_type, fallback_zero.return_ext_attr,
          fallback_zero.link_name_id});

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "ordered link-backed and fallback external declarations should publish");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "published external state should be reachable by FoundationVerifier");
  const auto view = imported.value().view();
  const auto ids = view.external_declarations();
  expect(ids.size() == 4 && ids[0].slot == 0 && ids[1].slot == 1 &&
             ids[2].slot == 2 && ids[3].slot == 3,
         "external declaration IDs must preserve vector authority and order");
  const auto raw_sign = view.external_declaration(ids[0]).value();
  const auto raw_void = view.external_declaration(ids[1]).value();
  const auto raw_struct = view.external_declaration(ids[2]).value();
  const auto raw_zero = view.external_declaration(ids[3]).value();
  expect(raw_sign.source_name == "fallback_sign" &&
             raw_sign.return_type == bir::Type{bir::TypeKind::I8} &&
             raw_sign.return_extension == bir::ReturnExtension::SignExt &&
             std::get<bir::FallbackExternalName>(raw_sign.identity).name ==
                 "fallback_sign" &&
             raw_void.return_type.kind == bir::TypeKind::Void &&
             raw_void.return_extension == bir::ReturnExtension::None &&
             std::holds_alternative<bir::LinkNameId>(raw_void.identity) &&
             raw_struct.return_type.kind == bir::TypeKind::Struct &&
             raw_struct.return_type.struct_name_id == payload_name &&
             std::holds_alternative<bir::LinkNameId>(raw_struct.identity) &&
             raw_zero.return_type == bir::Type{bir::TypeKind::I32} &&
             raw_zero.return_extension == bir::ReturnExtension::ZeroExt,
         "immutable extern views must preserve names, structured types, identities, and attrs");
  expect(view.external_declaration("fallback_zero").value() == ids[3] &&
             view.external_declaration(
                     std::get<bir::LinkNameId>(raw_struct.identity))
                     .value() == ids[2],
         "typed name and link lookups must resolve exact external identities");
}

void test_external_declaration_rejections_and_transactionality() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto linked = module.link_names.intern("linked_ext");
    const lir::LirExternDecl link_row{
        "linked_ext", "i32", lir::LirTypeRef::integer(32),
        lir::LirExtAttr::None, linked};
    const lir::LirExternDecl fallback_row{
        "fallback_ext", "void", lir::LirTypeRef("void"),
        lir::LirExtAttr::None, c4c::kInvalidLinkName};
    module.extern_decls = {link_row, fallback_row};
    module.extern_decl_link_name_map.emplace(
        linked, lir::LirModule::ExternDeclInfo{
                    link_row.name, link_row.return_type_str,
                    link_row.return_type, link_row.return_ext_attr,
                    link_row.link_name_id});
    module.extern_decl_name_map.emplace(
        fallback_row.name,
        lir::LirModule::ExternDeclInfo{
            fallback_row.name, fallback_row.return_type_str,
            fallback_row.return_type, fallback_row.return_ext_attr,
            fallback_row.link_name_id});
    module.functions.push_back(void_declaration("valid_before_bad_extern"));
    return module;
  };
  const auto rejected = [&](auto mutate, const std::string& message) {
    auto module = valid_module();
    mutate(module);
    auto imported = bir::lower_lir_to_raw_bir(module);
    expect(!imported.has_value() &&
               imported.error().code ==
                   bir::ImportErrorCode::UnsupportedExternDeclarations,
           message);
  };

  rejected(
      [](lir::LirModule& module) {
        module.extern_decl_name_map.erase("fallback_ext");
      },
      "missing external parity evidence must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        const auto extra = module.link_names.intern("extra_ext");
        module.extern_decl_link_name_map.emplace(
            extra, lir::LirModule::ExternDeclInfo{
                       "extra_ext", "i32", lir::LirTypeRef::integer(32),
                       lir::LirExtAttr::None, extra});
      },
      "extra external map evidence must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.extern_decl_name_map["fallback_ext"].return_type =
            lir::LirTypeRef::integer(64);
      },
      "conflicting external map evidence must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.extern_decls[1].name = "linked_ext";
        auto info = module.extern_decl_name_map.extract("fallback_ext");
        info.key() = "linked_ext";
        info.mapped().name = "linked_ext";
        module.extern_decl_name_map.insert(std::move(info));
      },
      "duplicate semantic external identities must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        const auto invalid = static_cast<c4c::LinkNameId>(999);
        auto info = module.extern_decl_link_name_map.begin()->second;
        module.extern_decl_link_name_map.clear();
        info.link_name_id = invalid;
        module.extern_decl_link_name_map.emplace(invalid, info);
        module.extern_decls[0].link_name_id = invalid;
      },
      "unresolved external link identities must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.extern_decls[0].name = "wrong_link_spelling";
        module.extern_decl_link_name_map.begin()->second.name =
            "wrong_link_spelling";
      },
      "link/name incoherence must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.extern_decls[1].name.clear();
      },
      "empty fallback names must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.extern_decls[0].return_type =
            lir::LirTypeRef("semantic raw type");
        module.extern_decls[0].return_type_str = "semantic raw type";
        auto& info = module.extern_decl_link_name_map.begin()->second;
        info.return_type = module.extern_decls[0].return_type;
        info.return_type_str = module.extern_decls[0].return_type_str;
      },
      "RawText external return types must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.extern_decls[0].return_type = lir::LirTypeRef(
            "not-an-integer", lir::LirTypeKind::Integer);
        module.extern_decls[0].return_type_str = "not-an-integer";
        auto& info = module.extern_decl_link_name_map.begin()->second;
        info.return_type = module.extern_decls[0].return_type;
        info.return_type_str = module.extern_decls[0].return_type_str;
      },
      "malformed structured external return types must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.extern_decls[1].return_ext_attr = lir::LirExtAttr::SignExt;
        module.extern_decl_name_map["fallback_ext"].return_ext_attr =
            lir::LirExtAttr::SignExt;
      },
      "void external returns cannot carry extension attrs");
  rejected(
      [](lir::LirModule& module) {
        const auto unknown = static_cast<lir::LirExtAttr>(255);
        module.extern_decls[0].return_ext_attr = unknown;
        module.extern_decl_link_name_map.begin()->second.return_ext_attr =
            unknown;
      },
      "unknown external return extension attrs must reject the whole module");
  rejected(
      [](lir::LirModule& module) {
        module.extern_decls[0].return_type_str = "i64";
        module.extern_decl_link_name_map.begin()->second.return_type_str = "i64";
      },
      "compatibility return text must agree with structured authority");
  rejected(
      [](lir::LirModule& module) {
        module.extern_decls[0].return_type_str = "%struct.StaleReturnShadow";
        module.extern_decl_link_name_map.begin()->second.return_type_str =
            "%struct.StaleReturnShadow";
      },
      "a stale extern return shadow must not override its structured carrier");

  bir::ModuleBuilder builder;
  expect(builder.add_link_name(1, "linked_ext").has_value(),
         "staged link name should be available to external receipt");
  expect(builder
             .add_external_declaration("", bir::Type{bir::TypeKind::I32},
                                       bir::ReturnExtension::None)
             .error() == bir::BuildError::EmptyExternalName,
         "builder must reject empty fallback identities");
  expect(builder
             .add_external_declaration(
                 "linked_ext", bir::Type{bir::TypeKind::I32},
                 bir::ReturnExtension::None, c4c::kInvalidLinkName)
             .error() == bir::BuildError::InvalidExternalLinkName,
         "builder must reject an explicitly invalid link identity");
  expect(builder
             .add_external_declaration("fallback_bad",
                                       bir::Type{bir::TypeKind::Void},
                                       bir::ReturnExtension::SignExt)
             .has_value(),
         "builder should retain malformed staged external state for diagnosis");
  expect(builder
             .add_external_declaration("fallback_bad",
                                       bir::Type{bir::TypeKind::Void},
                                       bir::ReturnExtension::None)
             .error() == bir::BuildError::DuplicateExternalDeclaration,
         "duplicate builder receipt must fail without a second row");
  auto published = std::move(builder).publish();
  expect(!published.has_value() &&
             published.error().reason == bir::PublishError::VerificationFailed,
         "FoundationVerifier must reject malformed staged external storage");
}

void test_global_object_receipt_and_views() {
  lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  const auto linked_name = module.link_names.intern("linked_global");
  const auto weak_linked_name = module.link_names.intern("weak_constant_global");
  const auto pointer_linked_name =
      module.link_names.intern("const_pointer_global");
  const auto init_fn_a = module.link_names.intern("init_fn_a");
  const auto init_fn_b = module.link_names.intern("init_fn_b");

  const auto external_global = [](std::string name, c4c::LinkNameId link,
                                  c4c::TypeBase base,
                                  lir::LirTypeRef type, int align,
                                  bool is_const) {
    lir::LirGlobal global;
    global.id = lir::LirGlobalId{0};
    global.name = std::move(name);
    global.link_name_id = link;
    global.type = scalar_type(base);
    global.is_const = is_const;
    global.linkage_vis = "external ";
    global.qualifier = "global ";
    global.llvm_type = type.str();
    global.llvm_type_ref = std::move(type);
    global.align_bytes = align;
    global.is_extern_decl = true;
    return global;
  };
  module.globals.push_back(external_global(
      "fallback_global", c4c::kInvalidLinkName, c4c::TB_INT,
      lir::LirTypeRef::integer(32), 4, false));
  module.globals.back().linkage_vis = "external hidden ";
  module.globals.push_back(external_global(
      "linked_global", linked_name, c4c::TB_DOUBLE,
      lir::LirTypeRef("double"), 8, true));
  auto initialized = external_global(
      "initialized_global", c4c::kInvalidLinkName, c4c::TB_INT,
      lir::LirTypeRef::integer(32), 4, false);
  initialized.linkage_vis = "protected ";
  initialized.is_extern_decl = false;
  initialized.init_text = std::string{"i32 7\n\0tail", 11};
  initialized.initializer_function_link_name_ids = {
      init_fn_b, init_fn_a, init_fn_b};
  module.globals.push_back(std::move(initialized));
  auto constant_initialized = external_global(
      "constant_initialized_global", c4c::kInvalidLinkName, c4c::TB_DOUBLE,
      lir::LirTypeRef("double"), 8, true);
  constant_initialized.linkage_vis.clear();
  constant_initialized.qualifier = "constant ";
  constant_initialized.is_extern_decl = false;
  constant_initialized.init_text = "double 1.25";
  constant_initialized.initializer_function_link_name_ids = {
      init_fn_a, init_fn_b};
  module.globals.push_back(std::move(constant_initialized));
  auto internal_initialized = external_global(
      "internal_initialized_global", c4c::kInvalidLinkName, c4c::TB_INT,
      lir::LirTypeRef::integer(32), 4, false);
  internal_initialized.is_internal = true;
  internal_initialized.linkage_vis = "internal hidden ";
  internal_initialized.is_extern_decl = false;
  internal_initialized.init_text = "i32 19";
  internal_initialized.initializer_function_link_name_ids = {init_fn_b};
  module.globals.push_back(std::move(internal_initialized));
  auto internal_constant_initialized = external_global(
      "internal_constant_initialized_global", c4c::kInvalidLinkName,
      c4c::TB_DOUBLE, lir::LirTypeRef("double"), 8, true);
  internal_constant_initialized.is_internal = true;
  internal_constant_initialized.linkage_vis = "internal ";
  internal_constant_initialized.qualifier = "constant ";
  internal_constant_initialized.is_extern_decl = false;
  internal_constant_initialized.init_text = "double 2.5";
  internal_constant_initialized.initializer_function_link_name_ids = {
      init_fn_b, init_fn_a};
  module.globals.push_back(std::move(internal_constant_initialized));
  auto weak_initialized = external_global(
      "weak_initialized_global", c4c::kInvalidLinkName, c4c::TB_INT,
      lir::LirTypeRef::integer(32), 4, false);
  weak_initialized.linkage_vis = "weak ";
  weak_initialized.is_extern_decl = false;
  weak_initialized.init_text = "i32 23";
  weak_initialized.initializer_function_link_name_ids = {init_fn_a};
  module.globals.push_back(std::move(weak_initialized));
  auto weak_constant_initialized = external_global(
      "weak_constant_global", weak_linked_name, c4c::TB_DOUBLE,
      lir::LirTypeRef("double"), 8, true);
  weak_constant_initialized.linkage_vis = "weak ";
  weak_constant_initialized.qualifier = "constant ";
  weak_constant_initialized.is_extern_decl = false;
  weak_constant_initialized.init_text = "double 3.5";
  weak_constant_initialized.initializer_function_link_name_ids = {
      init_fn_b, init_fn_a};
  module.globals.push_back(std::move(weak_constant_initialized));
  auto const_pointer_initialized = external_global(
      "const_pointer_global", pointer_linked_name, c4c::TB_INT,
      lir::LirTypeRef("ptr"), 8, true);
  const_pointer_initialized.type.ptr_level = 1;
  const_pointer_initialized.linkage_vis = "protected ";
  const_pointer_initialized.is_extern_decl = false;
  const_pointer_initialized.llvm_type_ref.reset();
  const_pointer_initialized.init_text = std::string{"ptr @target\0tail", 16};
  const_pointer_initialized.initializer_function_link_name_ids = {
      init_fn_a, init_fn_b, init_fn_a};
  module.globals.push_back(std::move(const_pointer_initialized));
  auto weak_external = external_global(
      "weak_external_global", c4c::kInvalidLinkName, c4c::TB_INT,
      lir::LirTypeRef::integer(32), 4, false);
  weak_external.linkage_vis = "extern_weak protected ";
  module.globals.push_back(std::move(weak_external));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "initializer-free external globals should publish transactionally");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "published global storage must be verifier reachable");
  const auto view = imported.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 10 && ids[0].slot == 0 && ids[1].slot == 1 &&
             ids[2].slot == 2 && ids[3].slot == 3 && ids[4].slot == 4 &&
             ids[5].slot == 5 && ids[6].slot == 6 && ids[7].slot == 7 &&
             ids[8].slot == 8 && ids[9].slot == 9,
         "Raw-BIR global IDs must follow source vector order, not LirGlobal.id");
  const auto fallback = view.global_object(ids[0]).value();
  const auto linked = view.global_object(ids[1]).value();
  const auto definition = view.global_object(ids[2]).value();
  const auto constant_definition = view.global_object(ids[3]).value();
  const auto internal_definition = view.global_object(ids[4]).value();
  const auto internal_constant_definition = view.global_object(ids[5]).value();
  const auto weak_definition = view.global_object(ids[6]).value();
  const auto weak_constant_definition = view.global_object(ids[7]).value();
  const auto const_pointer_definition = view.global_object(ids[8]).value();
  const auto weak_external_declaration = view.global_object(ids[9]).value();
  expect(fallback.source_name == "fallback_global" &&
             fallback.object_type == bir::Type{bir::TypeKind::I32} &&
             fallback.alignment == 4 && !fallback.is_internal &&
             !fallback.is_weak && !fallback.is_const &&
             fallback.visibility == bir::SymbolVisibility::Hidden &&
             fallback.is_extern_declaration &&
             std::get<bir::FallbackGlobalName>(fallback.identity).name ==
                 fallback.source_name &&
             linked.source_name == "linked_global" &&
             linked.object_type == bir::Type{bir::TypeKind::F64} &&
             linked.alignment == 8 && linked.is_const &&
             linked.visibility == bir::SymbolVisibility::Default &&
             std::holds_alternative<bir::LinkNameId>(linked.identity) &&
             !fallback.initializer && !linked.initializer &&
             !definition.is_extern_declaration && definition.initializer &&
             !definition.is_weak &&
             definition.visibility == bir::SymbolVisibility::Protected &&
             definition.initializer->opaque_payload ==
                 std::string{"i32 7\n\0tail", 11} &&
             constant_definition.object_type ==
                 bir::Type{bir::TypeKind::F64} &&
             constant_definition.is_const &&
             !constant_definition.is_extern_declaration &&
             constant_definition.initializer &&
             constant_definition.initializer->opaque_payload == "double 1.25",
         "global views must preserve type, identity, alignment, and semantic flags");
  const auto& initializer_links = definition.initializer->function_links;
  expect(initializer_links.size() == 3 &&
             view.spelling(initializer_links[0]).value() == "init_fn_b" &&
             view.spelling(initializer_links[1]).value() == "init_fn_a" &&
             initializer_links[2] == initializer_links[0],
         "initializer links must preserve ordered structured references through the link-name table");
  const auto& constant_initializer_links =
      constant_definition.initializer->function_links;
  expect(constant_initializer_links.size() == 2 &&
             view.spelling(constant_initializer_links[0]).value() ==
                 "init_fn_a" &&
             view.spelling(constant_initializer_links[1]).value() ==
                 "init_fn_b",
         "constant initializer links must preserve ordered structured references");
  expect(internal_definition.object_type == bir::Type{bir::TypeKind::I32} &&
             internal_definition.is_internal && !internal_definition.is_weak &&
             !internal_definition.is_const &&
             internal_definition.visibility == bir::SymbolVisibility::Hidden &&
             !internal_definition.is_extern_declaration &&
             internal_definition.initializer &&
             internal_definition.initializer->opaque_payload == "i32 19" &&
             internal_definition.initializer->function_links.size() == 1 &&
             view.spelling(
                     internal_definition.initializer->function_links[0])
                     .value() == "init_fn_b",
         "internal ordinary definitions must preserve typed flags, payloads, and initializer links");
  expect(internal_constant_definition.object_type ==
                 bir::Type{bir::TypeKind::F64} &&
             internal_constant_definition.is_internal &&
             !internal_constant_definition.is_weak &&
             internal_constant_definition.is_const &&
             !internal_constant_definition.is_extern_declaration &&
             internal_constant_definition.initializer &&
             internal_constant_definition.initializer->opaque_payload ==
                 "double 2.5" &&
             internal_constant_definition.initializer->function_links.size() ==
                 2 &&
             view.spelling(internal_constant_definition.initializer
                               ->function_links[0])
                     .value() == "init_fn_b" &&
             view.spelling(internal_constant_definition.initializer
                               ->function_links[1])
                     .value() == "init_fn_a",
         "internal constant definitions must preserve typed flags, payloads, and ordered initializer links");
  expect(weak_definition.object_type == bir::Type{bir::TypeKind::I32} &&
             !weak_definition.is_internal && weak_definition.is_weak &&
             !weak_definition.is_const &&
             !weak_definition.is_extern_declaration &&
             weak_definition.initializer &&
             weak_definition.initializer->opaque_payload == "i32 23" &&
             weak_definition.initializer->function_links.size() == 1 &&
             view.spelling(weak_definition.initializer->function_links[0])
                     .value() == "init_fn_a",
         "weak ordinary definitions must preserve typed linkage, type, payload, and initializer links");
  expect(weak_constant_definition.object_type ==
                 bir::Type{bir::TypeKind::F64} &&
             !weak_constant_definition.is_internal &&
             weak_constant_definition.is_weak &&
             weak_constant_definition.is_const &&
             !weak_constant_definition.is_extern_declaration &&
             std::holds_alternative<bir::LinkNameId>(
                 weak_constant_definition.identity) &&
             weak_constant_definition.initializer &&
             weak_constant_definition.initializer->opaque_payload ==
                 "double 3.5" &&
             weak_constant_definition.initializer->function_links.size() == 2 &&
             view.spelling(
                     weak_constant_definition.initializer->function_links[0])
                     .value() == "init_fn_b" &&
             view.spelling(
                     weak_constant_definition.initializer->function_links[1])
                     .value() == "init_fn_a",
         "weak constant definitions must preserve typed linkage, identity, type, payload, and ordered initializer links");
  expect(const_pointer_definition.object_type.kind == bir::TypeKind::Pointer &&
             const_pointer_definition.object_type.spelling == "ptr" &&
             const_pointer_definition.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 32, 1}} &&
             !const_pointer_definition.is_internal &&
             !const_pointer_definition.is_weak &&
             const_pointer_definition.is_const &&
             const_pointer_definition.visibility ==
                 bir::SymbolVisibility::Protected &&
             !const_pointer_definition.is_extern_declaration &&
             std::holds_alternative<bir::LinkNameId>(
                 const_pointer_definition.identity) &&
             const_pointer_definition.initializer &&
             const_pointer_definition.initializer->opaque_payload ==
                 std::string{"ptr @target\0tail", 16} &&
             const_pointer_definition.initializer->function_links.size() == 3 &&
             view.spelling(
                     const_pointer_definition.initializer->function_links[0])
                     .value() == "init_fn_a" &&
             view.spelling(
                     const_pointer_definition.initializer->function_links[1])
                     .value() == "init_fn_b" &&
             const_pointer_definition.initializer->function_links[2] ==
                 const_pointer_definition.initializer->function_links[0],
         "const-pointer global definitions must preserve exact typed authority, identity, opaque payload, and ordered initializer links");
  expect(weak_external_declaration.source_name == "weak_external_global" &&
             weak_external_declaration.object_type ==
                 bir::Type{bir::TypeKind::I32} &&
             !weak_external_declaration.is_internal &&
             weak_external_declaration.is_weak &&
             weak_external_declaration.visibility ==
                 bir::SymbolVisibility::Protected &&
             !weak_external_declaration.is_const &&
             weak_external_declaration.is_extern_declaration &&
             !weak_external_declaration.initializer,
         "weak external declarations must preserve both external-declaration and weak-linkage facts");
  expect(view.global_object("fallback_global").value() == ids[0] &&
             view.global_object(std::get<bir::LinkNameId>(linked.identity))
                     .value() == ids[1] &&
             view.global_object(
                     std::get<bir::LinkNameId>(weak_constant_definition.identity))
                     .value() == ids[7] &&
             view.global_object(
                     std::get<bir::LinkNameId>(const_pointer_definition.identity))
                     .value() == ids[8],
         "global name and link lookups must resolve ordered typed identities");

  module.globals[8].llvm_type_ref = lir::LirTypeRef("ptr");
  auto corroborated = bir::lower_lir_to_raw_bir(module);
  expect(corroborated.has_value(),
         "an agreeing optional pointer mirror should preserve receipt");
  const auto corroborated_view = corroborated.value().view();
  const auto corroborated_ids = corroborated_view.global_objects();
  expect(corroborated_ids.size() == 10 &&
             corroborated_view.global_object(corroborated_ids[8])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 32, 1}},
         "an optional pointer mirror that agrees exactly must remain corroborating evidence");
}

void test_scalar_global_type_authority_without_mirror() {
  lir::LirModule module;
  module.target_profile.arch = c4c::TargetArch::I686;

  lir::LirGlobal declaration;
  declaration.name = "producer_long_declaration";
  declaration.type = scalar_type(c4c::TB_LONG);
  declaration.type.inner_rank = -1;
  declaration.linkage_vis = "external hidden ";
  declaration.qualifier = "global ";
  declaration.llvm_type = "i32";
  declaration.align_bytes = 4;
  declaration.is_extern_decl = true;
  module.globals.push_back(std::move(declaration));

  lir::LirGlobal definition;
  definition.name = "producer_double_definition";
  definition.type = scalar_type(c4c::TB_DOUBLE);
  definition.type.inner_rank = -1;
  definition.is_const = true;
  definition.linkage_vis = "protected ";
  definition.qualifier = "constant ";
  definition.llvm_type = "double";
  definition.init_text = "double 6.25";
  definition.align_bytes = 8;
  definition.is_extern_decl = false;
  module.globals.push_back(std::move(definition));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "actual producer-shaped scalar globals should not require an optional type mirror");
  const auto view = imported.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 2,
         "scalar TypeSpec authority should preserve ordered global receipt");
  const auto raw_declaration = view.global_object(ids[0]).value();
  const auto raw_definition = view.global_object(ids[1]).value();
  expect(raw_declaration.object_type ==
                 bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
             raw_declaration.is_extern_declaration &&
             raw_declaration.visibility == bir::SymbolVisibility::Hidden &&
             !raw_declaration.initializer,
         "target-aware long TypeSpec authority must lower to i32 on I686 declarations");
  expect(raw_definition.object_type ==
                 bir::Type{bir::TypeKind::Floating, 64, "double"} &&
             !raw_definition.is_extern_declaration &&
             raw_definition.is_const &&
             raw_definition.visibility == bir::SymbolVisibility::Protected &&
             raw_definition.initializer &&
             raw_definition.initializer->opaque_payload == "double 6.25",
         "floating TypeSpec authority must retain definition, visibility, and initializer facts");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "mirror-free scalar global receipt must remain verifier reachable");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value() &&
             canonical.value().view().global_objects().size() == 2,
         "producer no-split scalar globals must reach Canonical BIR");
  for (const int invalid_inner_rank : {-2, 1}) {
    auto candidate = module;
    candidate.globals[0].type.inner_rank = invalid_inner_rank;
    expect(!bir::lower_lir_to_raw_bir(candidate).has_value() &&
               !bir::lower_lir_to_canonical_bir(candidate).has_value(),
           "scalar globals must reject invalid non-split inner-rank residue transactionally");
  }

  lir::LirModule lp64;
  lp64.target_profile = c4c::default_target_profile(c4c::TargetArch::X86_64);
  auto lp64_long = module.globals.front();
  lp64_long.name = "lp64_long_declaration";
  lp64_long.llvm_type = "i64";
  lp64.globals.push_back(std::move(lp64_long));
  const auto lp64_imported = bir::lower_lir_to_raw_bir(lp64);
  expect(lp64_imported.has_value() &&
             lp64_imported.value().view().global_object(
                 lp64_imported.value().view().global_objects().front()).value().object_type ==
                 bir::Type{bir::TypeKind::Integer, 64, "i64"},
         "LP64 long globals must receive i64 storage");

  auto malformed_i686 = module;
  malformed_i686.globals.front().llvm_type = "i64";
  expect(!bir::lower_lir_to_raw_bir(malformed_i686).has_value() &&
             !bir::lower_lir_to_canonical_bir(malformed_i686).has_value(),
         "I686 long globals must reject i64 target-policy conflicts transactionally");

  auto stale_type_shadow = module;
  stale_type_shadow.globals.front().llvm_type_ref = lir::LirTypeRef::integer(32);
  stale_type_shadow.globals.front().llvm_type = "i64";
  expect(!bir::lower_lir_to_raw_bir(stale_type_shadow).has_value() &&
             !bir::lower_lir_to_canonical_bir(stale_type_shadow).has_value(),
         "a stale rendered global type must not override complete structured type authority");
}

void test_enum_storage_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.target_profile.arch = c4c::TargetArch::X86_64;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto definition_link =
        module.link_names.intern("default_enum_definition");
    const auto init_link = module.link_names.intern("enum_init_function");

    lir::LirGlobal definition;
    definition.name = "default_enum_definition";
    definition.link_name_id = definition_link;
    definition.type = scalar_type(c4c::TB_ENUM);
    definition.type.enum_underlying_base = c4c::TB_VOID;
    definition.linkage_vis = "protected ";
    definition.qualifier = "global ";
    definition.llvm_type = "i32";
    definition.init_text = std::string{"i32 7\0enum", 10};
    definition.initializer_function_link_name_ids = {init_link};
    definition.align_bytes = 4;
    module.globals.push_back(std::move(definition));

    lir::LirGlobal declaration;
    declaration.name = "explicit_enum_extern";
    declaration.type = scalar_type(c4c::TB_ENUM);
    declaration.type.enum_underlying_base = c4c::TB_LONG;
    declaration.linkage_vis = "external hidden ";
    declaration.qualifier = "global ";
    declaration.llvm_type = "i64";
    declaration.align_bytes = 8;
    declaration.is_extern_decl = true;
    module.globals.push_back(std::move(declaration));

    lir::LirGlobal pointer;
    pointer.name = "deep_enum_pointer";
    pointer.type = scalar_type(c4c::TB_ENUM);
    pointer.type.enum_underlying_base = c4c::TB_UINT128;
    pointer.type.ptr_level = 3;
    pointer.linkage_vis = "extern_weak protected ";
    pointer.qualifier = "global ";
    pointer.llvm_type = "ptr";
    pointer.align_bytes = 16;
    pointer.is_extern_decl = true;
    module.globals.push_back(std::move(pointer));

    lir::LirGlobal array;
    array.name = "enum_pointer_element_array";
    array.type = scalar_type(c4c::TB_ENUM);
    array.type.enum_underlying_base = c4c::TB_UCHAR;
    array.type.ptr_level = 2;
    array.type.array_rank = 2;
    array.type.array_size = 3;
    array.type.array_dims[0] = 3;
    array.type.array_dims[1] = 5;
    array.linkage_vis = "external hidden ";
    array.qualifier = "global ";
    array.llvm_type = "[3 x [5 x ptr]]";
    array.align_bytes = 8;
    array.is_extern_decl = true;
    module.globals.push_back(std::move(array));
    return module;
  };

  auto module = valid_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value(),
         "producer-shaped enum storage globals must import through typed integer normalization");
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "normalized enum storage globals must be Foundation-verifier reachable");
  const auto view = raw.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 4 && ids[0].slot == 0 && ids[1].slot == 1 &&
             ids[2].slot == 2 && ids[3].slot == 3,
         "enum storage globals must preserve deterministic source order");
  const auto definition = view.global_object(ids[0]).value();
  const auto declaration = view.global_object(ids[1]).value();
  const auto pointer = view.global_object(ids[2]).value();
  const auto array = view.global_object(ids[3]).value();
  expect(definition.object_type ==
                 bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
             std::holds_alternative<bir::LinkNameId>(definition.identity) &&
             view.spelling(std::get<bir::LinkNameId>(definition.identity))
                     .value() == "default_enum_definition" &&
             !definition.is_internal && !definition.is_weak &&
             !definition.is_const && !definition.is_extern_declaration &&
             definition.visibility == bir::SymbolVisibility::Protected &&
             definition.alignment == 4 && definition.initializer &&
             definition.initializer->opaque_payload ==
                 std::string{"i32 7\0enum", 10} &&
             definition.initializer->function_links.size() == 1 &&
             view.spelling(definition.initializer->function_links[0]).value() ==
                 "enum_init_function",
         "default-underlying enum definitions must retain normalized i32 storage and all object facts");
  expect(declaration.object_type ==
                 bir::Type{bir::TypeKind::Integer, 64, "i64"} &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 declaration.identity) &&
             declaration.is_extern_declaration && !declaration.is_weak &&
             declaration.visibility == bir::SymbolVisibility::Hidden &&
             declaration.alignment == 8 && !declaration.initializer,
         "target-sized explicit-underlying enum externs must preserve exact integer storage and object facts");
  expect(pointer.object_type.kind == bir::TypeKind::Pointer &&
             pointer.object_type.spelling == "ptr" &&
             pointer.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 128, 3}} &&
             pointer.is_extern_declaration && pointer.is_weak &&
             pointer.visibility == bir::SymbolVisibility::Protected &&
             pointer.alignment == 16 && !pointer.initializer,
         "enum pointer globals must retain normalized scalar width, exact depth, and object facts");
  expect(array.object_type.kind == bir::TypeKind::Array &&
             array.object_type.spelling == "[3 x [5 x ptr]]" &&
             array.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Integer, 8, 2, {3, 5}}} &&
             array.is_extern_declaration && !array.is_weak &&
             array.visibility == bir::SymbolVisibility::Hidden &&
             array.alignment == 8 && !array.initializer,
         "fixed enum arrays must retain normalized element width, pointer depth, dimensions, and object facts");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  const auto canonical_ids =
      canonical.has_value() ? canonical.value().view().global_objects()
                            : std::vector<bir::GlobalObjectId>{};
  expect(canonical.has_value() && canonical_ids.size() == 4 &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[0])
                     .value()
                     .object_type ==
                 bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[1])
                     .value()
                     .object_type ==
                 bir::Type{bir::TypeKind::Integer, 64, "i64"} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[2])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 128, 3}} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[3])
                     .value()
                     .object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Integer, 8, 2, {3, 5}}},
         "enum direct, pointer, and fixed-array storage must publish Canonical BIR with exact normalized facts");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto rejected_raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!rejected_raw.has_value() &&
               rejected_raw.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.enum_underlying_base = c4c::TB_FLOAT;
      },
      "floating enum underlying storage must remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.enum_underlying_base = c4c::TB_STRUCT;
      },
      "aggregate enum underlying storage must remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.enum_underlying_base = c4c::TB_ENUM;
      },
      "recursive enum underlying storage must remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.enum_underlying_base = c4c::TB_COMPLEX_INT;
      },
      "complex enum underlying storage must remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.enum_underlying_base = c4c::TB_VA_LIST;
      },
      "va-list enum underlying storage must remain closed");
  rejected([](lir::LirModule& m) { m.globals[1].llvm_type = "i32"; },
           "enum storage spelling must exactly match normalized width");
  rejected(
      [](lir::LirModule& m) {
        m.globals[1].llvm_type_ref = lir::LirTypeRef::integer(32);
      },
      "enum scalar mirrors must corroborate normalized typed storage");
  rejected(
      [](lir::LirModule& m) {
        m.globals[3].llvm_type_ref = lir::LirTypeRef("[3 x [5 x ptr]]");
      },
      "producer-valid fixed enum arrays must not carry an LLVM type mirror");
  rejected(
      [](lir::LirModule& m) {
        auto& type = m.globals[0].type;
        type.is_vector = true;
        type.vector_lanes = 4;
        type.vector_bytes = 16;
        m.globals[0].llvm_type = "<4 x i32>";
      },
      "enum vectors remain outside the direct enum-storage packet");
}

void test_vrm_register_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto definition_link = module.link_names.intern("vrm4_definition");
    const auto init_link = module.link_names.intern("vrm_init_function");

    lir::LirGlobal definition;
    definition.name = "vrm4_definition";
    definition.link_name_id = definition_link;
    definition.type = scalar_type(c4c::TB_VRM_REGISTER);
    definition.type.vrm_width = 4;
    definition.linkage_vis = "protected ";
    definition.qualifier = "constant ";
    definition.llvm_type = "c4c.vrm4";
    definition.init_text = "c4c.vrm4 zeroinitializer";
    definition.initializer_function_link_name_ids = {init_link};
    definition.align_bytes = 4;
    definition.is_const = true;
    module.globals.push_back(std::move(definition));

    lir::LirGlobal declaration;
    declaration.name = "vrm8_extern";
    declaration.type = scalar_type(c4c::TB_VRM_REGISTER);
    declaration.type.vrm_width = 8;
    declaration.linkage_vis = "external hidden ";
    declaration.qualifier = "global ";
    declaration.llvm_type = "c4c.vrm8";
    declaration.align_bytes = 8;
    declaration.is_extern_decl = true;
    module.globals.push_back(std::move(declaration));

    lir::LirGlobal pointer;
    pointer.name = "deep_vrm_pointer";
    pointer.type = scalar_type(c4c::TB_VRM_REGISTER);
    pointer.type.vrm_width = 1;
    pointer.type.ptr_level = 3;
    pointer.linkage_vis = "extern_weak protected ";
    pointer.qualifier = "global ";
    pointer.llvm_type = "ptr";
    pointer.llvm_type_ref = lir::LirTypeRef("ptr");
    pointer.align_bytes = 8;
    pointer.is_extern_decl = true;
    module.globals.push_back(std::move(pointer));

    lir::LirGlobal array;
    array.name = "vrm_pointer_array";
    array.type = scalar_type(c4c::TB_VRM_REGISTER);
    array.type.vrm_width = 2;
    array.type.ptr_level = 2;
    array.type.array_rank = 2;
    array.type.array_size = 3;
    array.type.array_dims[0] = 3;
    array.type.array_dims[1] = 5;
    array.linkage_vis = "external hidden ";
    array.qualifier = "global ";
    array.llvm_type = "[3 x [5 x ptr]]";
    array.align_bytes = 8;
    array.is_extern_decl = true;
    module.globals.push_back(std::move(array));
    return module;
  };

  auto module = valid_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value(),
         "producer-shaped direct, pointer, and array VRM globals must import");
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "typed VRM globals must remain Foundation-verifier reachable");
  const auto view = raw.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 4 && ids[0].slot == 0 && ids[1].slot == 1 &&
             ids[2].slot == 2 && ids[3].slot == 3,
         "VRM globals must preserve deterministic source order");
  const auto definition = view.global_object(ids[0]).value();
  const auto declaration = view.global_object(ids[1]).value();
  const auto pointer = view.global_object(ids[2]).value();
  const auto array = view.global_object(ids[3]).value();

  expect(definition.object_type.kind == bir::TypeKind::VrmRegister &&
             definition.object_type.bit_width == 4 &&
             definition.object_type.spelling == "c4c.vrm4" &&
             std::holds_alternative<bir::LinkNameId>(definition.identity) &&
             view.spelling(std::get<bir::LinkNameId>(definition.identity))
                     .value() == "vrm4_definition" &&
             !definition.is_internal && !definition.is_weak &&
             definition.is_const && !definition.is_extern_declaration &&
             definition.visibility == bir::SymbolVisibility::Protected &&
             definition.alignment == 4 && definition.initializer &&
             definition.initializer->opaque_payload ==
                 "c4c.vrm4 zeroinitializer" &&
             definition.initializer->function_links.size() == 1 &&
             view.spelling(definition.initializer->function_links[0]).value() ==
                 "vrm_init_function",
         "direct VRM definitions must retain exact width, spelling, and object and initializer facts");
  expect(declaration.object_type.kind == bir::TypeKind::VrmRegister &&
             declaration.object_type.bit_width == 8 &&
             declaration.object_type.spelling == "c4c.vrm8" &&
             declaration.is_extern_declaration && !declaration.is_weak &&
             declaration.visibility == bir::SymbolVisibility::Hidden &&
             declaration.alignment == 8 && !declaration.initializer,
         "VRM externs must retain exact width, spelling, linkage, and object facts");
  expect(pointer.object_type.kind == bir::TypeKind::Pointer &&
             pointer.object_type.spelling == "ptr" &&
             pointer.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::VrmRegister, 1, 3}} &&
             pointer.is_extern_declaration && pointer.is_weak &&
             pointer.visibility == bir::SymbolVisibility::Protected &&
             pointer.alignment == 8 && !pointer.initializer,
         "deep VRM pointers must retain exact base width, pointer depth, and object facts");
  expect(array.object_type.kind == bir::TypeKind::Array &&
             array.object_type.spelling == "[3 x [5 x ptr]]" &&
             array.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::VrmRegister, 2, 2, {3, 5}}} &&
             array.is_extern_declaration && !array.is_weak &&
             array.visibility == bir::SymbolVisibility::Hidden &&
             array.alignment == 8 && !array.initializer,
         "fixed VRM pointer arrays must retain exact base width, depth, dimensions, and object facts");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  const auto canonical_ids =
      canonical.has_value() ? canonical.value().view().global_objects()
                            : std::vector<bir::GlobalObjectId>{};
  expect(canonical.has_value() && canonical_ids.size() == 4 &&
             canonical.value().view().global_object(canonical_ids[0])
                     .value().object_type == definition.object_type &&
             canonical.value().view().global_object(canonical_ids[1])
                     .value().object_type == declaration.object_type &&
             canonical.value().view().global_object(canonical_ids[2])
                     .value().object_type.pointer_facts ==
                 pointer.object_type.pointer_facts &&
             canonical.value().view().global_object(canonical_ids[3])
                     .value().object_type.array_facts ==
                 array.object_type.array_facts,
         "VRM direct, pointer, and array storage must publish exact Canonical BIR facts");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto rejected_raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!rejected_raw.has_value() &&
               rejected_raw.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected([](lir::LirModule& m) { m.globals[0].type.vrm_width = 0; },
           "zero direct VRM width must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].type.vrm_width = -1; },
           "negative direct VRM width must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].type.vrm_width = 3; },
           "unsupported direct VRM width must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].llvm_type = "c4c.vrm2"; },
           "VRM spelling must exactly corroborate typed width");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type_ref = lir::LirTypeRef("c4c.vrm4");
      },
      "producer-shaped direct VRM globals must reject unexpected mirrors");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.base = c4c::TB_INT;
        m.globals[0].llvm_type = "i32";
        m.globals[0].llvm_type_ref = lir::LirTypeRef::integer(32);
      },
      "residual VRM width must not fall through a corroborated scalar route");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.is_vector = true;
        m.globals[0].type.vector_lanes = 4;
        m.globals[0].type.vector_bytes = 4;
        m.globals[0].llvm_type = "<4 x c4c.vrm4>";
        m.globals[0].llvm_type_ref.reset();
      },
      "VRM vectors remain excluded from the existing scalar vector route");
  rejected([](lir::LirModule& m) { m.globals[2].type.is_fn_ptr = true; },
           "VRM function pointers remain outside scalar-pointer storage");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].type.is_ptr_to_array = true;
        m.globals[2].type.inner_rank = 1;
      },
      "pointer-to-array authority without recorded dimensions remains unsupported");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_lvalue_ref = true; },
           "VRM references remain outside global storage receipt");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.base = c4c::TB_STRUCT;
        m.globals[0].llvm_type = "{ i32 }";
        m.globals[0].llvm_type_ref = lir::LirTypeRef("{ i32 }");
      },
      "residual VRM width must not fall through a corroborated aggregate route");
  rejected([](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_VA_LIST; },
           "va-list TypeSpec neighbors must remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].llvm_type_ref = lir::LirTypeRef("c4c.vrm1");
      },
      "VRM pointers must reject non-pointer mirrors");
  rejected(
      [](lir::LirModule& m) {
        m.globals[3].llvm_type_ref = lir::LirTypeRef("[3 x [5 x ptr]]");
      },
      "VRM arrays must reject unexpected mirrors");
}

void test_complex_storage_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.target_profile.arch = c4c::TargetArch::I686;
    module.target_profile.os = c4c::TargetOs::Linux;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto definition_link =
        module.link_names.intern("complex_long_double_definition");
    const auto init_link = module.link_names.intern("complex_init_function");

    lir::LirGlobal definition;
    definition.name = "complex_long_double_definition";
    definition.link_name_id = definition_link;
    definition.type = scalar_type(c4c::TB_COMPLEX_LONGDOUBLE);
    definition.linkage_vis = "protected ";
    definition.qualifier = "constant ";
    definition.llvm_type = "{ x86_fp80, x86_fp80 }";
    definition.init_text = std::string{
        "{ x86_fp80, x86_fp80 } zeroinitializer\0complex", 46};
    definition.initializer_function_link_name_ids = {init_link};
    definition.align_bytes = 16;
    definition.is_const = true;
    module.globals.push_back(std::move(definition));

    lir::LirGlobal declaration;
    declaration.name = "complex_integer_extern";
    declaration.type = scalar_type(c4c::TB_COMPLEX_INT);
    declaration.linkage_vis = "external hidden ";
    declaration.qualifier = "global ";
    declaration.llvm_type = "{ i32, i32 }";
    declaration.align_bytes = 8;
    declaration.is_extern_decl = true;
    module.globals.push_back(std::move(declaration));

    lir::LirGlobal pointer;
    pointer.name = "deep_complex_integer_pointer";
    pointer.type = scalar_type(c4c::TB_COMPLEX_ULONG);
    pointer.type.ptr_level = 3;
    pointer.linkage_vis = "extern_weak protected ";
    pointer.qualifier = "global ";
    pointer.llvm_type = "ptr";
    pointer.align_bytes = 8;
    pointer.is_extern_decl = true;
    module.globals.push_back(std::move(pointer));

    lir::LirGlobal array;
    array.name = "complex_float_pointer_array";
    array.type = scalar_type(c4c::TB_COMPLEX_FLOAT);
    array.type.ptr_level = 2;
    array.type.array_rank = 2;
    array.type.array_size = 3;
    array.type.array_dims[0] = 3;
    array.type.array_dims[1] = 4;
    array.linkage_vis = "external hidden ";
    array.qualifier = "global ";
    array.llvm_type = "[3 x [4 x ptr]]";
    array.align_bytes = 8;
    array.is_extern_decl = true;
    module.globals.push_back(std::move(array));
    return module;
  };

  auto module = valid_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value(),
         "producer-shaped complex globals must import from typed TypeSpec authority");
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "typed complex globals must remain Foundation-verifier reachable");
  const auto view = raw.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 4 && ids[0].slot == 0 && ids[1].slot == 1 &&
             ids[2].slot == 2 && ids[3].slot == 3,
         "complex globals must preserve deterministic source order");
  const auto definition = view.global_object(ids[0]).value();
  const auto declaration = view.global_object(ids[1]).value();
  const auto pointer = view.global_object(ids[2]).value();
  const auto array = view.global_object(ids[3]).value();

  bir::Type expected_definition{bir::TypeKind::Complex, 80,
                                "{ x86_fp80, x86_fp80 }"};
  expected_definition.complex_facts =
      bir::ComplexTypeFacts{bir::TypeKind::Floating, 80};
  bir::Type expected_declaration{bir::TypeKind::Complex, 32,
                                 "{ i32, i32 }"};
  expected_declaration.complex_facts =
      bir::ComplexTypeFacts{bir::TypeKind::Integer, 32};
  expect(definition.object_type == expected_definition &&
             std::holds_alternative<bir::LinkNameId>(definition.identity) &&
             view.spelling(std::get<bir::LinkNameId>(definition.identity))
                     .value() == "complex_long_double_definition" &&
             !definition.is_internal && !definition.is_weak &&
             definition.is_const && !definition.is_extern_declaration &&
             definition.visibility == bir::SymbolVisibility::Protected &&
             definition.alignment == 16 && definition.initializer &&
             definition.initializer->opaque_payload ==
                 std::string{
                     "{ x86_fp80, x86_fp80 } zeroinitializer\0complex", 46} &&
             definition.initializer->function_links.size() == 1 &&
             view.spelling(definition.initializer->function_links[0]).value() ==
                 "complex_init_function",
         "floating complex definitions must retain exact component and object authority");
  expect(declaration.object_type == expected_declaration &&
             declaration.is_extern_declaration && !declaration.is_weak &&
             declaration.visibility == bir::SymbolVisibility::Hidden &&
             declaration.alignment == 8 && !declaration.initializer,
         "integer complex externs must retain exact component and linkage facts");
  expect(pointer.object_type.kind == bir::TypeKind::Pointer &&
             pointer.object_type.spelling == "ptr" &&
             pointer.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Complex, 64, 3,
                     bir::ComplexTypeFacts{bir::TypeKind::Integer, 64}}} &&
             pointer.is_extern_declaration && pointer.is_weak &&
             pointer.visibility == bir::SymbolVisibility::Protected &&
             pointer.alignment == 8 && !pointer.initializer,
         "deep complex pointers must retain typed components and exact depth");
  expect(array.object_type.kind == bir::TypeKind::Array &&
             array.object_type.spelling == "[3 x [4 x ptr]]" &&
             array.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Complex, 32, 2, {3, 4},
                     bir::ComplexTypeFacts{bir::TypeKind::Floating, 32}}} &&
             array.is_extern_declaration && !array.is_weak &&
             array.visibility == bir::SymbolVisibility::Hidden &&
             array.alignment == 8 && !array.initializer,
         "fixed complex arrays must retain typed components, dimensions, and pointer depth");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  const auto canonical_ids =
      canonical.has_value() ? canonical.value().view().global_objects()
                            : std::vector<bir::GlobalObjectId>{};
  expect(canonical.has_value() && canonical_ids.size() == 4 &&
             canonical.value().view().global_object(canonical_ids[0])
                     .value().object_type == expected_definition &&
             canonical.value().view().global_object(canonical_ids[1])
                     .value().object_type == expected_declaration &&
             canonical.value().view().global_object(canonical_ids[2])
                     .value().object_type.pointer_facts ==
                 pointer.object_type.pointer_facts &&
             canonical.value().view().global_object(canonical_ids[3])
                     .value().object_type.array_facts ==
                 array.object_type.array_facts,
         "complex direct, pointer, and array storage must publish exact Canonical BIR facts");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto rejected_raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!rejected_raw.has_value() &&
               rejected_raw.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected([](lir::LirModule& m) { m.globals[0].llvm_type = "{ double, double }"; },
           "complex storage spelling must exactly match typed components");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type_ref = lir::LirTypeRef(m.globals[0].llvm_type);
      },
      "complex globals must reject unexpected aggregate-shaped mirrors");
  rejected([](lir::LirModule& m) { m.globals[2].type.is_fn_ptr = true; },
           "complex function pointers remain outside scalar-pointer storage");
  rejected([](lir::LirModule& m) { m.globals[2].type.is_ptr_to_array = true; },
           "complex pointers to arrays remain outside scalar-pointer storage");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_lvalue_ref = true; },
           "complex references remain outside global storage receipt");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.is_vector = true;
        m.globals[0].type.vector_lanes = 2;
        m.globals[0].type.vector_bytes = 32;
        m.globals[0].llvm_type = "<2 x { x86_fp80, x86_fp80 }>";
      },
      "complex vectors remain outside direct scalar-vector receipt");
  rejected([](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_STRUCT; },
           "aggregate TypeSpec neighbors must not borrow complex storage authority");
  rejected([](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_VA_LIST; },
           "va-list TypeSpec neighbors must remain closed");
  rejected([](lir::LirModule& m) { m.globals[3].type.array_dims[1] = -2; },
           "malformed complex fixed-array dimensions must roll back transactionally");
}

void test_scalar_pointer_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto linked = module.link_names.intern("extern_integer_pointer");
    const auto ordinary_link =
        module.link_names.intern("ordinary_integer_pointer");
    const auto internal_const_link =
        module.link_names.intern("internal_const_float_pointer");
    const auto weak_const_link =
        module.link_names.intern("weak_const_integer_pointer");
    const auto init_a = module.link_names.intern("pointer_init_a");
    const auto init_b = module.link_names.intern("pointer_init_b");

    lir::LirGlobal external;
    external.name = "extern_integer_pointer";
    external.link_name_id = linked;
    external.type = scalar_type(c4c::TB_LONGLONG);
    external.type.ptr_level = 1;
    external.type.inner_rank = -1;
    external.linkage_vis = "external hidden ";
    external.qualifier = "global ";
    external.llvm_type = "ptr";
    external.align_bytes = 8;
    external.is_extern_decl = true;
    module.globals.push_back(std::move(external));

    lir::LirGlobal weak_external;
    weak_external.name = "weak_external_float_pointer";
    weak_external.type = scalar_type(c4c::TB_FLOAT);
    weak_external.type.ptr_level = 3;
    weak_external.linkage_vis = "extern_weak protected ";
    weak_external.qualifier = "global ";
    weak_external.llvm_type = "ptr";
    weak_external.align_bytes = 16;
    weak_external.is_extern_decl = true;
    module.globals.push_back(std::move(weak_external));

    lir::LirGlobal ordinary;
    ordinary.name = "ordinary_integer_pointer";
    ordinary.link_name_id = ordinary_link;
    ordinary.type = scalar_type(c4c::TB_UINT);
    ordinary.type.ptr_level = 2;
    ordinary.linkage_vis.clear();
    ordinary.qualifier = "global ";
    ordinary.llvm_type = "ptr";
    ordinary.align_bytes = 8;
    ordinary.init_text = std::string{"ptr @ordinary\0tail", 18};
    ordinary.initializer_function_link_name_ids = {init_b, init_a, init_b};
    module.globals.push_back(std::move(ordinary));

    lir::LirGlobal internal;
    internal.name = "internal_float_pointer";
    internal.type = scalar_type(c4c::TB_DOUBLE);
    internal.type.ptr_level = 1;
    internal.is_internal = true;
    internal.linkage_vis = "internal hidden ";
    internal.qualifier = "global ";
    internal.llvm_type = "ptr";
    internal.align_bytes = 16;
    internal.init_text = "ptr @internal";
    internal.initializer_function_link_name_ids = {init_a};
    module.globals.push_back(std::move(internal));

    lir::LirGlobal weak;
    weak.name = "weak_integer_pointer";
    weak.type = scalar_type(c4c::TB_SHORT);
    weak.type.ptr_level = 1;
    weak.linkage_vis = "weak protected ";
    weak.qualifier = "global ";
    weak.llvm_type = "ptr";
    weak.align_bytes = 32;
    weak.init_text = "ptr @weak";
    weak.initializer_function_link_name_ids = {init_a, init_b};
    module.globals.push_back(std::move(weak));

    lir::LirGlobal internal_const;
    internal_const.name = "internal_const_float_pointer";
    internal_const.link_name_id = internal_const_link;
    internal_const.type = scalar_type(c4c::TB_FLOAT);
    internal_const.type.ptr_level = 1;
    internal_const.is_internal = true;
    internal_const.is_const = true;
    internal_const.linkage_vis = "internal protected ";
    internal_const.qualifier = "global ";
    internal_const.llvm_type = "ptr";
    internal_const.align_bytes = 64;
    internal_const.init_text =
        std::string{"ptr @internal_const\0tail", 24};
    internal_const.initializer_function_link_name_ids = {init_b, init_a,
                                                         init_b};
    module.globals.push_back(std::move(internal_const));

    lir::LirGlobal weak_const;
    weak_const.name = "weak_const_integer_pointer";
    weak_const.link_name_id = weak_const_link;
    weak_const.type = scalar_type(c4c::TB_LONGLONG);
    weak_const.type.ptr_level = 1;
    weak_const.is_const = true;
    weak_const.linkage_vis = "weak hidden ";
    weak_const.qualifier = "global ";
    weak_const.llvm_type = "ptr";
    weak_const.align_bytes = 8;
    weak_const.init_text = std::string{"ptr @weak_const\0tail", 20};
    weak_const.initializer_function_link_name_ids = {init_a, init_b, init_a};
    module.globals.push_back(std::move(weak_const));
    return module;
  };

  auto module = valid_module();
  const auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "producer-shaped scalar-pointer extern declarations must import without a mirror");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "typed scalar-pointer extern declarations must be Foundation reachable");
  const auto view = imported.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 7 && ids[0].slot == 0 && ids[1].slot == 1 &&
             ids[2].slot == 2 && ids[3].slot == 3 && ids[4].slot == 4 &&
             ids[5].slot == 5 && ids[6].slot == 6,
         "typed scalar-pointer objects must preserve source order");
  const auto external = view.global_object(ids[0]).value();
  const auto weak_external = view.global_object(ids[1]).value();
  const auto ordinary = view.global_object(ids[2]).value();
  const auto internal = view.global_object(ids[3]).value();
  const auto weak = view.global_object(ids[4]).value();
  const auto internal_const = view.global_object(ids[5]).value();
  const auto weak_const = view.global_object(ids[6]).value();
  expect(external.source_name == "extern_integer_pointer" &&
             external.object_type.kind == bir::TypeKind::Pointer &&
             external.object_type.bit_width == 0 &&
             external.object_type.spelling == "ptr" &&
             external.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 64, 1}} &&
             std::holds_alternative<bir::LinkNameId>(external.identity) &&
             view.spelling(std::get<bir::LinkNameId>(external.identity))
                     .value() == "extern_integer_pointer" &&
             !external.is_internal && !external.is_weak &&
             !external.is_const && external.is_extern_declaration &&
             external.visibility == bir::SymbolVisibility::Hidden &&
             external.alignment == 8 && !external.initializer,
         "external scalar pointers must retain exact pointee and object facts");
  expect(weak_external.object_type.kind == bir::TypeKind::Pointer &&
             weak_external.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Floating, 32, 3}} &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 weak_external.identity) &&
             std::get<bir::FallbackGlobalName>(weak_external.identity).name ==
                 "weak_external_float_pointer" &&
             !weak_external.is_internal && weak_external.is_weak &&
             !weak_external.is_const && weak_external.is_extern_declaration &&
             weak_external.visibility == bir::SymbolVisibility::Protected &&
             weak_external.alignment == 16 && !weak_external.initializer,
         "deep weak external scalar pointers must retain exact typed depth, pointee, linkage, visibility, alignment, and identity");
  expect(ordinary.object_type.kind == bir::TypeKind::Pointer &&
             ordinary.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 32, 2}} &&
             std::holds_alternative<bir::LinkNameId>(ordinary.identity) &&
             view.spelling(std::get<bir::LinkNameId>(ordinary.identity))
                     .value() == "ordinary_integer_pointer" &&
             !ordinary.is_internal && !ordinary.is_weak &&
             !ordinary.is_const && !ordinary.is_extern_declaration &&
             ordinary.visibility == bir::SymbolVisibility::Default &&
             ordinary.alignment == 8 && ordinary.initializer &&
             ordinary.initializer->opaque_payload ==
                 std::string{"ptr @ordinary\0tail", 18} &&
             ordinary.initializer->function_links.size() == 3 &&
             view.spelling(ordinary.initializer->function_links[0]).value() ==
                 "pointer_init_b" &&
             view.spelling(ordinary.initializer->function_links[1]).value() ==
                 "pointer_init_a" &&
             ordinary.initializer->function_links[2] ==
                 ordinary.initializer->function_links[0],
         "deep ordinary scalar-pointer definitions must preserve exact typed depth, authority, identity, byte-exact payload, and ordered initializer links");
  expect(internal.object_type.kind == bir::TypeKind::Pointer &&
             internal.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Floating, 64, 1}} &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 internal.identity) &&
             std::get<bir::FallbackGlobalName>(internal.identity).name ==
                 "internal_float_pointer" &&
             internal.is_internal && !internal.is_weak &&
             !internal.is_const && !internal.is_extern_declaration &&
             internal.visibility == bir::SymbolVisibility::Hidden &&
             internal.alignment == 16 && internal.initializer &&
             internal.initializer->opaque_payload == "ptr @internal" &&
             internal.initializer->function_links.size() == 1 &&
             view.spelling(internal.initializer->function_links[0]).value() ==
                 "pointer_init_a",
         "internal scalar-pointer definitions must preserve pointee, linkage, visibility, alignment, and initializer facts");
  expect(weak.object_type.kind == bir::TypeKind::Pointer &&
             weak.object_type.pointer_facts ==
             std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 16, 1}} &&
             std::holds_alternative<bir::FallbackGlobalName>(weak.identity) &&
             std::get<bir::FallbackGlobalName>(weak.identity).name ==
                 "weak_integer_pointer" &&
             !weak.is_internal && weak.is_weak && !weak.is_const &&
             !weak.is_extern_declaration &&
             weak.visibility == bir::SymbolVisibility::Protected &&
             weak.alignment == 32 && weak.initializer &&
             weak.initializer->opaque_payload == "ptr @weak" &&
             weak.initializer->function_links.size() == 2 &&
             view.spelling(weak.initializer->function_links[0]).value() ==
                 "pointer_init_a" &&
             view.spelling(weak.initializer->function_links[1]).value() ==
                 "pointer_init_b",
         "weak scalar-pointer definitions must preserve typed pointee, weak linkage, visibility, and ordered initializer links");
  expect(internal_const.object_type.kind == bir::TypeKind::Pointer &&
             internal_const.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Floating, 32, 1}} &&
             std::holds_alternative<bir::LinkNameId>(
                 internal_const.identity) &&
             view.spelling(std::get<bir::LinkNameId>(internal_const.identity))
                     .value() == "internal_const_float_pointer" &&
             internal_const.is_internal && !internal_const.is_weak &&
             internal_const.is_const &&
             !internal_const.is_extern_declaration &&
             internal_const.visibility == bir::SymbolVisibility::Protected &&
             internal_const.alignment == 64 && internal_const.initializer &&
             internal_const.initializer->opaque_payload ==
                 std::string{"ptr @internal_const\0tail", 24} &&
             internal_const.initializer->function_links.size() == 3 &&
             view.spelling(internal_const.initializer->function_links[0])
                     .value() == "pointer_init_b" &&
             view.spelling(internal_const.initializer->function_links[1])
                     .value() == "pointer_init_a" &&
             internal_const.initializer->function_links[2] ==
                 internal_const.initializer->function_links[0],
         "internal const-pointer definitions must preserve typed pointee, linkage, identity, visibility, alignment, opaque payload, and ordered links");
  expect(weak_const.object_type.kind == bir::TypeKind::Pointer &&
             weak_const.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 64, 1}} &&
             std::holds_alternative<bir::LinkNameId>(weak_const.identity) &&
             view.spelling(std::get<bir::LinkNameId>(weak_const.identity))
                     .value() == "weak_const_integer_pointer" &&
             !weak_const.is_internal && weak_const.is_weak &&
             weak_const.is_const && !weak_const.is_extern_declaration &&
             weak_const.visibility == bir::SymbolVisibility::Hidden &&
             weak_const.alignment == 8 && weak_const.initializer &&
             weak_const.initializer->opaque_payload ==
                 std::string{"ptr @weak_const\0tail", 20} &&
             weak_const.initializer->function_links.size() == 3 &&
             view.spelling(weak_const.initializer->function_links[0]).value() ==
                 "pointer_init_a" &&
             view.spelling(weak_const.initializer->function_links[1]).value() ==
                 "pointer_init_b" &&
             weak_const.initializer->function_links[2] ==
                 weak_const.initializer->function_links[0],
         "weak const-pointer definitions must preserve typed pointee, weak linkage, identity, visibility, alignment, opaque payload, and ordered links");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  const auto canonical_ids =
      canonical.has_value() ? canonical.value().view().global_objects()
                            : std::vector<bir::GlobalObjectId>{};
  expect(canonical.has_value() && canonical_ids.size() == 7 &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[0])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 64, 1}} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[1])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Floating, 32, 3}} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[2])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 32, 2}} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[3])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Floating, 64, 1}} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[4])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 16, 1}} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[5])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Floating, 32, 1}} &&
             canonical.value()
                     .view()
                     .global_object(canonical_ids[6])
                     .value()
                     .object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Integer, 64, 1}},
         "producer-shaped scalar-pointer declarations and definitions must publish Canonical BIR");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw.has_value() &&
               raw.error().code == bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto canonical_candidate =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_candidate.has_value() &&
               canonical_candidate.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected([](lir::LirModule& m) { m.globals[0].type.ptr_level = -1; },
           "negative scalar-pointer extern depth must remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.inner_rank = -2; },
           "scalar pointers reject invalid negative inner-rank residue");
  rejected([](lir::LirModule& m) { m.globals[0].type.inner_rank = 1; },
           "scalar pointers reject positive inner rank without split authority");
  rejected([](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_STRUCT; },
           "aggregate scalar-pointer pointees must remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.is_ptr_to_array = true;
        m.globals[0].type.inner_rank = 1;
      },
      "pointer-to-array and inner-rank extern shapes must remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].llvm_type = "i64*"; },
           "scalar-pointer rendered spelling is parity-only and must be ptr");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type_ref = lir::LirTypeRef::integer(64);
      },
      "optional scalar-pointer mirrors must corroborate generic ptr evidence");
  rejected([](lir::LirModule& m) { m.globals[2].init_text.clear(); },
           "scalar-pointer definitions require initializer payloads");
  rejected([](lir::LirModule& m) { m.globals[2].qualifier = "constant "; },
           "nonconst scalar-pointer definitions reject constant qualifiers");
  rejected([](lir::LirModule& m) { m.globals[3].is_internal = false; },
           "internal scalar-pointer linkage must agree with its flag");
  rejected([](lir::LirModule& m) { m.globals[5].qualifier = "constant "; },
           "internal const-pointer definitions reject constant qualifiers");
  rejected([](lir::LirModule& m) { m.globals[6].init_text.clear(); },
           "weak const-pointer definitions require initializer payloads");
  rejected([](lir::LirModule& m) { m.globals[5].is_internal = false; },
           "internal const-pointer linkage must agree with its flag");
  rejected([](lir::LirModule& m) { m.globals[5].linkage_vis = "weak "; },
           "internal and weak const-pointer facts must not coexist");
  rejected([](lir::LirModule& m) { m.globals[5].is_extern_decl = true; },
           "internal const-pointer definitions cannot also be extern declarations");
  rejected([](lir::LirModule& m) { m.globals[6].is_extern_decl = true; },
           "weak const-pointer definitions cannot contradict external linkage");
  rejected([](lir::LirModule& m) { m.globals[5].type.ptr_level = -2; },
           "negative internal const-pointer depth must remain closed");
  rejected([](lir::LirModule& m) { m.globals[6].type.base = c4c::TB_STRUCT; },
           "weak const-pointer aggregate pointees must remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[5].llvm_type_ref = lir::LirTypeRef::integer(32);
      },
      "internal const-pointer mirrors must corroborate generic ptr evidence");
  rejected([](lir::LirModule& m) { m.globals[6].llvm_type = "i64*"; },
           "weak const-pointer rendered spelling is parity-only and must be ptr");
}

void test_pointer_to_array_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto definition_link =
        module.link_names.intern("complex_pointer_to_array");
    const auto init_a = module.link_names.intern("pointer_array_init_a");
    const auto init_b = module.link_names.intern("pointer_array_init_b");

    lir::LirGlobal definition;
    definition.name = "complex_pointer_to_array";
    definition.link_name_id = definition_link;
    definition.type = scalar_type(c4c::TB_COMPLEX_DOUBLE);
    definition.type.ptr_level = 2;
    definition.type.array_rank = 1;
    definition.type.array_size = 4;
    definition.type.array_dims[0] = 4;
    definition.type.is_ptr_to_array = true;
    definition.type.inner_rank = -1;
    definition.linkage_vis = "protected ";
    definition.qualifier = "global ";
    definition.llvm_type = "ptr";
    definition.align_bytes = 16;
    definition.init_text = std::string{"ptr @matrix\0tail", 16};
    definition.initializer_function_link_name_ids = {init_b, init_a, init_b};
    module.globals.push_back(std::move(definition));

    lir::LirGlobal weak_external;
    weak_external.name = "vrm_pointer_to_array";
    weak_external.type = scalar_type(c4c::TB_VRM_REGISTER);
    weak_external.type.vrm_width = 4;
    weak_external.type.ptr_level = 1;
    weak_external.type.array_rank = 3;
    weak_external.type.array_size = 2;
    weak_external.type.array_dims[0] = 2;
    weak_external.type.array_dims[1] = 0;
    weak_external.type.array_dims[2] = 5;
    weak_external.type.is_ptr_to_array = true;
    weak_external.type.inner_rank = 3;
    weak_external.linkage_vis = "extern_weak hidden ";
    weak_external.qualifier = "global ";
    weak_external.llvm_type = "ptr";
    weak_external.align_bytes = 8;
    weak_external.is_extern_decl = true;
    module.globals.push_back(std::move(weak_external));
    return module;
  };

  auto module = valid_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value(),
         "producer-shaped pure pointer-to-array globals must import from TypeSpec authority");
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "typed pure pointer-to-array globals must be Foundation reachable");
  const auto view = raw.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 2 && ids[0].slot == 0 && ids[1].slot == 1,
         "pure pointer-to-array globals must preserve source order");
  const auto definition = view.global_object(ids[0]).value();
  const auto weak_external = view.global_object(ids[1]).value();
  expect(definition.object_type.kind == bir::TypeKind::Pointer &&
             definition.object_type.bit_width == 0 &&
             definition.object_type.spelling == "ptr" &&
             definition.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Complex, 64, 2,
                     bir::ComplexTypeFacts{bir::TypeKind::Floating, 64},
                     bir::PointerArrayTypeFacts{{4}, -1}}} &&
             std::holds_alternative<bir::LinkNameId>(definition.identity) &&
             view.spelling(std::get<bir::LinkNameId>(definition.identity))
                     .value() == "complex_pointer_to_array" &&
             !definition.is_internal && !definition.is_weak &&
             !definition.is_const && !definition.is_extern_declaration &&
             definition.visibility == bir::SymbolVisibility::Protected &&
             definition.alignment == 16 && definition.initializer &&
             definition.initializer->opaque_payload ==
                 std::string{"ptr @matrix\0tail", 16} &&
             definition.initializer->function_links.size() == 3 &&
             view.spelling(definition.initializer->function_links[0]).value() ==
                 "pointer_array_init_b" &&
             view.spelling(definition.initializer->function_links[1]).value() ==
                 "pointer_array_init_a" &&
             definition.initializer->function_links[2] ==
                 definition.initializer->function_links[0],
         "initialized complex pointer-to-array definitions must preserve exact nested type and object facts");
  expect(weak_external.object_type.kind == bir::TypeKind::Pointer &&
             weak_external.object_type.spelling == "ptr" &&
             weak_external.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::VrmRegister, 4, 1, std::nullopt,
                     bir::PointerArrayTypeFacts{{2, 0, 5}, 3}}} &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 weak_external.identity) &&
             weak_external.is_weak && weak_external.is_extern_declaration &&
             !weak_external.is_internal && !weak_external.is_const &&
             weak_external.visibility == bir::SymbolVisibility::Hidden &&
             weak_external.alignment == 8 && !weak_external.initializer,
         "weak VRM pointer-to-array externs must preserve zero dimensions, exact-rank form, and all object facts");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  const auto canonical_ids =
      canonical.has_value() ? canonical.value().view().global_objects()
                            : std::vector<bir::GlobalObjectId>{};
  expect(canonical.has_value() && canonical_ids.size() == 2 &&
             canonical.value().view().global_object(canonical_ids[0])
                     .value().object_type == definition.object_type &&
             canonical.value().view().global_object(canonical_ids[1])
                     .value().object_type == weak_external.object_type,
         "pure pointer-to-array globals must publish exact Canonical BIR facts");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto rejected_raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!rejected_raw.has_value() &&
               rejected_raw.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_size = -1;
        m.globals[0].type.array_dims[0] = -1;
      },
      "negative pointer-to-array dimensions must remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_size = -2;
        m.globals[0].type.array_dims[0] = -2;
      },
      "unsized pointer-to-array dimensions must remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.array_dims[0] = 3; },
           "pointer-to-array outer size and first dimension must agree");
  rejected([](lir::LirModule& m) { m.globals[1].type.inner_rank = 2; },
           "mixed pointer-array shapes require rendered outer array storage");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_ptr_to_array = false; },
           "ordinary pointers must reject residual array shape");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_fn_ptr = true; },
           "function-pointer-to-array shapes remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_lvalue_ref = true; },
           "reference pointer-to-array shapes remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.is_vector = true;
        m.globals[0].type.vector_lanes = 2;
        m.globals[0].type.vector_bytes = 32;
      },
      "vector pointer-to-array shapes remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_STRUCT; },
           "aggregate pointer-to-array pointees remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_VA_LIST; },
           "va-list pointer-to-array pointees remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type_ref = lir::LirTypeRef("ptr");
      },
      "producer-shaped pointer-to-array globals reject unexpected mirrors");
  rejected([](lir::LirModule& m) { m.globals[0].llvm_type = "[4 x ptr]"; },
           "pointer-to-array storage spelling remains opaque ptr");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_size_expr =
            reinterpret_cast<c4c::Node*>(static_cast<std::uintptr_t>(1));
      },
      "computed pointer-to-array bounds remain closed");
}

void test_mixed_pointer_array_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto definition_link =
        module.link_names.intern("complex_mixed_pointer_array");
    const auto init_a = module.link_names.intern("mixed_pointer_init_a");
    const auto init_b = module.link_names.intern("mixed_pointer_init_b");

    lir::LirGlobal definition;
    definition.name = "complex_mixed_pointer_array";
    definition.link_name_id = definition_link;
    definition.type = scalar_type(c4c::TB_COMPLEX_FLOAT);
    definition.type.ptr_level = 2;
    definition.type.array_rank = 4;
    definition.type.array_size = 2;
    definition.type.array_dims[0] = 2;
    definition.type.array_dims[1] = 0;
    definition.type.array_dims[2] = 3;
    definition.type.array_dims[3] = 4;
    definition.type.is_ptr_to_array = true;
    definition.type.inner_rank = 2;
    definition.linkage_vis = "protected ";
    definition.qualifier = "global ";
    definition.llvm_type = "[2 x [0 x ptr]]";
    definition.align_bytes = 32;
    definition.init_text = std::string{"mixed\0payload", 13};
    definition.initializer_function_link_name_ids = {init_b, init_a, init_b};
    module.globals.push_back(std::move(definition));

    lir::LirGlobal weak_external;
    weak_external.name = "vrm_mixed_pointer_array";
    weak_external.type = scalar_type(c4c::TB_VRM_REGISTER);
    weak_external.type.vrm_width = 4;
    weak_external.type.ptr_level = 3;
    weak_external.type.array_rank = 5;
    weak_external.type.array_size = 5;
    weak_external.type.array_dims[0] = 5;
    weak_external.type.array_dims[1] = 1;
    weak_external.type.array_dims[2] = 0;
    weak_external.type.array_dims[3] = 7;
    weak_external.type.array_dims[4] = 2;
    weak_external.type.is_ptr_to_array = true;
    weak_external.type.inner_rank = 3;
    weak_external.linkage_vis = "extern_weak hidden ";
    weak_external.qualifier = "global ";
    weak_external.llvm_type = "[5 x [1 x ptr]]";
    weak_external.align_bytes = 16;
    weak_external.is_extern_decl = true;
    module.globals.push_back(std::move(weak_external));
    return module;
  };

  auto module = valid_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value(),
         "producer-shaped mixed pointer-array globals must import from structured TypeSpec authority");
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "typed mixed pointer-array globals must be Foundation reachable");
  const auto view = raw.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 2 && ids[0].slot == 0 && ids[1].slot == 1,
         "mixed pointer-array globals must preserve source order");
  const auto definition = view.global_object(ids[0]).value();
  const auto weak_external = view.global_object(ids[1]).value();
  expect(definition.object_type.kind == bir::TypeKind::Array &&
             definition.object_type.bit_width == 0 &&
             definition.object_type.spelling == "[2 x [0 x ptr]]" &&
             definition.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Complex, 32, 2, {2, 0},
                     bir::ComplexTypeFacts{bir::TypeKind::Floating, 32},
                     bir::PointerArrayTypeFacts{{3, 4}, 2}}} &&
             std::holds_alternative<bir::LinkNameId>(definition.identity) &&
             view.spelling(std::get<bir::LinkNameId>(definition.identity))
                     .value() == "complex_mixed_pointer_array" &&
             !definition.is_internal && !definition.is_weak &&
             !definition.is_const && !definition.is_extern_declaration &&
             definition.visibility == bir::SymbolVisibility::Protected &&
             definition.alignment == 32 && definition.initializer &&
             definition.initializer->opaque_payload ==
                 std::string{"mixed\0payload", 13} &&
             definition.initializer->function_links.size() == 3 &&
             view.spelling(definition.initializer->function_links[0]).value() ==
                 "mixed_pointer_init_b" &&
             view.spelling(definition.initializer->function_links[1]).value() ==
                 "mixed_pointer_init_a" &&
             definition.initializer->function_links[2] ==
                 definition.initializer->function_links[0],
         "initialized complex mixed pointer-array definitions must preserve split dimensions and object facts");
  expect(weak_external.object_type.kind == bir::TypeKind::Array &&
             weak_external.object_type.spelling == "[5 x [1 x ptr]]" &&
             weak_external.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::VrmRegister, 4, 3, {5, 1}, std::nullopt,
                     bir::PointerArrayTypeFacts{{0, 7, 2}, 3}}} &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 weak_external.identity) &&
             weak_external.is_weak && weak_external.is_extern_declaration &&
             !weak_external.is_internal && !weak_external.is_const &&
             weak_external.visibility == bir::SymbolVisibility::Hidden &&
             weak_external.alignment == 16 && !weak_external.initializer,
         "weak VRM mixed pointer-array externs must preserve exact outer and hidden dimensions");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  const auto canonical_ids =
      canonical.has_value() ? canonical.value().view().global_objects()
                            : std::vector<bir::GlobalObjectId>{};
  expect(canonical.has_value() && canonical_ids.size() == 2 &&
             canonical.value().view().global_object(canonical_ids[0])
                     .value().object_type == definition.object_type &&
             canonical.value().view().global_object(canonical_ids[1])
                     .value().object_type == weak_external.object_type,
         "mixed pointer-array globals must publish exact Canonical BIR facts");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto rejected_raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!rejected_raw.has_value() &&
               rejected_raw.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected([](lir::LirModule& m) { m.globals[0].type.inner_rank = 0; },
           "mixed pointer-array shapes require a nonzero inner rank");
  rejected([](lir::LirModule& m) { m.globals[0].type.inner_rank = 4; },
           "all-inner pointer-to-array shapes require opaque pointer storage");
  rejected([](lir::LirModule& m) { m.globals[0].type.inner_rank = 5; },
           "mixed pointer-array inner rank cannot exceed total rank");
  rejected([](lir::LirModule& m) { m.globals[0].type.array_dims[0] = -1; },
           "mixed pointer-array outer dimensions must be nonnegative");
  rejected([](lir::LirModule& m) { m.globals[0].type.array_dims[3] = -1; },
           "mixed pointer-array hidden dimensions must be nonnegative");
  rejected([](lir::LirModule& m) { m.globals[0].type.array_size = 9; },
           "mixed pointer-array size must agree with the first outer dimension");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_ptr_to_array = false; },
           "ordinary array routes must reject residual mixed shape");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_fn_ptr = true; },
           "function-pointer mixed shapes remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_lvalue_ref = true; },
           "reference mixed shapes remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.is_vector = true;
        m.globals[0].type.vector_lanes = 2;
        m.globals[0].type.vector_bytes = 16;
      },
      "vector mixed shapes remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_STRUCT; },
           "aggregate mixed pointees remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_VA_LIST; },
           "va-list mixed pointees remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type_ref =
            lir::LirTypeRef("[2 x [0 x ptr]]");
      },
      "mixed pointer-array globals reject unexpected mirrors");
  rejected([](lir::LirModule& m) { m.globals[0].llvm_type = "[2 x ptr]"; },
           "mixed pointer-array spelling must preserve every outer dimension");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type = "[2 x [0 x [3 x [4 x ptr]]]]";
      },
      "mixed pointer-array spelling must hide pointee dimensions behind opaque ptr");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_size_expr =
            reinterpret_cast<c4c::Node*>(static_cast<std::uintptr_t>(1));
      },
      "computed mixed pointer-array bounds remain closed");
}

void test_fixed_scalar_base_array_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto array_link = module.link_names.intern("fixed_scalar_array");
    const auto init_a = module.link_names.intern("array_init_a");
    const auto init_b = module.link_names.intern("array_init_b");

    lir::LirGlobal definition;
    definition.name = "fixed_scalar_array";
    definition.link_name_id = array_link;
    definition.type = scalar_type(c4c::TB_SHORT);
    definition.type.array_rank = 1;
    definition.type.array_size = 0;
    definition.type.array_dims[0] = 0;
    definition.type.inner_rank = -1;
    definition.is_const = true;
    definition.linkage_vis = "weak protected ";
    definition.qualifier = "constant ";
    definition.llvm_type = "[0 x i16]";
    definition.init_text = std::string{"opaque\0array-payload", 20};
    definition.initializer_function_link_name_ids = {init_b, init_a, init_b};
    definition.align_bytes = 16;
    module.globals.push_back(std::move(definition));

    lir::LirGlobal declaration;
    declaration.name = "extern_scalar_array";
    declaration.type = scalar_type(c4c::TB_DOUBLE);
    declaration.type.array_rank = 1;
    declaration.type.array_size = 3;
    declaration.type.array_dims[0] = 3;
    declaration.linkage_vis = "external hidden ";
    declaration.qualifier = "global ";
    declaration.llvm_type = "[3 x double]";
    declaration.align_bytes = 8;
    declaration.is_extern_decl = true;
    module.globals.push_back(std::move(declaration));

    lir::LirGlobal multidimensional;
    multidimensional.name = "extern_multidimensional_scalar_array";
    multidimensional.type = scalar_type(c4c::TB_UINT);
    multidimensional.type.array_rank = 3;
    multidimensional.type.array_size = 2;
    multidimensional.type.array_dims[0] = 2;
    multidimensional.type.array_dims[1] = 3;
    multidimensional.type.array_dims[2] = 7;
    multidimensional.linkage_vis = "external protected ";
    multidimensional.qualifier = "global ";
    multidimensional.llvm_type = "[2 x [3 x [7 x i32]]]";
    multidimensional.align_bytes = 32;
    multidimensional.is_extern_decl = true;
    module.globals.push_back(std::move(multidimensional));

    lir::LirGlobal pointer_elements;
    pointer_elements.name = "extern_multidimensional_pointer_array";
    pointer_elements.type = scalar_type(c4c::TB_FLOAT);
    pointer_elements.type.ptr_level = 1;
    pointer_elements.type.array_rank = 2;
    pointer_elements.type.array_size = 4;
    pointer_elements.type.array_dims[0] = 4;
    pointer_elements.type.array_dims[1] = 0;
    pointer_elements.linkage_vis = "external hidden ";
    pointer_elements.qualifier = "global ";
    pointer_elements.llvm_type = "[4 x [0 x ptr]]";
    pointer_elements.align_bytes = 16;
    pointer_elements.is_extern_decl = true;
    module.globals.push_back(std::move(pointer_elements));

    lir::LirGlobal deep_pointer_elements;
    deep_pointer_elements.name = "extern_deep_pointer_array";
    deep_pointer_elements.type = scalar_type(c4c::TB_LONGLONG);
    deep_pointer_elements.type.ptr_level = 3;
    deep_pointer_elements.type.array_rank = 2;
    deep_pointer_elements.type.array_size = 6;
    deep_pointer_elements.type.array_dims[0] = 6;
    deep_pointer_elements.type.array_dims[1] = 3;
    deep_pointer_elements.linkage_vis = "external protected ";
    deep_pointer_elements.qualifier = "global ";
    deep_pointer_elements.llvm_type = "[6 x [3 x ptr]]";
    deep_pointer_elements.align_bytes = 64;
    deep_pointer_elements.is_extern_decl = true;
    module.globals.push_back(std::move(deep_pointer_elements));
    return module;
  };

  auto module = valid_module();
  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "producer-shaped fixed scalar-base array definitions and declarations must import");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "fixed scalar-base array global storage must be verifier reachable");
  const auto view = imported.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 5 && ids[0].slot == 0 && ids[1].slot == 1 &&
             ids[2].slot == 2 && ids[3].slot == 3 && ids[4].slot == 4,
         "fixed scalar-base arrays must preserve deterministic source order");
  const auto definition = view.global_object(ids[0]).value();
  const auto declaration = view.global_object(ids[1]).value();
  const auto multidimensional = view.global_object(ids[2]).value();
  const auto pointer_elements = view.global_object(ids[3]).value();
  const auto deep_pointer_elements = view.global_object(ids[4]).value();
  expect(definition.object_type.kind == bir::TypeKind::Array &&
             definition.object_type.bit_width == 0 &&
             definition.object_type.spelling == "[0 x i16]" &&
             definition.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Integer, 16, 0, {0}}} &&
             std::holds_alternative<bir::LinkNameId>(definition.identity) &&
             !definition.is_internal && definition.is_weak &&
             definition.is_const &&
             definition.visibility == bir::SymbolVisibility::Protected &&
             definition.alignment == 16 &&
             !definition.is_extern_declaration && definition.initializer &&
             definition.initializer->opaque_payload ==
                 std::string{"opaque\0array-payload", 20} &&
             definition.initializer->function_links.size() == 3 &&
             view.spelling(definition.initializer->function_links[0]).value() ==
                 "array_init_b" &&
             view.spelling(definition.initializer->function_links[1]).value() ==
                 "array_init_a" &&
             definition.initializer->function_links[2] ==
                 definition.initializer->function_links[0],
         "fixed scalar array definitions must preserve exact typed shape and all object facts");
  expect(declaration.object_type.kind == bir::TypeKind::Array &&
             declaration.object_type.spelling == "[3 x double]" &&
             declaration.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Floating, 64, 0, {3}}} &&
             declaration.is_extern_declaration &&
             declaration.visibility == bir::SymbolVisibility::Hidden &&
             !declaration.initializer,
         "extern fixed scalar arrays must use the same typed array path");
  expect(multidimensional.object_type.kind == bir::TypeKind::Array &&
             multidimensional.object_type.spelling ==
                 "[2 x [3 x [7 x i32]]]" &&
             multidimensional.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Integer, 32, 0, {2, 3, 7}}} &&
             multidimensional.is_extern_declaration &&
             multidimensional.visibility ==
                 bir::SymbolVisibility::Protected &&
             multidimensional.alignment == 32 &&
             !multidimensional.initializer,
         "multidimensional scalar arrays must retain outer-to-inner typed dimensions and object facts");
  expect(pointer_elements.object_type.kind == bir::TypeKind::Array &&
             pointer_elements.object_type.bit_width == 0 &&
             pointer_elements.object_type.spelling == "[4 x [0 x ptr]]" &&
             pointer_elements.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Floating, 32, 1, {4, 0}}} &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 pointer_elements.identity) &&
             std::get<bir::FallbackGlobalName>(pointer_elements.identity).name ==
                 "extern_multidimensional_pointer_array" &&
             pointer_elements.is_extern_declaration &&
             pointer_elements.visibility == bir::SymbolVisibility::Hidden &&
             pointer_elements.alignment == 16 &&
             !pointer_elements.initializer,
         "pointer-element arrays must retain scalar pointee facts, pointer depth, ordered dimensions, and object facts");
  expect(deep_pointer_elements.object_type.kind == bir::TypeKind::Array &&
             deep_pointer_elements.object_type.bit_width == 0 &&
             deep_pointer_elements.object_type.spelling ==
                 "[6 x [3 x ptr]]" &&
             deep_pointer_elements.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Integer, 64, 3, {6, 3}}} &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 deep_pointer_elements.identity) &&
             deep_pointer_elements.is_extern_declaration &&
             deep_pointer_elements.visibility ==
                 bir::SymbolVisibility::Protected &&
             deep_pointer_elements.alignment == 64 &&
             !deep_pointer_elements.initializer,
         "deep pointer-element arrays must retain exact scalar base, width, pointer depth, dimensions, opaque ptr parity, and object facts");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  const auto canonical_array_ids =
      canonical.has_value() ? canonical.value().view().global_objects()
                            : std::vector<bir::GlobalObjectId>{};
  expect(canonical.has_value() && canonical_array_ids.size() == 5 &&
             canonical.value()
                     .view()
                     .global_object(canonical_array_ids[0])
                     .value()
                     .object_type == definition.object_type &&
             canonical.value()
                     .view()
                     .global_object(canonical_array_ids[3])
                     .value()
                     .object_type == pointer_elements.object_type &&
             canonical.value()
                     .view()
                     .global_object(canonical_array_ids[4])
                     .value()
                     .object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Integer, 64, 3, {6, 3}}},
         "producer-shaped fixed scalar-base arrays must publish Canonical BIR");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw.has_value() &&
               raw.error().code == bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_size = -1;
        m.globals[0].type.array_dims[0] = -1;
        m.globals[0].llvm_type = "[-1 x i16]";
      },
      "negative outer array dimensions must remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_size = -2;
        m.globals[0].type.array_dims[0] = -2;
        m.globals[0].llvm_type = "[-2 x i16]";
      },
      "unsized outer array sentinels must remain unsupported");
  rejected(
      [](lir::LirModule& m) { m.globals[0].type.array_dims[0] = 6; },
      "array_size and outer dimension must agree");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_rank = 9;
      },
      "array rank must fit the producer dimension vector capacity");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_rank = 2;
        m.globals[0].type.array_dims[1] = -2;
        m.globals[0].llvm_type = "[0 x [-2 x i16]]";
      },
      "negative inner dimensions must remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_rank = 2;
        m.globals[0].type.array_dims[1] = -1;
        m.globals[0].llvm_type = "[0 x [-1 x i16]]";
      },
      "unsized inner array sentinels must remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[3].type.ptr_level = -1;
      },
      "negative pointer-element array depth must remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[3].type.base = c4c::TB_STRUCT;
      },
      "aggregate pointees in pointer-element arrays must remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[3].type.is_ptr_to_array = true;
        m.globals[3].type.inner_rank = 1;
      },
      "mixed pointer-array declarators must use only their split outer dimensions in storage spelling");
  rejected(
      [](lir::LirModule& m) { m.globals[3].type.inner_rank = 1; },
      "inner array rank without pointer-to-array authority must remain unsupported");
  rejected(
      [](lir::LirModule& m) { m.globals[0].type.inner_rank = -2; },
      "fixed arrays reject invalid negative inner-rank residue");
  rejected(
      [](lir::LirModule& m) { m.globals[0].type.inner_rank = 1; },
      "fixed arrays reject positive inner rank without split authority");
  rejected(
      [](lir::LirModule& m) {
        m.globals[3].llvm_type_ref = lir::LirTypeRef("[4 x [0 x ptr]]");
      },
      "producer-valid pointer-element arrays must not carry llvm_type_ref");
  rejected(
      [](lir::LirModule& m) {
        m.globals[3].llvm_type = "[4 x [0 x i32]]";
      },
      "pointer-element array LLVM spelling remains parity-only and must match opaque ptr nesting");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.base = c4c::TB_STRUCT;
        m.globals[0].llvm_type = "[0 x %struct.Payload]";
      },
      "aggregate element arrays remain outside this packet");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type_ref = lir::LirTypeRef("[0 x i16]");
      },
      "producer-valid fixed scalar arrays must not carry llvm_type_ref");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].llvm_type = "[2 x [7 x [3 x i32]]]";
      },
      "nested rendered array spelling must match ordered typed dimensions");
}

void test_direct_vector_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto vector_link = module.link_names.intern("integer_vector_global");
    const auto init_a = module.link_names.intern("vector_init_a");
    const auto init_b = module.link_names.intern("vector_init_b");

    lir::LirGlobal definition;
    definition.name = "integer_vector_global";
    definition.link_name_id = vector_link;
    definition.type = scalar_type(c4c::TB_SHORT);
    definition.type.is_vector = true;
    definition.type.vector_lanes = 4;
    definition.type.vector_bytes = 8;
    definition.type.inner_rank = -1;
    definition.linkage_vis = "protected ";
    definition.qualifier = "global ";
    definition.llvm_type = "<4 x i16>";
    definition.init_text =
        std::string{"<i16 1, i16 2, i16 3, i16 4>\0tail", 33};
    definition.initializer_function_link_name_ids = {init_b, init_a, init_b};
    definition.align_bytes = 8;
    module.globals.push_back(std::move(definition));

    lir::LirGlobal declaration;
    declaration.name = "floating_vector_extern";
    declaration.type = scalar_type(c4c::TB_DOUBLE);
    declaration.type.is_vector = true;
    declaration.type.vector_lanes = 2;
    declaration.type.vector_bytes = 16;
    declaration.linkage_vis = "external hidden ";
    declaration.qualifier = "global ";
    declaration.llvm_type = "<2 x double>";
    declaration.align_bytes = 16;
    declaration.is_extern_decl = true;
    module.globals.push_back(std::move(declaration));

    lir::LirGlobal pointer_definition;
    pointer_definition.name = "deep_integer_vector_pointer";
    pointer_definition.type = scalar_type(c4c::TB_INT);
    pointer_definition.type.is_vector = true;
    pointer_definition.type.vector_lanes = 8;
    pointer_definition.type.vector_bytes = 32;
    pointer_definition.type.ptr_level = 2;
    pointer_definition.linkage_vis = "internal hidden ";
    pointer_definition.qualifier = "global ";
    pointer_definition.llvm_type = "ptr";
    pointer_definition.init_text = "ptr null";
    pointer_definition.align_bytes = 8;
    pointer_definition.is_internal = true;
    module.globals.push_back(std::move(pointer_definition));

    lir::LirGlobal vector_array;
    vector_array.name = "floating_vector_array";
    vector_array.type = scalar_type(c4c::TB_FLOAT);
    vector_array.type.is_vector = true;
    vector_array.type.vector_lanes = 4;
    vector_array.type.vector_bytes = 16;
    vector_array.type.array_rank = 1;
    vector_array.type.array_size = 3;
    vector_array.type.array_dims[0] = 3;
    vector_array.linkage_vis = "external protected ";
    vector_array.qualifier = "global ";
    vector_array.llvm_type = "[3 x <4 x float>]";
    vector_array.align_bytes = 16;
    vector_array.is_extern_decl = true;
    module.globals.push_back(std::move(vector_array));

    lir::LirGlobal vector_pointer_array;
    vector_pointer_array.name = "weak_vector_pointer_array";
    vector_pointer_array.type = scalar_type(c4c::TB_USHORT);
    vector_pointer_array.type.is_vector = true;
    vector_pointer_array.type.vector_lanes = 2;
    vector_pointer_array.type.vector_bytes = 4;
    vector_pointer_array.type.ptr_level = 3;
    vector_pointer_array.type.array_rank = 2;
    vector_pointer_array.type.array_size = 2;
    vector_pointer_array.type.array_dims[0] = 2;
    vector_pointer_array.type.array_dims[1] = 0;
    vector_pointer_array.linkage_vis = "extern_weak hidden ";
    vector_pointer_array.qualifier = "global ";
    vector_pointer_array.llvm_type = "[2 x [0 x ptr]]";
    vector_pointer_array.align_bytes = 32;
    vector_pointer_array.is_extern_decl = true;
    module.globals.push_back(std::move(vector_pointer_array));
    return module;
  };

  auto module = valid_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value(),
         "producer-shaped direct integer and floating vector globals must import");
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "typed direct vector globals must remain Foundation-verifier reachable");
  const auto view = raw.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 5 && ids[0].slot == 0 && ids[1].slot == 1 &&
             ids[2].slot == 2 && ids[3].slot == 3 && ids[4].slot == 4,
         "direct vector globals must preserve source order and stable identity");
  const auto definition = view.global_object(ids[0]).value();
  const auto declaration = view.global_object(ids[1]).value();
  const auto pointer_definition = view.global_object(ids[2]).value();
  const auto vector_array = view.global_object(ids[3]).value();
  const auto vector_pointer_array = view.global_object(ids[4]).value();
  expect(definition.object_type.kind == bir::TypeKind::Vector &&
             definition.object_type.spelling == "<4 x i16>" &&
             definition.object_type.vector_facts ==
                 std::optional<bir::VectorTypeFacts>{bir::VectorTypeFacts{
                     bir::TypeKind::Integer, 16, 4, 8}} &&
             std::holds_alternative<bir::LinkNameId>(definition.identity) &&
             !definition.is_internal && !definition.is_weak &&
             !definition.is_const &&
             definition.visibility == bir::SymbolVisibility::Protected &&
             definition.alignment == 8 &&
             !definition.is_extern_declaration && definition.initializer &&
             definition.initializer->opaque_payload ==
                 std::string{"<i16 1, i16 2, i16 3, i16 4>\0tail", 33} &&
             definition.initializer->function_links.size() == 3 &&
             view.spelling(definition.initializer->function_links[0]).value() ==
                 "vector_init_b" &&
             view.spelling(definition.initializer->function_links[1]).value() ==
                 "vector_init_a" &&
             definition.initializer->function_links[2] ==
                 definition.initializer->function_links[0],
         "integer vector definitions must preserve exact typed authority and all object and initializer facts");
  expect(declaration.object_type.kind == bir::TypeKind::Vector &&
             declaration.object_type.spelling == "<2 x double>" &&
             declaration.object_type.vector_facts ==
                 std::optional<bir::VectorTypeFacts>{bir::VectorTypeFacts{
                     bir::TypeKind::Floating, 64, 2, 16}} &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 declaration.identity) &&
             declaration.visibility == bir::SymbolVisibility::Hidden &&
             declaration.alignment == 16 &&
             declaration.is_extern_declaration && !declaration.initializer,
         "floating vector externs must preserve typed element, lanes, storage, linkage, visibility, and alignment");
  expect(pointer_definition.object_type.kind == bir::TypeKind::Pointer &&
             pointer_definition.object_type.spelling == "ptr" &&
             pointer_definition.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Vector, 0, 2, std::nullopt, std::nullopt,
                     bir::VectorTypeFacts{bir::TypeKind::Integer, 32, 8,
                                          32}}} &&
             pointer_definition.is_internal &&
             pointer_definition.visibility == bir::SymbolVisibility::Hidden &&
             pointer_definition.alignment == 8 &&
             !pointer_definition.is_extern_declaration &&
             pointer_definition.initializer &&
             pointer_definition.initializer->opaque_payload == "ptr null",
         "initialized deep pointers to vectors must preserve nested typed vector authority and object facts");
  expect(vector_array.object_type.kind == bir::TypeKind::Array &&
             vector_array.object_type.spelling == "[3 x <4 x float>]" &&
             vector_array.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Vector, 0, 0, {3}, std::nullopt,
                     std::nullopt,
                     bir::VectorTypeFacts{bir::TypeKind::Floating, 32, 4,
                                          16}}} &&
             vector_array.is_extern_declaration &&
             vector_array.visibility == bir::SymbolVisibility::Protected &&
             vector_array.alignment == 16 && !vector_array.initializer,
         "fixed arrays of visible vectors must preserve nested component, lane, storage, dimension, and object facts");
  expect(vector_pointer_array.object_type.kind == bir::TypeKind::Array &&
             vector_pointer_array.object_type.spelling ==
                 "[2 x [0 x ptr]]" &&
             vector_pointer_array.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Vector, 0, 3, {2, 0}, std::nullopt,
                     std::nullopt,
                     bir::VectorTypeFacts{bir::TypeKind::Integer, 16, 2,
                                          4}}} &&
             vector_pointer_array.is_extern_declaration &&
             vector_pointer_array.is_weak &&
             vector_pointer_array.visibility ==
                 bir::SymbolVisibility::Hidden &&
             vector_pointer_array.alignment == 32 &&
             !vector_pointer_array.initializer,
         "multidimensional weak arrays of deep vector pointers must preserve opaque spelling and all nested typed facts");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "producer-shaped direct vector globals must publish Canonical BIR");
  const auto canonical_view = canonical.value().view();
  const auto canonical_ids = canonical_view.global_objects();
  expect(canonical_ids.size() == 5 &&
             canonical_view.global_object(canonical_ids[0])
                     .value()
                     .object_type.vector_facts ==
                 definition.object_type.vector_facts &&
             canonical_view.global_object(canonical_ids[0])
                     .value()
                     .initializer->opaque_payload ==
                 definition.initializer->opaque_payload &&
             canonical_view.global_object(canonical_ids[1])
                     .value()
                     .object_type.vector_facts ==
                 declaration.object_type.vector_facts &&
             canonical_view.global_object(canonical_ids[1])
                 .value()
                 .is_extern_declaration &&
             canonical_view.global_object(canonical_ids[2])
                     .value()
                     .object_type == pointer_definition.object_type &&
             canonical_view.global_object(canonical_ids[3])
                     .value()
                     .object_type == vector_array.object_type &&
             canonical_view.global_object(canonical_ids[4])
                     .value()
                     .object_type == vector_pointer_array.object_type,
         "Canonical BIR must retain ordered direct and nested vector authority and object facts");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto rejected_raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!rejected_raw.has_value() &&
               rejected_raw.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected([](lir::LirModule& m) { m.globals[0].type.vector_lanes = 0; },
           "zero vector lane counts must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].type.vector_lanes = -4; },
           "negative vector lane counts must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].type.vector_bytes = 0; },
           "zero vector storage bytes must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].type.vector_bytes = -8; },
           "negative vector storage bytes must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].llvm_type = "<8 x i16>"; },
           "vector spelling must exactly corroborate typed lane and element facts");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type_ref = lir::LirTypeRef("<4 x i16>");
      },
      "producer-valid direct vectors must not carry llvm_type_ref");
  rejected([](lir::LirModule& m) { m.globals[0].type.vrm_width = 2; },
           "VRM metadata must remain excluded from direct vector globals");
  rejected([](lir::LirModule& m) { m.globals[0].type.inner_rank = -2; },
           "direct vectors reject invalid negative inner-rank residue");
  rejected([](lir::LirModule& m) { m.globals[0].type.inner_rank = 1; },
           "direct vectors reject positive inner rank without split authority");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.is_vector = false;
        m.globals[0].llvm_type = "i16";
      },
      "residual vector lanes and storage must not fall through as scalar authority");
  rejected([](lir::LirModule& m) { m.globals[0].type.ptr_level = 1; },
           "pointer-to-vector opaque spelling must exactly corroborate its structured shape");
  rejected(
      [](lir::LirModule& m) { m.globals[0].type.is_lvalue_ref = true; },
      "vector reference shapes remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.array_rank = 1;
        m.globals[0].type.array_size = 2;
        m.globals[0].type.array_dims[0] = 2;
      },
      "array-of-vector visible spelling must exactly corroborate its structured shape");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_fn_ptr = true; },
           "function-pointer vector shapes remain unsupported");
  rejected(
      [](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_STRUCT; },
      "aggregate vector bases remain unsupported");
  rejected(
      [](lir::LirModule& m) { m.globals[2].type.base = c4c::TB_ENUM; },
      "enum vector pointee bases must remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].type.base = c4c::TB_COMPLEX_FLOAT;
      },
      "complex vector pointee bases must remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].type.base = c4c::TB_VRM_REGISTER;
        m.globals[2].type.vrm_width = 4;
      },
      "VRM vector pointee bases must remain unsupported");
  rejected(
      [](lir::LirModule& m) { m.globals[2].type.base = c4c::TB_VA_LIST; },
      "va-list vector pointee bases must remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].type.is_ptr_to_array = true;
        m.globals[2].type.array_rank = 1;
        m.globals[2].type.array_size = 3;
        m.globals[2].type.array_dims[0] = 3;
        m.globals[2].type.inner_rank = 1;
      },
      "pointer-to-array split vector shapes must remain unsupported");
  rejected(
      [](lir::LirModule& m) { m.globals[2].type.inner_rank = 1; },
      "residual inner rank must remain unsupported for vector pointers");
  rejected(
      [](lir::LirModule& m) { m.globals[2].type.is_fn_ptr = true; },
      "function-pointer vector shapes must remain unsupported");
  rejected(
      [](lir::LirModule& m) { m.globals[2].type.is_rvalue_ref = true; },
      "vector reference shapes must remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].llvm_type_ref = lir::LirTypeRef("ptr");
      },
      "producer-valid vector pointers must not carry an LLVM type mirror");
  rejected(
      [](lir::LirModule& m) { m.globals[3].type.array_dims[0] = -1; },
      "negative vector array dimensions must remain unsupported");
  rejected(
      [](lir::LirModule& m) { m.globals[3].type.array_dims[0] = 4; },
      "vector array front dimensions must match array_size");
  rejected(
      [](lir::LirModule& m) {
        m.globals[3].llvm_type_ref =
            lir::LirTypeRef("[3 x <4 x float>]");
      },
      "producer-valid vector arrays must not carry an LLVM type mirror");
  rejected(
      [](lir::LirModule& m) {
        m.globals[3].type.is_ptr_to_array = true;
        m.globals[3].type.inner_rank = 1;
      },
      "split pointer-to-array vector arrays must remain unsupported");
  rejected(
      [](lir::LirModule& m) {
        m.globals[4].llvm_type = "[2 x [0 x <2 x i16>]]";
      },
      "arrays of vector pointers must retain opaque ptr spelling");
}

void test_function_pointer_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto definition_link =
        module.link_names.intern("integer_function_pointer");
    const auto initializer_target = module.link_names.intern("inc");

    lir::LirGlobal definition;
    definition.name = "integer_function_pointer";
    definition.link_name_id = definition_link;
    definition.type = scalar_type(c4c::TB_INT);
    definition.type.is_fn_ptr = true;
    definition.type.ptr_level = 1;
    definition.type.inner_rank = -1;
    definition.linkage_vis = "protected ";
    definition.qualifier = "global ";
    definition.llvm_type = "ptr";
    definition.init_text = "ptr @inc";
    definition.initializer_function_link_name_ids = {initializer_target};
    definition.align_bytes = 8;
    module.globals.push_back(std::move(definition));

    lir::LirGlobal declaration;
    declaration.name = "deep_float_function_pointer";
    declaration.type = scalar_type(c4c::TB_DOUBLE);
    declaration.type.is_fn_ptr = true;
    declaration.type.ptr_level = 2;
    declaration.type.inner_rank = -1;
    declaration.linkage_vis = "extern_weak hidden ";
    declaration.qualifier = "global ";
    declaration.llvm_type = "ptr";
    declaration.align_bytes = 16;
    declaration.is_extern_decl = true;
    module.globals.push_back(std::move(declaration));

    lir::LirGlobal array;
    array.name = "void_function_pointer_array";
    array.type = scalar_type(c4c::TB_VOID);
    array.type.is_fn_ptr = true;
    array.type.ptr_level = 1;
    array.type.inner_rank = -1;
    array.type.array_rank = 2;
    array.type.array_size = 3;
    array.type.array_dims[0] = 3;
    array.type.array_dims[1] = 2;
    array.linkage_vis = "external protected ";
    array.qualifier = "global ";
    array.llvm_type = "[3 x [2 x ptr]]";
    array.align_bytes = 8;
    array.is_extern_decl = true;
    module.globals.push_back(std::move(array));
    return module;
  };

  auto module = valid_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "producer-shaped ordinary function-pointer globals must reach verified Raw BIR");
  const auto view = raw.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 3,
         "function-pointer definition, extern, and array must preserve order");
  const auto definition = view.global_object(ids[0]).value();
  const auto declaration = view.global_object(ids[1]).value();
  const auto array = view.global_object(ids[2]).value();
  const auto integer_return = bir::FunctionPointerTypeFacts{
      bir::TypeKind::Integer, 32};
  const auto floating_return = bir::FunctionPointerTypeFacts{
      bir::TypeKind::Floating, 64};
  const auto void_return = bir::FunctionPointerTypeFacts{
      bir::TypeKind::Void, 0};
  expect(definition.object_type.kind == bir::TypeKind::Pointer &&
             definition.object_type.spelling == "ptr" &&
             definition.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Function, 0, 1, std::nullopt,
                     std::nullopt, std::nullopt, std::nullopt,
                     integer_return}} &&
             std::holds_alternative<bir::LinkNameId>(definition.identity) &&
             !definition.is_weak && !definition.is_const &&
             !definition.is_internal && !definition.is_extern_declaration &&
             definition.visibility == bir::SymbolVisibility::Protected &&
             definition.alignment == 8 && definition.initializer &&
             definition.initializer->opaque_payload == "ptr @inc" &&
             definition.initializer->function_links.size() == 1 &&
             view.spelling(definition.initializer->function_links[0]).value() ==
                 "inc",
         "function-pointer definitions must preserve return, declarator, object, and semantic initializer facts");
  expect(declaration.object_type.pointer_facts ==
                 std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                     bir::TypeKind::Function, 0, 2, std::nullopt,
                     std::nullopt, std::nullopt, std::nullopt,
                     floating_return}} &&
             declaration.is_extern_declaration && declaration.is_weak &&
             !declaration.is_internal && !declaration.is_const &&
             declaration.visibility == bir::SymbolVisibility::Hidden &&
             declaration.alignment == 16 && !declaration.initializer,
         "deeper function-pointer externs must preserve return width, exact depth, and object facts");
  expect(array.object_type.kind == bir::TypeKind::Array &&
             array.object_type.spelling == "[3 x [2 x ptr]]" &&
             array.object_type.array_facts ==
                 std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                     bir::TypeKind::Function, 0, 1, {3, 2}, std::nullopt,
                     std::nullopt, std::nullopt, std::nullopt,
                     void_return}} &&
             array.is_extern_declaration && !array.is_weak &&
             array.visibility == bir::SymbolVisibility::Protected &&
             array.alignment == 8 && !array.initializer,
         "fixed function-pointer arrays must preserve opaque elements, return kind, and dimensions");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  const auto canonical_ids =
      canonical.has_value() ? canonical.value().view().global_objects()
                            : std::vector<bir::GlobalObjectId>{};
  expect(canonical.has_value() && canonical_ids.size() == 3 &&
             canonical.value().view().global_object(canonical_ids[0])
                     .value().object_type.pointer_facts ==
                 definition.object_type.pointer_facts &&
             canonical.value().view().global_object(canonical_ids[1])
                     .value().object_type.pointer_facts ==
                 declaration.object_type.pointer_facts &&
             canonical.value().view().global_object(canonical_ids[2])
                     .value().object_type.array_facts ==
                 array.object_type.array_facts,
         "function-pointer global facts must survive Canonical BIR publication");

  const auto verifier_rejects = [](std::string name, bir::Type type) {
    bir::ModuleBuilder builder;
    if (!builder.add_global_object(std::move(name), std::move(type), 8, false,
                                   false, false, true)
             .has_value())
      return false;
    const auto result = std::move(builder).publish();
    return !result.has_value() &&
           result.error().reason == bir::PublishError::VerificationFailed;
  };
  bir::Type missing_function_facts{bir::TypeKind::Pointer};
  missing_function_facts.pointer_facts =
      bir::PointerTypeFacts{bir::TypeKind::Function, 0, 1};
  expect(verifier_rejects("missing_function_facts",
                          std::move(missing_function_facts)),
         "verifier must reject function pointers without return facts");
  bir::Type data_pointer_masquerade{bir::TypeKind::Pointer};
  data_pointer_masquerade.pointer_facts = bir::PointerTypeFacts{
      bir::TypeKind::Integer, 32, 1, std::nullopt, std::nullopt,
      std::nullopt, std::nullopt, integer_return};
  expect(verifier_rejects("data_pointer_masquerade",
                          std::move(data_pointer_masquerade)),
         "verifier must reject data pointers carrying function-pointer facts");
  bir::Type incompatible_function_facts{bir::TypeKind::Pointer};
  incompatible_function_facts.pointer_facts = bir::PointerTypeFacts{
      bir::TypeKind::Function, 0, 1,
      bir::ComplexTypeFacts{bir::TypeKind::Floating, 32}, std::nullopt,
      std::nullopt, std::nullopt, integer_return};
  expect(verifier_rejects("incompatible_function_facts",
                          std::move(incompatible_function_facts)),
         "verifier must reject incompatible data and function-pointer facts");
  bir::Type missing_array_function_facts{bir::TypeKind::Array, 0,
                                         "[2 x ptr]"};
  missing_array_function_facts.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::Function, 0, 1, {2}};
  expect(verifier_rejects("missing_array_function_facts",
                          std::move(missing_array_function_facts)),
         "verifier must reject function-pointer arrays without return facts");
  bir::Type zero_array_function_depth{bir::TypeKind::Array, 0, "[2 x ptr]"};
  zero_array_function_depth.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::Function, 0, 0, {2}, std::nullopt, std::nullopt,
      std::nullopt, std::nullopt, integer_return};
  expect(verifier_rejects("zero_array_function_depth",
                          std::move(zero_array_function_depth)),
         "verifier must reject function-pointer arrays without declarator depth");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto rejected_raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!rejected_raw.has_value() &&
               rejected_raw.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type_ref = lir::LirTypeRef("ptr");
      },
      "function-pointer globals reject unexpected mirrors");
  rejected([](lir::LirModule& m) { m.globals[0].type.ptr_level = 0; },
           "bare function object shapes remain closed for global storage");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_lvalue_ref = true; },
           "function-pointer reference shapes remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_ptr_to_array = true; },
           "split pointer-to-array function shapes remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.inner_rank = 1; },
           "function-pointer globals reject residual positive inner rank");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.is_vector = true;
        m.globals[0].type.vector_lanes = 2;
        m.globals[0].type.vector_bytes = 16;
      },
      "function-pointer vector conflicts remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_STRUCT; },
           "aggregate-return function pointers remain closed without exact identity");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.base = c4c::TB_COMPLEX_INT;
      },
      "complex-return function pointers remain outside ordinary return facts");
  rejected([](lir::LirModule& m) { m.globals[2].type.array_dims[1] = -1; },
           "function-pointer arrays reject negative dimensions");
  rejected([](lir::LirModule& m) { m.globals[2].type.array_size = 4; },
           "function-pointer arrays require matching front dimensions");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].llvm_type_ref = lir::LirTypeRef("[3 x [2 x ptr]]");
      },
      "function-pointer arrays reject unexpected mirrors");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].type.array_size_expr =
            reinterpret_cast<c4c::Node*>(static_cast<std::uintptr_t>(1));
      },
      "computed function-pointer array bounds remain closed");
}

void test_va_list_global_receipt_and_rejections() {
  const auto make_module = [](bool pointer_object) {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    if (pointer_object) {
      module.target_profile.triple = "riscv64-unknown-linux-gnu";
      module.target_profile.arch = c4c::TargetArch::Riscv64;
      module.target_profile.os = c4c::TargetOs::Linux;
      module.target_profile.backend_abi = c4c::BackendAbiKind::RiscvLp64D;
    } else {
      module.target_profile.triple = "x86_64-unknown-linux-gnu";
      module.target_profile.arch = c4c::TargetArch::X86_64;
      module.target_profile.os = c4c::TargetOs::Linux;
      module.target_profile.backend_abi = c4c::BackendAbiKind::SysV_X86_64;
      const auto va_list_id =
          module.struct_names.intern("%struct.__va_list_tag_");
      module.record_struct_decl(lir::LirStructDecl{
          va_list_id,
          {{lir::LirTypeRef::integer(32)}, {lir::LirTypeRef::integer(32)},
           {lir::LirTypeRef("ptr")}, {lir::LirTypeRef("ptr")}}});
    }

    const std::string storage =
        pointer_object ? "ptr" : "%struct.__va_list_tag_";
    const int storage_alignment = pointer_object ? 8 : 16;
    const auto definition_link = module.link_names.intern("va_list_definition");
    const auto init_link = module.link_names.intern("va_list_initializer");

    lir::LirGlobal definition;
    definition.name = "va_list_definition";
    definition.link_name_id = definition_link;
    definition.type = scalar_type(c4c::TB_VA_LIST);
    definition.type.inner_rank = -1;
    definition.linkage_vis = "weak protected ";
    definition.qualifier = "constant ";
    definition.llvm_type = storage;
    definition.init_text = storage + " zeroinitializer";
    definition.initializer_function_link_name_ids = {init_link};
    definition.align_bytes = storage_alignment;
    definition.is_const = true;
    module.globals.push_back(std::move(definition));

    lir::LirGlobal declaration;
    declaration.name = "va_list_extern";
    declaration.type = scalar_type(c4c::TB_VA_LIST);
    declaration.linkage_vis = "external hidden ";
    declaration.qualifier = "global ";
    declaration.llvm_type = storage;
    declaration.align_bytes = storage_alignment;
    declaration.is_extern_decl = true;
    module.globals.push_back(std::move(declaration));

    lir::LirGlobal pointer;
    pointer.name = "va_list_deep_pointer";
    pointer.type = scalar_type(c4c::TB_VA_LIST);
    pointer.type.ptr_level = 3;
    pointer.type.inner_rank = -1;
    pointer.linkage_vis = "extern_weak protected ";
    pointer.qualifier = "global ";
    pointer.llvm_type = "ptr";
    pointer.llvm_type_ref = lir::LirTypeRef("ptr");
    pointer.align_bytes = 8;
    pointer.is_extern_decl = true;
    module.globals.push_back(std::move(pointer));

    lir::LirGlobal array;
    array.name = "va_list_fixed_array";
    array.type = scalar_type(c4c::TB_VA_LIST);
    array.type.array_rank = 2;
    array.type.array_size = 2;
    array.type.array_dims[0] = 2;
    array.type.array_dims[1] = 3;
    array.type.inner_rank = -1;
    array.linkage_vis = "external protected ";
    array.qualifier = "global ";
    array.llvm_type = "[2 x [3 x " + storage + "]]";
    array.align_bytes = storage_alignment;
    array.is_extern_decl = true;
    module.globals.push_back(std::move(array));

    lir::LirGlobal pointer_array;
    pointer_array.name = "va_list_pointer_array";
    pointer_array.type = scalar_type(c4c::TB_VA_LIST);
    pointer_array.type.ptr_level = 2;
    pointer_array.type.array_rank = 1;
    pointer_array.type.array_size = 4;
    pointer_array.type.array_dims[0] = 4;
    pointer_array.linkage_vis = "external hidden ";
    pointer_array.qualifier = "global ";
    pointer_array.llvm_type = "[4 x ptr]";
    pointer_array.align_bytes = 8;
    pointer_array.is_extern_decl = true;
    module.globals.push_back(std::move(pointer_array));
    return module;
  };

  const auto check_profile = [&](bool pointer_object) {
    auto module = make_module(pointer_object);
    const auto va_list_id = pointer_object
                                ? c4c::kInvalidStructName
                                : module.struct_names.find(
                                      "%struct.__va_list_tag_");
    const bir::VaListTypeFacts facts{
        pointer_object, static_cast<std::uint32_t>(pointer_object ? 8 : 24),
        static_cast<std::uint32_t>(pointer_object ? 8 : 16), va_list_id};
    const std::string storage =
        pointer_object ? "ptr" : "%struct.__va_list_tag_";

    const auto raw = bir::lower_lir_to_raw_bir(module);
    expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
           "target-shaped va-list globals must reach verified Raw BIR");
    const auto view = raw.value().view();
    const auto ids = view.global_objects();
    expect(ids.size() == 5,
           "va-list direct, pointer, and fixed-array globals must preserve order");
    const auto definition = view.global_object(ids[0]).value();
    const auto declaration = view.global_object(ids[1]).value();
    const auto pointer = view.global_object(ids[2]).value();
    const auto array = view.global_object(ids[3]).value();
    const auto pointer_array = view.global_object(ids[4]).value();
    bir::Type direct{bir::TypeKind::VaList, 0, storage};
    direct.va_list_facts = facts;
    expect(definition.object_type == direct &&
               std::holds_alternative<bir::LinkNameId>(definition.identity) &&
               view.spelling(std::get<bir::LinkNameId>(definition.identity))
                       .value() == "va_list_definition" &&
               definition.is_weak &&
               definition.is_const && !definition.is_internal &&
               !definition.is_extern_declaration &&
               definition.visibility == bir::SymbolVisibility::Protected &&
               definition.alignment == (pointer_object ? 8 : 16) &&
               definition.initializer &&
               definition.initializer->opaque_payload ==
                   storage + " zeroinitializer" &&
               definition.initializer->function_links.size() == 1 &&
               view.spelling(definition.initializer->function_links[0])
                       .value() == "va_list_initializer",
           "va-list definitions must preserve exact storage and object facts");
    expect(declaration.object_type == direct &&
               std::holds_alternative<bir::FallbackGlobalName>(
                   declaration.identity) &&
               declaration.is_extern_declaration && !declaration.is_weak &&
               !declaration.is_internal && !declaration.is_const &&
               declaration.visibility == bir::SymbolVisibility::Hidden &&
               declaration.alignment == (pointer_object ? 8 : 16) &&
               !declaration.initializer,
           "va-list externs must preserve exact storage and declaration facts");
    expect(pointer.object_type.pointer_facts ==
                   std::optional<bir::PointerTypeFacts>{bir::PointerTypeFacts{
                       bir::TypeKind::VaList, 0, 3, std::nullopt,
                       std::nullopt, std::nullopt, facts}} &&
               pointer.is_extern_declaration && pointer.is_weak &&
               !pointer.is_internal && !pointer.is_const &&
               pointer.visibility == bir::SymbolVisibility::Protected &&
               pointer.alignment == 8 && !pointer.initializer,
           "deep va-list pointers must preserve nested target facts and depth");
    expect(array.object_type.array_facts ==
                   std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                       bir::TypeKind::VaList, 0, 0, {2, 3}, std::nullopt,
                       std::nullopt, std::nullopt, facts}} &&
               array.object_type.spelling ==
                   "[2 x [3 x " + storage + "]]" &&
               array.is_extern_declaration &&
               !array.is_internal && !array.is_weak && !array.is_const &&
               array.visibility == bir::SymbolVisibility::Protected &&
               array.alignment == (pointer_object ? 8 : 16) &&
               !array.initializer,
           "fixed va-list arrays must preserve storage form and dimensions");
    expect(pointer_array.object_type.array_facts ==
                   std::optional<bir::ArrayTypeFacts>{bir::ArrayTypeFacts{
                       bir::TypeKind::VaList, 0, 2, {4}, std::nullopt,
                       std::nullopt, std::nullopt, facts}} &&
               pointer_array.object_type.spelling == "[4 x ptr]" &&
               pointer_array.is_extern_declaration &&
               !pointer_array.is_internal && !pointer_array.is_weak &&
               !pointer_array.is_const &&
               pointer_array.visibility == bir::SymbolVisibility::Hidden &&
               pointer_array.alignment == 8 && !pointer_array.initializer,
           "va-list pointer-element arrays must preserve nested facts and depth");

    const auto canonical = bir::lower_lir_to_canonical_bir(module);
    const auto canonical_ids =
        canonical.has_value() ? canonical.value().view().global_objects()
                              : std::vector<bir::GlobalObjectId>{};
    expect(canonical.has_value() && canonical_ids.size() == 5 &&
               canonical.value().view().global_object(canonical_ids[0])
                       .value().object_type == direct &&
               canonical.value().view().global_object(canonical_ids[2])
                       .value().object_type.pointer_facts ==
                   pointer.object_type.pointer_facts &&
               canonical.value().view().global_object(canonical_ids[3])
                       .value().object_type.array_facts ==
                   array.object_type.array_facts &&
               canonical.value().view().global_object(canonical_ids[4])
                       .value().object_type.array_facts ==
                   pointer_array.object_type.array_facts,
           "va-list storage facts must survive Canonical BIR publication");
  };
  check_profile(false);
  check_profile(true);

  {
    auto inconsistent_apple_profile = make_module(false);
    inconsistent_apple_profile.target_profile.triple =
        "aarch64-unknown-linux-gnu";
    inconsistent_apple_profile.target_profile.arch = c4c::TargetArch::Aarch64;
    inconsistent_apple_profile.target_profile.os = c4c::TargetOs::Darwin;
    inconsistent_apple_profile.target_profile.backend_abi =
        c4c::BackendAbiKind::Aapcs64;
    inconsistent_apple_profile.struct_decls[0].fields = {
        {lir::LirTypeRef("ptr")}, {lir::LirTypeRef("ptr")},
        {lir::LirTypeRef("ptr")}, {lir::LirTypeRef::integer(32)},
        {lir::LirTypeRef::integer(32)}};
    for (auto& global : inconsistent_apple_profile.globals)
      global.align_bytes = 8;
    const auto imported =
        bir::lower_lir_to_raw_bir(inconsistent_apple_profile);
    expect(imported.has_value(),
           "the canonical helper-selected struct-backed profile must import");
    const auto objects = imported.value().view().global_objects();
    expect(!objects.empty(),
           "the canonical helper profile must retain direct va-list storage");
    const auto direct =
        imported.value().view().global_object(objects[0]).value();
    expect(direct.object_type.va_list_facts ==
                   std::optional<bir::VaListTypeFacts>{bir::VaListTypeFacts{
                       false, 32, 8,
                       inconsistent_apple_profile.struct_names.find(
                           "%struct.__va_list_tag_")}},
           "va-list import must follow the canonical Apple target helper when typed profile fields and triple disagree");
  }

  const auto verifier_rejects = [](std::string name, bir::Type type) {
    bir::ModuleBuilder builder;
    if (!builder.add_global_object(std::move(name), std::move(type), 8, false,
                                   false, false, true)
             .has_value())
      return false;
    const auto result = std::move(builder).publish();
    return !result.has_value() &&
           result.error().reason == bir::PublishError::VerificationFailed;
  };
  const bir::VaListTypeFacts pointer_facts{true, 8, 8,
                                           c4c::kInvalidStructName};
  bir::Type facts_on_wrong_kind{bir::TypeKind::Integer, 32, "i32"};
  facts_on_wrong_kind.va_list_facts = pointer_facts;
  expect(verifier_rejects("va_facts_wrong_kind",
                          std::move(facts_on_wrong_kind)),
         "verifier must reject va-list facts on the wrong kind");
  expect(verifier_rejects("va_missing_facts",
                          bir::Type{bir::TypeKind::VaList, 0, "ptr"}),
         "verifier must reject direct va-list types without facts");
  bir::Type malformed_direct{bir::TypeKind::VaList, 0, "ptr"};
  malformed_direct.va_list_facts = bir::VaListTypeFacts{
      true, 24, 8, c4c::kInvalidStructName};
  expect(verifier_rejects("va_bad_storage", std::move(malformed_direct)),
         "verifier must reject inconsistent va-list form, size, and alignment");
  bir::Type missing_struct_identity{bir::TypeKind::VaList, 0,
                                    "%struct.__va_list_tag_"};
  missing_struct_identity.va_list_facts = bir::VaListTypeFacts{
      false, 24, 16, c4c::kInvalidStructName};
  expect(verifier_rejects("va_missing_struct_identity",
                          std::move(missing_struct_identity)),
         "verifier must reject struct-backed va-list facts without identity");
  bir::Type pointer_without_nested{bir::TypeKind::Pointer};
  pointer_without_nested.pointer_facts =
      bir::PointerTypeFacts{bir::TypeKind::VaList, 0, 2};
  expect(verifier_rejects("va_pointer_missing_nested",
                          std::move(pointer_without_nested)),
         "verifier must require nested va-list pointer facts");
  bir::Type array_without_nested{bir::TypeKind::Array, 0, "[2 x ptr]"};
  array_without_nested.array_facts =
      bir::ArrayTypeFacts{bir::TypeKind::VaList, 0, 1, {2}};
  expect(verifier_rejects("va_array_missing_nested",
                          std::move(array_without_nested)),
         "verifier must require nested va-list array facts");
  bir::Type incompatible_nested{bir::TypeKind::Array, 0, "[2 x ptr]"};
  incompatible_nested.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::VaList, 0, 1, {2}, std::nullopt, std::nullopt,
      bir::VectorTypeFacts{bir::TypeKind::Integer, 32, 4, 16}, pointer_facts};
  expect(verifier_rejects("va_array_incompatible_nested",
                          std::move(incompatible_nested)),
         "verifier must reject incompatible nested va-list/vector facts");
  bir::Type incompatible_pointer_nested{bir::TypeKind::Pointer};
  incompatible_pointer_nested.pointer_facts = bir::PointerTypeFacts{
      bir::TypeKind::VaList, 0, 1,
      bir::ComplexTypeFacts{bir::TypeKind::Floating, 64}, std::nullopt,
      std::nullopt, pointer_facts};
  expect(verifier_rejects("va_pointer_incompatible_nested",
                          std::move(incompatible_pointer_nested)),
         "verifier must reject incompatible nested va-list/complex facts");
  bir::Type incompatible_pointer_array_nested{bir::TypeKind::Array, 0,
                                               "[2 x ptr]"};
  incompatible_pointer_array_nested.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::VaList, 0, 1, {2}, std::nullopt,
      bir::PointerArrayTypeFacts{{3}, 1}, std::nullopt, pointer_facts};
  expect(verifier_rejects("va_array_pointer_array_nested",
                          std::move(incompatible_pointer_array_nested)),
         "verifier must reject incompatible nested va-list/pointer-array facts");
  const bir::VaListTypeFacts forged_struct_facts{false, 24, 16, 999};
  bir::Type forged_direct{bir::TypeKind::VaList, 0,
                          "%struct.__va_list_tag_"};
  forged_direct.va_list_facts = forged_struct_facts;
  expect(verifier_rejects("va_forged_direct_id", std::move(forged_direct)),
         "Foundation verifier must reject a forged direct va-list struct identity");
  bir::Type forged_pointer{bir::TypeKind::Pointer};
  forged_pointer.pointer_facts = bir::PointerTypeFacts{
      bir::TypeKind::VaList, 0, 2, std::nullopt, std::nullopt, std::nullopt,
      forged_struct_facts};
  expect(verifier_rejects("va_forged_pointer_id", std::move(forged_pointer)),
         "Foundation verifier must reject a forged nested va-list pointer identity");
  bir::Type forged_array{bir::TypeKind::Array, 0,
                         "[2 x %struct.__va_list_tag_]"};
  forged_array.array_facts = bir::ArrayTypeFacts{
      bir::TypeKind::VaList, 0, 0, {2}, std::nullopt, std::nullopt,
      std::nullopt, forged_struct_facts};
  expect(verifier_rejects("va_forged_array_id", std::move(forged_array)),
         "Foundation verifier must reject a forged nested va-list array identity");
  {
    bir::ModuleBuilder wrong_layout_builder;
    expect(wrong_layout_builder
               .add_struct_name(1, "%struct.__va_list_tag_")
               .has_value(),
           "staged va-list layout test must retain the canonical struct name");
    expect(wrong_layout_builder
               .add_struct_declaration(
                   1,
                   {{bir::Type{bir::TypeKind::Integer, 32, "i32"}},
                    {bir::Type{bir::TypeKind::Pointer}},
                    {bir::Type{bir::TypeKind::Pointer}},
                    {bir::Type{bir::TypeKind::Pointer}}},
                   false, false)
               .has_value(),
           "builder must retain a deliberately wrong staged va-list layout");
    bir::Type staged_va_list{bir::TypeKind::VaList, 0,
                             "%struct.__va_list_tag_"};
    staged_va_list.va_list_facts = bir::VaListTypeFacts{false, 24, 16, 1};
    expect(wrong_layout_builder
               .add_global_object("wrong_staged_va_list_layout",
                                  std::move(staged_va_list), 16, false, false,
                                  false, true)
               .has_value(),
           "builder must retain the staged va-list global for verifier diagnosis");
    const auto rejected_wrong_layout =
        std::move(wrong_layout_builder).publish();
    expect(!rejected_wrong_layout.has_value() &&
               rejected_wrong_layout.error().reason ==
                   bir::PublishError::VerificationFailed,
           "Foundation verifier must reject a resolving canonical va-list declaration with the wrong layout");
  }

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = make_module(false);
    mutate(candidate);
    expect(!bir::lower_lir_to_raw_bir(candidate).has_value(),
           message + " (Raw rollback)");
    expect(!bir::lower_lir_to_canonical_bir(candidate).has_value(),
           message + " (Canonical rollback)");
  };
  rejected([](lir::LirModule& m) { m.globals[0].type.inner_rank = -2; },
           "direct va-list globals reject invalid negative inner-rank residue");
  rejected([](lir::LirModule& m) { m.globals[2].type.inner_rank = 1; },
           "va-list pointers reject positive inner rank without split authority");
  rejected([](lir::LirModule& m) { m.globals[3].type.inner_rank = -2; },
           "va-list arrays reject invalid negative inner-rank residue");
  rejected(
      [](lir::LirModule& m) {
        m.target_profile.arch = c4c::TargetArch::Riscv64;
        m.target_profile.backend_abi = c4c::BackendAbiKind::RiscvLp64;
      },
      "target/profile and va-list storage form must agree");
  rejected(
      [](lir::LirModule& m) {
        m.struct_decls.clear();
        m.struct_decl_index.clear();
      },
      "struct-backed va-list storage requires a resolvable declaration");
  rejected(
      [](lir::LirModule& m) {
        const auto wrong = m.struct_names.intern("%struct.wrong_va_list");
        m.struct_decls[0].name_id = wrong;
        m.struct_decl_index.clear();
        m.struct_decl_index.emplace(wrong, 0);
      },
      "struct-backed va-list storage rejects the wrong declaration identity");
  rejected(
      [](lir::LirModule& m) { m.struct_decls[0].is_packed = true; },
      "struct-backed va-list storage rejects packed declarations");
  rejected(
      [](lir::LirModule& m) { m.struct_decls[0].is_opaque = true; },
      "struct-backed va-list storage rejects opaque declarations");
  rejected(
      [](lir::LirModule& m) {
        m.struct_decls[0].fields = {
            {lir::LirTypeRef::integer(32)}, {lir::LirTypeRef("ptr")},
            {lir::LirTypeRef("ptr")}, {lir::LirTypeRef("ptr")}};
      },
      "struct-backed va-list storage rejects wrong producer layouts");
  rejected([](lir::LirModule& m) { m.globals[0].llvm_type = "ptr"; },
           "direct va-list spelling must match target storage");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type_ref =
            lir::LirTypeRef("%struct.__va_list_tag_");
      },
      "direct va-list storage rejects unexpected mirrors");
  rejected([](lir::LirModule& m) { m.globals[0].type.vrm_width = 1; },
           "va-list storage rejects residual VRM facts");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.is_vector = true;
        m.globals[0].type.vector_lanes = 2;
        m.globals[0].type.vector_bytes = 16;
      },
      "va-list storage rejects residual vector facts");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.enum_underlying_base = c4c::TB_INT;
      },
      "va-list storage rejects residual enum facts");
  rejected([](lir::LirModule& m) { m.globals[2].type.is_fn_ptr = true; },
           "va-list function pointers remain closed");
  rejected([](lir::LirModule& m) { m.globals[0].type.is_lvalue_ref = true; },
           "va-list references remain closed");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].type.is_ptr_to_array = true;
        m.globals[2].type.array_rank = 1;
        m.globals[2].type.array_size = 2;
        m.globals[2].type.array_dims[0] = 2;
        m.globals[2].type.inner_rank = 1;
      },
      "va-list pointer-to-array shapes remain closed");
  rejected([](lir::LirModule& m) { m.globals[3].type.array_dims[1] = -1; },
           "va-list fixed arrays reject negative dimensions");
  rejected([](lir::LirModule& m) { m.globals[3].type.array_size = 3; },
           "va-list fixed arrays require matching front dimensions");
}

void test_named_aggregate_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());

    const auto pair_id = module.struct_names.intern("%struct.PairGlobal");
    const auto choice_id = module.struct_names.intern("%struct.ChoiceGlobal");
    module.record_struct_decl(lir::LirStructDecl{
        pair_id,
        {{lir::LirTypeRef::integer(32)}, {lir::LirTypeRef("double")}}});
    module.record_struct_decl(lir::LirStructDecl{
        choice_id,
        {{lir::LirTypeRef::integer(64)}, {lir::LirTypeRef("double")}}});

    const auto pair_link = module.link_names.intern("named_pair_global");
    const auto extern_pair_link =
        module.link_names.intern("extern_named_pair_global");
    const auto init_a = module.link_names.intern("aggregate_init_a");
    const auto init_b = module.link_names.intern("aggregate_init_b");

    lir::LirGlobal pair;
    pair.name = "named_pair_global";
    pair.link_name_id = pair_link;
    pair.type = scalar_type(c4c::TB_STRUCT);
    pair.type.inner_rank = -1;
    pair.linkage_vis = "protected ";
    pair.qualifier = "global ";
    pair.llvm_type = "%struct.PairGlobal";
    pair.llvm_type_ref =
        lir::LirTypeRef::struct_type(pair.llvm_type, pair_id);
    pair.init_text = std::string{"{ i32 7, double 2.5 }\0tail", 26};
    pair.initializer_function_link_name_ids = {init_b, init_a, init_b};
    pair.align_bytes = 8;
    module.globals.push_back(std::move(pair));

    lir::LirGlobal choice;
    choice.name = "named_choice_global";
    choice.type = scalar_type(c4c::TB_UNION);
    choice.is_internal = true;
    choice.is_const = true;
    choice.linkage_vis = "internal hidden ";
    choice.qualifier = "constant ";
    choice.llvm_type = "%struct.ChoiceGlobal";
    choice.llvm_type_ref =
        lir::LirTypeRef::union_type(choice.llvm_type, choice_id);
    choice.init_text = "{ i64 11 }";
    choice.initializer_function_link_name_ids = {init_a, init_b};
    choice.align_bytes = 16;
    module.globals.push_back(std::move(choice));

    lir::LirGlobal extern_pair;
    extern_pair.name = "extern_named_pair_global";
    extern_pair.link_name_id = extern_pair_link;
    extern_pair.type = scalar_type(c4c::TB_STRUCT);
    extern_pair.linkage_vis = "external hidden ";
    extern_pair.qualifier = "global ";
    extern_pair.llvm_type = "%struct.PairGlobal";
    extern_pair.llvm_type_ref =
        lir::LirTypeRef::struct_type(extern_pair.llvm_type, pair_id);
    extern_pair.align_bytes = 32;
    extern_pair.is_extern_decl = true;
    module.globals.push_back(std::move(extern_pair));

    lir::LirGlobal weak_extern_choice;
    weak_extern_choice.name = "weak_extern_named_choice_global";
    weak_extern_choice.type = scalar_type(c4c::TB_UNION);
    weak_extern_choice.linkage_vis = "extern_weak protected ";
    weak_extern_choice.qualifier = "global ";
    weak_extern_choice.llvm_type = "%struct.ChoiceGlobal";
    weak_extern_choice.llvm_type_ref = lir::LirTypeRef::union_type(
        weak_extern_choice.llvm_type, choice_id);
    weak_extern_choice.align_bytes = 64;
    weak_extern_choice.is_extern_decl = true;
    module.globals.push_back(std::move(weak_extern_choice));
    return module;
  };

  auto module = valid_module();
  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "producer-shaped named struct and union definitions must import");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "named aggregate global storage must be verifier reachable");
  const auto view = imported.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 4,
         "named aggregate globals must preserve deterministic source order");
  const auto pair = view.global_object(ids[0]).value();
  const auto choice = view.global_object(ids[1]).value();
  const auto extern_pair = view.global_object(ids[2]).value();
  const auto weak_extern_choice = view.global_object(ids[3]).value();
  expect(pair.object_type.kind == bir::TypeKind::Struct &&
             pair.object_type.struct_name_id ==
                 module.globals[0].llvm_type_ref->struct_name_id() &&
             pair.object_type.spelling == "%struct.PairGlobal" &&
             std::holds_alternative<bir::LinkNameId>(pair.identity) &&
             !pair.is_internal && !pair.is_weak && !pair.is_const &&
             pair.visibility == bir::SymbolVisibility::Protected &&
             pair.alignment == 8 && !pair.is_extern_declaration &&
             pair.initializer &&
             pair.initializer->opaque_payload ==
                 std::string{"{ i32 7, double 2.5 }\0tail", 26} &&
             pair.initializer->function_links.size() == 3 &&
             view.spelling(pair.initializer->function_links[0]).value() ==
                 "aggregate_init_b" &&
             view.spelling(pair.initializer->function_links[1]).value() ==
                 "aggregate_init_a" &&
             pair.initializer->function_links[2] ==
                 pair.initializer->function_links[0],
         "named struct globals must preserve semantic type identity and all object facts");
  expect(choice.object_type.kind == bir::TypeKind::Struct &&
             choice.object_type.struct_name_id ==
                 module.globals[1].llvm_type_ref->struct_name_id() &&
             choice.object_type.spelling == "%struct.ChoiceGlobal" &&
             std::holds_alternative<bir::FallbackGlobalName>(choice.identity) &&
             choice.is_internal && !choice.is_weak && choice.is_const &&
             choice.visibility == bir::SymbolVisibility::Hidden &&
             choice.alignment == 16 && !choice.is_extern_declaration &&
             choice.initializer &&
             choice.initializer->opaque_payload == "{ i64 11 }" &&
             choice.initializer->function_links.size() == 2 &&
             view.spelling(choice.initializer->function_links[0]).value() ==
                 "aggregate_init_a" &&
             view.spelling(choice.initializer->function_links[1]).value() ==
                 "aggregate_init_b",
         "named union globals must preserve Struct identity, const linkage facts, and ordered initializer links");
  expect(extern_pair.object_type.kind == bir::TypeKind::Struct &&
             extern_pair.object_type.struct_name_id ==
                 module.globals[2].llvm_type_ref->struct_name_id() &&
             extern_pair.object_type.spelling == "%struct.PairGlobal" &&
             std::holds_alternative<bir::LinkNameId>(extern_pair.identity) &&
             !extern_pair.is_internal && !extern_pair.is_weak &&
             !extern_pair.is_const &&
             extern_pair.visibility == bir::SymbolVisibility::Hidden &&
             extern_pair.alignment == 32 &&
             extern_pair.is_extern_declaration && !extern_pair.initializer,
         "named struct externs must preserve link-backed identity, structured authority, visibility, alignment, and no initializer");
  expect(weak_extern_choice.object_type.kind == bir::TypeKind::Struct &&
             weak_extern_choice.object_type.struct_name_id ==
                 module.globals[3].llvm_type_ref->struct_name_id() &&
             weak_extern_choice.object_type.spelling ==
                 "%struct.ChoiceGlobal" &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 weak_extern_choice.identity) &&
             std::get<bir::FallbackGlobalName>(weak_extern_choice.identity)
                     .name == "weak_extern_named_choice_global" &&
             !weak_extern_choice.is_internal && weak_extern_choice.is_weak &&
             !weak_extern_choice.is_const &&
             weak_extern_choice.visibility ==
                 bir::SymbolVisibility::Protected &&
             weak_extern_choice.alignment == 64 &&
             weak_extern_choice.is_extern_declaration &&
             !weak_extern_choice.initializer,
         "named union weak externs must preserve fallback identity, structured authority, visibility, alignment, and no initializer");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "producer-shaped named aggregate definitions and externs must publish verified Canonical BIR");
  const auto canonical_view = canonical.value().view();
  const auto canonical_ids = canonical_view.global_objects();
  expect(canonical_ids.size() == 4 &&
             canonical_view.global_object(canonical_ids[2])
                     .value()
                     .object_type.struct_name_id ==
                 module.globals[2].llvm_type_ref->struct_name_id() &&
             canonical_view.global_object(canonical_ids[2])
                 .value()
                 .is_extern_declaration &&
             !canonical_view.global_object(canonical_ids[2])
                  .value()
                  .initializer &&
             canonical_view.global_object(canonical_ids[3])
                     .value()
                     .object_type.struct_name_id ==
                 module.globals[3].llvm_type_ref->struct_name_id() &&
             canonical_view.global_object(canonical_ids[3]).value().is_weak &&
             canonical_view.global_object(canonical_ids[3])
                 .value()
                 .is_extern_declaration &&
             !canonical_view.global_object(canonical_ids[3])
                  .value()
                  .initializer,
         "Canonical BIR must retain ordered named aggregate extern authority and declaration facts");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto rejected_module = valid_module();
    mutate(rejected_module);
    const auto raw = bir::lower_lir_to_raw_bir(rejected_module);
    expect(!raw.has_value() &&
               raw.error().code == bir::ImportErrorCode::UnsupportedGlobals,
           message);
    const auto canonical = bir::lower_lir_to_canonical_bir(rejected_module);
    expect(!canonical.has_value() &&
               canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (canonical rollback)");
  };
  rejected([](lir::LirModule& m) { m.globals[0].type.inner_rank = -2; },
           "direct aggregates reject invalid negative inner-rank residue");
  rejected([](lir::LirModule& m) { m.globals[0].type.inner_rank = 1; },
           "direct aggregates reject positive inner rank without split authority");
  rejected([](lir::LirModule& m) { m.globals[2].llvm_type_ref.reset(); },
           "named aggregate externs require structured type identity transactionally");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].llvm_type_ref = lir::LirTypeRef::struct_type(
            "%struct.UnresolvedGlobal", static_cast<c4c::StructNameId>(999));
        m.globals[2].llvm_type = "%struct.UnresolvedGlobal";
      },
      "unresolved aggregate extern StructNameId authority must reject transactionally");
  rejected(
      [](lir::LirModule& m) {
        const auto choice_id = m.globals[1].llvm_type_ref->struct_name_id();
        m.globals[2].llvm_type_ref =
            lir::LirTypeRef::struct_type("%struct.PairGlobal", choice_id);
      },
      "mismatched aggregate extern spelling and StructNameId must reject transactionally");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].llvm_type = "%struct.ChoiceGlobal";
      },
      "aggregate extern rendered spelling must match its structured authority transactionally");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].type.base = c4c::TB_UNION;
        m.globals[0].llvm_type = "{ i32, [3 x i8] }";
        m.globals[0].llvm_type_ref = lir::LirTypeRef(m.globals[0].llvm_type);
      },
      "literal union globals must remain closed transactionally");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].type.base = c4c::TB_STRUCT;
        m.globals[2].llvm_type = "{ i32, [3 x i8] }";
        m.globals[2].llvm_type_ref =
            lir::LirTypeRef(m.globals[2].llvm_type);
      },
      "unkeyed literal aggregate extern declarations must remain closed transactionally");
  rejected([](lir::LirModule& m) { m.globals[2].init_text = "zeroinitializer"; },
           "named aggregate extern declarations cannot carry initializer payloads");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].initializer_function_link_name_ids = {
            m.globals[0].initializer_function_link_name_ids[0]};
      },
      "named aggregate extern declarations cannot carry initializer links");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].is_const = true;
        m.globals[2].qualifier = "constant ";
      },
      "named aggregate extern declarations require the producer global qualifier");
  rejected([](lir::LirModule& m) { m.globals[2].is_internal = true; },
           "named aggregate extern declarations cannot contradict external linkage with internal flags");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].type.ptr_level = 1;
        m.globals[2].llvm_type = "ptr";
        m.globals[2].llvm_type_ref = lir::LirTypeRef("ptr");
      },
      "pointer-to-aggregate extern shapes must remain closed transactionally");
  rejected(
      [](lir::LirModule& m) {
        m.globals[2].type.array_rank = 1;
        m.globals[2].type.array_size = 1;
        m.globals[2].type.array_dims[0] = 1;
        m.globals[2].llvm_type = "[1 x %struct.PairGlobal]";
        m.globals[2].llvm_type_ref = lir::LirTypeRef(m.globals[2].llvm_type);
      },
      "array aggregate extern shapes must remain closed transactionally");
}

void test_aggregate_store_backed_structured_receiver() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());

    const auto pair_id =
        module.struct_names.intern("%struct.StoreBackedPair");
    lir::LirStructDecl pair_decl;
    pair_decl.name_id = pair_id;
    pair_decl.fields = {{lir::LirTypeRef::integer(32)},
                        {lir::LirTypeRef("double")}};
    module.record_struct_decl(pair_decl);

    lir::LirAggregateStoreEntry pair_entry;
    pair_entry.name_id = pair_id;
    pair_entry.fields = pair_decl.fields;
    pair_entry.is_packed = pair_decl.is_packed;
    pair_entry.is_opaque = pair_decl.is_opaque;
    pair_entry.layout_kind = lir::LirAggregateLayoutKind::Direct;
    module.aggregate_store.push_back(std::move(pair_entry));

    const auto stale_id =
        module.struct_names.intern("%struct.DeclarationOnlyShadow");
    module.record_struct_decl(lir::LirStructDecl{
        stale_id,
        {{lir::LirTypeRef::integer(64)}}});

    const auto pair_link = module.link_names.intern("store_backed_pair");
    lir::LirGlobal pair;
    pair.name = "store_backed_pair";
    pair.link_name_id = pair_link;
    pair.type = scalar_type(c4c::TB_STRUCT);
    pair.type.inner_rank = -1;
    pair.linkage_vis = "protected ";
    pair.qualifier = "global ";
    pair.llvm_type = "%struct.StoreBackedPair";
    pair.llvm_type_ref = lir::LirTypeRef::struct_type(pair.llvm_type, pair_id);
    pair.init_text = "{ i32 13, double 4.5 }";
    pair.align_bytes = 8;
    module.globals.push_back(std::move(pair));
    return module;
  };

  auto module = valid_module();
  const auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "store-backed aggregate receiver must import valid canonical store facts");
  const auto view = imported.value().view();
  const auto struct_names = view.struct_names();
  const auto struct_decls = view.struct_declarations();
  expect(struct_decls.size() == 1,
         "populated aggregate store must publish one structured declaration");
  const auto pair_decl = view.struct_declaration(struct_decls[0]).value();
  expect(view.spelling(pair_decl.name).value() ==
                 "%struct.StoreBackedPair",
         "populated aggregate store must be the BIR structured declaration authority");
  expect(pair_decl.name == struct_names[0] && !pair_decl.is_packed &&
             !pair_decl.is_opaque && pair_decl.fields.size() == 2 &&
             pair_decl.fields[0].type == bir::Type{bir::TypeKind::I32} &&
             pair_decl.fields[1].type == bir::Type{bir::TypeKind::F64},
         "BIR receiver must spell layout from canonical aggregate store declaration facts");
  const auto globals = view.global_objects();
  expect(globals.size() == 1 &&
             view.global_object(globals[0]).value().object_type.struct_name_id ==
                 module.aggregate_store[0].name_id,
         "store-backed global import must preserve the canonical struct identity");
  expect(bir::lower_lir_to_canonical_bir(module).has_value(),
         "valid store-backed aggregate receipt must canonicalize");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    expect(!bir::lower_lir_to_raw_bir(candidate).has_value(),
           message + " (Raw rollback)");
    expect(!bir::lower_lir_to_canonical_bir(candidate).has_value(),
           message + " (Canonical rollback)");
  };
  rejected(
      [](lir::LirModule& m) {
        m.aggregate_store[0].name_id =
            m.struct_names.intern("%struct.MissingStoreDecl");
      },
      "store-backed receiver rejects missing declaration facts before fallback");
  rejected(
      [](lir::LirModule& m) {
        m.aggregate_store[0].fields = {{lir::LirTypeRef::integer(64)}};
      },
      "store-backed receiver rejects incoherent recorded field facts");
  rejected(
      [](lir::LirModule& m) { m.aggregate_store[0].is_packed = true; },
      "store-backed receiver rejects incoherent packed facts");
  rejected(
      [](lir::LirModule& m) {
        m.aggregate_store[0].is_union = true;
        m.aggregate_store[0].layout_kind = lir::LirAggregateLayoutKind::Direct;
      },
      "store-backed receiver rejects incoherent aggregate kind facts");
}

void test_flexible_member_literal_struct_global_receipt_and_rejections() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());

    const auto object_link =
        module.link_names.intern("flexible_member_global");
    const auto init_a = module.link_names.intern("flexible_init_a");
    const auto init_b = module.link_names.intern("flexible_init_b");

    lir::LirGlobal global;
    global.name = "flexible_member_global";
    global.link_name_id = object_link;
    global.type = scalar_type(c4c::TB_STRUCT);
    global.is_const = true;
    global.linkage_vis = "weak protected ";
    global.qualifier = "constant ";
    global.llvm_type = "{ i32, [5 x i8] }";
    global.llvm_type_ref = lir::LirTypeRef(global.llvm_type);
    global.init_text = std::string{"opaque\0flexible-payload", 23};
    global.initializer_function_link_name_ids = {init_b, init_a, init_b};
    global.align_bytes = 16;
    module.globals.push_back(std::move(global));
    return module;
  };

  auto module = valid_module();
  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "producer-shaped flexible-member literal struct globals must import");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "literal struct global storage must be verifier reachable");
  const auto view = imported.value().view();
  const auto ids = view.global_objects();
  expect(ids.size() == 1 && ids[0].slot == 0,
         "literal struct globals must preserve deterministic source order");
  const auto global = view.global_object(ids[0]).value();
  expect(global.object_type.kind == bir::TypeKind::Struct &&
             global.object_type.struct_name_id == c4c::kInvalidStructName &&
             global.object_type.spelling == "{ i32, [5 x i8] }" &&
             std::holds_alternative<bir::LinkNameId>(global.identity) &&
             std::get<bir::LinkNameId>(global.identity).slot == 0 &&
             !global.is_internal && global.is_weak && global.is_const &&
             global.visibility == bir::SymbolVisibility::Protected &&
             global.alignment == 16 && !global.is_extern_declaration &&
             global.initializer &&
             global.initializer->opaque_payload ==
                 std::string{"opaque\0flexible-payload", 23} &&
             global.initializer->function_links.size() == 3 &&
             view.spelling(global.initializer->function_links[0]).value() ==
                 "flexible_init_b" &&
             view.spelling(global.initializer->function_links[1]).value() ==
                 "flexible_init_a" &&
             global.initializer->function_links[2] ==
                 global.initializer->function_links[0],
         "literal struct globals must preserve byte-exact typed spelling and all object facts");

  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value() &&
             canonical.value().view().global_objects().size() == 1,
         "producer-shaped literal struct globals must publish Canonical BIR");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw.has_value() &&
               raw.error().code == bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (Canonical rollback)");
  };
  rejected([](lir::LirModule& m) { m.globals[0].type.base = c4c::TB_UNION; },
           "a literal union mirror must remain unsupported");
  rejected([](lir::LirModule& m) { m.globals[0].llvm_type_ref.reset(); },
           "literal struct globals require their typed mirror");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].llvm_type = "{ i32, [6 x i8] }";
      },
      "literal struct compatibility spelling must match its typed mirror");
  rejected(
      [](lir::LirModule& m) {
        m.globals[0].is_extern_decl = true;
        m.globals[0].linkage_vis = "external protected ";
        m.globals[0].init_text.clear();
        m.globals[0].initializer_function_link_name_ids.clear();
      },
      "literal aggregate extern declarations must remain unsupported");
  rejected([](lir::LirModule& m) { m.globals[0].type.array_rank = 1; },
           "TypeSpec array aggregate globals must remain unsupported");
  rejected([](lir::LirModule& m) { m.globals[0].type.ptr_level = 1; },
           "pointer-to-aggregate globals must remain unsupported");
}

void test_global_object_rejections_and_transactionality() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());
    const auto linked = module.link_names.intern("linked_global");
    lir::LirGlobal global;
    global.name = "linked_global";
    global.link_name_id = linked;
    global.type = scalar_type(c4c::TB_INT);
    global.linkage_vis = "external ";
    global.qualifier = "global ";
    global.llvm_type = "i32";
    global.llvm_type_ref = lir::LirTypeRef::integer(32);
    global.align_bytes = 4;
    global.is_extern_decl = true;
    module.globals.push_back(global);
    module.functions.push_back(void_declaration("valid_before_bad_global"));
    return module;
  };
  const auto rejected = [&](auto mutate, const std::string& message) {
    auto module = valid_module();
    mutate(module);
    auto imported = bir::lower_lir_to_raw_bir(module);
    expect(!imported.has_value() &&
               imported.error().code == bir::ImportErrorCode::UnsupportedGlobals,
           message);
    auto canonical = bir::lower_lir_to_canonical_bir(module);
    expect(!canonical.has_value() &&
               canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedGlobals,
           message + " (canonical rollback)");
  };

  rejected([](lir::LirModule& m) { m.globals[0].name.clear(); },
           "empty global names must reject the whole module");
  rejected([](lir::LirModule& m) { m.globals.push_back(m.globals[0]); },
           "duplicate global identities must reject the whole module");
  rejected([](lir::LirModule& m) {
             m.globals[0].link_name_id = static_cast<c4c::LinkNameId>(999);
           },
           "unresolved global link identities must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].name = "wrong_spelling"; },
           "link spelling mismatches must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].llvm_type_ref.reset();
             m.globals[0].type.ptr_level = -2;
             m.globals[0].llvm_type = "ptr";
           },
           "negative pointer extern depth must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].type.ptr_level = 1;
             m.globals[0].llvm_type = "i8*";
             m.globals[0].llvm_type_ref.reset();
             m.globals[0].is_const = true;
             m.globals[0].init_text = "ptr null";
           },
           "const-pointer structured authority must reject rendered parity conflicts transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].type.ptr_level = 1;
             m.globals[0].llvm_type = "ptr";
             m.globals[0].llvm_type_ref = lir::LirTypeRef::integer(32);
             m.globals[0].is_const = true;
             m.globals[0].init_text = "ptr null";
           },
           "const-pointer mirrors must corroborate structured authority transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].type.ptr_level = -2;
             m.globals[0].llvm_type = "ptr";
             m.globals[0].llvm_type_ref.reset();
             m.globals[0].is_const = true;
             m.globals[0].init_text = "ptr null";
           },
           "negative const-pointer definition depth must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].llvm_type_ref.reset();
             m.globals[0].type.array_rank = 2;
             m.globals[0].type.array_size = 1;
             m.globals[0].type.array_dims[0] = 1;
             m.globals[0].type.array_dims[1] = -2;
             m.globals[0].llvm_type = "[1 x [0 x i32]]";
           },
           "unsized multidimensional array globals must remain unsupported transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].llvm_type_ref.reset();
             m.globals[0].type = scalar_type(c4c::TB_STRUCT);
             m.globals[0].llvm_type = "%struct.Payload";
           },
           "mirror-free aggregate globals must remain unsupported transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].llvm_type_ref.reset();
             m.globals[0].llvm_type = "i64";
           },
           "mirror-free scalar rendered parity conflicts must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].llvm_type_ref =
                 lir::LirTypeRef("not-an-integer", lir::LirTypeKind::Integer);
             m.globals[0].llvm_type = "not-an-integer";
           },
           "malformed structured global types must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].llvm_type = "i64"; },
           "rendered type compatibility conflicts must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].type = scalar_type(c4c::TB_LONG); },
           "source type compatibility conflicts must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].linkage_vis = "weak "; },
           "linkage compatibility conflicts must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].linkage_vis = "extern_weak ";
             m.globals[0].is_extern_decl = false;
             m.globals[0].init_text = "i32 0";
           },
           "extern-weak definition-shaped rows must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].linkage_vis = "extern_weak ";
             m.globals[0].qualifier = "constant ";
           },
           "extern-weak declarations with constant qualifiers must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].linkage_vis = "extern_weak ";
             m.globals[0].init_text = "i32 0";
           },
           "extern-weak declarations with initializers must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis = "weak hidden hidden ";
             m.globals[0].init_text = "i32 0";
           },
           "duplicated visibility spellings must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].linkage_vis = "external concealed ";
           },
           "unknown visibility spellings must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].linkage_vis = "external hidden protected ";
           },
           "multiple visibility spellings must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].linkage_vis = "internal protected ";
           },
           "visibility cannot conceal linkage contradictions with independent flags");
  rejected([](lir::LirModule& m) { m.globals[0].qualifier = "constant "; },
           "qualifier compatibility conflicts must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].is_const = true;
             m.globals[0].init_text = "i32 0";
           },
           "constant definitions with global qualifiers must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].type.ptr_level = 1;
             m.globals[0].llvm_type = "ptr";
             m.globals[0].llvm_type_ref = lir::LirTypeRef("ptr");
             m.globals[0].is_const = true;
             m.globals[0].qualifier = "constant ";
             m.globals[0].init_text = "ptr null";
           },
           "const-pointer definitions with constant qualifiers must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].type.ptr_level = 1;
             m.globals[0].llvm_type = "ptr";
             m.globals[0].llvm_type_ref = lir::LirTypeRef("ptr");
           },
           "non-const ordinary pointer definitions require initializer payloads transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].qualifier = "constant ";
             m.globals[0].init_text = "i32 0";
           },
           "non-const definitions with constant qualifiers must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].is_internal = true; },
           "incoherent external/global flags must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].is_internal = true;
             m.globals[0].linkage_vis.clear();
             m.globals[0].init_text = "i32 0";
           },
           "internal flags without internal linkage must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis = "internal ";
             m.globals[0].init_text = "i32 0";
           },
           "internal linkage without internal flags must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].is_internal = true;
             m.globals[0].is_const = true;
             m.globals[0].linkage_vis = "internal ";
             m.globals[0].init_text = "i32 0";
           },
           "internal constant definitions with global qualifiers must reject transactionally");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].is_internal = true;
             m.globals[0].linkage_vis = "internal ";
             m.globals[0].qualifier = "constant ";
             m.globals[0].init_text = "i32 0";
           },
           "internal non-const definitions with constant qualifiers must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].align_bytes = 3; },
           "non-power-of-two global alignment must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].is_extern_decl = false; },
           "definitions without initializer payloads must reject transactionally");
  rejected([](lir::LirModule& m) { m.globals[0].init_text = "i32 0"; },
           "text initializers must never be silently dropped or parsed");
  rejected([](lir::LirModule& m) {
             m.globals[0].initializer_function_link_name_ids.push_back(
                 m.globals[0].link_name_id);
           },
           "extern declarations cannot carry initializer reference evidence");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].init_text = "i32 0";
             m.globals[0].initializer_function_link_name_ids = {
                 static_cast<c4c::LinkNameId>(999)};
           },
           "dangling initializer function links must reject the whole module");
  rejected([](lir::LirModule& m) {
             m.globals[0].is_extern_decl = false;
             m.globals[0].linkage_vis.clear();
             m.globals[0].init_text = "i32 0";
             m.globals[0].initializer_function_link_name_ids = {
                 c4c::kInvalidLinkName};
           },
           "invalid initializer function links must reject the whole module");

  bir::ModuleBuilder builder;
  expect(builder.add_link_name(1, "linked_global").has_value(),
         "staged link name should be available to global receipt");
  expect(builder
             .add_global_object("", bir::Type{bir::TypeKind::I32}, 4,
                                false, false, false, true)
             .error() == bir::BuildError::EmptyGlobalName,
         "builder must reject empty global identities");
  expect(builder
             .add_global_object("linked_global",
                                bir::Type{bir::TypeKind::I32}, 4,
                                false, false, false, true,
                                c4c::kInvalidLinkName)
             .error() == bir::BuildError::InvalidGlobalLinkName,
         "builder must reject explicitly invalid link identities");
  expect(builder
             .add_global_object("bad_initializer_link",
                                bir::Type{bir::TypeKind::I32}, 4,
                                false, false, false, false, std::nullopt,
                                std::string{"i32 0"},
                                {static_cast<c4c::LinkNameId>(999)})
             .error() == bir::BuildError::InvalidGlobalInitializerLinkName,
         "builder must resolve every initializer link before appending a global");
  expect(builder
             .add_global_object("bad_invalid_initializer_link",
                                bir::Type{bir::TypeKind::I32}, 4,
                                false, false, false, false, std::nullopt,
                                std::string{"i32 0"},
                                {c4c::kInvalidLinkName})
             .error() == bir::BuildError::InvalidGlobalInitializerLinkName,
         "builder must reject invalid initializer link sentinels");
  expect(builder
             .add_global_object("malformed_global",
                                bir::Type{bir::TypeKind::Void}, 3,
                                true, false, false, false)
             .has_value(),
         "builder should retain malformed staged global state for verifier diagnosis");
  expect(builder
             .add_global_object("malformed_global",
                                bir::Type{bir::TypeKind::I32}, 4,
                                false, false, false, true)
             .error() == bir::BuildError::DuplicateGlobalObject,
         "duplicate staged global receipt must remain append-only");
  auto published = std::move(builder).publish();
  expect(!published.has_value() &&
             published.error().reason == bir::PublishError::VerificationFailed,
         "FoundationVerifier must reject malformed staged global state");

  bir::ModuleBuilder coherence_builder;
  expect(coherence_builder
             .add_global_object("extern_with_initializer",
                                bir::Type{bir::TypeKind::I32}, 4,
                                false, false, false, true, std::nullopt,
                                std::string{"i32 0"})
             .has_value(),
         "builder should stage initializer coherence errors for verifier diagnosis");
  auto incoherent = std::move(coherence_builder).publish();
  expect(!incoherent.has_value() &&
             incoherent.error().reason == bir::PublishError::VerificationFailed,
         "FoundationVerifier must reject extern globals carrying initializers");

  bir::ModuleBuilder coherent_weak_builder;
  expect(coherent_weak_builder
             .add_global_object("coherent_weak_definition",
                                bir::Type{bir::TypeKind::I32}, 4,
                                false, true, false, false, std::nullopt,
                                std::string{"i32 0"})
             .has_value(),
         "builder should stage coherent typed weak definitions");
  expect(std::move(coherent_weak_builder).publish().has_value(),
         "FoundationVerifier should publish coherent typed weak definitions");

  bir::ModuleBuilder weak_internal_builder;
  expect(weak_internal_builder
             .add_global_object("weak_internal_definition",
                                bir::Type{bir::TypeKind::I32}, 4,
                                true, true, false, false, std::nullopt,
                                std::string{"i32 0"})
             .has_value(),
         "builder should stage weak/internal coherence errors for verifier diagnosis");
  auto weak_internal = std::move(weak_internal_builder).publish();
  expect(!weak_internal.has_value() &&
             weak_internal.error().reason ==
                 bir::PublishError::VerificationFailed,
         "FoundationVerifier must reject globals typed as both weak and internal");

  bir::ModuleBuilder weak_extern_builder;
  expect(weak_extern_builder
             .add_global_object("weak_extern_declaration",
                                bir::Type{bir::TypeKind::I32}, 4,
                                false, true, false, true)
             .has_value(),
         "builder should stage coherent typed weak external declarations");
  auto weak_extern = std::move(weak_extern_builder).publish();
  expect(weak_extern.has_value(),
         "FoundationVerifier must admit coherent typed weak external declarations");
  const auto weak_extern_ids = weak_extern.value().view().global_objects();
  const auto weak_extern_view =
      weak_extern.value().view().global_object(weak_extern_ids[0]).value();
  expect(weak_extern_ids.size() == 1 && weak_extern_view.is_weak &&
             weak_extern_view.is_extern_declaration &&
             !weak_extern_view.is_internal && !weak_extern_view.initializer,
         "typed weak external views must preserve both linkage facts without an initializer");

  bir::ModuleBuilder invalid_visibility_builder;
  expect(invalid_visibility_builder
             .add_global_object(
                 "invalid_visibility", bir::Type{bir::TypeKind::I32}, 4,
                 false, false, false, true, std::nullopt, std::nullopt, {},
                 static_cast<bir::SymbolVisibility>(99))
             .has_value(),
         "builder should stage closed-enum visibility errors for verifier diagnosis");
  auto invalid_visibility = std::move(invalid_visibility_builder).publish();
  expect(!invalid_visibility.has_value() &&
             invalid_visibility.error().reason ==
                 bir::PublishError::VerificationFailed,
         "FoundationVerifier must reject unknown typed visibility alternatives");

  bir::ModuleBuilder no_partial_builder;
  expect(no_partial_builder.add_link_name(1, "init_target").has_value(),
         "initializer target should enter the sole link-name table");
  expect(no_partial_builder
             .add_global_object("rejected_definition",
                                bir::Type{bir::TypeKind::I32}, 4,
                                false, false, false, false, std::nullopt,
                                std::string{"i32 0"},
                                {static_cast<c4c::LinkNameId>(99)})
             .error() == bir::BuildError::InvalidGlobalInitializerLinkName,
         "dangling initializer receipt must fail before publication state changes");
  const auto first_after_rejection =
      no_partial_builder.add_global_object(
          "accepted_definition", bir::Type{bir::TypeKind::I32}, 4,
          false, false, false, false, std::nullopt, std::string{"i32 0"}, {1});
  expect(first_after_rejection.has_value() &&
             first_after_rejection.value().slot == 0,
         "failed initializer resolution must not append a partial global row");
  expect(std::move(no_partial_builder).publish().has_value(),
         "a valid definition after rejected receipt should publish normally");
}

void test_specialization_metadata_receipt_and_rejections() {
  lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  const c4c::LinkNameId first_link =
      module.link_names.intern("_Z4makeIiEvT_");
  const c4c::LinkNameId second_link =
      module.link_names.intern("_Z4makeIdEvT_");
  module.spec_entries.push_back(
      {"type=i32;value=7", "ns::make<T>", "_Z4makeIiEvT_", first_link});
  module.spec_entries.push_back(
      {"type=f64;value=9", "ns::make<T>", "_Z4makeIdEvT_", second_link});

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         imported.has_value()
             ? "structured specialization metadata should import"
             : "structured specialization metadata should import: " +
                   imported.error().detail);
  const auto view = imported.value().view();
  const auto ids = view.specializations();
  expect(ids.size() == 2 && ids[0].slot == 0 && ids[1].slot == 1,
         "specialization IDs must preserve source vector order");
  const auto first = view.specialization(ids[0]);
  const auto second = view.specialization(ids[1]);
  expect(first.has_value() && second.has_value(),
         "ordered specialization IDs must resolve through the immutable view");
  expect(first.value().spec_key == "type=i32;value=7" &&
             first.value().template_origin == "ns::make<T>" &&
             first.value().mangled_name == "_Z4makeIiEvT_" &&
             view.spelling(first.value().mangled_link_name).value() ==
                 "_Z4makeIiEvT_" &&
             second.value().spec_key == "type=f64;value=9" &&
             second.value().template_origin == "ns::make<T>" &&
             second.value().mangled_name == "_Z4makeIdEvT_" &&
             view.spelling(second.value().mangled_link_name).value() ==
                 "_Z4makeIdEvT_",
         "specialization views must retain every exact structured field and typed link identity");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "foundation verification must reach and accept specialization storage");

  const auto rejected = [](lir::LirModule candidate) {
    auto raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw.has_value() &&
               raw.error().code ==
                   bir::ImportErrorCode::UnsupportedSpecializations,
           "malformed specialization metadata must publish no RawBir");
    auto canonical = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical.has_value() &&
               canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedSpecializations,
           "malformed specialization metadata must reject the whole module transactionally");
  };

  for (int empty_field = 0; empty_field != 3; ++empty_field) {
    lir::LirModule candidate;
    candidate.link_name_texts = std::make_shared<c4c::TextTable>();
    candidate.link_names.attach_text_table(candidate.link_name_texts.get());
    candidate.struct_names.attach_text_table(candidate.link_name_texts.get());
    const auto link = candidate.link_names.intern("_Z3rowv");
    lir::LirSpecEntry row{"key", "origin", "_Z3rowv", link};
    if (empty_field == 0) row.spec_key.clear();
    if (empty_field == 1) row.template_origin.clear();
    if (empty_field == 2) row.mangled_name.clear();
    candidate.spec_entries.push_back(std::move(row));
    rejected(std::move(candidate));
  }

  lir::LirModule invalid_link;
  invalid_link.spec_entries.push_back(
      {"key", "origin", "_Z3rowv", c4c::kInvalidLinkName});
  rejected(std::move(invalid_link));

  lir::LirModule unresolved_link;
  unresolved_link.spec_entries.push_back(
      {"key", "origin", "_Z3rowv", static_cast<c4c::LinkNameId>(99)});
  rejected(std::move(unresolved_link));

  lir::LirModule mismatched_link;
  mismatched_link.link_name_texts = std::make_shared<c4c::TextTable>();
  mismatched_link.link_names.attach_text_table(
      mismatched_link.link_name_texts.get());
  mismatched_link.struct_names.attach_text_table(
      mismatched_link.link_name_texts.get());
  const auto mismatch = mismatched_link.link_names.intern("_Z5otherv");
  mismatched_link.spec_entries.push_back(
      {"key", "origin", "_Z3rowv", mismatch});
  rejected(std::move(mismatched_link));

  lir::LirModule duplicate_semantic;
  duplicate_semantic.link_name_texts = std::make_shared<c4c::TextTable>();
  duplicate_semantic.link_names.attach_text_table(
      duplicate_semantic.link_name_texts.get());
  duplicate_semantic.struct_names.attach_text_table(
      duplicate_semantic.link_name_texts.get());
  const auto semantic_a = duplicate_semantic.link_names.intern("_Z4samei");
  const auto semantic_b = duplicate_semantic.link_names.intern("_Z4samed");
  duplicate_semantic.spec_entries.push_back(
      {"same-key", "same-origin", "_Z4samei", semantic_a});
  duplicate_semantic.spec_entries.push_back(
      {"same-key", "same-origin", "_Z4samed", semantic_b});
  rejected(std::move(duplicate_semantic));

  lir::LirModule duplicate_link;
  duplicate_link.link_name_texts = std::make_shared<c4c::TextTable>();
  duplicate_link.link_names.attach_text_table(
      duplicate_link.link_name_texts.get());
  duplicate_link.struct_names.attach_text_table(
      duplicate_link.link_name_texts.get());
  const auto shared_link = duplicate_link.link_names.intern("_Z6sharedv");
  duplicate_link.spec_entries.push_back(
      {"key-a", "origin", "_Z6sharedv", shared_link});
  duplicate_link.spec_entries.push_back(
      {"key-b", "origin", "_Z6sharedv", shared_link});
  rejected(std::move(duplicate_link));

  bir::ModuleBuilder builder;
  expect(builder.add_link_name(1, "_Z5firstv").has_value() &&
             builder.add_link_name(2, "_Z6secondv").has_value() &&
             builder.add_link_name(3, "_Z5thirdv").has_value(),
         "builder specialization test needs an ordered link-name domain");
  expect(builder.add_specialization("", "origin", "_Z5firstv", 1).error() ==
             bir::BuildError::EmptySpecializationField,
         "builder must reject empty specialization fields");
  expect(builder.add_specialization("key", "origin", "_Z5firstv", 99).error() ==
             bir::BuildError::InvalidSpecializationLinkName,
         "builder must reject unresolved specialization link identity");
  expect(builder.add_specialization("key", "origin", "_Z6secondv", 1).error() ==
             bir::BuildError::InvalidSpecializationLinkName,
         "builder must reject link spelling mismatch");
  const auto accepted =
      builder.add_specialization("key", "origin", "_Z5firstv", 1);
  expect(accepted.has_value() && accepted.value().slot == 0,
         "failed specialization receipts must append no partial state");
  expect(builder.add_specialization("key", "origin", "_Z6secondv", 2).error() ==
             bir::BuildError::DuplicateSpecialization,
         "builder must reject duplicate template-origin/spec-key identity");
  expect(builder.add_specialization("other", "origin", "_Z5firstv", 1).error() ==
             bir::BuildError::DuplicateSpecialization,
         "builder must reject conflicting reuse of mangled link identity");
  const auto second_accepted =
      builder.add_specialization("other", "origin", "_Z6secondv", 2);
  expect(second_accepted.has_value() && second_accepted.value().slot == 1,
         "valid receipt after duplicate rejection must keep contiguous order");
  expect(std::move(builder).publish().has_value(),
         "valid specialization builder state should publish after rejected receipts");
}

void test_accumulated_module_surface_checkpoint() {
  const auto valid_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    module.struct_names.attach_text_table(module.link_name_texts.get());

    const auto external_link = module.link_names.intern("checkpoint_external");
    const auto aggregate_link =
        module.link_names.intern("checkpoint_aggregate");
    const auto init_a = module.link_names.intern("checkpoint_init_a");
    const auto init_b = module.link_names.intern("checkpoint_init_b");
    const auto specialization_link =
        module.link_names.intern("_Z18checkpoint_makeIiEvT_");
    const auto aggregate_name =
        module.struct_names.intern("%struct.CheckpointAggregate");

    lir::LirStructDecl aggregate_decl;
    aggregate_decl.name_id = aggregate_name;
    aggregate_decl.is_packed = true;
    aggregate_decl.fields = {{lir::LirTypeRef::integer(32)},
                             {lir::LirTypeRef("double")}};
    module.record_struct_decl(std::move(aggregate_decl));

    module.string_pool = {
        lir::LirStringConst{"@.checkpoint.0", std::string{"A\0B", 3}, 4},
        lir::LirStringConst{"@.checkpoint.1", "opaque\\00tail", -1},
    };
    module.str_pool_map.emplace("checkpoint-source", "@.checkpoint.0");
    module.str_pool_idx = 2;

    const lir::LirExternDecl external{
        "checkpoint_external", "i32", lir::LirTypeRef::integer(32),
        lir::LirExtAttr::ZeroExt, external_link};
    module.extern_decls.push_back(external);
    module.extern_decl_link_name_map.emplace(
        external_link,
        lir::LirModule::ExternDeclInfo{
            external.name, external.return_type_str, external.return_type,
            external.return_ext_attr, external.link_name_id});

    lir::LirGlobal external_global;
    external_global.name = "checkpoint_fallback_global";
    external_global.type = scalar_type(c4c::TB_INT);
    external_global.linkage_vis = "external hidden ";
    external_global.qualifier = "global ";
    external_global.llvm_type = "i32";
    external_global.llvm_type_ref = lir::LirTypeRef::integer(32);
    external_global.align_bytes = 4;
    external_global.is_extern_decl = true;
    module.globals.push_back(std::move(external_global));

    lir::LirGlobal aggregate_global;
    aggregate_global.name = "checkpoint_aggregate";
    aggregate_global.link_name_id = aggregate_link;
    aggregate_global.type = scalar_type(c4c::TB_STRUCT);
    aggregate_global.linkage_vis = "protected ";
    aggregate_global.qualifier = "global ";
    aggregate_global.llvm_type = "%struct.CheckpointAggregate";
    aggregate_global.llvm_type_ref = lir::LirTypeRef::struct_type(
        aggregate_global.llvm_type, aggregate_name);
    aggregate_global.init_text = "{ i32 7, double 2.5 }";
    aggregate_global.initializer_function_link_name_ids = {
        init_b, init_a, init_b};
    aggregate_global.align_bytes = 8;
    module.globals.push_back(std::move(aggregate_global));

    module.spec_entries.push_back(
        {"type=i32;value=7", "checkpoint::make<T>",
         "_Z18checkpoint_makeIiEvT_", specialization_link});
    return module;
  };

  auto module = valid_module();
  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(),
         "all admitted Step 2-3 module families should coexist in Raw BIR");
  expect(bir::FoundationVerifier::verify(imported.value()).ok(),
         "the accumulated module surface must remain verifier reachable");

  const auto view = imported.value().view();
  const auto link_names = view.link_names();
  expect(link_names.size() == 5 && link_names[0].slot == 0 &&
             link_names[1].slot == 1 && link_names[2].slot == 2 &&
             link_names[3].slot == 3 && link_names[4].slot == 4 &&
             view.spelling(link_names[0]).value() == "checkpoint_external" &&
             view.spelling(link_names[1]).value() == "checkpoint_aggregate" &&
             view.spelling(link_names[2]).value() == "checkpoint_init_a" &&
             view.spelling(link_names[3]).value() == "checkpoint_init_b" &&
             view.spelling(link_names[4]).value() ==
                 "_Z18checkpoint_makeIiEvT_",
         "the combined module must preserve stable ordered symbol identity");

  const auto struct_names = view.struct_names();
  const auto struct_decls = view.struct_declarations();
  expect(struct_names.size() == 1 && struct_decls.size() == 1 &&
             view.spelling(struct_names[0]).value() ==
                 "%struct.CheckpointAggregate",
         "the combined module must preserve its named aggregate identity");
  const auto aggregate_decl = view.struct_declaration(struct_decls[0]).value();
  expect(aggregate_decl.name == struct_names[0] && aggregate_decl.is_packed &&
             aggregate_decl.fields.size() == 2 &&
             aggregate_decl.fields[0].type == bir::Type{bir::TypeKind::I32} &&
             aggregate_decl.fields[1].type == bir::Type{bir::TypeKind::F64},
         "the combined module must retain the structured declaration view");

  const auto strings = view.string_data();
  expect(strings.size() == 2 && strings[0].slot == 0 && strings[1].slot == 1,
         "combined string data must retain vector authority and order");
  const auto first_string = view.string_data(strings[0]).value();
  const auto second_string = view.string_data(strings[1]).value();
  expect(first_string.pool_name == "@.checkpoint.0" &&
             first_string.raw_bytes == std::string{"A\0B", 3} &&
             first_string.byte_length == 4 &&
             second_string.pool_name == "@.checkpoint.1" &&
             second_string.raw_bytes == "opaque\\00tail" &&
             second_string.byte_length == -1 &&
             view.string_data("@.checkpoint.0").value() == strings[0],
         "combined string views must preserve typed identities and opaque bytes");

  const auto externals = view.external_declarations();
  expect(externals.size() == 1 && externals[0].slot == 0,
         "the external declaration must retain its ordered index");
  const auto external = view.external_declaration(externals[0]).value();
  expect(external.source_name == "checkpoint_external" &&
             external.return_type == bir::Type{bir::TypeKind::I32} &&
             external.return_extension == bir::ReturnExtension::ZeroExt &&
             std::holds_alternative<bir::LinkNameId>(external.identity) &&
             std::get<bir::LinkNameId>(external.identity) == link_names[0] &&
             view.external_declaration(link_names[0]).value() == externals[0],
         "the external index and symbol lookup must resolve one typed identity");

  const auto globals = view.global_objects();
  expect(globals.size() == 2 && globals[0].slot == 0 && globals[1].slot == 1,
         "multiple admitted global forms must preserve source order");
  const auto fallback_global = view.global_object(globals[0]).value();
  const auto aggregate_global = view.global_object(globals[1]).value();
  expect(fallback_global.object_type == bir::Type{bir::TypeKind::I32} &&
             fallback_global.is_extern_declaration &&
             fallback_global.visibility == bir::SymbolVisibility::Hidden &&
             std::holds_alternative<bir::FallbackGlobalName>(
                 fallback_global.identity) &&
             !fallback_global.initializer,
         "the combined module must preserve the admitted fallback extern global");
  expect(aggregate_global.object_type.kind == bir::TypeKind::Struct &&
             aggregate_global.object_type.struct_name_id ==
                 module.globals[1].llvm_type_ref->struct_name_id() &&
             aggregate_global.object_type.spelling ==
                 "%struct.CheckpointAggregate" &&
             std::get<bir::LinkNameId>(aggregate_global.identity) ==
                 link_names[1] &&
             aggregate_global.visibility == bir::SymbolVisibility::Protected &&
             aggregate_global.alignment == 8 &&
             !aggregate_global.is_extern_declaration &&
             aggregate_global.initializer &&
             aggregate_global.initializer->opaque_payload ==
                 "{ i32 7, double 2.5 }" &&
             aggregate_global.initializer->function_links ==
                 std::vector<bir::LinkNameId>{link_names[3], link_names[2],
                                               link_names[3]},
         "the named aggregate global must retain identity and initializer topology");

  const auto specializations = view.specializations();
  expect(specializations.size() == 1 && specializations[0].slot == 0,
         "specialization metadata must retain its ordered typed identity");
  const auto specialization = view.specialization(specializations[0]).value();
  expect(specialization.spec_key == "type=i32;value=7" &&
             specialization.template_origin == "checkpoint::make<T>" &&
             specialization.mangled_name == "_Z18checkpoint_makeIiEvT_" &&
             specialization.mangled_link_name == link_names[4],
         "specialization metadata must cross-reference the shared symbol table");

  auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "the same accumulated module must publish Canonical BIR");
  const auto canonical_view = canonical.value().view();
  expect(canonical_view.struct_declarations().size() == 1 &&
             canonical_view.string_data().size() == 2 &&
             canonical_view.external_declarations().size() == 1 &&
             canonical_view.global_objects().size() == 2 &&
             canonical_view.specializations().size() == 1,
         "canonical publication must retain every accumulated module family");

  const auto rejected = [&](auto mutate, bir::ImportErrorCode expected,
                            const std::string& message) {
    auto candidate = valid_module();
    mutate(candidate);
    const auto raw = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw.has_value() && raw.error().code == expected,
           message + " (Raw rollback)");
    const auto rejected_canonical =
        bir::lower_lir_to_canonical_bir(candidate);
    expect(!rejected_canonical.has_value() &&
               rejected_canonical.error().code == expected,
           message + " (Canonical rollback)");
  };
  rejected(
      [](lir::LirModule& candidate) {
        candidate.globals[1].initializer_function_link_name_ids.back() =
            static_cast<c4c::LinkNameId>(999);
      },
      bir::ImportErrorCode::UnsupportedGlobals,
      "a late invalid initializer cross-reference must publish no partial module");
  rejected(
      [](lir::LirModule& candidate) {
        candidate.spec_entries[0].mangled_link_name_id =
            static_cast<c4c::LinkNameId>(999);
      },
      bir::ImportErrorCode::UnsupportedSpecializations,
      "a late invalid specialization cross-reference must publish no partial module");
}

void test_inline_asm_shape_rejection() {
  lir::LirModule module;
  auto block = return_block(0, "entry");
  auto unsupported = void_inline_asm("opaque", "=r,r");
  unsupported.args_str = "i64 %arg";
  block.insts.push_back(std::move(unsupported));
  module.functions.push_back(void_definition("bad_asm", {std::move(block)}));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(!imported.has_value(),
         "unstructured inline-asm arguments must publish no RawBir");
  expect(imported.error().code == bir::ImportErrorCode::UnsupportedInlineAsmShape,
         "unstructured argument rejection should remain structured");
  auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(!canonical.has_value() &&
             canonical.error().code ==
                 bir::ImportErrorCode::UnsupportedInlineAsmShape,
         "textual-only inline asm must publish no partial CanonicalBir");
}

void test_inline_asm_output_store_receipt_and_rejections(
    std::uint32_t width, std::size_t global_index, std::uint32_t source_id) {
  const auto make_module = [=] {
    auto module = direct_global_integer_store_module();
    auto& block = module.functions[0].blocks[0];
    block.insts.clear();
    auto producer = void_inline_asm("opaque asm %0", "=r");
    producer.asm_text = "presentation-only asm";
    producer.constraints = "presentation-only constraints";
    producer.args_str = "presentation-only arguments";
    producer.result = lir::LirOperand::ssa("%compatibility-only-result",
                                            lir::LirValueId{source_id + 999});
    producer.original_asm_text = "presentation-only original asm";
    producer.original_constraint_text.clear();
    producer.ordinary_results = {{
        lir::LirOperand::ssa("%presentation-output", lir::LirValueId{source_id}),
        lir::LirTypeRef::integer(width), lir::LirInlineAsmValueRole::Output, 0}};
    block.insts.push_back(std::move(producer));
    block.insts.push_back(lir::LirStoreOp{
        lir::LirTypeRef::integer(width),
        lir::LirOperand::ssa("%different-display", lir::LirValueId{source_id}),
        lir::LirOperand::global("@presentation-destination",
                                module.globals[global_index].link_name_id)});
    return module;
  };
  const auto inspect = [=](const auto& graph, std::string_view layer) {
    const auto view = graph.view();
    const auto function_id = view.functions().front();
    const auto function = view.function(function_id).value();
    const auto instructions = function.instructions(function.blocks().front()).value();
    expect(instructions.size() == 2, std::string(layer) + " must retain asm and Store");
    const auto asm_instruction = function.instruction(instructions[0]).value();
    const auto store_instruction = function.instruction(instructions[1]).value();
    expect(std::holds_alternative<bir::InlineAsmNode>(asm_instruction.payload()) &&
               asm_instruction.results().size() == 1 &&
               store_instruction.operands() == asm_instruction.results(),
           std::string(layer) + " must connect the source-backed asm result to Store");
    const auto result = function.value(asm_instruction.results()[0]).value();
    expect(result.type == bir::Type{bir::TypeKind::Integer, width,
                                    "i" + std::to_string(width)} &&
               result.source_id == bir::SourceValueId{function_id, source_id} &&
               function.source_value(bir::SourceValueId{function_id, source_id}).value() ==
                   asm_instruction.results()[0],
           std::string(layer) + " must retain the native output LirValueId");
  };
  const auto module = make_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "the checked inline-asm output/store route must publish verified Raw BIR");
  inspect(raw.value(), "Raw BIR");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(), "the checked i32 inline-asm output/store route must canonicalize");
  inspect(canonical.value(), "Canonical BIR");

  const auto rejected = [&](auto mutate, std::string_view message) {
    auto candidate = make_module();
    mutate(candidate);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value(), std::string(message) + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value(), std::string(message) + " (Canonical rollback)");
  };
  rejected([](auto& m) { std::get<lir::LirInlineAsmOp>(m.functions[0].blocks[0].insts[0]).ordinary_results.clear(); },
           "missing binding must reject");
  rejected([](auto& m) { std::get<lir::LirInlineAsmOp>(m.functions[0].blocks[0].insts[0]).ordinary_results[0].value = lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid()); },
           "invalid binding ID must reject");
  rejected([](auto& m) { auto& op = std::get<lir::LirInlineAsmOp>(m.functions[0].blocks[0].insts[0]); op.ordinary_results.push_back(op.ordinary_results.front()); },
           "duplicate binding must reject");
  rejected([](auto& m) { std::get<lir::LirInlineAsmOp>(m.functions[0].blocks[0].insts[0]).ordinary_results[0].role = lir::LirInlineAsmValueRole::ReadWrite; },
           "wrong binding role must reject");
  rejected([](auto& m) { std::get<lir::LirInlineAsmOp>(m.functions[0].blocks[0].insts[0]).ordinary_results[0].constraint_index = 1; },
           "wrong binding index must reject");
  rejected([=](auto& m) { std::get<lir::LirInlineAsmOp>(m.functions[0].blocks[0].insts[0]).ordinary_results[0].type = lir::LirTypeRef::integer(width == 32 ? 64 : 32); },
           "wrong binding type must reject");
  rejected([=](auto& m) { std::get<lir::LirStoreOp>(m.functions[0].blocks[0].insts[1]).val = lir::LirOperand::ssa("%unknown", lir::LirValueId{source_id + 1}); },
           "unknown Store source must reject");
  rejected([=](auto& m) { std::get<lir::LirStoreOp>(m.functions[0].blocks[0].insts[1]).type_str = lir::LirTypeRef::integer(width == 32 ? 64 : 32); },
           "Store type mismatch must reject");
  rejected([](auto& m) { m.functions[0].blocks[0].insts.pop_back(); },
           "missing Store use must reject");
  rejected([](auto& m) { m.functions[0].blocks[0].insts.push_back(std::get<lir::LirStoreOp>(m.functions[0].blocks[0].insts[1])); },
           "duplicate Store use must reject");
  rejected([](auto& m) { auto other = m.functions[0]; other.name = "foreign"; other.blocks[0].insts.erase(other.blocks[0].insts.begin()); m.functions.push_back(std::move(other)); },
           "cross-function Store use must reject");
}

void test_i32_inline_asm_output_store_receipt_and_rejections() {
  test_inline_asm_output_store_receipt_and_rejections(32, 0, 42);
}

void test_i64_inline_asm_output_store_receipt_and_rejections() {
  test_inline_asm_output_store_receipt_and_rejections(64, 1, 43);
}

lir::LirCallOp direct_void_call(c4c::LinkNameId target) {
  lir::LirCallOp call;
  call.return_type = lir::LirTypeRef("void");
  call.callee = lir::LirOperand::raw("%misleading_indirect_display");
  call.direct_callee_link_name_id = target;
  call.callee_type_suffix = "(i64) presentation-only";
  call.args_str = "i64 99 presentation-only";
  lir::LirCallSignature signature;
  signature.return_type_ref = lir::LirTypeRef("void");
  signature.has_void_param_list = true;
  call.callee_signature = std::move(signature);
  return call;
}

lir::LirModule direct_void_call_module() {
  lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  const auto caller_link = module.link_names.intern("direct_void_caller");
  const auto recursive_link = module.link_names.intern("direct_void_recursive");
  const auto target_link = module.link_names.intern("direct_void_target");

  auto caller_block = return_block(0, "caller_entry");
  caller_block.insts.push_back(direct_void_call(target_link));
  auto caller = void_definition("misleading_caller_name",
                                {std::move(caller_block)});
  caller.link_name_id = caller_link;
  module.functions.push_back(std::move(caller));

  auto recursive_block = return_block(0, "recursive_entry");
  recursive_block.insts.push_back(direct_void_call(recursive_link));
  auto recursive = void_definition("misleading_recursive_name",
                                   {std::move(recursive_block)});
  recursive.link_name_id = recursive_link;
  module.functions.push_back(std::move(recursive));

  auto declaration = void_declaration("misleading_target_declaration");
  declaration.link_name_id = target_link;
  module.functions.push_back(std::move(declaration));

  auto definition = void_definition(
      "misleading_target_definition", {return_block(0, "target_entry")});
  definition.link_name_id = target_link;
  module.functions.push_back(std::move(definition));
  return module;
}

void attach_direct_void_function_signature_ref(lir::LirModule& module) {
  lir::LirFunctionSignatureStoreEntry caller_entry;
  caller_entry.return_type_ref = lir::LirTypeRef("void");
  caller_entry.has_void_param_list = true;
  module.functions[0].function_signature_ref =
      module.register_function_signature(std::move(caller_entry));

  lir::LirFunctionSignatureStoreEntry recursive_entry;
  recursive_entry.return_type_ref = lir::LirTypeRef("void");
  recursive_entry.has_void_param_list = true;
  const lir::LirFunctionSignatureRef recursive_ref =
      module.register_function_signature(std::move(recursive_entry));
  module.functions[1].function_signature_ref = recursive_ref;
  auto& recursive_call =
      std::get<lir::LirCallOp>(module.functions[1].blocks[0].insts[0]);
  recursive_call.callee_signature_ref = recursive_ref;

  lir::LirFunctionSignatureStoreEntry target_entry;
  target_entry.return_type_ref = lir::LirTypeRef("void");
  target_entry.has_void_param_list = true;
  const lir::LirFunctionSignatureRef target_ref =
      module.register_function_signature(std::move(target_entry));
  module.functions[2].function_signature_ref = target_ref;
  module.functions[3].function_signature_ref = target_ref;
  auto& forward_call =
      std::get<lir::LirCallOp>(module.functions[0].blocks[0].insts[0]);
  forward_call.callee_signature_ref = target_ref;
}

void expect_direct_void_call_view(const bir::ModuleView& view,
                                  std::size_t caller_index,
                                  bir::FunctionId expected_target,
                                  const std::string& context) {
  const auto functions = view.functions();
  expect(caller_index < functions.size(), context + " caller must exist");
  const auto function = view.function(functions[caller_index]).value();
  const auto blocks = function.blocks();
  expect(blocks.size() == 1, context + " caller must retain one block");
  const auto instructions = function.instructions(blocks[0]).value();
  expect(instructions.size() == 1,
         context + " caller must retain exactly one instruction");
  const auto instruction = function.instruction(instructions[0]).value();
  expect(instruction.opcode() == bir::Opcode::Call && instruction.call() &&
             instruction.call()->callee == expected_target &&
             instruction.operands().empty() && instruction.results().empty(),
         context +
             " must expose one immutable zero-operand/zero-result typed Call");
}

void test_direct_zero_argument_void_call_receipt() {
  auto module = direct_void_call_module();
  auto raw = bir::lower_lir_to_raw_bir(module);
  if (!raw.has_value())
    fail("forward, recursive, and merged direct void calls should import: " +
         raw.error().detail);
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "direct void calls must remain foundation-verifier reachable");
  const auto raw_functions = raw.value().view().functions();
  expect(raw_functions.size() == 3,
         "declaration and definition must merge to one FunctionId");
  expect_direct_void_call_view(raw.value().view(), 0, raw_functions[2],
                               "Raw forward call");
  expect_direct_void_call_view(raw.value().view(), 1, raw_functions[1],
                               "Raw recursive call");

  auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(),
         "the same direct void call module should canonicalize");
  const auto canonical_functions = canonical.value().view().functions();
  expect(canonical_functions.size() == 3,
         "Canonical BIR must retain merged target identity");
  expect_direct_void_call_view(canonical.value().view(), 0,
                               canonical_functions[2],
                               "Canonical forward call");
  expect_direct_void_call_view(canonical.value().view(), 1,
                               canonical_functions[1],
                               "Canonical recursive call");

  auto store_backed = direct_void_call_module();
  attach_direct_void_function_signature_ref(store_backed);
  auto& missing_retained_call =
      std::get<lir::LirCallOp>(store_backed.functions[0].blocks[0].insts[0]);
  missing_retained_call.callee =
      lir::LirOperand::global("@direct_void_target",
                              missing_retained_call.direct_callee_link_name_id);
  missing_retained_call.callee_signature.reset();
  auto raw_missing_retained = bir::lower_lir_to_raw_bir(store_backed);
  expect(raw_missing_retained.has_value() &&
             bir::FoundationVerifier::verify(raw_missing_retained.value()).ok(),
         "direct void call must use module signature store when retained signature is absent");
  const auto raw_store_functions = raw_missing_retained.value().view().functions();
  expect_direct_void_call_view(raw_missing_retained.value().view(), 0,
                               raw_store_functions[2],
                               "Raw store-backed forward call");
  auto canonical_missing_retained = bir::lower_lir_to_canonical_bir(store_backed);
  expect(canonical_missing_retained.has_value(),
         "store-backed direct void call without retained signature must canonicalize");

  auto stale_text = direct_void_call_module();
  attach_direct_void_function_signature_ref(stale_text);
  auto& stale_text_call =
      std::get<lir::LirCallOp>(stale_text.functions[0].blocks[0].insts[0]);
  stale_text_call.callee =
      lir::LirOperand::global("@direct_void_target",
                              stale_text_call.direct_callee_link_name_id);
  stale_text_call.callee_type_suffix = "(i32 stale text only)";
  stale_text_call.args_str = "i64 stale text only";
  stale_text_call.callee_signature->fixed_param_types = {"i32 stale text"};
  auto raw_stale_text = bir::lower_lir_to_raw_bir(stale_text);
  expect(raw_stale_text.has_value() &&
             bir::FoundationVerifier::verify(raw_stale_text.value()).ok(),
         "direct void call must ignore retained text when signature ref resolves");
}

void test_direct_zero_argument_void_call_builder_contract() {
  bir::ModuleBuilder builder;
  const bir::FunctionSignature void_signature{bir::Type{bir::TypeKind::Void},
                                               {}, false};
  const auto target =
      builder.create_function(void_signature, "builder_target", true);
  const auto caller =
      builder.create_function(void_signature, "builder_caller", false);
  expect(target.has_value() && caller.has_value(),
         "builder call fixture functions should be created");
  auto edited = builder.with_function(
      caller.value(), [&](bir::FunctionBuilder& function) {
        const auto block = function.create_block("entry");
        if (!block) return bir::Result<void, bir::BuildError>::failure(block.error());
        const auto call = function.append(block.value(),
                                          bir::CallSpec{target.value()});
        if (!call)
          return bir::Result<void, bir::BuildError>::failure(call.error());
        expect(call.value().results.empty(),
               "builder direct void Call must return no result IDs");
        return function.set_terminator(block.value(),
                                       bir::ReturnTerm{std::nullopt});
      });
  expect(edited.has_value(), "builder should append an exact direct void Call");
  auto raw = std::move(builder).publish();
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "builder direct void Call must publish and verify");

  bir::ModuleBuilder rejected;
  const bir::FunctionSignature value_signature{bir::Type{bir::TypeKind::I32},
                                                {}, false};
  const auto value_target =
      rejected.create_function(value_signature, "value_target", true);
  const auto rejected_caller =
      rejected.create_function(void_signature, "rejected_caller", false);
  expect(value_target.has_value() && rejected_caller.has_value(),
         "negative builder fixture functions should be created");
  auto rejected_edit = rejected.with_function(
      rejected_caller.value(), [&](bir::FunctionBuilder& function) {
        const auto block = function.create_block("entry");
        if (!block) return bir::Result<void, bir::BuildError>::failure(block.error());
        const auto call = function.append(block.value(),
                                          bir::CallSpec{value_target.value()});
        expect(!call.has_value() &&
                   call.error() == bir::BuildError::UnsupportedOpcode,
               "builder must reject a nonvoid target for the bounded Call");
        return bir::Result<void, bir::BuildError>::failure(call.error());
      });
  expect(!rejected_edit.has_value(),
         "rejected Call edits must not publish a partial body");

  bir::ModuleBuilder foreign_owner;
  const auto foreign_target =
      foreign_owner.create_function(void_signature, "foreign_target", true);
  bir::ModuleBuilder local_owner;
  const auto local_caller =
      local_owner.create_function(void_signature, "local_caller", false);
  expect(foreign_target.has_value() && local_caller.has_value(),
         "foreign-owner Call fixture functions should be created");
  auto foreign_edit = local_owner.with_function(
      local_caller.value(), [&](bir::FunctionBuilder& function) {
        const auto block = function.create_block("entry");
        if (!block)
          return bir::Result<void, bir::BuildError>::failure(block.error());
        const auto call = function.append(
            block.value(), bir::CallSpec{foreign_target.value()});
        expect(!call.has_value() &&
                   call.error() == bir::BuildError::InvalidFunction,
               "builder must reject a Call target owned by another module");
        return bir::Result<void, bir::BuildError>::failure(call.error());
      });
  expect(!foreign_edit.has_value(),
         "foreign-target rejection must not publish a partial body");
}

void test_native_floating_call_result_builder_contract() {
  bir::ModuleBuilder builder;
  const bir::FunctionSignature void_signature{bir::Type{bir::TypeKind::Void},
                                               {}, false};
  const bir::FunctionSignature f32_signature{bir::Type{bir::TypeKind::F32},
                                              {}, false};
  const bir::FunctionSignature f64_signature{bir::Type{bir::TypeKind::F64},
                                              {}, false};
  const auto f32_target =
      builder.create_function(f32_signature, "native_f32_target", true);
  const auto f64_target =
      builder.create_function(f64_signature, "native_f64_target", true);
  const auto caller =
      builder.create_function(void_signature, "native_float_caller", false);
  const auto sibling_caller = builder.create_function(
      void_signature, "native_float_sibling_caller", false);
  expect(f32_target.has_value() && f64_target.has_value() && caller.has_value() &&
             sibling_caller.has_value(),
         "native floating Call fixture functions should be created");

  auto edited = builder.with_function(
      caller.value(), [&](bir::FunctionBuilder& function) {
        const auto block = function.create_block("entry");
        if (!block)
          return bir::Result<void, bir::BuildError>::failure(block.error());
        const auto f32_call = function.append(
            block.value(), bir::CallSpec{f32_target.value(), {}, 7});
        const auto f64_call = function.append(
            block.value(), bir::CallSpec{f64_target.value(), {}, 8});
        expect(f32_call.has_value() && f64_call.has_value() &&
                   f32_call.value().results.size() == 1 &&
                   f64_call.value().results.size() == 1,
               "native F32 and F64 Calls must publish one source-backed result");
        return function.set_terminator(block.value(),
                                       bir::ReturnTerm{std::nullopt});
      });
  expect(edited.has_value(), "native floating Calls should build successfully");
  auto sibling_edited = builder.with_function(
      sibling_caller.value(), [&](bir::FunctionBuilder& function) {
        const auto block = function.create_block("entry");
        if (!block)
          return bir::Result<void, bir::BuildError>::failure(block.error());
        const auto call = function.append(
            block.value(), bir::CallSpec{f32_target.value(), {}, 7});
        if (!call)
          return bir::Result<void, bir::BuildError>::failure(call.error());
        return function.set_terminator(block.value(),
                                       bir::ReturnTerm{std::nullopt});
      });
  expect(sibling_edited.has_value(),
         "the same source value number must remain valid in another Call owner");
  auto raw = std::move(builder).publish();
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "native floating Calls must publish through exact result linkage verification");
  const auto view = raw.value().view();
  const auto caller_view = view.function(caller.value()).value();
  const auto sibling_view = view.function(sibling_caller.value()).value();
  const auto instructions =
      caller_view.instructions(caller_view.blocks()[0]).value();
  expect(instructions.size() == 2,
         "native floating Call fixture must retain both ordered Calls");
  for (std::size_t index = 0; index < instructions.size(); ++index) {
    const auto call = caller_view.instruction(instructions[index]).value();
    const auto result = caller_view.value(call.results()[0]).value();
    const auto expected_type = index == 0 ? bir::Type{bir::TypeKind::F32}
                                          : bir::Type{bir::TypeKind::F64};
    expect(call.opcode() == bir::Opcode::Call && call.call() &&
               call.operands().empty() && call.results().size() == 1 &&
               result.type == expected_type && result.source_id ==
                   bir::SourceValueId{caller.value(),
                                      static_cast<std::uint32_t>(7 + index)} &&
               caller_view.source_value(*result.source_id).value() ==
                   call.results()[0],
           "native floating Call result must retain exact signature, owner, source index, and instruction linkage");
  }
  const auto sibling_instructions =
      sibling_view.instructions(sibling_view.blocks()[0]).value();
  expect(sibling_instructions.size() == 1 &&
             sibling_view.source_value(
                 bir::SourceValueId{sibling_caller.value(), 7}).has_value() &&
             !caller_view.source_value(
                 bir::SourceValueId{sibling_caller.value(), 7}).has_value(),
         "same source value numbers must not cross Call owner boundaries");

  bir::ModuleBuilder rejected;
  const auto void_target =
      rejected.create_function(void_signature, "void_target", true);
  const auto rejected_f32_target =
      rejected.create_function(f32_signature, "rejected_f32_target", true);
  const auto rejected_f64_target =
      rejected.create_function(f64_signature, "rejected_f64_target", true);
  const bir::FunctionSignature generic_float_signature{
      bir::Type{bir::TypeKind::Floating, 80, "x86_fp80"}, {}, false};
  const auto generic_float_target = rejected.create_function(
      generic_float_signature, "generic_float_target", true);
  const auto rejected_caller =
      rejected.create_function(void_signature, "rejected_float_caller", false);
  expect(void_target.has_value() && rejected_f32_target.has_value() &&
             rejected_f64_target.has_value() && generic_float_target.has_value() &&
             rejected_caller.has_value(),
         "native floating Call negative fixture functions should be created");
  auto rejected_edit = rejected.with_function(
      rejected_caller.value(), [&](bir::FunctionBuilder& function) {
        const auto block = function.create_block("entry");
        if (!block)
          return bir::Result<void, bir::BuildError>::failure(block.error());
        const auto void_result = function.append(
            block.value(), bir::CallSpec{void_target.value(), {}, 11});
        expect(!void_result.has_value() &&
                   void_result.error() == bir::BuildError::UnsupportedOpcode,
               "void Call must reject a source result");
        const auto f32_call = function.append(
            block.value(), bir::CallSpec{rejected_f32_target.value(), {}, 11});
        const auto duplicate = function.append(
            block.value(), bir::CallSpec{rejected_f64_target.value(), {}, 11});
        const auto generic_float = function.append(
            block.value(), bir::CallSpec{generic_float_target.value(), {}, 13});
        const auto f64_call = function.append(
            block.value(), bir::CallSpec{rejected_f64_target.value(), {}, 12});
        expect(f32_call.has_value() && !duplicate.has_value() &&
                   duplicate.error() == bir::BuildError::DuplicateSourceValue &&
                   !generic_float.has_value() &&
                   generic_float.error() == bir::BuildError::UnsupportedOpcode &&
                   f64_call.has_value(),
               "unselected floating forms and duplicate sources must reject without rolling back native Calls");
        return function.set_terminator(block.value(),
                                       bir::ReturnTerm{std::nullopt});
      });
  expect(rejected_edit.has_value(),
         "rejected Call attempts must leave the native floating builder transaction usable");
  auto rejected_raw = std::move(rejected).publish();
  expect(rejected_raw.has_value() &&
             bir::FoundationVerifier::verify(rejected_raw.value()).ok(),
         "void-result and duplicate-source rejections must leave no malformed Call linkage");
}

void test_direct_zero_argument_void_call_rejections() {
  const auto rejected = [](auto mutate, const std::string& message) {
    auto module = direct_void_call_module();
    auto& call = std::get<lir::LirCallOp>(
        module.functions[0].blocks[0].insts[0]);
    mutate(module, call);
    const auto raw = bir::lower_lir_to_raw_bir(module);
    expect(!raw.has_value() &&
               raw.error().code ==
                   bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Raw rollback)");
    const auto canonical = bir::lower_lir_to_canonical_bir(module);
    expect(!canonical.has_value() &&
               canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Canonical rollback)");
  };

  rejected([](auto&, auto& call) { call.direct_callee_link_name_id = 999; },
           "unresolved direct LinkNameId must reject");
  rejected([](auto&, auto& call) { call.direct_callee_link_name_id = 0; },
           "an indirect call without native target identity must reject");
  rejected([](auto&, auto& call) { call.result = lir::LirOperand::raw("%r"); },
           "a presentation-only result must not become result authority");
  rejected([](auto&, auto& call) {
             call.result = lir::LirOperand::ssa("", lir::LirValueId{7});
           },
           "an empty display must not hide result authority");
  rejected([](auto&, auto& call) {
             call.return_type = lir::LirTypeRef::integer(32);
           },
           "nonvoid call return types must reject");
  rejected([](auto&, auto& call) {
             call.return_ext_attr = lir::LirExtAttr::ZeroExt;
           },
           "call return extension must be None");
  rejected([](auto&, auto& call) { call.structured_args.emplace_back(); },
           "structured call arguments remain excluded");
  rejected([](auto&, auto& call) {
             call.arg_type_refs.push_back(lir::LirTypeRef::integer(32));
           },
           "argument type refs remain excluded");
  rejected([](auto&, auto& call) { call.callee_signature.reset(); },
           "missing structured callee signature must reject");
  rejected([](auto&, auto& call) {
             call.callee_signature_ref = lir::LirFunctionSignatureRef{999};
             call.callee_signature.reset();
           },
           "missing retained callee signature without a resolved store entry must reject");
  rejected([](auto&, auto& call) {
             call.callee_signature->return_type_ref.reset();
           },
           "missing typed callee return must reject");
  rejected([](auto&, auto& call) {
             call.callee_signature->return_ext_attr =
                 lir::LirExtAttr::SignExt;
           },
           "callee return extension must be None");
  rejected([](auto&, auto& call) {
             call.callee_signature->is_variadic = true;
           },
           "variadic callee signatures remain excluded");
  rejected([](auto&, auto& call) {
             call.callee_signature->has_unspecified_params = true;
           },
           "unspecified callee signatures remain excluded");
  rejected([](auto&, auto& call) {
             call.callee_signature->fixed_param_types.push_back("i32");
             call.callee_signature->fixed_param_type_refs.push_back(
                 lir::LirTypeRef::integer(32));
             call.callee_signature->has_void_param_list = false;
           },
           "fixed call parameters remain excluded");
  rejected([](auto& module, auto&) {
             module.functions[3].return_type = scalar_type(c4c::TB_INT);
             module.functions[3].signature_return_type_ref =
                 lir::LirTypeRef::integer(32);
           },
           "target signature disagreement must reject atomically");

  const auto store_rejected = [](auto mutate, const std::string& message) {
    auto module = direct_void_call_module();
    attach_direct_void_function_signature_ref(module);
    auto& call = std::get<lir::LirCallOp>(module.functions[0].blocks[0].insts[0]);
    mutate(module, call);
    const auto raw = bir::lower_lir_to_raw_bir(module);
    expect(!raw.has_value() &&
               raw.error().code ==
                   bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Raw rollback)");
    const auto canonical = bir::lower_lir_to_canonical_bir(module);
    expect(!canonical.has_value() &&
               canonical.error().code ==
                   bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Canonical rollback)");
  };
  store_rejected([](auto&, auto& call) {
                   call.callee_signature->return_type_ref =
                       lir::LirTypeRef::integer(32);
                 },
                 "retained/store direct void return mismatch must reject");
  store_rejected([](auto& module, auto& call) {
                   module.function_signature_store[call.callee_signature_ref.value]
                       .return_type_ref = lir::LirTypeRef::integer(32);
                 },
                 "signature-store direct void return mismatch must reject");
  store_rejected([](auto& module, auto& call) {
                   module.function_signature_store[call.callee_signature_ref.value]
                       .fixed_param_type_refs = {lir::LirTypeRef::integer(32)};
                 },
                 "signature-store direct void parameter mismatch must reject");
  store_rejected([](auto& module, auto&) {
                   module.functions[3].signature_return_type_ref =
                       lir::LirTypeRef::integer(32);
                 },
                 "target direct void signature mismatch must reject with store-backed call");
}

lir::LirFunction direct_integer_function(std::string name, bool declaration,
                                         std::vector<c4c::TypeBase> parameters = {}) {
  lir::LirFunction function = declaration
      ? void_declaration(std::move(name))
      : void_definition(std::move(name), {return_block(0, "entry")});
  function.return_type = scalar_type(c4c::TB_INT);
  function.return_type.inner_rank = -1;
  function.signature_return_type_ref = lir::LirTypeRef::integer(32);
  for (const auto parameter : parameters) {
    auto type = scalar_type(parameter);
    type.inner_rank = -1;
    function.params.emplace_back("%parameter-display", type);
    function.signature_params.push_back({"%parameter-signature", type, false});
    function.signature_param_type_refs.push_back(stable_parameter_mirror(parameter));
  }
  return function;
}

lir::LirCallOp direct_integer_call(c4c::LinkNameId target) {
  lir::LirCallOp call;
  call.result = lir::LirOperand::ssa("%misleading-result", lir::LirValueId{9});
  call.return_type = lir::LirTypeRef::integer(32);
  call.callee = lir::LirOperand::raw("%misleading-indirect-display");
  call.direct_callee_link_name_id = target;
  call.args_str = "i64 999 presentation-only";
  lir::LirCallSignature signature;
  signature.return_type_ref = lir::LirTypeRef::integer(32);
  signature.fixed_param_types = {"i32", "i32"};
  signature.fixed_param_type_refs = {lir::LirTypeRef::integer(32),
                                     lir::LirTypeRef::integer(32)};
  call.callee_signature = std::move(signature);
  call.arg_type_refs = {lir::LirTypeRef::integer(32), lir::LirTypeRef::integer(32)};
  call.structured_args = {
      {"i32", lir::LirOperand::integer("misleading-immediate", 7),
       lir::LirTypeRef::integer(32)},
      {"i32", lir::LirOperand::ssa("%misleading-ssa", lir::LirValueId{3}),
       lir::LirTypeRef::integer(32)},
  };
  return call;
}

lir::LirModule direct_integer_call_module() {
  lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  const auto caller_link = module.link_names.intern("direct_integer_caller");
  const auto target_link = module.link_names.intern("direct_integer_target");

  auto caller = direct_integer_function("misleading_caller", false);
  caller.link_name_id = caller_link;
  caller.blocks[0].insts.push_back(
      lir::LirConstInt{lir::LirValueId{3}, scalar_type(c4c::TB_INT), 41});
  caller.blocks[0].insts.push_back(direct_integer_call(target_link));
  caller.blocks[0].terminator = lir::LirRet{
      lir::LirOperand::ssa("%misleading-return", lir::LirValueId{9}),
      lir::LirTypeRef::integer(32)};
  module.functions.push_back(std::move(caller));

  auto target = direct_integer_function("misleading_target", true,
                                        {c4c::TB_INT, c4c::TB_INT});
  target.link_name_id = target_link;
  module.functions.push_back(std::move(target));
  return module;
}

void attach_direct_integer_function_signature_ref(lir::LirModule& module) {
  auto& call = std::get<lir::LirCallOp>(module.functions[0].blocks[0].insts[1]);
  lir::LirFunctionSignatureStoreEntry caller_entry;
  caller_entry.return_type_ref = lir::LirTypeRef::integer(32);
  module.functions[0].function_signature_ref =
      module.register_function_signature(std::move(caller_entry));

  lir::LirFunctionSignatureStoreEntry entry;
  entry.return_type_ref = lir::LirTypeRef::integer(32);
  entry.fixed_param_type_refs = {lir::LirTypeRef::integer(32),
                                 lir::LirTypeRef::integer(32)};
  entry.fixed_param_is_byval = {false, false};
  const lir::LirFunctionSignatureRef ref =
      module.register_function_signature(std::move(entry));
  module.functions[1].function_signature_ref = ref;
  call.callee_signature_ref = ref;
}

void test_direct_integer_call_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto caller_id = view.functions()[0];
    const auto caller = view.function(caller_id).value();
    const auto instructions = caller.instructions(caller.blocks()[0]).value();
    expect(instructions.size() == 1, layer + " must retain one typed Call");
    const auto call = caller.instruction(instructions[0]).value();
    expect(call.opcode() == bir::Opcode::Call && call.call() &&
               call.call()->callee == view.functions()[1] &&
               call.operands().size() == 2 && call.results().size() == 1,
           layer + " must retain typed direct callee, ordered arguments, and result");
    const auto result = caller.value(call.results()[0]).value();
    expect(result.type == bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
               result.source_id == bir::SourceValueId{caller_id, 9},
           layer + " Call result must retain the owning LirValueId");
    expect(caller.source_value(bir::SourceValueId{caller_id, 9}).value() ==
               call.results()[0],
           layer + " Call result source lookup must be owner-scoped");
  };

  const auto module = direct_integer_call_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "structured direct integer call must publish verified Raw BIR");
  inspect(raw.value(), "Raw BIR");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(), "structured direct integer call must canonicalize");
  inspect(canonical.value(), "Canonical BIR");

  auto missing_retained_signature = direct_integer_call_module();
  attach_direct_integer_function_signature_ref(missing_retained_signature);
  auto& missing_retained_call = std::get<lir::LirCallOp>(
      missing_retained_signature.functions[0].blocks[0].insts[1]);
  missing_retained_call.callee =
      lir::LirOperand::global("@direct_integer_target",
                              missing_retained_call.direct_callee_link_name_id);
  missing_retained_call.callee_signature.reset();
  const auto raw_missing_retained =
      bir::lower_lir_to_raw_bir(missing_retained_signature);
  expect(raw_missing_retained.has_value() &&
             bir::FoundationVerifier::verify(raw_missing_retained.value()).ok(),
         "direct integer call must use module signature store when retained signature is absent" +
             (raw_missing_retained.has_value()
                  ? std::string{}
                  : ": " + raw_missing_retained.error().detail));
  inspect(raw_missing_retained.value(), "Raw BIR store-backed direct integer call");
  const auto canonical_missing_retained =
      bir::lower_lir_to_canonical_bir(missing_retained_signature);
  expect(canonical_missing_retained.has_value(),
         "store-backed direct integer call without retained signature must canonicalize");
  inspect(canonical_missing_retained.value(),
          "Canonical BIR store-backed direct integer call");

  auto stale_text_signature = direct_integer_call_module();
  attach_direct_integer_function_signature_ref(stale_text_signature);
  auto& stale_text_call = std::get<lir::LirCallOp>(
      stale_text_signature.functions[0].blocks[0].insts[1]);
  stale_text_call.callee =
      lir::LirOperand::global("@direct_integer_target",
                              stale_text_call.direct_callee_link_name_id);
  stale_text_call.callee_signature->fixed_param_types = {
      "i64 stale text only", "i1 stale text only"};
  stale_text_call.callee_type_suffix = "(ptr stale suffix)";
  stale_text_call.args_str = "double stale mirror";
  const auto raw_stale_text = bir::lower_lir_to_raw_bir(stale_text_signature);
  expect(raw_stale_text.has_value() &&
             bir::FoundationVerifier::verify(raw_stale_text.value()).ok(),
         "direct integer call must ignore retained text when signature ref resolves");
  inspect(raw_stale_text.value(), "Raw BIR stale-text direct integer call");

  const auto rejected = [](auto mutate, const std::string& message) {
    auto rejected_module = direct_integer_call_module();
    attach_direct_integer_function_signature_ref(rejected_module);
    auto& call = std::get<lir::LirCallOp>(rejected_module.functions[0].blocks[0].insts[1]);
    mutate(rejected_module, call);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(rejected_module);
    expect(!raw_rejected.has_value() && raw_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(rejected_module);
    expect(!canonical_rejected.has_value() && canonical_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Canonical rollback)");
  };
  rejected([](auto&, auto& call) { call.direct_callee_link_name_id = 999; },
           "missing direct callee identity must reject");
  rejected([](auto&, auto& call) {
             call.structured_args[1].operand =
                 lir::LirOperand::ssa("%foreign", lir::LirValueId{77});
           }, "cross-owner or missing SSA authority must reject");
  rejected([](auto&, auto& call) {
             call.callee_signature->fixed_param_type_refs[1] =
                 lir::LirTypeRef::integer(64);
           }, "callee signature disagreement must reject");
  rejected([](auto& module, auto& call) {
             module.function_signature_store[call.callee_signature_ref.value]
                 .fixed_param_type_refs[1] = lir::LirTypeRef::integer(64);
           }, "signature-store disagreement must reject");
  rejected([](auto&, auto& call) {
             call.structured_args[1].type_ref = lir::LirTypeRef::integer(64);
           }, "argument type disagreement must reject");
  rejected([](auto&, auto& call) {
             call.structured_args.pop_back();
           }, "argument count disagreement must reject");
  rejected([](auto&, auto& call) {
             call.structured_args[0].operand = lir::LirOperand::global("@alternative", 1);
           }, "non-immediate non-SSA argument alternatives must reject");
  rejected([](auto&, auto& call) { call.return_type = lir::LirTypeRef("double"); },
           "floating calls must remain fail-closed");
}

lir::LirFunction direct_native_floating_function(std::string name,
                                                  bool declaration) {
  lir::LirFunction function = declaration
      ? void_declaration(std::move(name))
      : void_definition(std::move(name), {return_block(0, "entry")});
  function.return_type = scalar_type(c4c::TB_DOUBLE);
  function.return_type.inner_rank = -1;
  function.signature_return_type_ref = lir::LirTypeRef("double");
  auto void_parameter = scalar_type(c4c::TB_VOID);
  void_parameter.inner_rank = -1;
  function.params.emplace_back("%void-display", void_parameter);
  function.signature_has_void_param_list = true;
  return function;
}

lir::LirCallOp direct_native_floating_call(c4c::LinkNameId target) {
  lir::LirCallOp call;
  call.result = lir::LirOperand::ssa("%misleading-double-result",
                                     lir::LirValueId{9});
  call.return_type = lir::LirTypeRef("double");
  call.callee = lir::LirOperand::raw("%presentation-only-indirect-display");
  call.direct_callee_link_name_id = target;
  call.callee_type_suffix = "(i64) presentation-only";
  call.args_str = "i64 99 presentation-only";
  lir::LirCallSignature signature;
  signature.return_type_ref = lir::LirTypeRef("double");
  signature.has_void_param_list = true;
  call.callee_signature = std::move(signature);
  return call;
}

lir::LirModule direct_native_floating_call_module() {
  lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  const auto caller_link = module.link_names.intern("direct_native_float_caller");
  const auto target_link = module.link_names.intern("direct_native_float_target");

  auto caller = void_definition("misleading_native_float_caller",
                                {return_block(0, "entry")});
  caller.link_name_id = caller_link;
  caller.blocks[0].insts.push_back(direct_native_floating_call(target_link));
  module.functions.push_back(std::move(caller));

  auto target = direct_native_floating_function(
      "misleading_native_float_target", true);
  target.link_name_id = target_link;
  module.functions.push_back(std::move(target));
  return module;
}

void attach_direct_native_floating_function_signature_ref(
    lir::LirModule& module) {
  lir::LirFunctionSignatureStoreEntry caller_entry;
  caller_entry.return_type_ref = lir::LirTypeRef("void");
  caller_entry.has_void_param_list = true;
  module.functions[0].function_signature_ref =
      module.register_function_signature(std::move(caller_entry));

  lir::LirFunctionSignatureStoreEntry target_entry;
  target_entry.return_type_ref = lir::LirTypeRef("double");
  target_entry.has_void_param_list = true;
  const lir::LirFunctionSignatureRef target_ref =
      module.register_function_signature(std::move(target_entry));
  module.functions[1].function_signature_ref = target_ref;
  auto& call =
      std::get<lir::LirCallOp>(module.functions[0].blocks[0].insts[0]);
  call.callee_signature_ref = target_ref;
}

lir::LirModule downstream_double_fadd_module() {
  auto module = direct_native_floating_call_module();
  auto& block = module.functions[0].blocks[0];
  block.insts.push_back(lir::LirConstFloat{
      lir::LirValueId{10}, scalar_type(c4c::TB_DOUBLE), 1.25});
  block.insts.push_back(lir::LirBinOp{
      lir::LirOperand::ssa("%misleading-double-fadd", lir::LirValueId{11}),
      lir::LirBinaryOpcode::FAdd, lir::LirTypeRef("double"),
      lir::LirOperand::ssa("%presentation-only-call-use", lir::LirValueId{9}),
      lir::LirOperand::ssa("%presentation-only-rhs", lir::LirValueId{10})});
  block.insts.push_back(lir::LirBinOp{
      lir::LirOperand::ssa("%misleading-double-fmul", lir::LirValueId{12}),
      lir::LirBinaryOpcode::FMul, lir::LirTypeRef("double"),
      lir::LirOperand::ssa("%presentation-only-fadd-use", lir::LirValueId{11}),
      lir::LirOperand::ssa("%presentation-only-rhs", lir::LirValueId{10})});
  return module;
}

void test_downstream_double_fadd_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto caller_id = view.functions()[0];
    const auto caller = view.function(caller_id).value();
    const auto insts = caller.instructions(caller.blocks()[0]).value();
    expect(insts.size() == 3,
           layer + " must retain the direct double Call, FAdd, and FMul instructions");
    const auto call = caller.instruction(insts[0]).value();
    const auto fadd = caller.instruction(insts[1]).value();
    const auto fmul = caller.instruction(insts[2]).value();
    const auto result = caller.value(fadd.results()[0]).value();
    expect(call.opcode() == bir::Opcode::Call && call.results().size() == 1 &&
               fadd.opcode() == bir::Opcode::Binary && fadd.binary() &&
               fadd.binary()->opcode == bir::BinaryOpcode::FAdd &&
               fadd.binary()->type == bir::Type{bir::TypeKind::F64, 64, "double"} &&
               fadd.operands().size() == 2 &&
               fadd.operands()[0] == call.results()[0] &&
               caller.value(fadd.operands()[1]).value().type ==
                   bir::Type{bir::TypeKind::F64, 64, "double"} &&
               fadd.results().size() == 1 &&
               result.type == bir::Type{bir::TypeKind::F64, 64, "double"} &&
               result.source_id == bir::SourceValueId{caller_id, 11} &&
               caller.source_value(*result.source_id).value() == fadd.results()[0] &&
               fmul.opcode() == bir::Opcode::Binary && fmul.binary() &&
               fmul.binary()->opcode == bir::BinaryOpcode::FMul &&
               fmul.binary()->type == bir::Type{bir::TypeKind::F64, 64, "double"} &&
               fmul.operands().size() == 2 && fmul.operands()[0] == fadd.results()[0] &&
               fmul.results().size() == 1 &&
               caller.value(fmul.results()[0]).value().source_id ==
                   bir::SourceValueId{caller_id, 12},
           layer + " must retain one source-backed F64 FAdd-to-FMul chain with ordered current-function SSA edges");
  };

  const auto module = downstream_double_fadd_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "downstream double FAdd must publish verified Raw BIR");
  inspect(raw.value(), "Raw BIR");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(), "downstream double FAdd must canonicalize");
  inspect(canonical.value(), "Canonical BIR");

  const auto rejected = [](auto mutate, const std::string& message) {
    auto candidate = downstream_double_fadd_module();
    auto& fadd = std::get<lir::LirBinOp>(candidate.functions[0].blocks[0].insts[2]);
    auto& fmul = std::get<lir::LirBinOp>(candidate.functions[0].blocks[0].insts[3]);
    mutate(candidate, fadd, fmul);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() && raw_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value() && canonical_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Canonical rollback)");
  };
  rejected([](auto&, auto& fadd, auto&) {
             fadd.lhs = lir::LirOperand::ssa("%missing", lir::LirValueId{77});
           }, "missing accepted direct-call result must reject");
  rejected([](auto&, auto& fadd, auto&) {
             fadd.result = lir::LirOperand::ssa("%duplicate", lir::LirValueId{9});
           }, "duplicate FAdd result identity must reject");
  rejected([](auto&, auto& fadd, auto&) {
             fadd.rhs = lir::LirOperand::ssa("%foreign", lir::LirValueId{77});
           }, "cross-owner or missing RHS SSA authority must reject");
  rejected([](auto&, auto& fadd, auto&) { fadd.type_str = lir::LirTypeRef("float"); },
           "wrong FAdd type must reject");
  rejected([](auto&, auto& fadd, auto&) { fadd.opcode = lir::LirBinaryOpcode::FSub; },
           "non-FAdd opcode must reject");
  rejected([](auto&, auto& fadd, auto&) {
             fadd.rhs = lir::LirOperand::integer("literal", 1);
           }, "non-SSA FAdd operand must reject");
  rejected([](auto&, auto& fadd, auto&) { fadd.result = lir::LirOperand::raw("%raw"); },
           "malformed FAdd result authority must reject");
  rejected([](auto&, auto&, auto& fmul) {
             fmul.lhs = lir::LirOperand::ssa("%missing-fadd", lir::LirValueId{77});
           }, "missing FAdd-to-FMul linkage must reject");
  rejected([](auto&, auto&, auto& fmul) {
             fmul.result = lir::LirOperand::ssa("%duplicate-fmul", lir::LirValueId{11});
           }, "duplicate FMul result identity must reject");
  rejected([](auto&, auto&, auto& fmul) {
             fmul.rhs = lir::LirOperand::ssa("%foreign-fmul", lir::LirValueId{77});
           }, "unresolved FMul rhs authority must reject");
  rejected([](auto&, auto&, auto& fmul) { fmul.opcode = lir::LirBinaryOpcode::FAdd; },
           "non-FMul opcode must reject");
  rejected([](auto&, auto&, auto& fmul) { fmul.type_str = lir::LirTypeRef("float"); },
           "wrong FMul type must reject");
}

lir::LirModule downstream_double_olt_compare_module() {
  auto module = downstream_double_fadd_module();
  auto& block = module.functions[0].blocks[0];
  block.insts.push_back(lir::LirCmpOp{
      lir::LirOperand::ssa("%misleading-double-olt", lir::LirValueId{13}), true,
      lir::LirCmpPredicate::OLt, lir::LirTypeRef("double"),
      lir::LirOperand::ssa("%presentation-only-fmul-use", lir::LirValueId{12}),
      lir::LirOperand::ssa("%presentation-only-call-use", lir::LirValueId{9})});
  block.insts.push_back(lir::LirCastOp{
      .kind = lir::LirCastKind::ZExt,
      .from_type = lir::LirTypeRef::integer(1),
      .operand = lir::LirOperand::ssa("%presentation-only-olt-use", lir::LirValueId{13}),
      .to_type = lir::LirTypeRef::integer(32),
  });
  return module;
}

void test_downstream_double_olt_compare_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto caller = view.function(view.functions()[0]).value();
    const auto insts = caller.instructions(caller.blocks()[0]).value();
    expect(insts.size() == 4, layer + " must retain the compare but not the compatibility ZExt");
    const auto compare = caller.instruction(insts[3]).value();
    const auto result = caller.value(compare.results()[0]).value();
    expect(compare.opcode() == bir::Opcode::Compare && compare.compare() &&
               compare.compare()->predicate == bir::ComparePredicate::OLt &&
               compare.compare()->type == bir::Type{bir::TypeKind::F64, 64, "double"} &&
               compare.operands().size() == 2 && compare.operands()[0] ==
                   caller.instruction(insts[2]).value().results()[0] &&
               compare.results().size() == 1 && result.type == bir::Type{bir::TypeKind::I1, 1, "i1"} &&
               result.source_id == bir::SourceValueId{view.functions()[0], 13},
           layer + " must retain the native double OLt result and its F64 producer edge");
  };
  const auto module = downstream_double_olt_compare_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "double OLt with its compatibility use must publish verified Raw BIR");
  inspect(raw.value(), "Raw BIR");

  const auto rejected = [](auto mutate, const std::string& message) {
    auto candidate = downstream_double_olt_compare_module();
    auto& compare = std::get<lir::LirCmpOp>(candidate.functions[0].blocks[0].insts[4]);
    auto& use = std::get<lir::LirCastOp>(candidate.functions[0].blocks[0].insts[5]);
    mutate(candidate, compare, use);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() && raw_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Raw rollback)");
  };
  rejected([](auto&, auto& compare, auto&) { compare.result = lir::LirOperand::ssa("%bad", lir::LirValueId::invalid()); },
           "invalid compare result must reject");
  rejected([](auto&, auto& compare, auto&) { compare.predicate = lir::LirCmpPredicate::OEq; },
           "wrong floating predicate must reject");
  rejected([](auto&, auto& compare, auto&) { compare.is_float = false; },
           "wrong compare mode must reject");
  rejected([](auto&, auto& compare, auto&) { compare.type_str = lir::LirTypeRef("float"); },
           "wrong compare type must reject");
  rejected([](auto&, auto& compare, auto&) { compare.lhs = lir::LirOperand::ssa("%missing", lir::LirValueId{77}); },
           "unresolved compare producer must reject");
  rejected([](auto&, auto&, auto& use) { use.operand = lir::LirOperand::ssa("%missing", lir::LirValueId{77}); },
           "unresolved compatibility use must reject");
  rejected([](auto&, auto&, auto& use) { use.result = lir::LirOperand::ssa("%received", lir::LirValueId{14}); },
           "compatibility ZExt result must remain unreceived");
  rejected([](auto& candidate, auto&, auto&) { candidate.functions[0].blocks[0].insts.pop_back(); },
           "missing compatibility use must reject");
}

lir::LirModule scalar_double_to_float_fptrunc_module() {
  auto module = downstream_double_fadd_module();
  auto& block = module.functions[0].blocks[0];
  block.insts.push_back(lir::LirConstFloat{
      lir::LirValueId{13}, scalar_type(c4c::TB_FLOAT), 1.5});
  block.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand::ssa("%presentation-only-fptrunc-result", lir::LirValueId{14}),
      .kind = lir::LirCastKind::FPTrunc,
      .from_type = lir::LirTypeRef("double"),
      .operand = lir::LirOperand::ssa("%presentation-only-double-fmul", lir::LirValueId{12}),
      .to_type = lir::LirTypeRef("float"),
  });
  block.insts.push_back(lir::LirBinOp{
      lir::LirOperand::ssa("%presentation-only-float-fmul", lir::LirValueId{15}),
      lir::LirBinaryOpcode::FMul, lir::LirTypeRef("float"),
      lir::LirOperand::ssa("%presentation-only-fptrunc-use", lir::LirValueId{14}),
      lir::LirOperand::ssa("%presentation-only-float-rhs", lir::LirValueId{13})});
  return module;
}

void test_scalar_double_to_float_fptrunc_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto caller_id = view.functions()[0];
    const auto caller = view.function(caller_id).value();
    const auto insts = caller.instructions(caller.blocks()[0]).value();
    expect(insts.size() == 5, layer + " must retain the scalar FPTrunc and float FMul");
    const auto fptrunc = caller.instruction(insts[3]).value();
    const auto fmul = caller.instruction(insts[4]).value();
    expect(fptrunc.opcode() == bir::Opcode::Cast && fptrunc.cast() &&
               fptrunc.cast()->kind == bir::CastKind::FPTrunc &&
               fptrunc.cast()->from_type == bir::Type{bir::TypeKind::F64, 64, "double"} &&
               fptrunc.cast()->to_type == bir::Type{bir::TypeKind::F32, 32, "float"} &&
               fptrunc.operands() == std::vector<bir::ValueId>{caller.instruction(insts[2]).value().results()[0]} &&
               fptrunc.results().size() == 1 &&
               caller.value(fptrunc.results()[0]).value().source_id == bir::SourceValueId{caller_id, 14} &&
               fmul.binary() && fmul.binary()->opcode == bir::BinaryOpcode::FMul &&
               fmul.binary()->type == bir::Type{bir::TypeKind::F32, 32, "float"} &&
               fmul.operands().size() == 2 && fmul.operands()[0] == fptrunc.results()[0] &&
               fmul.results().size() == 1 &&
               caller.value(fmul.results()[0]).value().source_id == bir::SourceValueId{caller_id, 15},
           layer + " must preserve the native double-to-float FPTrunc and exact float FMul use");
  };
  const auto module = scalar_double_to_float_fptrunc_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "scalar FPTrunc must publish verified Raw BIR");
  inspect(raw.value(), "Raw BIR");

  const auto rejected = [](auto mutate, const std::string& message) {
    auto candidate = scalar_double_to_float_fptrunc_module();
    auto& cast = std::get<lir::LirCastOp>(candidate.functions[0].blocks[0].insts[5]);
    auto& use = std::get<lir::LirBinOp>(candidate.functions[0].blocks[0].insts[6]);
    mutate(candidate, cast, use);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() && raw_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Raw rollback)");
  };
  rejected([](auto&, auto& cast, auto&) { cast.result = lir::LirOperand::raw("%missing"); }, "missing cast result authority must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.operand = lir::LirOperand::integer("bad", 0); }, "non-SSA cast source must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.operand = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); }, "unresolved or cross-owner cast source must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.result = lir::LirOperand::ssa("%duplicate", lir::LirValueId{12}); }, "duplicate cast result must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.kind = lir::LirCastKind::FPExt; }, "other floating casts must remain fail-closed");
  rejected([](auto&, auto& cast, auto&) { cast.from_type = lir::LirTypeRef("float"); }, "wrong cast source endpoint must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.to_type = lir::LirTypeRef("double"); }, "wrong cast destination endpoint must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.lhs = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); }, "unresolved downstream float FMul use must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.opcode = lir::LirBinaryOpcode::FAdd; }, "non-FMul downstream use must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.type_str = lir::LirTypeRef("double"); }, "wrong downstream float FMul type must reject atomically");
  rejected([](auto& candidate, auto&, auto&) { candidate.functions[0].blocks[0].insts.pop_back(); }, "missing downstream float FMul use must reject atomically");
}

lir::LirModule scalar_float_to_double_fpext_module() {
  auto module = scalar_double_to_float_fptrunc_module();
  auto& block = module.functions[0].blocks[0];
  block.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand::ssa("%presentation-only-fpext-result", lir::LirValueId{16}),
      .kind = lir::LirCastKind::FPExt,
      .from_type = lir::LirTypeRef("float"),
      .operand = lir::LirOperand::ssa("%presentation-only-fptrunc-source", lir::LirValueId{14}),
      .to_type = lir::LirTypeRef("double"),
  });
  block.insts.push_back(lir::LirBinOp{
      lir::LirOperand::ssa("%presentation-only-double-fmul", lir::LirValueId{17}),
      lir::LirBinaryOpcode::FMul, lir::LirTypeRef("double"),
      lir::LirOperand::ssa("%presentation-only-fpext-use", lir::LirValueId{16}),
      lir::LirOperand::ssa("%presentation-only-double-rhs", lir::LirValueId{12})});
  return module;
}

void test_scalar_float_to_double_fpext_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto caller_id = view.functions()[0];
    const auto caller = view.function(caller_id).value();
    const auto insts = caller.instructions(caller.blocks()[0]).value();
    expect(insts.size() == 7, layer + " must retain the scalar FPExt and double FMul");
    const auto fpext = caller.instruction(insts[5]).value();
    const auto fmul = caller.instruction(insts[6]).value();
    expect(fpext.opcode() == bir::Opcode::Cast && fpext.cast() &&
               fpext.cast()->kind == bir::CastKind::FPExt &&
               fpext.cast()->from_type == bir::Type{bir::TypeKind::F32, 32, "float"} &&
               fpext.cast()->to_type == bir::Type{bir::TypeKind::F64, 64, "double"} &&
               fpext.operands() == std::vector<bir::ValueId>{caller.instruction(insts[3]).value().results()[0]} &&
               fpext.results().size() == 1 &&
               caller.value(fpext.results()[0]).value().source_id == bir::SourceValueId{caller_id, 16} &&
               fmul.binary() && fmul.binary()->opcode == bir::BinaryOpcode::FMul &&
               fmul.binary()->type == bir::Type{bir::TypeKind::F64, 64, "double"} &&
               fmul.operands().size() == 2 && fmul.operands()[0] == fpext.results()[0] &&
               fmul.results().size() == 1 &&
               caller.value(fmul.results()[0]).value().source_id == bir::SourceValueId{caller_id, 17},
           layer + " must preserve the native float-to-double FPExt and exact double FMul use");
  };
  const auto module = scalar_float_to_double_fpext_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  if (!raw.has_value())
    fail("scalar FPExt must publish Raw BIR: " + raw.error().detail +
         (raw.error().verification_errors.empty() ? "" :
          ": " + raw.error().verification_errors.front().message));
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "scalar FPExt must publish verified Raw BIR");
  inspect(raw.value(), "Raw BIR");

  const auto rejected = [](auto mutate, const std::string& message) {
    auto candidate = scalar_float_to_double_fpext_module();
    auto& cast = std::get<lir::LirCastOp>(candidate.functions[0].blocks[0].insts[7]);
    auto& use = std::get<lir::LirBinOp>(candidate.functions[0].blocks[0].insts[8]);
    mutate(candidate, cast, use);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() && raw_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Raw rollback)");
  };
  rejected([](auto&, auto& cast, auto&) { cast.result = lir::LirOperand::raw("%missing"); }, "missing cast result authority must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.operand = lir::LirOperand::integer("bad", 0); }, "non-SSA cast source must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.operand = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); }, "unresolved or cross-owner cast source must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.result = lir::LirOperand::ssa("%duplicate", lir::LirValueId{14}); }, "duplicate cast result must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.kind = lir::LirCastKind::FPTrunc; }, "other floating casts must remain fail-closed");
  rejected([](auto&, auto& cast, auto&) { cast.from_type = lir::LirTypeRef("double"); }, "wrong cast source endpoint must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.to_type = lir::LirTypeRef("float"); }, "wrong cast destination endpoint must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.lhs = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); }, "unresolved downstream double FMul use must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.opcode = lir::LirBinaryOpcode::FAdd; }, "non-FMul downstream use must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.type_str = lir::LirTypeRef("float"); }, "wrong downstream double FMul type must reject atomically");
  rejected([](auto& candidate, auto&, auto&) { candidate.functions[0].blocks[0].insts.pop_back(); }, "missing downstream double FMul use must reject atomically");
}

lir::LirModule normalized_i32_add_module();

lir::LirModule scalar_signed_i32_to_double_sitofp_module() {
  auto module = normalized_i32_add_module();
  auto& block = module.functions[0].blocks[0];
  block.insts.push_back(lir::LirConstFloat{
      lir::LirValueId{34}, scalar_type(c4c::TB_DOUBLE), 2.5});
  block.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand::ssa("%presentation-only-sitofp-result", lir::LirValueId{35}),
      .kind = lir::LirCastKind::SIToFP,
      .from_type = lir::LirTypeRef::integer(32),
      .operand = lir::LirOperand::ssa("%presentation-only-signed-add", lir::LirValueId{33}),
      .to_type = lir::LirTypeRef("double"),
  });
  block.insts.push_back(lir::LirBinOp{
      lir::LirOperand::ssa("%presentation-only-sitofp-fmul", lir::LirValueId{36}),
      lir::LirBinaryOpcode::FMul, lir::LirTypeRef("double"),
      lir::LirOperand::ssa("%presentation-only-sitofp-use", lir::LirValueId{35}),
      lir::LirOperand::ssa("%presentation-only-double-rhs", lir::LirValueId{34})});
  return module;
}

void test_scalar_signed_i32_to_double_sitofp_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto caller_id = view.functions()[0];
    const auto caller = view.function(caller_id).value();
    const auto insts = caller.instructions(caller.blocks()[0]).value();
    expect(insts.size() == 4, layer + " must retain the signed i32 Add, SIToFP, and double FMul");
    const auto add = caller.instruction(insts[1]).value();
    const auto sitofp = caller.instruction(insts[2]).value();
    const auto fmul = caller.instruction(insts[3]).value();
    expect(add.binary() && add.binary()->opcode == bir::BinaryOpcode::Add &&
               add.binary()->type == bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
               sitofp.opcode() == bir::Opcode::Cast && sitofp.cast() &&
               sitofp.cast()->kind == bir::CastKind::SIToFP &&
               sitofp.cast()->from_type == bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
               sitofp.cast()->to_type == bir::Type{bir::TypeKind::F64, 64, "double"} &&
               sitofp.operands() == std::vector<bir::ValueId>{add.results()[0]} &&
               sitofp.results().size() == 1 &&
               caller.value(sitofp.results()[0]).value().source_id == bir::SourceValueId{caller_id, 35} &&
               fmul.binary() && fmul.binary()->opcode == bir::BinaryOpcode::FMul &&
               fmul.binary()->type == bir::Type{bir::TypeKind::F64, 64, "double"} &&
               fmul.operands().size() == 2 && fmul.operands()[0] == sitofp.results()[0] &&
               fmul.results().size() == 1 &&
               caller.value(fmul.results()[0]).value().source_id == bir::SourceValueId{caller_id, 36},
           layer + " must preserve the native signed i32-to-double SIToFP and exact double FMul use");
  };
  const auto module = scalar_signed_i32_to_double_sitofp_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "scalar SIToFP must publish verified Raw BIR" +
             (raw.has_value() ? std::string{} : ": " + raw.error().detail));
  inspect(raw.value(), "Raw BIR");

  const auto rejected = [](auto mutate, const std::string& message) {
    auto candidate = scalar_signed_i32_to_double_sitofp_module();
    auto& cast = std::get<lir::LirCastOp>(candidate.functions[0].blocks[0].insts[3]);
    auto& use = std::get<lir::LirBinOp>(candidate.functions[0].blocks[0].insts[4]);
    mutate(candidate, cast, use);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() && raw_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Raw rollback)");
  };
  rejected([](auto&, auto& cast, auto&) { cast.result = lir::LirOperand::raw("%missing"); }, "missing cast result authority must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.operand = lir::LirOperand::integer("bad", 0); }, "non-SSA cast source must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.operand = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); }, "unresolved or cross-owner cast source must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.result = lir::LirOperand::ssa("%duplicate", lir::LirValueId{33}); }, "duplicate cast result must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.from_type = lir::LirTypeRef::integer(64); }, "wrong cast source endpoint must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.to_type = lir::LirTypeRef("float"); }, "wrong cast destination endpoint must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.lhs = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); }, "unresolved downstream double FMul use must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.opcode = lir::LirBinaryOpcode::FAdd; }, "non-FMul downstream use must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.type_str = lir::LirTypeRef("float"); }, "wrong downstream double FMul type must reject atomically");
  rejected([](auto& candidate, auto&, auto&) { candidate.functions[0].blocks[0].insts.pop_back(); }, "missing downstream double FMul use must reject atomically");
}

lir::LirModule scalar_unsigned_i32_to_double_uitofp_module() {
  auto module = normalized_i32_add_module();
  auto& block = module.functions[0].blocks[0];
  block.insts.push_back(lir::LirConstFloat{
      lir::LirValueId{34}, scalar_type(c4c::TB_DOUBLE), 2.5});
  block.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand::ssa("%presentation-only-uitofp-result", lir::LirValueId{35}),
      .kind = lir::LirCastKind::UIToFP,
      .from_type = lir::LirTypeRef::integer(32),
      .operand = lir::LirOperand::ssa("%presentation-only-unsigned-add", lir::LirValueId{33}),
      .to_type = lir::LirTypeRef("double"),
  });
  block.insts.push_back(lir::LirBinOp{
      lir::LirOperand::ssa("%presentation-only-uitofp-fmul", lir::LirValueId{36}),
      lir::LirBinaryOpcode::FMul, lir::LirTypeRef("double"),
      lir::LirOperand::ssa("%presentation-only-uitofp-use", lir::LirValueId{35}),
      lir::LirOperand::ssa("%presentation-only-double-rhs", lir::LirValueId{34})});
  return module;
}

void test_scalar_unsigned_i32_to_double_uitofp_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto caller_id = view.functions()[0];
    const auto caller = view.function(caller_id).value();
    const auto insts = caller.instructions(caller.blocks()[0]).value();
    expect(insts.size() == 4, layer + " must retain the unsigned i32 Add, UIToFP, and double FMul");
    const auto add = caller.instruction(insts[1]).value();
    const auto uitofp = caller.instruction(insts[2]).value();
    const auto fmul = caller.instruction(insts[3]).value();
    expect(add.binary() && add.binary()->opcode == bir::BinaryOpcode::Add &&
               add.binary()->type == bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
               uitofp.opcode() == bir::Opcode::Cast && uitofp.cast() &&
               uitofp.cast()->kind == bir::CastKind::UIToFP &&
               uitofp.cast()->from_type == bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
               uitofp.cast()->to_type == bir::Type{bir::TypeKind::F64, 64, "double"} &&
               uitofp.operands() == std::vector<bir::ValueId>{add.results()[0]} &&
               uitofp.results().size() == 1 &&
               caller.value(uitofp.results()[0]).value().source_id == bir::SourceValueId{caller_id, 35} &&
               fmul.binary() && fmul.binary()->opcode == bir::BinaryOpcode::FMul &&
               fmul.binary()->type == bir::Type{bir::TypeKind::F64, 64, "double"} &&
               fmul.operands().size() == 2 && fmul.operands()[0] == uitofp.results()[0] &&
               fmul.results().size() == 1 &&
               caller.value(fmul.results()[0]).value().source_id == bir::SourceValueId{caller_id, 36},
           layer + " must preserve the native unsigned i32-to-double UIToFP and exact double FMul use");
  };
  const auto module = scalar_unsigned_i32_to_double_uitofp_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "scalar UIToFP must publish verified Raw BIR" +
             (raw.has_value() ? std::string{} : ": " + raw.error().detail));
  inspect(raw.value(), "Raw BIR");

  const auto rejected = [](auto mutate, const std::string& message) {
    auto candidate = scalar_unsigned_i32_to_double_uitofp_module();
    auto& cast = std::get<lir::LirCastOp>(candidate.functions[0].blocks[0].insts[3]);
    auto& use = std::get<lir::LirBinOp>(candidate.functions[0].blocks[0].insts[4]);
    mutate(candidate, cast, use);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() && raw_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Raw rollback)");
  };
  rejected([](auto&, auto& cast, auto&) { cast.result = lir::LirOperand::raw("%missing"); }, "missing cast result authority must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.operand = lir::LirOperand::integer("bad", 0); }, "non-SSA cast source must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.operand = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); }, "unresolved or cross-owner cast source must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.result = lir::LirOperand::ssa("%duplicate", lir::LirValueId{33}); }, "duplicate cast result must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.kind = lir::LirCastKind::FPToUI; }, "other conversion families must remain fail-closed");
  rejected([](auto&, auto& cast, auto&) { cast.from_type = lir::LirTypeRef::integer(64); }, "wrong cast source endpoint must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.to_type = lir::LirTypeRef("float"); }, "wrong cast destination endpoint must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.lhs = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); }, "unresolved downstream double FMul use must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.opcode = lir::LirBinaryOpcode::FAdd; }, "non-FMul downstream use must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.type_str = lir::LirTypeRef("float"); }, "wrong downstream double FMul type must reject atomically");
  rejected([](auto& candidate, auto&, auto&) { candidate.functions[0].blocks[0].insts.pop_back(); }, "missing downstream double FMul use must reject atomically");
}

lir::LirModule scalar_double_to_signed_i32_fptosi_module() {
  auto module = downstream_double_fadd_module();
  auto& block = module.functions[0].blocks[0];
  block.insts.pop_back();
  block.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand::ssa("%presentation-only-fptosi-result", lir::LirValueId{12}),
      .kind = lir::LirCastKind::FPToSI,
      .from_type = lir::LirTypeRef("double"),
      .operand = lir::LirOperand::ssa("%presentation-only-fadd", lir::LirValueId{11}),
      .to_type = lir::LirTypeRef::integer(32),
  });
  block.insts.push_back(lir::LirBinOp{
      lir::LirOperand::ssa("%presentation-only-fptosi-add", lir::LirValueId{13}),
      lir::LirBinaryOpcode::Add, lir::LirTypeRef::integer(32),
      lir::LirOperand::ssa("%presentation-only-fptosi-use", lir::LirValueId{12}),
      lir::LirOperand::integer("presentation-four", 4)});
  return module;
}

void test_scalar_double_to_signed_i32_fptosi_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto caller_id = view.functions()[0];
    const auto caller = view.function(caller_id).value();
    const auto insts = caller.instructions(caller.blocks()[0]).value();
    expect(insts.size() == 4, layer + " must retain the double FAdd, FPToSI, and i32 Add");
    const auto fadd = caller.instruction(insts[1]).value();
    const auto fptosi = caller.instruction(insts[2]).value();
    const auto add = caller.instruction(insts[3]).value();
    expect(fadd.binary() && fadd.binary()->opcode == bir::BinaryOpcode::FAdd &&
               fadd.binary()->type == bir::Type{bir::TypeKind::F64, 64, "double"} &&
               fptosi.opcode() == bir::Opcode::Cast && fptosi.cast() &&
               fptosi.cast()->kind == bir::CastKind::FPToSI &&
               fptosi.cast()->from_type == bir::Type{bir::TypeKind::F64, 64, "double"} &&
               fptosi.cast()->to_type == bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
               fptosi.operands() == std::vector<bir::ValueId>{fadd.results()[0]} &&
               fptosi.results().size() == 1 &&
               caller.value(fptosi.results()[0]).value().source_id == bir::SourceValueId{caller_id, 12} &&
               add.binary() && add.binary()->opcode == bir::BinaryOpcode::Add &&
               add.binary()->type == bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
               add.operands().size() == 2 && add.operands()[0] == fptosi.results()[0] &&
               add.results().size() == 1 &&
               caller.value(add.results()[0]).value().source_id == bir::SourceValueId{caller_id, 13},
           layer + " must preserve the native double-to-signed-i32 FPToSI and exact i32 Add use");
  };
  const auto module = scalar_double_to_signed_i32_fptosi_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "scalar FPToSI must publish verified Raw BIR" +
             (raw.has_value() ? std::string{} : ": " + raw.error().detail));
  inspect(raw.value(), "Raw BIR");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(), "scalar FPToSI must canonicalize");
  inspect(canonical.value(), "Canonical BIR");

  const auto rejected = [](auto mutate, const std::string& message) {
    auto candidate = scalar_double_to_signed_i32_fptosi_module();
    auto& cast = std::get<lir::LirCastOp>(candidate.functions[0].blocks[0].insts[3]);
    auto& use = std::get<lir::LirBinOp>(candidate.functions[0].blocks[0].insts[4]);
    mutate(candidate, cast, use);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() && raw_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value() && canonical_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Canonical rollback)");
  };
  rejected([](auto&, auto& cast, auto&) { cast.result = lir::LirOperand::raw("%missing"); }, "missing cast result authority must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.operand = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); }, "unresolved or cross-owner cast source must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.result = lir::LirOperand::ssa("%duplicate", lir::LirValueId{11}); }, "duplicate cast result must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.kind = lir::LirCastKind::UIToFP; }, "wrong floating-to-integer direction must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.from_type = lir::LirTypeRef("float"); }, "wrong cast source endpoint must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.to_type = lir::LirTypeRef::integer(64); }, "wrong cast destination endpoint must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.lhs = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); }, "unresolved downstream i32 Add use must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.opcode = lir::LirBinaryOpcode::Mul; }, "non-Add downstream use must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.type_str = lir::LirTypeRef::integer(64); }, "wrong downstream i32 Add type must reject atomically");
  rejected([](auto& candidate, auto&, auto&) { candidate.functions[0].blocks[0].insts.pop_back(); }, "missing downstream i32 Add use must reject atomically");
}

lir::LirModule scalar_double_to_unsigned_i32_fptoui_module() {
  auto module = downstream_double_fadd_module();
  auto& block = module.functions[0].blocks[0];
  block.insts.pop_back();
  block.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand::ssa("%presentation-only-fptoui-result", lir::LirValueId{12}),
      .kind = lir::LirCastKind::FPToUI,
      .from_type = lir::LirTypeRef("double"),
      .operand = lir::LirOperand::ssa("%presentation-only-fadd", lir::LirValueId{11}),
      .to_type = lir::LirTypeRef::integer(32),
  });
  block.insts.push_back(lir::LirBinOp{
      lir::LirOperand::ssa("%presentation-only-fptoui-add", lir::LirValueId{13}),
      lir::LirBinaryOpcode::Add, lir::LirTypeRef::integer(32),
      lir::LirOperand::ssa("%presentation-only-fptoui-use", lir::LirValueId{12}),
      lir::LirOperand::integer("presentation-four", 4)});
  return module;
}

void test_scalar_double_to_unsigned_i32_fptoui_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto function_id = view.functions()[0];
    const auto function = view.function(function_id).value();
    const auto insts = function.instructions(function.blocks()[0]).value();
    const auto fadd = function.instruction(insts[1]).value();
    const auto fptoui = function.instruction(insts[2]).value();
    const auto add = function.instruction(insts[3]).value();
    expect(insts.size() == 4 && fadd.binary() &&
               fadd.binary()->opcode == bir::BinaryOpcode::FAdd &&
               fptoui.cast() && fptoui.cast()->kind == bir::CastKind::FPToUI &&
               fptoui.cast()->from_type == bir::Type{bir::TypeKind::F64, 64, "double"} &&
               fptoui.cast()->to_type == bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
               fptoui.operands() == std::vector<bir::ValueId>{fadd.results()[0]} &&
               fptoui.results().size() == 1 &&
               function.value(fptoui.results()[0]).value().source_id ==
                   bir::SourceValueId{function_id, 12} &&
               add.binary() && add.binary()->opcode == bir::BinaryOpcode::Add &&
               add.operands()[0] == fptoui.results()[0] && add.results().size() == 1 &&
               function.value(add.results()[0]).value().source_id ==
                   bir::SourceValueId{function_id, 13},
           layer + " must preserve the native double-to-unsigned-i32 FPToUI and exact i32 Add use");
  };
  const auto module = scalar_double_to_unsigned_i32_fptoui_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "scalar FPToUI must publish verified Raw BIR");
  inspect(raw.value(), "Raw BIR");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(), "scalar FPToUI must canonicalize");
  inspect(canonical.value(), "Canonical BIR");
  const auto rejected = [](auto mutate, const std::string& message) {
    auto candidate = scalar_double_to_unsigned_i32_fptoui_module();
    auto& cast = std::get<lir::LirCastOp>(candidate.functions[0].blocks[0].insts[3]);
    auto& use = std::get<lir::LirBinOp>(candidate.functions[0].blocks[0].insts[4]);
    mutate(candidate, cast, use);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value(), message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value(), message + " (Canonical rollback)");
  };
  rejected([](auto&, auto& cast, auto&) { cast.result = lir::LirOperand::raw("%missing"); }, "missing cast result authority must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.operand = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); }, "unresolved or cross-owner cast source must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.result = lir::LirOperand::ssa("%duplicate", lir::LirValueId{11}); }, "duplicate cast result must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.kind = lir::LirCastKind::UIToFP; }, "wrong floating-to-integer direction must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.from_type = lir::LirTypeRef("float"); }, "wrong cast source endpoint must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.to_type = lir::LirTypeRef::integer(64); }, "wrong cast destination endpoint must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.lhs = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); }, "unresolved downstream i32 Add use must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.opcode = lir::LirBinaryOpcode::Mul; }, "non-Add downstream use must reject atomically");
  rejected([](auto&, auto&, auto& use) { use.type_str = lir::LirTypeRef::integer(64); }, "wrong downstream i32 Add type must reject atomically");
  rejected([](auto& candidate, auto&, auto&) { candidate.functions[0].blocks[0].insts.pop_back(); }, "missing downstream i32 Add use must reject atomically");
}

lir::LirModule normalized_i32_add_module();

lir::LirModule wide_ffs_select_trunc_module() {
  auto module = normalized_i32_add_module();
  auto& block = module.functions[0].blocks[0];
  block.insts.clear();
  block.insts.push_back(lir::LirSelectOp{
      lir::LirOperand::ssa("%wide-ffs-select", lir::LirValueId{40}),
      lir::LirTypeRef::integer(64),
      lir::LirOperand::ssa("%producer-verified-condition", lir::LirValueId{70}),
      lir::LirOperand::integer("zero", 0),
      lir::LirOperand::raw("%producer-verified-plus-one")});
  block.insts.push_back(lir::LirCastOp{
      lir::LirOperand::ssa("%wide-ffs-trunc", lir::LirValueId{41}),
      lir::LirCastKind::Trunc, lir::LirTypeRef::integer(64),
      lir::LirOperand::ssa("%wide-ffs-select-use", lir::LirValueId{40}),
      lir::LirTypeRef::integer(32)});
  block.insts.push_back(lir::LirBinOp{
      lir::LirOperand::ssa("%wide-ffs-add", lir::LirValueId{42}),
      lir::LirBinaryOpcode::Add, lir::LirTypeRef::integer(32),
      lir::LirOperand::ssa("%wide-ffs-trunc-use", lir::LirValueId{41}),
      lir::LirOperand::integer("one", 1)});
  block.terminator = lir::LirRet{lir::LirOperand::ssa("%wide-ffs-return", lir::LirValueId{42}),
                                 lir::LirTypeRef::integer(32)};
  return module;
}

void test_wide_ffs_select_trunc_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto function_id = view.functions()[0];
    const auto function = view.function(function_id).value();
    const auto insts = function.instructions(function.blocks()[0]).value();
    const auto select = function.instruction(insts[0]).value();
    const auto trunc = function.instruction(insts[1]).value();
    const auto add = function.instruction(insts[2]).value();
    expect(insts.size() == 3 && select.select() &&
               select.select()->type == bir::Type{bir::TypeKind::Integer, 64, "i64"} &&
               select.operands().empty() && select.results().size() == 1 &&
               function.value(select.results()[0]).value().source_id ==
                   bir::SourceValueId{function_id, 40} &&
               trunc.cast() && trunc.cast()->kind == bir::CastKind::Trunc &&
               trunc.cast()->from_type == bir::Type{bir::TypeKind::Integer, 64, "i64"} &&
               trunc.cast()->to_type == bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
               trunc.operands() == std::vector<bir::ValueId>{select.results()[0]} &&
               add.binary() && add.binary()->opcode == bir::BinaryOpcode::Add &&
               add.operands()[0] == trunc.results()[0],
           layer + " must preserve only the typed wide ffs select result through Trunc to Add");
  };
  const auto module = wide_ffs_select_trunc_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "wide ffs select trunc must publish verified Raw BIR");
  inspect(raw.value(), "Raw BIR");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(), "wide ffs select trunc must canonicalize");
  inspect(canonical.value(), "Canonical BIR");
  const auto rejected = [](auto mutate, const std::string& message) {
    auto candidate = wide_ffs_select_trunc_module();
    auto& select = std::get<lir::LirSelectOp>(candidate.functions[0].blocks[0].insts[0]);
    auto& cast = std::get<lir::LirCastOp>(candidate.functions[0].blocks[0].insts[1]);
    auto& use = std::get<lir::LirBinOp>(candidate.functions[0].blocks[0].insts[2]);
    mutate(candidate, select, cast, use);
    expect(!bir::lower_lir_to_raw_bir(candidate).has_value(), message + " (Raw rollback)");
    expect(!bir::lower_lir_to_canonical_bir(candidate).has_value(), message + " (Canonical rollback)");
  };
  rejected([](auto&, auto& select, auto&, auto&) { select.result = lir::LirOperand::raw("%missing"); }, "missing select result authority must reject atomically");
  rejected([](auto&, auto& select, auto&, auto&) { select.type_str = lir::LirTypeRef::integer(32); }, "wrong select result type must reject atomically");
  rejected([](auto&, auto& select, auto&, auto&) { select.result = lir::LirOperand::ssa("%duplicate", lir::LirValueId{41}); }, "duplicate select result must reject atomically");
  rejected([](auto&, auto&, auto& cast, auto&) { cast.operand = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); }, "unresolved trunc source must reject atomically");
  rejected([](auto&, auto&, auto& cast, auto&) { cast.kind = lir::LirCastKind::SExt; }, "wrong cast kind or direction must reject atomically");
  rejected([](auto&, auto&, auto& cast, auto&) { cast.to_type = lir::LirTypeRef::integer(64); }, "non-narrowing trunc must reject atomically");
  rejected([](auto&, auto&, auto&, auto& use) { use.lhs = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); }, "unresolved Add use must reject atomically");
  rejected([](auto&, auto&, auto&, auto& use) { use.opcode = lir::LirBinaryOpcode::Mul; }, "wrong downstream use kind must reject atomically");
  rejected([](auto& candidate, auto&, auto&, auto&) { candidate.functions[0].blocks[0].insts.pop_back(); }, "missing downstream Add use must reject atomically");
}

lir::LirModule normalized_i32_add_module() {
  auto module = direct_global_integer_load_module();
  auto& function = module.functions[0];
  function.return_type = scalar_type(c4c::TB_INT);
  function.return_type.inner_rank = -1;
  function.signature_return_type_ref = lir::LirTypeRef::integer(32);
  auto& block = function.blocks[0];
  block.insts.erase(block.insts.begin() + 1);
  block.insts.push_back(lir::LirBinOp{
      lir::LirOperand::ssa("%presentation-add", lir::LirValueId{33}),
      lir::LirBinaryOpcode::Add, lir::LirTypeRef::integer(32),
      lir::LirOperand::ssa("%presentation-load", lir::LirValueId{31}),
      lir::LirOperand::integer("presentation-one", 1)});
  block.terminator = lir::LirRet{
      lir::LirOperand::ssa("%presentation-return", lir::LirValueId{33}),
      lir::LirTypeRef::integer(32)};
  return module;
}

void test_normalized_i32_add_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto function_id = view.functions()[0];
    const auto function = view.function(function_id).value();
    const auto instructions = function.instructions(function.blocks()[0]).value();
    const auto load = function.instruction(instructions[0]).value();
    const auto add = function.instruction(instructions[1]).value();
    const auto result = function.value(add.results()[0]).value();
    const auto terminator = function.terminator(function.blocks()[0]).value();
    const auto* returned = std::get_if<bir::ReturnTerm>(&terminator);
    expect(instructions.size() == 2 && load.load() && add.binary() &&
               add.binary()->opcode == bir::BinaryOpcode::Add &&
               add.binary()->type == bir::Type{bir::TypeKind::I32} &&
               add.operands().size() == 2 &&
               add.operands()[0] == load.results()[0] &&
               add.results().size() == 1 &&
               result.type == bir::Type{bir::TypeKind::I32} &&
               result.source_id == bir::SourceValueId{function_id, 33} &&
               function.source_value(bir::SourceValueId{function_id, 33}).value() ==
                   add.results()[0] &&
               returned && returned->value && *returned->value == add.results()[0],
           layer + " must retain the source-backed Load-plus-one i32 Add and return edge");
    const auto rhs = function.value(add.operands()[1]).value();
    const auto* constant = std::get_if<bir::ConstantDef>(&rhs.definition);
    const auto integer = constant ? view.constant(constant->constant).value() : bir::ConstantDefinition{};
    const auto* payload = constant ? std::get_if<bir::IntegerConstant>(&integer.payload) : nullptr;
    expect(rhs.type == bir::Type{bir::TypeKind::I32} && !rhs.source_id && payload &&
               payload->value == 1,
           layer + " must materialize only the native Add immediate one as an i32 constant");
  };
  const auto module = normalized_i32_add_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "the normalized Load-plus-one i32 Add must publish verified Raw BIR");
  inspect(raw.value(), "Raw BIR");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(), "the normalized i32 Add route must canonicalize");
  inspect(canonical.value(), "Canonical BIR");

  const auto rejected = [](auto mutate, const std::string& message) {
    auto candidate = normalized_i32_add_module();
    auto& add = std::get<lir::LirBinOp>(candidate.functions[0].blocks[0].insts[1]);
    mutate(candidate, add);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value(), message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value(), message + " (Canonical rollback)");
  };
  rejected([](auto&, auto& add) { add.result = lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid()); },
           "invalid Add result ID must reject");
  rejected([](auto&, auto& add) { add.result = lir::LirOperand::ssa("%duplicate", lir::LirValueId{31}); },
           "duplicate Add result ID must reject");
  rejected([](auto&, auto& add) { add.lhs = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); },
           "unknown Add lhs must reject");
  rejected([](auto&, auto& add) { add.rhs = lir::LirOperand::integer("not-one", 2); },
           "nonselected Add immediate must reject");
  rejected([](auto&, auto& add) { add.rhs = lir::LirOperand::integer("out-of-range", 1LL << 32); },
           "out-of-range Add immediate must reject");
  rejected([](auto&, auto& add) { add.rhs = lir::LirOperand::ssa("%rhs", lir::LirValueId{31}); },
           "SSA Add rhs must reject");
  rejected([](auto&, auto& add) { add.opcode = lir::LirBinaryOpcode::Mul; },
           "non-Add opcode must reject");
  rejected([](auto&, auto& add) { add.type_str = lir::LirTypeRef::integer(64); },
           "non-i32 Add type must reject");
  rejected([](auto&, auto& add) { add.result = lir::LirOperand("%raw"); },
           "malformed Add result linkage must reject");
  rejected([](auto& candidate, auto& add) {
             auto foreign = candidate.functions[0];
             foreign.name = "foreign_i32_add_owner";
             auto& foreign_load = std::get<lir::LirLoadOp>(foreign.blocks[0].insts[0]);
             foreign_load.result = lir::LirOperand::ssa("%foreign-load", lir::LirValueId{77});
             auto& foreign_add = std::get<lir::LirBinOp>(foreign.blocks[0].insts[1]);
             foreign_add.lhs = lir::LirOperand::ssa("%foreign-load", lir::LirValueId{77});
             foreign_add.result = lir::LirOperand::ssa("%foreign-add", lir::LirValueId{78});
             foreign.blocks[0].terminator = lir::LirRet{
                 lir::LirOperand::ssa("%foreign-return", lir::LirValueId{78}),
                 lir::LirTypeRef::integer(32)};
             candidate.functions.push_back(std::move(foreign));
             add.lhs = lir::LirOperand::ssa("%cross", lir::LirValueId{77});
           }, "cross-function Add lhs must reject");
}

lir::LirModule normalized_i32_mul_module() {
  auto module = normalized_i32_add_module();
  auto& block = module.functions[0].blocks[0];
  block.insts.push_back(lir::LirBinOp{
      lir::LirOperand::ssa("%presentation-mul", lir::LirValueId{34}),
      lir::LirBinaryOpcode::Mul, lir::LirTypeRef::integer(32),
      lir::LirOperand::ssa("%presentation-add", lir::LirValueId{33}),
      lir::LirOperand::integer("presentation-two", 2)});
  block.terminator = lir::LirRet{
      lir::LirOperand::ssa("%presentation-return", lir::LirValueId{34}),
      lir::LirTypeRef::integer(32)};
  return module;
}

void test_normalized_i32_mul_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto function_id = view.functions()[0];
    const auto function = view.function(function_id).value();
    const auto instructions = function.instructions(function.blocks()[0]).value();
    const auto load = function.instruction(instructions[0]).value();
    const auto add = function.instruction(instructions[1]).value();
    const auto mul = function.instruction(instructions[2]).value();
    const auto result = function.value(mul.results()[0]).value();
    const auto terminator = function.terminator(function.blocks()[0]).value();
    const auto* returned = std::get_if<bir::ReturnTerm>(&terminator);
    expect(instructions.size() == 3 && load.load() && add.binary() && mul.binary() &&
               add.binary()->opcode == bir::BinaryOpcode::Add &&
               mul.binary()->opcode == bir::BinaryOpcode::Mul &&
               mul.binary()->type == bir::Type{bir::TypeKind::I32} &&
               add.operands()[0] == load.results()[0] &&
               mul.operands().size() == 2 && mul.operands()[0] == add.results()[0] &&
               result.source_id == bir::SourceValueId{function_id, 34} &&
               function.source_value(bir::SourceValueId{function_id, 34}).value() ==
                   mul.results()[0] &&
               returned && returned->value && *returned->value == mul.results()[0],
           layer + " must retain Load -> Add(one) -> Mul(two) and its source-backed return");
    const auto rhs = function.value(mul.operands()[1]).value();
    const auto* constant = std::get_if<bir::ConstantDef>(&rhs.definition);
    const auto integer = constant ? view.constant(constant->constant).value() : bir::ConstantDefinition{};
    const auto* payload = constant ? std::get_if<bir::IntegerConstant>(&integer.payload) : nullptr;
    expect(rhs.type == bir::Type{bir::TypeKind::I32} && !rhs.source_id && payload &&
               payload->value == 2,
           layer + " must materialize only the native Mul immediate two as an i32 constant");
  };
  const auto module = normalized_i32_mul_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "the normalized Load-plus-one-plus-two i32 chain must publish verified Raw BIR");
  inspect(raw.value(), "Raw BIR");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(), "the normalized i32 Mul route must canonicalize");
  inspect(canonical.value(), "Canonical BIR");

  const auto rejected = [](auto mutate, const std::string& message) {
    auto candidate = normalized_i32_mul_module();
    auto& mul = std::get<lir::LirBinOp>(candidate.functions[0].blocks[0].insts[2]);
    mutate(candidate, mul);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value(), message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value(), message + " (Canonical rollback)");
  };
  rejected([](auto&, auto& mul) { mul.result = lir::LirOperand::raw("%raw"); },
           "missing Mul result identity must reject");
  rejected([](auto&, auto& mul) {
             mul.result = lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
           }, "invalid Mul result ID must reject");
  rejected([](auto&, auto& mul) {
             mul.result = lir::LirOperand::ssa("%duplicate", lir::LirValueId{33});
           }, "duplicate Mul result ID must reject");
  rejected([](auto&, auto& mul) {
             mul.lhs = lir::LirOperand::ssa("%unknown", lir::LirValueId{77});
           }, "unknown Mul lhs must reject");
  rejected([](auto&, auto& mul) {
             mul.lhs = lir::LirOperand::ssa("%load", lir::LirValueId{31});
           }, "non-Add Mul lhs must reject");
  rejected([](auto&, auto& mul) { mul.rhs = lir::LirOperand::integer("not-two", 1); },
           "non-two Mul immediate must reject");
  rejected([](auto&, auto& mul) {
             mul.rhs = lir::LirOperand::integer("out-of-range", 1LL << 32);
           }, "out-of-range Mul immediate must reject");
  rejected([](auto&, auto& mul) {
             mul.rhs = lir::LirOperand::ssa("%rhs", lir::LirValueId{33});
           }, "SSA Mul rhs must reject");
  rejected([](auto&, auto& mul) { mul.opcode = lir::LirBinaryOpcode::Add; },
           "non-Mul opcode must reject");
  rejected([](auto&, auto& mul) { mul.type_str = lir::LirTypeRef::integer(64); },
           "non-i32 Mul type must reject");
  rejected([](auto& candidate, auto& mul) {
             auto foreign = candidate.functions[0];
             foreign.name = "foreign_i32_mul_owner";
             foreign.link_name_id = candidate.link_names.intern("foreign_i32_mul_owner");
             auto& foreign_load = std::get<lir::LirLoadOp>(foreign.blocks[0].insts[0]);
             foreign_load.result = lir::LirOperand::ssa("%foreign-load", lir::LirValueId{77});
             auto& foreign_add = std::get<lir::LirBinOp>(foreign.blocks[0].insts[1]);
             foreign_add.lhs = lir::LirOperand::ssa("%foreign-load", lir::LirValueId{77});
             foreign_add.result = lir::LirOperand::ssa("%foreign-add", lir::LirValueId{78});
             auto& foreign_mul = std::get<lir::LirBinOp>(foreign.blocks[0].insts[2]);
             foreign_mul.lhs = lir::LirOperand::ssa("%foreign-add", lir::LirValueId{78});
             foreign_mul.result = lir::LirOperand::ssa("%foreign-mul", lir::LirValueId{79});
             foreign.blocks[0].terminator = lir::LirRet{
                 lir::LirOperand::ssa("%foreign-return", lir::LirValueId{79}),
                 lir::LirTypeRef::integer(32)};
             candidate.functions.push_back(std::move(foreign));
             mul.lhs = lir::LirOperand::ssa("%cross", lir::LirValueId{78});
           }, "cross-function Mul lhs must reject");
}

void test_direct_native_floating_call_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto caller_id = view.functions()[0];
    const auto caller = view.function(caller_id).value();
    const auto instructions = caller.instructions(caller.blocks()[0]).value();
    expect(instructions.size() == 1,
           layer + " must retain one native floating Call without receiving FAdd");
    const auto call = caller.instruction(instructions[0]).value();
    const auto result = caller.value(call.results()[0]).value();
    expect(call.opcode() == bir::Opcode::Call && call.call() &&
               call.call()->callee == view.functions()[1] &&
               call.operands().empty() && call.results().size() == 1 &&
               result.type == bir::Type{bir::TypeKind::F64, 64, "double"} &&
               result.source_id == bir::SourceValueId{caller_id, 9} &&
               caller.source_value(*result.source_id).value() == call.results()[0],
           layer + " must retain the direct native double callee and owning source result");
  };

  const auto module = direct_native_floating_call_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "resolved direct native double Call must publish verified Raw BIR");
  inspect(raw.value(), "Raw BIR");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(), "resolved direct native double Call must canonicalize");
  inspect(canonical.value(), "Canonical BIR");

  auto missing_retained_signature = direct_native_floating_call_module();
  attach_direct_native_floating_function_signature_ref(missing_retained_signature);
  auto& missing_retained_call = std::get<lir::LirCallOp>(
      missing_retained_signature.functions[0].blocks[0].insts[0]);
  missing_retained_call.callee =
      lir::LirOperand::global("@direct_native_float_target",
                              missing_retained_call.direct_callee_link_name_id);
  missing_retained_call.callee_signature.reset();
  const auto raw_missing_retained =
      bir::lower_lir_to_raw_bir(missing_retained_signature);
  expect(raw_missing_retained.has_value() &&
             bir::FoundationVerifier::verify(raw_missing_retained.value()).ok(),
         "direct native floating call must use module signature store when retained signature is absent");
  inspect(raw_missing_retained.value(), "Raw BIR store-backed native floating call");
  const auto canonical_missing_retained =
      bir::lower_lir_to_canonical_bir(missing_retained_signature);
  expect(canonical_missing_retained.has_value(),
         "store-backed native floating call without retained signature must canonicalize");
  inspect(canonical_missing_retained.value(),
          "Canonical BIR store-backed native floating call");

  auto stale_text_signature = direct_native_floating_call_module();
  attach_direct_native_floating_function_signature_ref(stale_text_signature);
  auto& stale_text_call = std::get<lir::LirCallOp>(
      stale_text_signature.functions[0].blocks[0].insts[0]);
  stale_text_call.callee =
      lir::LirOperand::global("@direct_native_float_target",
                              stale_text_call.direct_callee_link_name_id);
  stale_text_call.callee_signature->fixed_param_types = {"float stale text only"};
  stale_text_call.callee_type_suffix = "(ptr stale suffix)";
  stale_text_call.args_str = "i64 stale mirror";
  const auto raw_stale_text = bir::lower_lir_to_raw_bir(stale_text_signature);
  expect(raw_stale_text.has_value() &&
             bir::FoundationVerifier::verify(raw_stale_text.value()).ok(),
         "direct native floating call must ignore retained text when signature ref resolves");
  inspect(raw_stale_text.value(), "Raw BIR stale-text native floating call");

  const auto rejected = [](auto mutate, const std::string& message) {
    auto candidate = direct_native_floating_call_module();
    auto& call = std::get<lir::LirCallOp>(candidate.functions[0].blocks[0].insts[0]);
    mutate(candidate, call);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() && raw_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value() && canonical_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Canonical rollback)");
  };
  rejected([](auto&, auto& call) { call.direct_callee_link_name_id = 999; },
           "missing direct native callee identity must reject");
  rejected([](auto&, auto& call) { call.result = lir::LirOperand::raw("%raw"); },
           "presentation-only native call result must reject");
  rejected([](auto&, auto& call) {
             call.result = lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
           }, "invalid native call result identity must reject");
  rejected([](auto& module, auto&) {
             module.functions[0].blocks[0].insts.push_back(
                 direct_native_floating_call(module.functions[1].link_name_id));
           }, "duplicate native call result identity must reject");
  rejected([](auto&, auto& call) {
             call.callee_signature->return_type_ref = lir::LirTypeRef("float");
           }, "native call signature return disagreement must reject");
  rejected([](auto&, auto& call) { call.return_type = lir::LirTypeRef("float"); },
           "native call return disagreement must reject");
  rejected([](auto&, auto& call) { call.callee_signature->is_variadic = true; },
           "variadic native call carrier must reject");
  rejected([](auto&, auto& call) {
             call.callee_signature->has_void_param_list = false;
           }, "non-void native call carrier must reject");
  rejected([](auto&, auto& call) {
             call.callee_signature->fixed_param_types = {"double"};
             call.callee_signature->fixed_param_type_refs = {lir::LirTypeRef("double")};
           }, "argument-bearing native call carrier must reject");
  rejected([](auto& module, auto&) {
             module.functions[1].signature_has_void_param_list = false;
           }, "non-void native callee declaration must reject");
  rejected([](auto&, auto& call) { call.return_type = lir::LirTypeRef("float"); },
           "nonmatching floating call return must reject");
  rejected([](auto& module, auto&) {
             auto duplicate = direct_native_floating_function(
                 "conflicting_native_float_target", true);
             duplicate.link_name_id = module.functions[1].link_name_id;
             duplicate.return_type = scalar_type(c4c::TB_FLOAT);
             duplicate.return_type.inner_rank = -1;
             duplicate.signature_return_type_ref = lir::LirTypeRef("float");
             module.functions.push_back(std::move(duplicate));
           }, "duplicate native callee identity with conflicting return must reject");

  const auto store_rejected = [](auto mutate, const std::string& message) {
    auto candidate = direct_native_floating_call_module();
    attach_direct_native_floating_function_signature_ref(candidate);
    auto& call =
        std::get<lir::LirCallOp>(candidate.functions[0].blocks[0].insts[0]);
    mutate(candidate, call);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() && raw_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value() && canonical_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Canonical rollback)");
  };
  store_rejected([](auto&, auto& call) {
                   call.callee_signature->return_type_ref =
                       lir::LirTypeRef("float");
                 },
                 "retained/store native floating return mismatch must reject");
  store_rejected([](auto& module, auto& call) {
                   module.function_signature_store[call.callee_signature_ref.value]
                       .return_type_ref = lir::LirTypeRef("float");
                 },
                 "signature-store native floating return mismatch must reject");
  store_rejected([](auto& module, auto& call) {
                   module.function_signature_store[call.callee_signature_ref.value]
                       .fixed_param_type_refs = {lir::LirTypeRef("double")};
                 },
                 "signature-store native floating parameter mismatch must reject");
  store_rejected([](auto& module, auto&) {
                   module.functions[1].signature_return_type_ref =
                       lir::LirTypeRef("float");
                 },
                 "target native floating signature mismatch must reject with store-backed call");
}

// Native intrinsics deliberately use a link-name payload rather than CallNode.
lir::LirCallOp native_intrinsic(c4c::LinkNameId link, lir::LirIntrinsicKind kind,
                                lir::LirValueId result, lir::LirOperand operand) {
  lir::LirCallOp call;
  call.result = lir::LirOperand::ssa("%intrinsic", result);
  call.return_type = lir::LirTypeRef::integer(32);
  call.callee = lir::LirOperand::global("@display", link);
  call.direct_callee_link_name_id = link;
  call.intrinsic_kind = kind;
  lir::LirCallSignature signature;
  signature.return_type_ref = lir::LirTypeRef::integer(32);
  signature.fixed_param_types = {"i32"};
  signature.fixed_param_type_refs = {lir::LirTypeRef::integer(32)};
  call.arg_type_refs = {lir::LirTypeRef::integer(32)};
  call.structured_args = {{"i32", std::move(operand), lir::LirTypeRef::integer(32)}};
  if (kind != lir::LirIntrinsicKind::Ctpop) {
    call.zero_count_behavior = lir::LirZeroCountBehavior::Defined;
    signature.fixed_param_types.push_back("i1");
    signature.fixed_param_type_refs.push_back(lir::LirTypeRef::integer(1));
    call.arg_type_refs.push_back(lir::LirTypeRef::integer(1));
    call.structured_args.push_back({"i1", lir::LirOperand::integer("flag", 0),
                                    lir::LirTypeRef::integer(1)});
  }
  call.callee_signature = std::move(signature);
  return call;
}

lir::LirModule native_intrinsic_module() {
  lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  const auto caller_link = module.link_names.intern("native_intrinsic_caller");
  const auto cttz_link = module.link_names.intern("llvm.cttz.i32");
  const auto ctlz_link = module.link_names.intern("llvm.ctlz.i32");
  auto caller = direct_integer_function("intrinsic_caller", false);
  caller.link_name_id = caller_link;
  caller.blocks[0].insts.push_back({lir::LirConstInt{lir::LirValueId{3}, scalar_type(c4c::TB_INT), 41}});
  caller.blocks[0].insts.push_back(native_intrinsic(cttz_link, lir::LirIntrinsicKind::Cttz,
      lir::LirValueId{9}, lir::LirOperand::ssa("%value", lir::LirValueId{3})));
  caller.blocks[0].insts.push_back(native_intrinsic(ctlz_link, lir::LirIntrinsicKind::Ctlz,
      lir::LirValueId{10}, lir::LirOperand::ssa("%value", lir::LirValueId{9})));
  caller.blocks[0].terminator = lir::LirRet{lir::LirOperand::ssa("%return", lir::LirValueId{10}), lir::LirTypeRef::integer(32)};
  module.functions.push_back(std::move(caller));
  return module;
}

void attach_native_intrinsic_function_signature_refs(lir::LirModule& module) {
  auto& cttz =
      std::get<lir::LirCallOp>(module.functions[0].blocks[0].insts[1]);
  auto& ctlz =
      std::get<lir::LirCallOp>(module.functions[0].blocks[0].insts[2]);

  lir::LirFunctionSignatureStoreEntry caller_entry;
  caller_entry.return_type_ref = lir::LirTypeRef::integer(32);
  module.functions[0].function_signature_ref =
      module.register_function_signature(std::move(caller_entry));

  lir::LirFunctionSignatureStoreEntry count_entry;
  count_entry.return_type_ref = lir::LirTypeRef::integer(32);
  count_entry.fixed_param_type_refs = {lir::LirTypeRef::integer(32),
                                       lir::LirTypeRef::integer(1)};
  count_entry.fixed_param_is_byval = {false, false};
  const lir::LirFunctionSignatureRef count_ref =
      module.register_function_signature(std::move(count_entry));
  cttz.callee_signature_ref = count_ref;
  ctlz.callee_signature_ref = count_ref;
}

void test_native_intrinsic_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto caller_id = view.functions()[0];
    const auto caller = view.function(caller_id).value();
    const auto insts = caller.instructions(caller.blocks()[0]).value();
    expect(insts.size() == 2, layer + " must retain both native intrinsic instructions");
    const auto cttz = caller.instruction(insts[0]).value();
    const auto ctlz = caller.instruction(insts[1]).value();
    expect(cttz.opcode() == bir::Opcode::Call && !cttz.call() && cttz.intrinsic_call() &&
               cttz.intrinsic_call()->kind == bir::IntrinsicKind::Cttz &&
               cttz.intrinsic_call()->zero_count_is_undef == std::optional<bool>{false} &&
               cttz.operands().size() == 2 && cttz.results().size() == 1,
           layer + " must retain separately tagged cttz and native flag");
    expect(ctlz.opcode() == bir::Opcode::Call && !ctlz.call() && ctlz.intrinsic_call() &&
               ctlz.intrinsic_call()->kind == bir::IntrinsicKind::Ctlz &&
               ctlz.intrinsic_call()->zero_count_is_undef == std::optional<bool>{false} &&
               ctlz.operands().size() == 2,
           layer + " must retain separately tagged ctlz and native flag");
    expect(view.source_id(cttz.intrinsic_call()->callee_link_name).value() == 2 &&
               view.source_id(ctlz.intrinsic_call()->callee_link_name).value() == 3 &&
               caller.value(cttz.results()[0]).value().source_id == bir::SourceValueId{caller_id, 9} &&
               caller.value(ctlz.results()[0]).value().source_id == bir::SourceValueId{caller_id, 10},
           layer + " must retain LinkNameId and source-backed owning results");
  };
  const auto module = native_intrinsic_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  if (!raw.has_value())
    fail("native integer intrinsics must publish Raw BIR: " + raw.error().detail +
         (raw.error().verification_errors.empty() ? "" :
          ": " + raw.error().verification_errors.front().message));
  expect(bir::FoundationVerifier::verify(raw.value()).ok(),
         "native integer intrinsics must publish verified Raw BIR");
  inspect(raw.value(), "Raw BIR");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(), "native integer intrinsics must canonicalize");
  inspect(canonical.value(), "Canonical BIR");

  auto missing_retained_signature = native_intrinsic_module();
  attach_native_intrinsic_function_signature_refs(missing_retained_signature);
  auto& missing_retained_cttz = std::get<lir::LirCallOp>(
      missing_retained_signature.functions[0].blocks[0].insts[1]);
  missing_retained_cttz.callee_signature.reset();
  const auto raw_missing_retained =
      bir::lower_lir_to_raw_bir(missing_retained_signature);
  expect(raw_missing_retained.has_value() &&
             bir::FoundationVerifier::verify(raw_missing_retained.value()).ok(),
         "native integer intrinsic must use module signature store when retained signature is absent" +
             (raw_missing_retained.has_value()
                  ? std::string{}
                  : ": " + raw_missing_retained.error().detail));
  inspect(raw_missing_retained.value(), "Raw BIR store-backed native intrinsic");
  const auto canonical_missing_retained =
      bir::lower_lir_to_canonical_bir(missing_retained_signature);
  expect(canonical_missing_retained.has_value(),
         "store-backed native integer intrinsic without retained signature must canonicalize");
  inspect(canonical_missing_retained.value(),
          "Canonical BIR store-backed native intrinsic");

  auto stale_text_signature = native_intrinsic_module();
  attach_native_intrinsic_function_signature_refs(stale_text_signature);
  auto& stale_text_cttz = std::get<lir::LirCallOp>(
      stale_text_signature.functions[0].blocks[0].insts[1]);
  stale_text_cttz.callee_signature->fixed_param_types = {
      "i64 stale text only", "i8 stale text only"};
  stale_text_cttz.args_str = "stale display only";
  const auto raw_stale_text = bir::lower_lir_to_raw_bir(stale_text_signature);
  expect(raw_stale_text.has_value() &&
             bir::FoundationVerifier::verify(raw_stale_text.value()).ok(),
         "native integer intrinsic must ignore retained text when signature ref resolves");
  inspect(raw_stale_text.value(), "Raw BIR stale-text native intrinsic");

  const auto rejected = [](auto mutate, const std::string& message) {
    auto candidate = native_intrinsic_module();
    auto& call = std::get<lir::LirCallOp>(candidate.functions[0].blocks[0].insts[1]);
    mutate(candidate, call);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() && raw_rejected.error().code == bir::ImportErrorCode::UnsupportedOrdinaryInstruction, message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value() && canonical_rejected.error().code == bir::ImportErrorCode::UnsupportedOrdinaryInstruction, message + " (Canonical rollback)");
  };
  rejected([](auto&, auto& call) { call.direct_callee_link_name_id = 999; }, "unknown intrinsic link must reject atomically");
  rejected([](auto&, auto& call) { call.structured_args[1].operand = lir::LirOperand::integer("bad", 1); }, "behavior flag mismatch must reject atomically");
  rejected([](auto&, auto& call) { call.structured_args[0].operand = lir::LirOperand::global("@bad", 1); }, "unsupported intrinsic operands must reject atomically");
  rejected([](auto&, auto& call) { call.structured_args.pop_back(); }, "wrong intrinsic operand count must reject atomically");
  rejected([](auto&, auto& call) { call.intrinsic_kind = static_cast<lir::LirIntrinsicKind>(99); }, "other intrinsic kinds must remain fail-closed");

  const auto store_rejected = [](auto mutate, const std::string& message) {
    auto candidate = native_intrinsic_module();
    attach_native_intrinsic_function_signature_refs(candidate);
    auto& call =
        std::get<lir::LirCallOp>(candidate.functions[0].blocks[0].insts[1]);
    mutate(candidate, call);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() &&
               raw_rejected.error().code ==
                   bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value() &&
               canonical_rejected.error().code ==
                   bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Canonical rollback)");
  };
  store_rejected([](auto&, auto& call) {
                   call.callee_signature->fixed_param_type_refs[1] =
                       lir::LirTypeRef::integer(32);
                 },
                 "retained/store native intrinsic flag mismatch must reject");
  store_rejected([](auto& module, auto& call) {
                   module.function_signature_store[call.callee_signature_ref.value]
                       .fixed_param_type_refs[1] = lir::LirTypeRef::integer(32);
                 },
                 "signature-store native intrinsic flag mismatch must reject");
  store_rejected([](auto&, auto& call) {
                   call.arg_type_refs[1] = lir::LirTypeRef::integer(32);
                 },
                 "argument native intrinsic flag mismatch must reject");
}

lir::LirModule builtin_ffs_cttz_add_one_module(unsigned width) {
  auto module = native_intrinsic_module();
  auto& caller = module.functions[0];
  auto& block = caller.blocks[0];
  auto& cttz = std::get<lir::LirCallOp>(block.insts[1]);
  const auto type = lir::LirTypeRef::integer(width);
  cttz.return_type = type;
  cttz.callee_signature->return_type_ref = type;
  cttz.callee_signature->fixed_param_types[0] = width == 64 ? "i64" : "i32";
  cttz.callee_signature->fixed_param_type_refs[0] = type;
  cttz.arg_type_refs[0] = type;
  cttz.structured_args[0] = {width == 64 ? "i64" : "i32",
                              lir::LirOperand::integer("value", 41), type};
  block.insts.erase(block.insts.begin() + 2, block.insts.end());
  block.insts.push_back(lir::LirBinOp{
      lir::LirOperand::ssa("%cttz-plus-one", lir::LirValueId{10}),
      lir::LirBinaryOpcode::Add, type,
      lir::LirOperand::ssa("%cttz-result", lir::LirValueId{9}),
      lir::LirOperand::integer("one", 1)});
  caller.return_type = width == 64 ? scalar_type(c4c::TB_LONGLONG) : scalar_type(c4c::TB_INT);
  caller.return_type.inner_rank = -1;
  caller.signature_return_type_ref = type;
  block.terminator = lir::LirRet{
      lir::LirOperand::ssa("%cttz-plus-one-return", lir::LirValueId{10}),
      type};
  return module;
}

void test_builtin_ffs_cttz_add_one_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer, unsigned width) {
    const auto view = graph.view();
    const auto caller_id = view.functions()[0];
    const auto caller = view.function(caller_id).value();
    const auto instructions = caller.instructions(caller.blocks()[0]).value();
    const auto cttz = caller.instruction(instructions[0]).value();
    const auto add = caller.instruction(instructions[1]).value();
    const auto result = caller.value(add.results()[0]).value();
    const auto type = bir::Type{bir::TypeKind::Integer, width, width == 64 ? "i64" : "i32"};
    expect(instructions.size() == 2 && cttz.intrinsic_call() &&
               cttz.intrinsic_call()->kind == bir::IntrinsicKind::Cttz &&
               cttz.intrinsic_call()->type == type &&
               cttz.intrinsic_call()->zero_count_is_undef == std::optional<bool>{false} &&
               cttz.results().size() == 1 && add.binary() &&
               add.binary()->opcode == bir::BinaryOpcode::Add &&
               add.binary()->type == type &&
               add.operands().size() == 2 && add.operands()[0] == cttz.results()[0] &&
               result.type == type &&
               result.source_id == bir::SourceValueId{caller_id, 10} &&
               caller.source_value(*result.source_id).value() == add.results()[0],
           layer + " must retain only the exact same-width builtin-ffs Cttz result as Add-one lhs");
    const auto rhs = caller.value(add.operands()[1]).value();
    const auto* constant = std::get_if<bir::ConstantDef>(&rhs.definition);
    const auto definition = constant ? view.constant(constant->constant).value()
                                     : bir::ConstantDefinition{};
    const auto* integer = constant ? std::get_if<bir::IntegerConstant>(&definition.payload)
                                   : nullptr;
    expect(rhs.type == type && integer && integer->value == 1,
           layer + " must materialize the same-width Cttz Add immediate one");
  };

  for (const auto width : {32U, 64U}) {
    const auto module = builtin_ffs_cttz_add_one_module(width);
    const auto raw = bir::lower_lir_to_raw_bir(module);
    expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
           "builtin ffs Cttz Add-one must publish verified Raw BIR" +
               (raw.has_value() ? std::string{} : ": " + raw.error().detail));
    inspect(raw.value(), "Raw BIR", width);
    const auto canonical = bir::lower_lir_to_canonical_bir(module);
    expect(canonical.has_value(), "builtin ffs Cttz Add-one must canonicalize");
    inspect(canonical.value(), "Canonical BIR", width);

    const auto rejected = [width](auto mutate, const std::string& message) {
      auto candidate = builtin_ffs_cttz_add_one_module(width);
      auto& cttz = std::get<lir::LirCallOp>(candidate.functions[0].blocks[0].insts[1]);
      auto& add = std::get<lir::LirBinOp>(candidate.functions[0].blocks[0].insts[2]);
      mutate(candidate, cttz, add);
      const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
      expect(!raw_rejected.has_value() && raw_rejected.error().code ==
                 bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
             message + " (Raw rollback)");
      const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
      expect(!canonical_rejected.has_value() && canonical_rejected.error().code ==
                 bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
             message + " (Canonical rollback)");
    };
    rejected([](auto&, auto& cttz, auto&) { cttz.result = lir::LirOperand::raw("%missing"); },
             "missing Cttz result authority must reject atomically");
    rejected([](auto&, auto& cttz, auto&) {
               cttz.result = lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
             }, "invalid Cttz result identity must reject atomically");
    rejected([](auto&, auto& cttz, auto&) {
               cttz.result = lir::LirOperand::ssa("%duplicate", lir::LirValueId{3});
             }, "duplicate Cttz result identity must reject atomically");
    rejected([](auto&, auto& cttz, auto&) { cttz.intrinsic_kind = lir::LirIntrinsicKind::Ctpop; },
             "non-Cttz intrinsic provenance must reject atomically");
    rejected([](auto& candidate, auto& cttz, auto&) {
               cttz.direct_callee_link_name_id = candidate.functions[0].link_name_id;
             }, "mismatched direct Cttz LinkNameId must reject atomically");
    rejected([](auto&, auto& cttz, auto&) { cttz.callee_signature->is_variadic = true; },
             "variadic Cttz signature must reject atomically");
    rejected([](auto&, auto& cttz, auto&) { cttz.callee_signature->fixed_param_types[1] = "i32"; },
             "wrong Cttz flag signature must reject atomically");
    rejected([](auto&, auto& cttz, auto&) {
               cttz.zero_count_behavior = lir::LirZeroCountBehavior::Undefined;
             }, "wrong defined-zero Cttz behavior must reject atomically");
    rejected([](auto&, auto&, auto& add) {
               add.lhs = lir::LirOperand::ssa("%unknown", lir::LirValueId{77});
             }, "unresolved Cttz result-to-Add lhs must reject atomically");
    rejected([](auto&, auto&, auto& add) {
               add.lhs = lir::LirOperand::ssa("%wrong", lir::LirValueId{3});
             }, "wrong Cttz result-to-Add lhs must reject atomically");
    rejected([](auto&, auto&, auto& add) { add.rhs = lir::LirOperand::integer("two", 2); },
             "non-one Cttz Add immediate must reject atomically");
    rejected([width](auto&, auto&, auto& add) {
               add.type_str = lir::LirTypeRef::integer(width == 32 ? 64 : 32);
             }, "wrong-width Cttz Add must reject atomically");
    rejected([](auto& candidate, auto&, auto& add) {
             auto foreign = candidate.functions[0];
             foreign.name = "foreign_cttz_owner";
             foreign.link_name_id = candidate.link_names.intern("foreign_cttz_owner");
             auto& foreign_cttz = std::get<lir::LirCallOp>(foreign.blocks[0].insts[1]);
             foreign_cttz.result = lir::LirOperand::ssa("%foreign-cttz", lir::LirValueId{77});
             auto& foreign_add = std::get<lir::LirBinOp>(foreign.blocks[0].insts[2]);
             foreign_add.lhs = lir::LirOperand::ssa("%foreign-cttz", lir::LirValueId{77});
             foreign_add.result = lir::LirOperand::ssa("%foreign-add", lir::LirValueId{78});
             foreign.blocks[0].terminator = lir::LirRet{
                 lir::LirOperand::ssa("%foreign-return", lir::LirValueId{78}),
                 foreign_add.type_str};
             candidate.functions.push_back(std::move(foreign));
             add.lhs = lir::LirOperand::ssa("%foreign-cttz", lir::LirValueId{77});
           }, "cross-owner Cttz result use must reject atomically");
  }
}

lir::LirModule builtin_ctz_final_use_module(unsigned width) {
  auto module = native_intrinsic_module();
  auto& caller = module.functions[0];
  auto& block = caller.blocks[0];
  auto& cttz = std::get<lir::LirCallOp>(block.insts[1]);
  const auto type = lir::LirTypeRef::integer(width);
  cttz.return_type = type;
  cttz.callee_signature->return_type_ref = type;
  cttz.callee_signature->fixed_param_types[0] = width == 64 ? "i64" : "i32";
  cttz.callee_signature->fixed_param_type_refs[0] = type;
  cttz.arg_type_refs[0] = type;
  cttz.structured_args[0] = {width == 64 ? "i64" : "i32",
                              lir::LirOperand::integer("value", 41), type};
  cttz.zero_count_behavior = lir::LirZeroCountBehavior::Undefined;
  cttz.structured_args[1] = {"i1", lir::LirOperand::integer("undef", 1),
                              lir::LirTypeRef::integer(1)};
  block.insts.erase(block.insts.begin() + 2, block.insts.end());
  if (width == 64) {
    block.insts.push_back(lir::LirCastOp{
        .result = lir::LirOperand::ssa("%ctz-trunc", lir::LirValueId{10}),
        .kind = lir::LirCastKind::Trunc,
        .from_type = lir::LirTypeRef::integer(64),
        .operand = lir::LirOperand::ssa("%ctz-wide", lir::LirValueId{9}),
        .to_type = lir::LirTypeRef::integer(32)});
  }
  const auto add_id = width == 64 ? lir::LirValueId{11} : lir::LirValueId{10};
  const auto lhs_id = width == 64 ? lir::LirValueId{10} : lir::LirValueId{9};
  block.insts.push_back(lir::LirBinOp{
      lir::LirOperand::ssa("%ctz-final-add", add_id), lir::LirBinaryOpcode::Add,
      lir::LirTypeRef::integer(32), lir::LirOperand::ssa("%ctz-final-lhs", lhs_id),
      lir::LirOperand::integer("one", 1)});
  caller.return_type = scalar_type(c4c::TB_INT);
  caller.return_type.inner_rank = -1;
  caller.signature_return_type_ref = lir::LirTypeRef::integer(32);
  block.terminator = lir::LirRet{lir::LirOperand::ssa("%ctz-return", add_id),
                                 lir::LirTypeRef::integer(32)};
  return module;
}

void test_builtin_ctz_call_narrow_final_use_receipt_and_rejections() {
  for (const auto width : {32U, 64U}) {
    const auto module = builtin_ctz_final_use_module(width);
    const auto raw = bir::lower_lir_to_raw_bir(module);
    expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
           "builtin ctz final-use chain must publish verified Raw BIR");
    expect(bir::lower_lir_to_canonical_bir(module).has_value(),
           "builtin ctz final-use chain must canonicalize");
    const auto rejected = [width](auto mutate, const std::string& message) {
      auto candidate = builtin_ctz_final_use_module(width);
      auto& block = candidate.functions[0].blocks[0];
      auto& cttz = std::get<lir::LirCallOp>(block.insts[1]);
      auto* trunc = width == 64 ? &std::get<lir::LirCastOp>(block.insts[2]) : nullptr;
      auto& add = std::get<lir::LirBinOp>(block.insts[width == 64 ? 3 : 2]);
      mutate(candidate, cttz, trunc, add);
      expect(!bir::lower_lir_to_raw_bir(candidate).has_value(), message + " (Raw rollback)");
      expect(!bir::lower_lir_to_canonical_bir(candidate).has_value(),
             message + " (Canonical rollback)");
    };
    rejected([](auto&, auto& cttz, auto*, auto&) { cttz.result = lir::LirOperand::raw("%raw"); },
             "missing ctz result authority must reject atomically");
    rejected([](auto&, auto& cttz, auto*, auto&) { cttz.intrinsic_kind = lir::LirIntrinsicKind::Ctpop; },
             "other intrinsic kind must reject atomically");
    rejected([](auto&, auto& cttz, auto*, auto&) { cttz.zero_count_behavior = lir::LirZeroCountBehavior::Defined; },
             "defined-zero ctz must reject atomically");
    rejected([](auto&, auto& cttz, auto*, auto&) { cttz.structured_args[1].operand = lir::LirOperand::integer("false", 0); },
             "false ctz flag must reject atomically");
    rejected([](auto&, auto&, auto*, auto& add) { add.lhs = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); },
             "unresolved ctz final use must reject atomically");
    if (width == 64)
      rejected([](auto&, auto&, auto* trunc, auto&) { trunc->kind = lir::LirCastKind::ZExt; },
               "non-Trunc ctz narrowing must reject atomically");
    auto misleading = builtin_ctz_final_use_module(width);
    auto& call = std::get<lir::LirCallOp>(misleading.functions[0].blocks[0].insts[1]);
    call.result.str() = "%display-only";
    call.callee.str() = "@display-only";
    call.args_str = "display-only";
    expect(bir::lower_lir_to_raw_bir(misleading).has_value(),
           "ctz receipt must retain native authority despite misleading displays");
  }
}

lir::LirModule builtin_clz_final_use_module(unsigned width) {
  auto module = builtin_ctz_final_use_module(width);
  auto& ctlz = std::get<lir::LirCallOp>(module.functions[0].blocks[0].insts[1]);
  ctlz.intrinsic_kind = lir::LirIntrinsicKind::Ctlz;
  return module;
}

void test_builtin_clz_call_narrow_final_use_receipt_and_rejections() {
  for (const auto width : {32U, 64U}) {
    const auto module = builtin_clz_final_use_module(width);
    const auto raw = bir::lower_lir_to_raw_bir(module);
    expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
           "builtin clz final-use chain must publish verified Raw BIR");
    expect(bir::lower_lir_to_canonical_bir(module).has_value(),
           "builtin clz final-use chain must canonicalize");
    const auto rejected = [width](auto mutate, const std::string& message) {
      auto candidate = builtin_clz_final_use_module(width);
      auto& block = candidate.functions[0].blocks[0];
      auto& ctlz = std::get<lir::LirCallOp>(block.insts[1]);
      auto* trunc = width == 64 ? &std::get<lir::LirCastOp>(block.insts[2]) : nullptr;
      auto& add = std::get<lir::LirBinOp>(block.insts[width == 64 ? 3 : 2]);
      mutate(candidate, ctlz, trunc, add);
      expect(!bir::lower_lir_to_raw_bir(candidate).has_value(), message + " (Raw rollback)");
      expect(!bir::lower_lir_to_canonical_bir(candidate).has_value(),
             message + " (Canonical rollback)");
    };
    rejected([](auto&, auto& ctlz, auto*, auto&) { ctlz.result = lir::LirOperand::raw("%raw"); },
             "missing clz result authority must reject atomically");
    rejected([](auto&, auto& ctlz, auto*, auto&) { ctlz.intrinsic_kind = lir::LirIntrinsicKind::Ctpop; },
             "other intrinsic kind must reject atomically");
    rejected([](auto&, auto& ctlz, auto*, auto&) { ctlz.zero_count_behavior = lir::LirZeroCountBehavior::Defined; },
             "defined-zero clz must reject atomically");
    rejected([](auto&, auto& ctlz, auto*, auto&) { ctlz.structured_args[1].operand = lir::LirOperand::integer("false", 0); },
             "false clz flag must reject atomically");
    rejected([](auto&, auto&, auto*, auto& add) { add.lhs = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); },
             "unresolved clz final use must reject atomically");
    if (width == 64)
      rejected([](auto&, auto&, auto* trunc, auto&) { trunc->kind = lir::LirCastKind::ZExt; },
               "non-Trunc clz narrowing must reject atomically");
    auto misleading = builtin_clz_final_use_module(width);
    auto& call = std::get<lir::LirCallOp>(misleading.functions[0].blocks[0].insts[1]);
    call.result.str() = "%display-only";
    call.callee.str() = "@display-only";
    call.args_str = "display-only";
    expect(bir::lower_lir_to_raw_bir(misleading).has_value(),
           "clz receipt must retain native authority despite misleading displays");
  }
}

lir::LirModule builtin_popcount_final_use_module(unsigned width) {
  auto module = builtin_ctz_final_use_module(width);
  auto& ctpop = std::get<lir::LirCallOp>(module.functions[0].blocks[0].insts[1]);
  ctpop.intrinsic_kind = lir::LirIntrinsicKind::Ctpop;
  const auto link = module.link_names.intern(width == 64 ? "llvm.ctpop.i64" : "llvm.ctpop.i32");
  ctpop.callee = lir::LirOperand::global("@display", link);
  ctpop.direct_callee_link_name_id = link;
  ctpop.zero_count_behavior.reset();
  ctpop.callee_signature->fixed_param_types.pop_back();
  ctpop.callee_signature->fixed_param_type_refs.pop_back();
  ctpop.arg_type_refs.pop_back();
  ctpop.structured_args.pop_back();
  return module;
}

void test_builtin_popcount_call_narrow_final_use_receipt_and_rejections() {
  for (const auto width : {32U, 64U}) {
    const auto module = builtin_popcount_final_use_module(width);
    const auto raw = bir::lower_lir_to_raw_bir(module);
    expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
           "builtin popcount final-use chain must publish verified Raw BIR");
    expect(bir::lower_lir_to_canonical_bir(module).has_value(),
           "builtin popcount final-use chain must canonicalize");
    auto store_backed = builtin_popcount_final_use_module(width);
    auto& store_backed_ctpop =
        std::get<lir::LirCallOp>(store_backed.functions[0].blocks[0].insts[1]);
    lir::LirFunctionSignatureStoreEntry store_backed_entry;
    store_backed_entry.return_type_ref = store_backed_ctpop.return_type;
    store_backed_entry.fixed_param_type_refs = {store_backed_ctpop.return_type};
    store_backed_entry.fixed_param_is_byval = {false};
    store_backed_ctpop.callee_signature_ref =
        store_backed.register_function_signature(std::move(store_backed_entry));
    store_backed_ctpop.callee_signature.reset();
    const auto raw_store_backed = bir::lower_lir_to_raw_bir(store_backed);
    expect(raw_store_backed.has_value() &&
               bir::FoundationVerifier::verify(raw_store_backed.value()).ok(),
           "store-backed popcount must accept absent retained one-parameter signature");
    expect(bir::lower_lir_to_canonical_bir(store_backed).has_value(),
           "store-backed popcount without retained signature must canonicalize");

    auto stale_store_backed = builtin_popcount_final_use_module(width);
    auto& stale_store_backed_ctpop = std::get<lir::LirCallOp>(
        stale_store_backed.functions[0].blocks[0].insts[1]);
    lir::LirFunctionSignatureStoreEntry stale_store_backed_entry;
    stale_store_backed_entry.return_type_ref = stale_store_backed_ctpop.return_type;
    stale_store_backed_entry.fixed_param_type_refs = {
        stale_store_backed_ctpop.return_type};
    stale_store_backed_entry.fixed_param_is_byval = {false};
    stale_store_backed_ctpop.callee_signature_ref =
        stale_store_backed.register_function_signature(
            std::move(stale_store_backed_entry));
    stale_store_backed_ctpop.callee_signature->fixed_param_types = {
        "i1 stale text only"};
    stale_store_backed_ctpop.args_str = "stale display only";
    expect(bir::lower_lir_to_raw_bir(stale_store_backed).has_value(),
           "store-backed popcount must ignore retained text for one-parameter signature");

    const auto rejected = [width](auto mutate, const std::string& message) {
      auto candidate = builtin_popcount_final_use_module(width);
      auto& block = candidate.functions[0].blocks[0];
      auto& ctpop = std::get<lir::LirCallOp>(block.insts[1]);
      auto* trunc = width == 64 ? &std::get<lir::LirCastOp>(block.insts[2]) : nullptr;
      auto& add = std::get<lir::LirBinOp>(block.insts[width == 64 ? 3 : 2]);
      mutate(candidate, ctpop, trunc, add);
      expect(!bir::lower_lir_to_raw_bir(candidate).has_value(), message + " (Raw rollback)");
      expect(!bir::lower_lir_to_canonical_bir(candidate).has_value(),
             message + " (Canonical rollback)");
    };
    rejected([](auto&, auto& ctpop, auto*, auto&) { ctpop.result = lir::LirOperand::raw("%raw"); },
             "missing popcount result authority must reject atomically");
    rejected([](auto&, auto& ctpop, auto*, auto&) {
               ctpop.result = lir::LirOperand::ssa("%duplicate", lir::LirValueId{3});
             }, "duplicate popcount result identity must reject atomically");
    rejected([](auto&, auto& ctpop, auto*, auto&) { ctpop.intrinsic_kind = lir::LirIntrinsicKind::Ctlz; },
             "other intrinsic kind must reject atomically");
    rejected([](auto& candidate, auto& ctpop, auto*, auto&) {
               ctpop.direct_callee_link_name_id = candidate.functions[0].link_name_id;
             }, "mismatched direct popcount LinkNameId must reject atomically");
    rejected([](auto&, auto& ctpop, auto*, auto&) { ctpop.callee_signature->is_variadic = true; },
             "variadic popcount signature must reject atomically");
    rejected([](auto&, auto& ctpop, auto*, auto&) {
               ctpop.callee_signature->fixed_param_types[0] = "i1";
             }, "wrong popcount signature must reject atomically");
    rejected([](auto&, auto& ctpop, auto*, auto&) {
               ctpop.zero_count_behavior = lir::LirZeroCountBehavior::Undefined;
             }, "popcount zero-count behavior must reject atomically");
    rejected([](auto&, auto& ctpop, auto*, auto&) {
               ctpop.structured_args.push_back({"i1", lir::LirOperand::integer("extra", 1),
                                                lir::LirTypeRef::integer(1)});
             }, "wrong popcount argument count must reject atomically");
    rejected([](auto& candidate, auto& ctpop, auto*, auto&) {
               lir::LirFunctionSignatureStoreEntry entry;
               entry.return_type_ref = ctpop.return_type;
               entry.fixed_param_type_refs = {lir::LirTypeRef::integer(1)};
               entry.fixed_param_is_byval = {false};
               ctpop.callee_signature_ref =
                   candidate.register_function_signature(std::move(entry));
             }, "store-backed popcount one-parameter mismatch must reject atomically");
    rejected([](auto&, auto&, auto*, auto& add) {
               add.lhs = lir::LirOperand::ssa("%unknown", lir::LirValueId{77});
             }, "unresolved popcount final use must reject atomically");
    if (width == 64)
      rejected([](auto&, auto&, auto* trunc, auto&) { trunc->kind = lir::LirCastKind::ZExt; },
               "non-Trunc popcount narrowing must reject atomically");
    if (width == 64)
      rejected([](auto&, auto&, auto* trunc, auto&) {
                 trunc->from_type = lir::LirTypeRef::integer(32);
               }, "wrong popcount narrowing endpoints must reject atomically");
    rejected([](auto& candidate, auto&, auto*, auto& add) {
               auto foreign = candidate.functions[0];
               foreign.name = "foreign_popcount_owner";
               foreign.link_name_id = candidate.link_names.intern("foreign_popcount_owner");
               auto& foreign_ctpop = std::get<lir::LirCallOp>(foreign.blocks[0].insts[1]);
               foreign_ctpop.result = lir::LirOperand::ssa("%foreign-popcount", lir::LirValueId{77});
               candidate.functions.push_back(std::move(foreign));
               add.lhs = lir::LirOperand::ssa("%foreign-popcount", lir::LirValueId{77});
             }, "cross-owner popcount result use must reject atomically");
    auto misleading = builtin_popcount_final_use_module(width);
    auto& ctpop = std::get<lir::LirCallOp>(misleading.functions[0].blocks[0].insts[1]);
    ctpop.result.str() = "%display-only";
    ctpop.callee.str() = "@display-only";
    ctpop.args_str = "display-only";
    expect(bir::lower_lir_to_raw_bir(misleading).has_value(),
           "popcount receipt must retain native authority despite misleading displays");
  }
}

lir::LirModule builtin_ffs_add_select_false_arm_module(unsigned width) {
  auto module = native_intrinsic_module();
  auto& caller = module.functions[0];
  auto& block = caller.blocks[0];
  auto& cttz = std::get<lir::LirCallOp>(block.insts[1]);
  const auto type = lir::LirTypeRef::integer(width);
  cttz.return_type = type;
  cttz.callee_signature->return_type_ref = type;
  cttz.callee_signature->fixed_param_types[0] = width == 64 ? "i64" : "i32";
  cttz.callee_signature->fixed_param_type_refs[0] = type;
  cttz.arg_type_refs[0] = type;
  cttz.structured_args[0] = {width == 64 ? "i64" : "i32",
                              lir::LirOperand::integer("value", 41), type};
  block.insts.erase(block.insts.begin() + 2, block.insts.end());
  block.insts.push_back(lir::LirBinOp{
      lir::LirOperand::ssa("%ffs-plus-one", lir::LirValueId{10}),
      lir::LirBinaryOpcode::Add, type,
      lir::LirOperand::ssa("%cttz", lir::LirValueId{9}),
      lir::LirOperand::integer("one", 1)});
  block.insts.push_back(lir::LirCmpOp{
      lir::LirOperand::ssa("%ffs-is-zero", lir::LirValueId{12}), false,
      lir::LirCmpPredicate::Eq, type,
      lir::LirOperand::integer("value", 41), lir::LirOperand::integer("zero", 0)});
  block.insts.push_back(lir::LirSelectOp{
      lir::LirOperand::ssa("%ffs-select", lir::LirValueId{11}), type,
      lir::LirOperand::ssa("%ffs-is-zero", lir::LirValueId{12}), lir::LirOperand::raw("zero"),
      lir::LirOperand::ssa("%ffs-plus-one", lir::LirValueId{10})});
  caller.return_type = width == 64 ? scalar_type(c4c::TB_LONGLONG) : scalar_type(c4c::TB_INT);
  caller.return_type.inner_rank = -1;
  caller.signature_return_type_ref = type;
  block.terminator = lir::LirRet{lir::LirOperand::ssa("%return", lir::LirValueId{11}), type};
  return module;
}

void test_builtin_ffs_add_select_false_arm_receipt_and_rejections() {
  for (const auto width : {32U, 64U}) {
    const auto module = builtin_ffs_add_select_false_arm_module(width);
    const auto raw = bir::lower_lir_to_raw_bir(module);
    expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
           "builtin ffs Add-one false-arm chain must publish verified Raw BIR" +
               (raw.has_value() ? std::string{} : ": " + raw.error().detail +
                   (raw.error().verification_errors.empty() ? "" : ": " + raw.error().verification_errors.front().message)));
    const auto view = raw.value().view();
    const auto function = view.function(view.functions()[0]).value();
    const auto insts = function.instructions(function.blocks()[0]).value();
    const auto add = function.instruction(insts[1]).value();
    const auto compare = function.instruction(insts[2]).value();
    const auto select = function.instruction(insts[3]).value();
    expect(add.binary() && add.binary()->opcode == bir::BinaryOpcode::Add &&
               add.binary()->type.bit_width == width && add.results().size() == 1 &&
               compare.compare() && compare.compare()->predicate == bir::ComparePredicate::Eq &&
               compare.compare()->type.bit_width == width && compare.results().size() == 1 &&
               select.select() && select.select()->type.bit_width == width &&
               select.operands() == std::vector<bir::ValueId>{compare.results()[0], add.results()[0]},
           "builtin ffs Select must retain its exact Eq-zero condition and Add-one false-arm edges");
    for (const auto& mutation : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
      auto candidate = builtin_ffs_add_select_false_arm_module(width);
      auto& add_op = std::get<lir::LirBinOp>(candidate.functions[0].blocks[0].insts[2]);
      auto& compare_op = std::get<lir::LirCmpOp>(candidate.functions[0].blocks[0].insts[3]);
      auto& select_op = std::get<lir::LirSelectOp>(candidate.functions[0].blocks[0].insts[4]);
      if (mutation == 0) add_op.result = lir::LirOperand::raw("%missing");
      if (mutation == 1) add_op.result = lir::LirOperand::ssa("%duplicate", lir::LirValueId{9});
      if (mutation == 2) add_op.opcode = lir::LirBinaryOpcode::Mul;
      if (mutation == 3) add_op.rhs = lir::LirOperand::integer("two", 2);
      if (mutation == 4) select_op.false_val = lir::LirOperand::ssa("%missing", lir::LirValueId{77});
      if (mutation == 5) select_op.false_val = lir::LirOperand::integer("one", 1);
      if (mutation == 6) compare_op.result = lir::LirOperand::raw("%missing");
      if (mutation == 7) compare_op.predicate = lir::LirCmpPredicate::Slt;
      if (mutation == 8) compare_op.rhs = lir::LirOperand::integer("one", 1);
      if (mutation == 9) select_op.cond = lir::LirOperand::ssa("%missing", lir::LirValueId{77});
      if (mutation == 10) compare_op.lhs = lir::LirOperand::ssa("%cttz", lir::LirValueId{9});
      expect(!bir::lower_lir_to_raw_bir(candidate).has_value(),
             "malformed builtin ffs Add/select linkage must roll back the complete module");
    }
  }
}

lir::LirModule intrinsic_i64_trunc_module() {
  auto module = native_intrinsic_module();
  auto& caller = module.functions[0];
  auto& intrinsic = std::get<lir::LirCallOp>(caller.blocks[0].insts[1]);
  intrinsic.return_type = lir::LirTypeRef::integer(64);
  intrinsic.callee_signature->return_type_ref = lir::LirTypeRef::integer(64);
  intrinsic.callee_signature->fixed_param_types[0] = "i64";
  intrinsic.callee_signature->fixed_param_type_refs[0] = lir::LirTypeRef::integer(64);
  intrinsic.arg_type_refs[0] = lir::LirTypeRef::integer(64);
  intrinsic.structured_args[0] = {"i64", lir::LirOperand::integer("wide", 41),
                                  lir::LirTypeRef::integer(64)};
  intrinsic.result = lir::LirOperand::ssa("%intrinsic.i64", lir::LirValueId{9});
  caller.blocks[0].insts.erase(caller.blocks[0].insts.begin() + 2);
  caller.blocks[0].insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand::ssa("%trunc", lir::LirValueId{10}),
      .kind = lir::LirCastKind::Trunc,
      .from_type = lir::LirTypeRef::integer(64),
      .operand = lir::LirOperand::ssa("%intrinsic.i64", lir::LirValueId{9}),
      .to_type = lir::LirTypeRef::integer(32),
  });
  caller.blocks[0].terminator = lir::LirRet{
      lir::LirOperand::ssa("%return", lir::LirValueId{10}), lir::LirTypeRef::integer(32)};
  return module;
}

void test_native_intrinsic_i64_trunc_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto caller_id = view.functions()[0];
    const auto caller = view.function(caller_id).value();
    const auto insts = caller.instructions(caller.blocks()[0]).value();
    expect(insts.size() == 2, layer + " must retain intrinsic and Trunc separately");
    const auto intrinsic = caller.instruction(insts[0]).value();
    const auto trunc = caller.instruction(insts[1]).value();
    expect(intrinsic.intrinsic_call() && intrinsic.results().size() == 1 &&
               intrinsic.intrinsic_call()->type == bir::Type{bir::TypeKind::Integer, 64, "i64"} &&
               trunc.opcode() == bir::Opcode::Cast && trunc.cast() &&
               trunc.cast()->kind == bir::CastKind::Trunc &&
               trunc.cast()->from_type == bir::Type{bir::TypeKind::Integer, 64, "i64"} &&
               trunc.cast()->to_type == bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
               trunc.operands() == std::vector<bir::ValueId>{intrinsic.results()[0]} &&
               trunc.results().size() == 1 &&
               caller.value(trunc.results()[0]).value().source_id ==
                   bir::SourceValueId{caller_id, 10},
           layer + " must retain a typed intrinsic-result i64-to-i32 Trunc receipt");
  };
  const auto module = intrinsic_i64_trunc_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "i64 intrinsic Trunc must publish verified Raw BIR");
  inspect(raw.value(), "Raw BIR");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(), "i64 intrinsic Trunc must canonicalize");
  inspect(canonical.value(), "Canonical BIR");
  const auto rejected = [](auto mutate, const std::string& message) {
    auto candidate = intrinsic_i64_trunc_module();
    auto& cast = std::get<lir::LirCastOp>(candidate.functions[0].blocks[0].insts[2]);
    mutate(candidate, cast);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() && raw_rejected.error().code == bir::ImportErrorCode::UnsupportedOrdinaryInstruction, message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value() && canonical_rejected.error().code == bir::ImportErrorCode::UnsupportedOrdinaryInstruction, message + " (Canonical rollback)");
  };
  rejected([](auto&, auto& cast) { cast.kind = lir::LirCastKind::ZExt; }, "other casts must remain fail-closed");
  rejected([](auto&, auto& cast) { cast.operand = lir::LirOperand::integer("bad", 0); }, "immediate cast operands must remain fail-closed");
  rejected([](auto&, auto& cast) { cast.operand = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); }, "unknown cast operands must reject atomically");
  rejected([](auto&, auto& cast) { cast.from_type = lir::LirTypeRef::integer(32); }, "wrong cast endpoints must reject atomically");
  rejected([](auto&, auto& cast) { cast.result = lir::LirOperand::ssa("%duplicate", lir::LirValueId{9}); }, "duplicate cast result ids must reject atomically");
}

lir::LirModule scalar_i32_to_i64_sext_module() {
  auto module = direct_integer_call_module();
  auto& caller = module.functions[0];
  caller.return_type = scalar_type(c4c::TB_LONGLONG);
  caller.return_type.inner_rank = -1;
  caller.signature_return_type_ref = lir::LirTypeRef::integer(64);
  auto& block = caller.blocks[0];
  block.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand::ssa("%presentation-only-sext-result", lir::LirValueId{10}),
      .kind = lir::LirCastKind::SExt,
      .from_type = lir::LirTypeRef::integer(32),
      .operand = lir::LirOperand::ssa("%presentation-only-sext-source", lir::LirValueId{9}),
      .to_type = lir::LirTypeRef::integer(64),
  });
  block.insts.push_back(lir::LirBinOp{
      lir::LirOperand::ssa("%presentation-only-i64-add", lir::LirValueId{11}),
      lir::LirBinaryOpcode::Add, lir::LirTypeRef::integer(64),
      lir::LirOperand::ssa("%presentation-only-sext-use", lir::LirValueId{10}),
      lir::LirOperand::integer("presentation-only-one", 1)});
  block.terminator = lir::LirRet{
      lir::LirOperand::ssa("%presentation-only-i64-return", lir::LirValueId{11}),
      lir::LirTypeRef::integer(64)};
  return module;
}

void test_scalar_i32_to_i64_sext_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto caller_id = view.functions()[0];
    const auto caller = view.function(caller_id).value();
    const auto insts = caller.instructions(caller.blocks()[0]).value();
    expect(insts.size() == 3, layer + " must retain Call, SExt, and downstream Add");
    const auto call = caller.instruction(insts[0]).value();
    const auto sext = caller.instruction(insts[1]).value();
    const auto add = caller.instruction(insts[2]).value();
    expect(call.results().size() == 1 && sext.cast() &&
               sext.cast()->kind == bir::CastKind::SExt &&
               sext.cast()->from_type == bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
               sext.cast()->to_type == bir::Type{bir::TypeKind::Integer, 64, "i64"} &&
               sext.operands() == std::vector<bir::ValueId>{call.results()[0]} &&
               sext.results().size() == 1 &&
               caller.value(sext.results()[0]).value().source_id == bir::SourceValueId{caller_id, 10} &&
               add.binary() && add.binary()->opcode == bir::BinaryOpcode::Add &&
               add.binary()->type == bir::Type{bir::TypeKind::Integer, 64, "i64"} &&
               add.operands().size() == 2 && add.operands()[0] == sext.results()[0] &&
               caller.value(add.results()[0]).value().source_id == bir::SourceValueId{caller_id, 11},
           layer + " must preserve the exact typed current-function SExt-to-i64-Add chain");
  };
  const auto module = scalar_i32_to_i64_sext_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "explicit i32-to-i64 SExt must publish verified Raw BIR" +
             (raw.has_value() ? std::string{} : ": " + raw.error().detail));
  inspect(raw.value(), "Raw BIR");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(), "explicit i32-to-i64 SExt must canonicalize");
  inspect(canonical.value(), "Canonical BIR");
  const auto rejected = [](auto mutate, const std::string& message) {
    auto candidate = scalar_i32_to_i64_sext_module();
    auto& cast = std::get<lir::LirCastOp>(candidate.functions[0].blocks[0].insts[2]);
    auto& add = std::get<lir::LirBinOp>(candidate.functions[0].blocks[0].insts[3]);
    mutate(candidate, cast, add);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() && raw_rejected.error().code == bir::ImportErrorCode::UnsupportedOrdinaryInstruction, message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value() && canonical_rejected.error().code == bir::ImportErrorCode::UnsupportedOrdinaryInstruction, message + " (Canonical rollback)");
  };
  rejected([](auto&, auto& cast, auto&) { cast.result = lir::LirOperand::raw("%missing"); }, "missing cast result authority must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.operand = lir::LirOperand::integer("not-ssa", 1); }, "wrong cast operand alternative must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.operand = lir::LirOperand::ssa("%unknown", lir::LirValueId{77}); }, "unresolved or cross-owner cast source must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.result = lir::LirOperand::ssa("%duplicate", lir::LirValueId{9}); }, "duplicate cast result must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.kind = lir::LirCastKind::ZExt; }, "wrong cast kind must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.from_type = lir::LirTypeRef::integer(64); }, "wrong cast source endpoint must reject atomically");
  rejected([](auto&, auto& cast, auto&) { cast.to_type = lir::LirTypeRef::integer(32); }, "wrong cast destination endpoint must reject atomically");
  rejected([](auto&, auto&, auto& add) { add.lhs = lir::LirOperand::ssa("%unresolved-use", lir::LirValueId{77}); }, "unresolved downstream i64 Add use must reject atomically");
}

lir::LirModule selected_global_i32_slt_compare_module() {
  auto module = direct_global_integer_load_module();
  auto& block = module.functions[0].blocks[0];
  block.insts.erase(block.insts.begin() + 1);
  block.insts.push_back(lir::LirCmpOp{
      lir::LirOperand::ssa("%presentation-only-slt-result", lir::LirValueId{33}),
      false, lir::LirCmpPredicate::Slt, lir::LirTypeRef::integer(32),
      lir::LirOperand::ssa("%presentation-only-load", lir::LirValueId{31}),
      lir::LirOperand::integer("presentation-only-seven", 7)});
  return module;
}

void test_selected_global_i32_slt_compare_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto function_id = view.functions()[0];
    const auto function = view.function(function_id).value();
    const auto instructions = function.instructions(function.blocks()[0]).value();
    const auto load = function.instruction(instructions[0]).value();
    const auto compare = function.instruction(instructions[1]).value();
    const auto result = function.value(compare.results()[0]).value();
    const auto* definition = std::get_if<bir::InstResultDef>(&result.definition);
    expect(instructions.size() == 2 && load.load() && compare.opcode() == bir::Opcode::Compare &&
               compare.compare() && compare.compare()->predicate == bir::ComparePredicate::Slt &&
               compare.compare()->type == bir::Type{bir::TypeKind::I32} &&
               compare.operands().size() == 2 && compare.operands()[0] == load.results()[0] &&
               function.value(compare.operands()[1]).value().type == bir::Type{bir::TypeKind::I32} &&
               compare.results().size() == 1 && result.type == bir::Type{bir::TypeKind::I1} &&
               result.source_id == bir::SourceValueId{function_id, 33} && definition &&
               definition->instruction == instructions[1] && definition->result_index == 0 &&
               function.source_value(bir::SourceValueId{function_id, 33}).value() ==
                   compare.results()[0],
           layer + " must retain the exact typed i32 SLT selected-load/immediate-seven receipt");
  };

  const auto module = selected_global_i32_slt_compare_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "explicit selected-global i32 SLT must publish verified Raw BIR" +
             (raw.has_value() ? std::string{} : ": " + raw.error().detail));
  inspect(raw.value(), "Raw BIR");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(), "explicit selected-global i32 SLT must canonicalize");
  inspect(canonical.value(), "Canonical BIR");

  const auto rejected = [](auto mutate, const std::string& message) {
    auto candidate = selected_global_i32_slt_compare_module();
    auto& compare = std::get<lir::LirCmpOp>(candidate.functions[0].blocks[0].insts[1]);
    mutate(candidate, compare);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() && raw_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value() && canonical_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Canonical rollback)");
  };
  rejected([](auto&, auto& compare) { compare.result = lir::LirOperand::raw("%missing"); },
           "missing compare result authority must reject atomically");
  rejected([](auto&, auto& compare) {
             compare.result = lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
           }, "invalid compare result identity must reject atomically");
  rejected([](auto&, auto& compare) {
             compare.result = lir::LirOperand::ssa("%duplicate", lir::LirValueId{31});
           }, "duplicate compare result identity must reject atomically");
  rejected([](auto&, auto& compare) { compare.is_float = true; },
           "floating compare mode must remain fail-closed");
  rejected([](auto&, auto& compare) { compare.predicate = lir::LirCmpPredicate::Eq; },
           "non-SLT compare predicates must remain fail-closed");
  rejected([](auto&, auto& compare) { compare.type_str = lir::LirTypeRef::integer(64); },
           "non-i32 compare types must remain fail-closed");
  rejected([](auto&, auto& compare) {
             compare.lhs = lir::LirOperand::integer("not-a-load", 7);
           }, "non-SSA compare lhs must reject atomically");
  rejected([](auto&, auto& compare) {
             compare.lhs = lir::LirOperand::ssa("%foreign-or-missing", lir::LirValueId{32});
           }, "unresolved current-function compare lhs must reject atomically");
  rejected([](auto&, auto& compare) {
             compare.rhs = lir::LirOperand::ssa("%not-immediate", lir::LirValueId{31});
           }, "non-immediate compare rhs must reject atomically");
  rejected([](auto&, auto& compare) {
             compare.rhs = lir::LirOperand::integer("not-seven", 6);
           }, "non-seven compare rhs must reject atomically");
  rejected([](auto&, auto& compare) { compare.rhs = lir::LirOperand::raw("7"); },
           "display-only compare rhs must reject atomically");

  auto following_zext = selected_global_i32_slt_compare_module();
  following_zext.functions[0].blocks[0].insts.push_back(
      lir::LirCastOp{.kind = lir::LirCastKind::ZExt});
  const auto zext_raw = bir::lower_lir_to_raw_bir(following_zext);
  expect(!zext_raw.has_value() && zext_raw.error().code ==
             bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
         "the following monostate ZExt must remain unsupported transactionally (Raw rollback)");
  const auto zext_canonical = bir::lower_lir_to_canonical_bir(following_zext);
  expect(!zext_canonical.has_value() && zext_canonical.error().code ==
             bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
         "the following monostate ZExt must remain unsupported transactionally (Canonical rollback)");
}

lir::LirModule selected_global_i32_abs_module() {
  auto module = normalized_i32_add_module();
  auto& block = module.functions[0].blocks[0];
  block.insts.insert(block.insts.begin() + 1, lir::LirAbsOp{
      lir::LirOperand::ssa("%presentation-abs", lir::LirValueId{32}),
      lir::LirOperand::ssa("%presentation-load", lir::LirValueId{31}),
      lir::LirTypeRef::integer(32)});
  auto& add = std::get<lir::LirBinOp>(block.insts[2]);
  add.lhs = lir::LirOperand::ssa("%presentation-abs-use", lir::LirValueId{32});
  return module;
}

void test_selected_global_i32_abs_receipt_and_rejections() {
  const auto inspect = [](const auto& graph, const std::string& layer) {
    const auto view = graph.view();
    const auto function_id = view.functions()[0];
    const auto function = view.function(function_id).value();
    const auto instructions = function.instructions(function.blocks()[0]).value();
    const auto load = function.instruction(instructions[0]).value();
    const auto abs = function.instruction(instructions[1]).value();
    const auto add = function.instruction(instructions[2]).value();
    const auto result = function.value(abs.results()[0]).value();
    expect(instructions.size() == 3 && load.load() && abs.opcode() == bir::Opcode::Abs &&
               abs.abs() && abs.abs()->type == bir::Type{bir::TypeKind::I32} &&
               abs.operands() == std::vector<bir::ValueId>{load.results()[0]} &&
               abs.results().size() == 1 && result.type == bir::Type{bir::TypeKind::I32} &&
               result.source_id == bir::SourceValueId{function_id, 32} &&
               function.source_value(*result.source_id).value() == abs.results()[0] &&
               add.binary() && add.binary()->opcode == bir::BinaryOpcode::Add &&
               add.operands()[0] == abs.results()[0],
           layer + " must retain the typed selected-load i32 Abs and its admitted later Add use");
  };

  const auto module = selected_global_i32_abs_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "selected-global i32 Abs must publish verified Raw BIR");
  inspect(raw.value(), "Raw BIR");
  const auto canonical = bir::lower_lir_to_canonical_bir(module);
  expect(canonical.has_value(), "selected-global i32 Abs must canonicalize");
  inspect(canonical.value(), "Canonical BIR");

  const auto rejected = [](auto mutate, const std::string& message) {
    auto candidate = selected_global_i32_abs_module();
    auto& abs = std::get<lir::LirAbsOp>(candidate.functions[0].blocks[0].insts[1]);
    mutate(candidate, abs);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value() && raw_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Raw rollback)");
    const auto canonical_rejected = bir::lower_lir_to_canonical_bir(candidate);
    expect(!canonical_rejected.has_value() && canonical_rejected.error().code ==
               bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
           message + " (Canonical rollback)");
  };
  rejected([](auto&, auto& abs) { abs.result = lir::LirOperand::ssa("%invalid", lir::LirValueId{}); },
           "invalid Abs result must reject atomically");
  rejected([](auto&, auto& abs) { abs.result = lir::LirOperand::ssa("%duplicate", lir::LirValueId{31}); },
           "duplicate Abs result must reject atomically");
  rejected([](auto&, auto& abs) { abs.arg = lir::LirOperand::ssa("%unresolved", lir::LirValueId{77}); },
           "unresolved Abs argument must reject atomically");
  rejected([](auto&, auto& abs) { abs.arg = lir::LirOperand::integer("immediate", 1); },
           "immediate Abs argument must reject atomically");
  rejected([](auto&, auto& abs) { abs.int_type = lir::LirTypeRef::integer(64); },
           "non-i32 Abs must reject atomically");
  rejected([](auto&, auto& abs) {
             abs.int_type = lir::LirTypeRef("i32", lir::LirTypeKind::RawText);
           },
           "presentation-derived Abs type must reject atomically");
  rejected([](auto& candidate, auto& abs) {
             auto& local_load = std::get<lir::LirLoadOp>(
                 candidate.functions[0].blocks[0].insts[0]);
             local_load.result = lir::LirOperand::ssa("%local-other", lir::LirValueId{99});
             lir::LirFunction foreign = candidate.functions[0];
             foreign.name = "foreign_abs_owner";
             foreign.blocks[0].insts.erase(foreign.blocks[0].insts.begin() + 1,
                                           foreign.blocks[0].insts.end());
             candidate.functions.push_back(std::move(foreign));
             abs.arg = lir::LirOperand::ssa("%foreign-load", lir::LirValueId{31});
           }, "cross-owner Abs argument must reject atomically");
  rejected([](auto& candidate, auto&) {
             auto& load = std::get<lir::LirLoadOp>(candidate.functions[0].blocks[0].insts[0]);
             load.ptr = lir::LirOperand::global("@missing", c4c::LinkNameId{999});
           }, "malformed selected-load linkage must reject atomically");
}

lir::LirModule mixed_accepted_row_dispatcher_module() {
  auto module = normalized_i32_add_module();
  auto& function = module.functions[0];
  function.return_type = scalar_type(c4c::TB_LONGLONG);
  function.return_type.inner_rank = -1;
  function.signature_return_type_ref = lir::LirTypeRef::integer(64);
  auto& block = function.blocks[0];
  block.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand::ssa("%mixed-sext", lir::LirValueId{34}),
      .kind = lir::LirCastKind::SExt,
      .from_type = lir::LirTypeRef::integer(32),
      .operand = lir::LirOperand::ssa("%mixed-add", lir::LirValueId{33}),
      .to_type = lir::LirTypeRef::integer(64),
  });
  block.insts.push_back(lir::LirCmpOp{
      lir::LirOperand::ssa("%mixed-slt", lir::LirValueId{35}),
      false, lir::LirCmpPredicate::Slt, lir::LirTypeRef::integer(32),
      lir::LirOperand::ssa("%mixed-load", lir::LirValueId{31}),
      lir::LirOperand::integer("mixed-seven", 7)});
  block.terminator = lir::LirRet{
      lir::LirOperand::ssa("%mixed-return", lir::LirValueId{34}),
      lir::LirTypeRef::integer(64)};
  return module;
}

void test_mixed_accepted_row_dispatcher_transactionality() {
  const auto module = mixed_accepted_row_dispatcher_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "mixed accepted Load/Add/SExt/SLT rows must publish verified Raw BIR");

  const auto view = raw.value().view();
  const auto function_id = view.functions()[0];
  const auto function = view.function(function_id).value();
  const auto instructions = function.instructions(function.blocks()[0]).value();
  const auto load = function.instruction(instructions[0]).value();
  const auto add = function.instruction(instructions[1]).value();
  const auto sext = function.instruction(instructions[2]).value();
  const auto compare = function.instruction(instructions[3]).value();
  const auto terminator = function.terminator(function.blocks()[0]).value();
  const auto* returned = std::get_if<bir::ReturnTerm>(&terminator);
  expect(instructions.size() == 4 && load.load() && add.binary() &&
             add.binary()->opcode == bir::BinaryOpcode::Add &&
             add.binary()->type == bir::Type{bir::TypeKind::I32} &&
             add.operands().size() == 2 && add.operands()[0] == load.results()[0] &&
             sext.cast() && sext.cast()->kind == bir::CastKind::SExt &&
             sext.cast()->from_type == bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
             sext.cast()->to_type == bir::Type{bir::TypeKind::Integer, 64, "i64"} &&
             sext.operands() == std::vector<bir::ValueId>{add.results()[0]} &&
             compare.compare() &&
             compare.compare()->predicate == bir::ComparePredicate::Slt &&
             compare.compare()->type == bir::Type{bir::TypeKind::I32} &&
             compare.operands().size() == 2 && compare.operands()[0] == load.results()[0] &&
             function.value(compare.operands()[1]).value().type == bir::Type{bir::TypeKind::I32} &&
             function.value(add.results()[0]).value().source_id ==
                 bir::SourceValueId{function_id, 33} &&
             function.value(sext.results()[0]).value().source_id ==
                 bir::SourceValueId{function_id, 34} &&
             function.value(compare.results()[0]).value().source_id ==
                 bir::SourceValueId{function_id, 35} && returned && returned->value &&
             *returned->value == sext.results()[0],
         "mixed dispatcher receipt must retain typed source-order and result-use authority");

  auto malformed_final_compare = mixed_accepted_row_dispatcher_module();
  auto& compare_authority = std::get<lir::LirCmpOp>(
      malformed_final_compare.functions[0].blocks[0].insts[3]);
  compare_authority.predicate = lir::LirCmpPredicate::Eq;
  const auto rejected = bir::lower_lir_to_raw_bir(malformed_final_compare);
  expect(!rejected.has_value() && rejected.error().code ==
             bir::ImportErrorCode::UnsupportedOrdinaryInstruction,
         "a malformed final admitted compare must reject the whole mixed module without Raw-BIR publication");
}

void test_selected_hoisted_alloca_authority_receipt_and_rejections() {
  lir::LirModule module;
  auto texts = std::make_shared<c4c::TextTable>();
  module.link_name_texts = texts;
  module.link_names.attach_text_table(texts.get());
  const auto owner = module.link_names.intern("typed_alloca_owner");
  lir::LirFunction function = void_definition("typed_alloca_owner", {
      return_block(0, "entry")});
  function.link_name_id = owner;
  function.alloca_insts.push_back(lir::LirAllocaOp{
      lir::LirOperand::ssa("%misleading_alloca", lir::LirValueId{41}),
      lir::LirTypeRef::integer(32), {}, 0,
      lir::LirCurrentFunctionLocalObjectPointer{
          lir::LirValueId{41}, lir::LirObjectId{7}, owner,
          lir::LirTypeRef(lir::LirBuiltinType::Pointer), lir::LirTypeRef::integer(32), true}});
  function.blocks[0].insts.push_back(lir::LirStoreOp{
      lir::LirTypeRef::integer(32), lir::LirOperand::integer("misleading", 7),
      lir::LirOperand::ssa("%misleading.store.pointer", lir::LirValueId{41}),
      lir::LirCurrentFunctionLocalObjectPointer{
          lir::LirValueId{41}, lir::LirObjectId{7}, owner,
          lir::LirTypeRef(lir::LirBuiltinType::Pointer), lir::LirTypeRef::integer(32), true}, true});
  module.functions.push_back(function);
  const auto& source_alloca = std::get<lir::LirAllocaOp>(module.functions[0].alloca_insts[0]);
  expect(source_alloca.count.str().empty() && source_alloca.result.value_id() &&
             source_alloca.local_object_authority &&
             source_alloca.local_object_authority->pointer_type.str() == "ptr" &&
             source_alloca.local_object_authority->pointee_type.str() == "i32" &&
             source_alloca.local_object_authority->owner == module.functions[0].link_name_id,
         "selected alloca test fixture must retain exact typed producer authority");

  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         std::string("one selected live hoisted alloca authority must publish verified Raw BIR: ") +
             (raw.has_value() ? "foundation verifier rejected it" : raw.error().detail));
  const auto view = raw.value().view();
  const auto function_view = view.function(view.functions()[0]).value();
  const auto entry_insts = function_view.instructions(function_view.blocks()[0]).value();
  const auto alloca = function_view.instruction(entry_insts[0]).value().alloca_authority();
  expect(alloca && alloca->result == bir::SourceValueId{function_view.id(), 41} &&
             alloca->pointer_definition == alloca->result &&
             alloca->object.owner == function_view.id() && alloca->object.value == 7 &&
             alloca->pointer_type == bir::Type{bir::TypeKind::Pointer} &&
             alloca->pointee_type == bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
             alloca->live,
         "Raw BIR alloca receipt must retain only typed result and local-object authority");
  const auto local_store = function_view.instruction(entry_insts[1]).value().local_store_authority();
  expect(local_store && local_store->pointer_definition == bir::SourceValueId{function_view.id(), 41} &&
             local_store->object.owner == function_view.id() && local_store->object.value == 7 &&
             local_store->stored_type == bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
             local_store->immediate == 7 && local_store->live,
         "Raw BIR local store receipt must retain typed immediate and local-object authority");

  auto invalid_store_immediate = module;
  std::get<lir::LirStoreOp>(invalid_store_immediate.functions[0].blocks[0].insts[0]).val =
      lir::LirOperand::raw("7");
  expect(!bir::lower_lir_to_raw_bir(invalid_store_immediate).has_value(),
         "raw selected local store value must reject transactionally");
  auto invalid_store_type = module;
  std::get<lir::LirStoreOp>(invalid_store_type.functions[0].blocks[0].insts[0]).type_str =
      lir::LirTypeRef::integer(64);
  expect(!bir::lower_lir_to_raw_bir(invalid_store_type).has_value(),
         "mismatched selected local store type must reject transactionally");

  auto missing_definition = module;
  std::get<lir::LirAllocaOp>(missing_definition.functions[0].alloca_insts[0])
      .local_object_authority->pointer_definition = lir::LirValueId::invalid();
  expect(!bir::lower_lir_to_raw_bir(missing_definition).has_value(),
         "missing alloca pointer definition authority must reject transactionally");
  auto invalid_object = module;
  std::get<lir::LirAllocaOp>(invalid_object.functions[0].alloca_insts[0])
      .local_object_authority->object = lir::LirObjectId::invalid();
  expect(!bir::lower_lir_to_raw_bir(invalid_object).has_value(),
         "invalid alloca object authority must reject transactionally");
  auto foreign_owner = module;
  const auto foreign = foreign_owner.link_names.intern("foreign_alloca_owner");
  std::get<lir::LirAllocaOp>(foreign_owner.functions[0].alloca_insts[0])
      .local_object_authority->owner = foreign;
  expect(!bir::lower_lir_to_raw_bir(foreign_owner).has_value(),
         "foreign alloca owner authority must reject transactionally");
  auto mismatched_pointee = module;
  std::get<lir::LirAllocaOp>(mismatched_pointee.functions[0].alloca_insts[0])
      .local_object_authority->pointee_type = lir::LirTypeRef::integer(64);
  expect(!bir::lower_lir_to_raw_bir(mismatched_pointee).has_value(),
         "mismatched alloca pointee authority must reject transactionally");
  auto dead = module;
  std::get<lir::LirAllocaOp>(dead.functions[0].alloca_insts[0])
      .local_object_authority->live = false;
  expect(!bir::lower_lir_to_raw_bir(dead).has_value(),
         "dead alloca authority must reject transactionally");
  auto repeated = module;
  repeated.functions[0].alloca_insts.push_back(repeated.functions[0].alloca_insts[0]);
  expect(!bir::lower_lir_to_raw_bir(repeated).has_value(),
         "repeated alloca authority record must reject transactionally");
}

lir::LirModule amd64_sysv_overflow_aggregate_memcpy_module() {
  lir::LirModule module;
  auto texts = std::make_shared<c4c::TextTable>();
  module.link_name_texts = texts;
  module.link_names.attach_text_table(texts.get());
  const auto owner = module.link_names.intern("amd64_overflow_aggregate_owner");
  const lir::LirTypeRef va_list_type("{ i32, i32, ptr, ptr }");
  const lir::LirTypeRef payload_type("{ i64, i64, i64 }");
  const lir::LirCurrentFunctionLocalObjectPointer va_list{
      lir::LirValueId{41}, lir::LirObjectId{7}, owner,
      lir::LirTypeRef(lir::LirBuiltinType::Pointer), va_list_type, true};
  const lir::LirCurrentFunctionLocalObjectPointer destination{
      lir::LirValueId{42}, lir::LirObjectId{8}, owner,
      lir::LirTypeRef(lir::LirBuiltinType::Pointer), payload_type, true};
  lir::LirBlock entry = return_block(0, "entry");
  entry.insts.push_back(lir::LirGepOp{
      lir::LirOperand::ssa("%overflow.field", lir::LirValueId{43}), va_list_type,
      lir::LirOperand::ssa("%va", lir::LirValueId{41}), false,
      {lir::LirGepIndex::typed(lir::LirTypeRef::integer(32), lir::LirOperand::integer("zero", 0)),
       lir::LirGepIndex::typed(lir::LirTypeRef::integer(32), lir::LirOperand::integer("two", 2))}});
  entry.insts.push_back(lir::LirLoadOp{
      lir::LirOperand::ssa("%overflow.source", lir::LirValueId{44}),
      lir::LirTypeRef(lir::LirBuiltinType::Pointer),
      lir::LirOperand::ssa("%overflow.field", lir::LirValueId{43})});
  lir::LirMemcpyOp copy{
      lir::LirOperand::ssa("%payload.destination", lir::LirValueId{42}),
      lir::LirOperand::ssa("%overflow.source", lir::LirValueId{44}),
      lir::LirOperand::integer("payload.bytes", 24), false};
  copy.requires_native_memory_va_authority = true;
  copy.amd64_sysv_overflow_aggregate_carrier =
      lir::LirAmd64SysVOverflowAggregateCarrier{
          va_list, lir::LirValueId{43}, lir::LirValueId{44},
          lir::LirAmd64SysVOverflowStorage::Amd64SysVOverflowArgArea,
          destination, lir::LirValueId{45}, payload_type,
          lir::LirTypeRef::integer(64), lir::LirIntegerImmediate{24}};
  entry.insts.push_back(std::move(copy));
  entry.insts.push_back(lir::LirLoadOp{
      lir::LirOperand::ssa("%payload.result", lir::LirValueId{45}), payload_type,
      lir::LirOperand::ssa("%payload.destination", lir::LirValueId{42})});
  lir::LirFunction function = void_definition("amd64_overflow_aggregate_owner", {entry});
  function.link_name_id = owner;
  function.alloca_insts.push_back(lir::LirAllocaOp{
      lir::LirOperand::ssa("%va", lir::LirValueId{41}), va_list_type, {}, 0, va_list});
  function.alloca_insts.push_back(lir::LirAllocaOp{
      lir::LirOperand::ssa("%payload.destination", lir::LirValueId{42}), payload_type, {}, 0,
      destination});
  module.functions.push_back(std::move(function));
  return module;
}

void test_amd64_sysv_overflow_aggregate_memcpy_receipt_and_rejections() {
  const auto module = amd64_sysv_overflow_aggregate_memcpy_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "selected AMD64 SysV aggregate overflow memcpy must publish verified Raw BIR");
  const auto view = raw.value().view();
  const auto function = view.function(view.functions()[0]).value();
  const auto instructions = function.instructions(function.blocks()[0]).value();
  const auto* copy = function.instruction(instructions[2]).value()
      .amd64_sysv_overflow_aggregate_memcpy();
  expect(copy && copy->va_list_pointer == bir::SourceValueId{function.id(), 41} &&
             copy->overflow_field_address == bir::SourceValueId{function.id(), 43} &&
             copy->overflow_pointer_load == bir::SourceValueId{function.id(), 44} &&
             copy->destination == bir::SourceValueId{function.id(), 42} &&
             copy->final_load == bir::SourceValueId{function.id(), 45} &&
             copy->payload_type.kind == bir::TypeKind::Struct && copy->size_bytes == 24,
         "Raw BIR receipt must retain the typed local and derived-overflow identities");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = module;
    auto& copy = std::get<lir::LirMemcpyOp>(candidate.functions[0].blocks[0].insts[2]);
    mutate(candidate, copy);
    expect(!bir::lower_lir_to_raw_bir(candidate).has_value(), message);
  };
  rejected([](auto&, auto& copy) { copy.requires_native_memory_va_authority = false; },
           "unselected overflow aggregate carrier must reject transactionally");
  rejected([](auto&, auto& copy) { copy.is_volatile = true; },
           "volatile overflow aggregate carrier must reject transactionally");
  rejected([](auto&, auto& copy) { copy.amd64_sysv_overflow_aggregate_carrier->overflow_pointer_load = lir::LirValueId{99}; },
           "non-derived overflow source must reject transactionally");
  rejected([](auto& candidate, auto& copy) {
             copy.amd64_sysv_overflow_aggregate_carrier->va_list_object.owner =
                 candidate.link_names.intern("foreign_overflow_owner");
           }, "foreign overflow local must reject transactionally");
  rejected([](auto&, auto& copy) { copy.amd64_sysv_overflow_aggregate_carrier->destination.live = false; },
           "dead overflow destination must reject transactionally");
  rejected([](auto&, auto& copy) { copy.dst = lir::LirOperand::ssa("%wrong", lir::LirValueId{41}); },
           "destination-disagreeing overflow memcpy must reject transactionally");
  rejected([](auto&, auto& copy) { copy.amd64_sysv_overflow_aggregate_carrier->payload_size = lir::LirIntegerImmediate{8}; },
           "size-disagreeing overflow carrier must reject transactionally");
}

void test_selected_local_scalar_load_authority_receipt_and_rejections() {
  lir::LirModule module;
  auto texts = std::make_shared<c4c::TextTable>();
  module.link_name_texts = texts;
  module.link_names.attach_text_table(texts.get());
  const auto owner = module.link_names.intern("typed_local_load_owner");
  lir::LirBlock entry = return_block(0, "entry");
  entry.insts.push_back(lir::LirLoadOp{
      lir::LirOperand::ssa("%misleading.local.result", lir::LirValueId{42}),
      lir::LirTypeRef::integer(32),
      lir::LirOperand::ssa("%misleading.local.pointer", lir::LirValueId{41}), true,
      lir::LirCurrentFunctionLocalObjectPointer{
          lir::LirValueId{41}, lir::LirObjectId{7}, owner,
          lir::LirTypeRef(lir::LirBuiltinType::Pointer), lir::LirTypeRef::integer(32), true}});
  entry.terminator = lir::LirRet{
      lir::LirOperand::ssa("%not-used-for-authority", lir::LirValueId{42}),
      lir::LirTypeRef::integer(32)};
  lir::LirFunction function = void_definition("typed_local_load_owner", {entry});
  function.return_type = scalar_type(c4c::TB_INT);
  function.signature_return_type_ref = lir::LirTypeRef::integer(32);
  function.link_name_id = owner;
  function.alloca_insts.push_back(lir::LirAllocaOp{
      lir::LirOperand::ssa("%misleading.alloca.pointer", lir::LirValueId{41}),
      lir::LirTypeRef::integer(32), {}, 0,
      lir::LirCurrentFunctionLocalObjectPointer{
          lir::LirValueId{41}, lir::LirObjectId{7}, owner,
          lir::LirTypeRef(lir::LirBuiltinType::Pointer), lir::LirTypeRef::integer(32), true}});
  module.functions.push_back(std::move(function));

  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "one selected native local-scalar load must publish verified Raw BIR");
  const auto view = raw.value().view();
  const auto function_view = view.function(view.functions()[0]).value();
  const auto instructions = function_view.instructions(function_view.blocks()[0]).value();
  const auto local_load = function_view.instruction(instructions[1]).value().local_load_authority();
  expect(local_load && local_load->result == bir::SourceValueId{function_view.id(), 42} &&
             local_load->pointer_definition == bir::SourceValueId{function_view.id(), 41} &&
             local_load->object.owner == function_view.id() && local_load->object.value == 7 &&
             local_load->pointer_type == bir::Type{bir::TypeKind::Pointer} &&
             local_load->loaded_type == bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
             local_load->live,
         "Raw BIR local load must preserve only typed result, pointer, and local-object authority");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = module;
    auto& load = std::get<lir::LirLoadOp>(candidate.functions[0].blocks[0].insts[0]);
    mutate(candidate, load);
    expect(!bir::lower_lir_to_raw_bir(candidate).has_value(), message);
  };
  rejected([](auto&, auto& load) { load.requires_native_result_authority = false; },
           "local load without native-result admission must reject transactionally");
  rejected([](auto&, auto& load) { load.result = lir::LirOperand::ssa("%bad", lir::LirValueId::invalid()); },
           "local load with invalid result must reject transactionally");
  rejected([](auto&, auto& load) { load.ptr = lir::LirOperand::ssa("%other", lir::LirValueId{99}); },
           "local load pointer must equal its authority definition");
  rejected([](auto& candidate, auto& load) {
             load.local_object_authority->owner = candidate.link_names.intern("foreign_local_load_owner");
           }, "foreign local load object owner must reject transactionally");
  rejected([](auto&, auto& load) { load.local_object_authority->object = lir::LirObjectId{8}; },
           "local load object must match its selected current-function pointer authority");
  rejected([](auto&, auto& load) { load.local_object_authority->pointee_type = lir::LirTypeRef::integer(64); },
           "local load pointee type must equal its load type");
  rejected([](auto&, auto& load) { load.local_object_authority->live = false; },
           "dead local load authority must reject transactionally");
}

void test_selected_vla_stack_save_receipt_and_rejections() {
  lir::LirModule module;
  auto texts = std::make_shared<c4c::TextTable>();
  module.link_name_texts = texts;
  module.link_names.attach_text_table(texts.get());
  const auto owner = module.link_names.intern("typed_vla_stack_save_owner");
  lir::LirBlock entry = return_block(0, "entry");
  entry.insts.push_back(lir::LirStackSaveOp{
      lir::LirOperand::ssa("%misleading.saved.pointer", lir::LirValueId{42}),
      lir::LirCurrentFunctionLocalObjectPointer{
          lir::LirValueId{42}, lir::LirObjectId{7}, owner,
          lir::LirTypeRef(lir::LirBuiltinType::Pointer),
          lir::LirTypeRef(lir::LirBuiltinType::Pointer), true}, true});
  entry.insts.push_back(lir::LirStackRestoreOp{
      lir::LirOperand::ssa("%misleading.saved.pointer", lir::LirValueId{42}),
      lir::LirCurrentFunctionLocalObjectPointer{
          lir::LirValueId{42}, lir::LirObjectId{7}, owner,
          lir::LirTypeRef(lir::LirBuiltinType::Pointer),
          lir::LirTypeRef(lir::LirBuiltinType::Pointer), true}, true,
      lir::LirStackRestoreOp::LirStackRestoreLifetimeTransition{
          lir::LirStackRestoreOp::LirStackRestoreLifetimeTransition::Kind::
              RestoreSavedVlaStackCheckpoint,
          lir::LirValueId{42}}});
  lir::LirFunction function = void_definition("typed_vla_stack_save_owner", {entry});
  function.link_name_id = owner;
  module.functions.push_back(std::move(function));

  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "one selected VLA stack save must publish verified Raw BIR");
  const auto view = raw.value().view();
  const auto function_view = view.function(view.functions()[0]).value();
  const auto instructions = function_view.instructions(function_view.blocks()[0]).value();
  const auto stack_save = function_view.instruction(instructions[0]).value().stack_save_authority();
  expect(stack_save && stack_save->result == bir::SourceValueId{function_view.id(), 42} &&
             stack_save->pointer_definition == bir::SourceValueId{function_view.id(), 42} &&
             stack_save->object.owner == function_view.id() && stack_save->object.value == 7 &&
             stack_save->pointer_type == bir::Type{bir::TypeKind::Pointer} &&
             stack_save->pointee_type == bir::Type{bir::TypeKind::Pointer} && stack_save->live,
         "Raw BIR VLA stack save must retain only typed result and live local-object authority");
  const auto stack_restore_authority =
      function_view.instruction(instructions[1]).value().stack_restore_authority();
  expect(stack_restore_authority &&
             stack_restore_authority->saved_pointer_definition == bir::SourceValueId{function_view.id(), 42} &&
             stack_restore_authority->object.owner == function_view.id() && stack_restore_authority->object.value == 7 &&
             stack_restore_authority->pointer_type == bir::Type{bir::TypeKind::Pointer} &&
             stack_restore_authority->pointee_type == bir::Type{bir::TypeKind::Pointer} && stack_restore_authority->live,
         "Raw BIR VLA stack restore must retain only typed saved-checkpoint authority");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = module;
    auto& stack_save = std::get<lir::LirStackSaveOp>(candidate.functions[0].blocks[0].insts[0]);
    mutate(candidate, stack_save);
    expect(!bir::lower_lir_to_raw_bir(candidate).has_value() &&
               !bir::lower_lir_to_canonical_bir(candidate).has_value(), message);
  };
  rejected([](auto&, auto& stack_save) { stack_save.requires_native_stack_save_authority = false; },
           "nonselected stack save must reject transactionally");
  rejected([](auto&, auto& stack_save) { stack_save.result = lir::LirOperand::ssa("%bad", lir::LirValueId::invalid()); },
           "stack save with invalid result must reject transactionally");
  rejected([](auto&, auto& stack_save) { stack_save.local_object_authority->pointer_definition = lir::LirValueId{99}; },
           "stack save result must equal its native pointer definition");
  rejected([](auto& candidate, auto& stack_save) {
             stack_save.local_object_authority->owner = candidate.link_names.intern("foreign_vla_stack_save_owner");
           }, "foreign VLA stack-save owner must reject transactionally");
  rejected([](auto&, auto& stack_save) { stack_save.local_object_authority->object = lir::LirObjectId::invalid(); },
           "malformed VLA stack-save object must reject transactionally");
  rejected([](auto&, auto& stack_save) { stack_save.local_object_authority->pointee_type = lir::LirTypeRef::integer(64); },
           "VLA stack-save pointee type must remain pointer-typed");
  rejected([](auto&, auto& stack_save) { stack_save.local_object_authority->live = false; },
           "dead VLA stack-save authority must reject transactionally");
  const auto rejected_restore = [&](auto mutate, const std::string& message) {
    auto candidate = module;
    auto& stack_restore =
        std::get<lir::LirStackRestoreOp>(candidate.functions[0].blocks[0].insts[1]);
    mutate(candidate, stack_restore);
    expect(!bir::lower_lir_to_raw_bir(candidate).has_value() &&
               !bir::lower_lir_to_canonical_bir(candidate).has_value(), message);
  };
  rejected_restore([](auto&, auto& stack_restore) {
    stack_restore.requires_native_stack_restore_authority = false;
  }, "unselected stack restore must reject transactionally");
  rejected_restore([](auto&, auto& stack_restore) {
    stack_restore.saved_ptr = lir::LirOperand::integer("not-an-ssa", 0);
  }, "non-SSA stack restore must reject transactionally");
  rejected_restore([](auto&, auto& stack_restore) {
    stack_restore.saved_ptr = lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  }, "invalid stack restore saved pointer must reject transactionally");
  rejected_restore([](auto&, auto& stack_restore) {
    stack_restore.saved_ptr = lir::LirOperand::ssa("%unbound", lir::LirValueId{99});
  }, "unbound stack restore saved pointer must reject transactionally");
  rejected_restore([](auto&, auto& stack_restore) {
    stack_restore.local_object_authority->pointer_definition = lir::LirValueId{99};
  }, "mismatched stack restore authority definition must reject transactionally");
  rejected_restore([](auto& candidate, auto& stack_restore) {
    stack_restore.local_object_authority->owner =
        candidate.link_names.intern("foreign_vla_stack_restore_owner");
  }, "foreign stack restore owner must reject transactionally");
  rejected_restore([](auto&, auto& stack_restore) {
    stack_restore.local_object_authority->object = lir::LirObjectId::invalid();
  }, "invalid stack restore object must reject transactionally");
  rejected_restore([](auto&, auto& stack_restore) {
    stack_restore.local_object_authority->pointer_type = lir::LirTypeRef::integer(64);
  }, "nonpointer stack restore authority type must reject transactionally");
  rejected_restore([](auto&, auto& stack_restore) {
    stack_restore.local_object_authority->pointee_type = lir::LirTypeRef::integer(64);
  }, "nonpointer stack restore pointee type must reject transactionally");
  rejected_restore([](auto&, auto& stack_restore) {
    stack_restore.local_object_authority->live = false;
  }, "dead stack restore authority must reject transactionally");
  rejected_restore([](auto&, auto& stack_restore) {
    stack_restore.lifetime_transition.reset();
  }, "stack restore without checkpoint transition must reject transactionally");
  rejected_restore([](auto&, auto& stack_restore) {
    stack_restore.lifetime_transition->kind =
        static_cast<lir::LirStackRestoreOp::LirStackRestoreLifetimeTransition::Kind>(99);
  }, "stack restore with invalid checkpoint transition kind must reject transactionally");
  rejected_restore([](auto&, auto& stack_restore) {
    stack_restore.lifetime_transition->saved_pointer_definition = lir::LirValueId{99};
  }, "stack restore with mismatched checkpoint transition must reject transactionally");
  auto two_saves = module;
  two_saves.functions[0].blocks[0].insts.push_back(lir::LirStackSaveOp{
      lir::LirOperand::ssa("%second", lir::LirValueId{43}),
      lir::LirCurrentFunctionLocalObjectPointer{
          lir::LirValueId{43}, lir::LirObjectId{8}, owner,
          lir::LirTypeRef(lir::LirBuiltinType::Pointer),
          lir::LirTypeRef(lir::LirBuiltinType::Pointer), true}, true});
  expect(!bir::lower_lir_to_raw_bir(two_saves).has_value(),
         "a second selected VLA stack save must reject transactionally");
  auto stack_restore = module;
  stack_restore.functions[0].blocks[0].insts.push_back(lir::LirStackRestoreOp{
      lir::LirOperand::ssa("%saved", lir::LirValueId{42}),
      lir::LirCurrentFunctionLocalObjectPointer{
          lir::LirValueId{42}, lir::LirObjectId{7}, owner,
          lir::LirTypeRef(lir::LirBuiltinType::Pointer),
          lir::LirTypeRef(lir::LirBuiltinType::Pointer), true}});
  expect(!bir::lower_lir_to_raw_bir(stack_restore).has_value(),
         "a second or unselected VLA stack restore must reject transactionally");
  auto dynamic_vla_alloca = module;
  dynamic_vla_alloca.functions[0].alloca_insts.push_back(lir::LirAllocaOp{
      lir::LirOperand::ssa("%dynamic.vla", lir::LirValueId{43}),
      lir::LirTypeRef::integer(32),
      lir::LirOperand::ssa("%vla.count", lir::LirValueId{99}), 0,
      lir::LirCurrentFunctionLocalObjectPointer{
          lir::LirValueId{43}, lir::LirObjectId{8}, owner,
          lir::LirTypeRef(lir::LirBuiltinType::Pointer),
          lir::LirTypeRef::integer(32), true}});
  expect(!bir::lower_lir_to_raw_bir(dynamic_vla_alloca).has_value(),
         "dynamic VLA allocation must remain transactionally rejected");
}

void test_selected_direct_static_local_array_gep_receipt_and_rejections() {
  lir::LirModule module;
  auto texts = std::make_shared<c4c::TextTable>();
  module.link_name_texts = texts;
  module.link_names.attach_text_table(texts.get());
  const auto owner = module.link_names.intern("typed_local_array_gep_owner");
  lir::LirBlock entry = return_block(0, "entry");
  entry.insts.push_back(lir::LirGepOp{
      lir::LirOperand::ssa("%misleading.local.array.result", lir::LirValueId{42}),
      lir::LirTypeRef::integer(32),
      lir::LirOperand::ssa("%misleading.local.array.base", lir::LirValueId{41}), true,
      {lir::LirGepIndex::typed(lir::LirTypeRef::integer(64),
                               lir::LirOperand::integer("misleading-index", 1))},
      true,
      lir::LirCurrentFunctionLocalObjectPointer{
          lir::LirValueId{41}, lir::LirObjectId{7}, owner,
          lir::LirTypeRef(lir::LirBuiltinType::Pointer), lir::LirTypeRef("[2 x i32]"), true,
          lir::LirTypeRef::integer(32)},
      true});
  lir::LirFunction function = void_definition("typed_local_array_gep_owner", {entry});
  function.link_name_id = owner;
  function.alloca_insts.push_back(lir::LirAllocaOp{
      lir::LirOperand::ssa("%misleading.local.array.alloca", lir::LirValueId{41}),
      lir::LirTypeRef("[2 x i32]"), {}, 0,
      lir::LirCurrentFunctionLocalObjectPointer{
          lir::LirValueId{41}, lir::LirObjectId{7}, owner,
          lir::LirTypeRef(lir::LirBuiltinType::Pointer), lir::LirTypeRef("[2 x i32]"), true}});
  module.functions.push_back(std::move(function));

  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "one selected direct static-local-array GEP must publish verified Raw BIR");
  const auto view = raw.value().view();
  const auto function_view = view.function(view.functions()[0]).value();
  const auto instructions = function_view.instructions(function_view.blocks()[0]).value();
  const auto local_gep = function_view.instruction(instructions[1]).value().local_array_gep_authority();
  expect(local_gep && local_gep->result == bir::SourceValueId{function_view.id(), 42} &&
             local_gep->pointer_definition == bir::SourceValueId{function_view.id(), 41} &&
             local_gep->object.owner == function_view.id() && local_gep->object.value == 7 &&
             local_gep->pointer_type == bir::Type{bir::TypeKind::Pointer} &&
             local_gep->pointee_type == bir::Type{bir::TypeKind::Array, 0, "[2 x i32]"} &&
             local_gep->element_type == bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
             local_gep->immediate_index == 1 && local_gep->live,
         "Raw BIR local-array GEP must retain typed result, base, i64 immediate, and local-object authority");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = module;
    auto& gep = std::get<lir::LirGepOp>(candidate.functions[0].blocks[0].insts[0]);
    mutate(candidate, gep);
    expect(!bir::lower_lir_to_raw_bir(candidate).has_value() &&
               !bir::lower_lir_to_canonical_bir(candidate).has_value(), message);
  };
  rejected([](auto&, auto& gep) { gep.requires_native_local_gep_authority = false; },
           "local-array GEP without native-local admission must reject transactionally");
  rejected([](auto&, auto& gep) { gep.local_object_authority.reset(); },
           "local-array GEP without local authority must reject transactionally");
  rejected([](auto&, auto& gep) { gep.result = lir::LirOperand::ssa("%bad", lir::LirValueId::invalid()); },
           "local-array GEP with invalid result must reject transactionally");
  rejected([](auto&, auto& gep) { gep.ptr = lir::LirOperand::ssa("%bad", lir::LirValueId{99}); },
           "local-array GEP base must equal its native pointer definition");
  rejected([](auto&, auto& gep) { gep.indices[0] = lir::LirGepIndex::raw("i64 1"); },
           "raw local-array GEP index must reject transactionally");
  rejected([](auto&, auto& gep) { gep.indices[0] = lir::LirGepIndex::typed(lir::LirTypeRef::integer(64), lir::LirOperand::ssa("%index", lir::LirValueId{41})); },
           "SSA local-array GEP index must reject transactionally");
  rejected([](auto&, auto& gep) { gep.indices[0] = lir::LirGepIndex::typed(lir::LirTypeRef::integer(32), lir::LirOperand::integer("1", 1)); },
           "non-i64 local-array GEP index must reject transactionally");
  rejected([](auto&, auto& gep) { gep.element_type = lir::LirTypeRef::integer(64); },
           "local-array GEP element type must match authority");
  rejected([](auto& candidate, auto& gep) { gep.local_object_authority->owner = candidate.link_names.intern("foreign_local_array_gep_owner"); },
           "foreign local-array GEP owner must reject transactionally");
  rejected([](auto&, auto& gep) { gep.local_object_authority->object = lir::LirObjectId{8}; },
           "incoherent local-array GEP object must reject transactionally");
  rejected([](auto&, auto& gep) { gep.local_object_authority->pointee_type = lir::LirTypeRef("[3 x i32]"); },
           "local-array GEP pointee authority must match its alloca");
  rejected([](auto&, auto& gep) { gep.local_object_authority->live = false; },
           "dead local-array GEP authority must reject transactionally");
}

void test_typed_phi_edge_authority_receipt_and_rejections() {
  lir::LirBlock left;
  left.id = lir::LirBlockId{1}; left.label = "left";
  left.insts.push_back(lir::LirConstInt{lir::LirValueId{11}, scalar_type(c4c::TB_INT), 7});
  left.terminator = lir::LirBr{"join", lir::LirBlockId{3}};
  lir::LirBlock right;
  right.id = lir::LirBlockId{2}; right.label = "right";
  right.insts.push_back(lir::LirConstInt{lir::LirValueId{12}, scalar_type(c4c::TB_INT), 9});
  right.terminator = lir::LirBr{"join", lir::LirBlockId{3}};
  lir::LirBlock join;
  join.id = lir::LirBlockId{3}; join.label = "join";
  join.insts.push_back(lir::LirPhiOp{lir::LirOperand::ssa("%misleading_phi", lir::LirValueId{13}), lir::LirTypeRef::integer(32), {
      {lir::LirOperand::ssa("%not-authoritative", lir::LirValueId{11}), "not-left", lir::LirBlockId{1}, lir::LirSuccessorOccurrenceId::direct_branch()},
      {lir::LirOperand::ssa("%not-authoritative", lir::LirValueId{12}), "not-right", lir::LirBlockId{2}, lir::LirSuccessorOccurrenceId::direct_branch()}}});
  join.terminator = lir::LirRet{lir::LirOperand::ssa("%also-misleading", lir::LirValueId{13}), lir::LirTypeRef::integer(32)};
  lir::LirFunction function; function.name = "typed_phi"; function.signature_text = "define i32 @typed_phi()";
  function.return_type = scalar_type(c4c::TB_INT);
  function.blocks = {left, right, join}; function.entry = lir::LirBlockId{1};
  lir::LirModule module; module.functions.push_back(function);
  auto verifier_module = module;
  verifier_module.functions[0].blocks[0].insts[0] = lir::LirBinOp{
      lir::LirOperand::ssa("%then.value", lir::LirValueId{11}), "sub",
      lir::LirTypeRef::integer(32), "0", "7"};
  verifier_module.functions[0].blocks[1].insts[0] = lir::LirBinOp{
      lir::LirOperand::ssa("%else.unary.minus", lir::LirValueId{12}), "sub",
      lir::LirTypeRef::integer(32), "0", "9"};
  auto& verifier_incoming = std::get<lir::LirPhiOp>(
      verifier_module.functions[0].blocks[2].insts[0]).incoming;
  verifier_incoming[0].label = "left";
  verifier_incoming[1].label = "right";
  lir::verify_module(verifier_module);
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value(), "typed PHI rows must publish without trusting presentation mirrors");
  const auto view = raw.value().view(); const auto function_view = view.function(view.functions()[0]).value();
  const auto instructions = function_view.instructions(function_view.blocks()[2]).value();
  const auto phi = function_view.instruction(instructions[0]).value();
  expect(phi.phi() && phi.phi()->incoming.size() == 2 && phi.phi()->incoming[0].edge.predecessor == function_view.blocks()[0] &&
             phi.phi()->incoming[1].edge.predecessor == function_view.blocks()[1] &&
             phi.phi()->incoming[0].edge.destination == function_view.blocks()[2],
         "Raw BIR PHI must retain ordered exact predecessor-to-destination edge authority");
  auto malformed = module;
  auto& incoming = std::get<lir::LirPhiOp>(malformed.functions[0].blocks[2].insts[0]).incoming[1];
  incoming.predecessor = lir::LirBlockId{1};
  const auto rejected = bir::lower_lir_to_raw_bir(malformed);
  expect(!rejected.has_value(), "duplicate PHI edge authority must reject transactionally");

  auto missing_value_authority = verifier_module;
  std::get<lir::LirPhiOp>(missing_value_authority.functions[0].blocks[2].insts[0])
      .incoming[1].value = lir::LirOperand("%display-only");
  try {
    lir::verify_module(missing_value_authority);
    fail("PHI value without native current-function authority must reject");
  } catch (const lir::LirVerifyError&) {
  }

  auto missing_occurrence = module;
  std::get<lir::LirPhiOp>(missing_occurrence.functions[0].blocks[2].insts[0])
      .incoming[0].successor_occurrence.reset();
  expect(!bir::lower_lir_to_raw_bir(missing_occurrence).has_value(),
         "a PHI row without typed occurrence authority must reject transactionally");

  auto special = module;
  auto& special_incoming = std::get<lir::LirPhiOp>(
      special.functions[0].blocks[2].insts[0]).incoming[0];
  special_incoming.value = lir::LirOperand::special_token(lir::LirSpecialToken::Undef);
  const auto special_raw = bir::lower_lir_to_raw_bir(special);
  expect(special_raw.has_value(),
         "an authorized PHI SpecialToken carrier must publish without text recovery");
  const auto special_function = special_raw.value().view().function(
      special_raw.value().view().functions()[0]).value();
  const auto special_phi = special_function.instruction(
      special_function.instructions(special_function.blocks()[2]).value()[0]).value();
  const auto special_value = special_function.value(special_phi.phi()->incoming[0].value).value();
  const auto* special_constant = std::get_if<bir::ConstantDef>(&special_value.definition);
  expect(special_constant && std::holds_alternative<bir::SpecialConstant>(
             special_raw.value().view().constant(special_constant->constant).value().payload),
         "Raw BIR must retain the PHI SpecialToken as a closed semantic constant");
  auto misleading_special = special;
  std::get<lir::LirPhiOp>(misleading_special.functions[0].blocks[2].insts[0])
      .incoming[0].value = lir::LirOperand::special_token(
          "not-undef", lir::LirSpecialToken::Undef);
  expect(bir::lower_lir_to_raw_bir(misleading_special).has_value(),
         "PHI SpecialToken materialization must use native authority, not its display mirror");
  auto invalid_special = special;
  std::get<lir::LirPhiOp>(invalid_special.functions[0].blocks[2].insts[0])
      .incoming[0].value = lir::LirOperand::special_token(
          "invalid", static_cast<lir::LirSpecialToken>(255));
  expect(!bir::lower_lir_to_raw_bir(invalid_special).has_value(),
         "an out-of-domain PHI SpecialToken carrier must reject transactionally");

  const auto parallel_phi_module = [](bool use_switch) {
    auto result = selected_global_i32_slt_compare_module();
    result.functions[0].return_type = scalar_type(c4c::TB_INT);
    result.functions[0].return_type.inner_rank = -1;
    result.functions[0].signature_return_type_ref = lir::LirTypeRef::integer(32);
    auto& predecessor = result.functions[0].blocks[0];
    if (use_switch) {
      predecessor.terminator = lir::LirSwitch{"%selector", "i32", "join", {{0, "join"}},
          lir::LirBlockId{1}, {lir::LirBlockId{1}}, lir::LirValueId{31}};
    } else {
      predecessor.terminator = lir::LirCondBr{"%condition", "join", "join",
          lir::LirBlockId{1}, lir::LirBlockId{1}, lir::LirValueId{33}};
    }
    lir::LirBlock join;
    join.id = lir::LirBlockId{1}; join.label = "join";
    join.insts.push_back(lir::LirPhiOp{lir::LirOperand::ssa("%phi", lir::LirValueId{34}),
        lir::LirTypeRef::integer(32), {{lir::LirOperand::ssa("%value", lir::LirValueId{31}),
        "pred", lir::LirBlockId{0}, use_switch ? lir::LirSuccessorOccurrenceId::switch_default()
                                                  : lir::LirSuccessorOccurrenceId::conditional_true()},
        {lir::LirOperand::ssa("%value", lir::LirValueId{31}), "pred", lir::LirBlockId{0},
        use_switch ? lir::LirSuccessorOccurrenceId::switch_case(0)
                   : lir::LirSuccessorOccurrenceId::conditional_false()}}});
    join.terminator = lir::LirRet{lir::LirOperand::integer("0", 0),
                                  lir::LirTypeRef::integer(32)};
    result.functions[0].blocks.push_back(join);
    lir::LirBlock other;
    other.id = lir::LirBlockId{2}; other.label = "other";
    other.terminator = lir::LirRet{lir::LirOperand::integer("0", 0),
                                   lir::LirTypeRef::integer(32)};
    result.functions[0].blocks.push_back(other);
    return result;
  };
  for (const bool use_switch : {false, true}) {
    const auto parallel = parallel_phi_module(use_switch);
    const auto parallel_raw = bir::lower_lir_to_raw_bir(parallel);
    expect(parallel_raw.has_value(), (use_switch
           ? "typed parallel switch PHI occurrences must publish Raw BIR: "
           : "typed parallel conditional PHI occurrences must publish Raw BIR: ") +
           (parallel_raw.has_value() ? "" : parallel_raw.error().detail));
    expect(bir::FoundationVerifier::verify(parallel_raw.value()).ok(), use_switch
           ? "typed parallel switch PHI occurrences must verify"
           : "typed parallel conditional PHI occurrences must verify");
    const auto parallel_function = parallel_raw.value().view().function(
        parallel_raw.value().view().functions()[0]).value();
    const auto parallel_phi = parallel_function.instruction(
        parallel_function.instructions(parallel_function.blocks()[1]).value()[0]).value().phi();
    expect(parallel_phi && parallel_phi->incoming.size() == 2 &&
               parallel_phi->incoming[0].edge.occurrence == 0 &&
               parallel_phi->incoming[1].edge.occurrence == 1,
           "parallel PHI receipt must preserve distinct selected edge occurrences in input order");
  }
  auto invalid_occurrence = parallel_phi_module(false);
  std::get<lir::LirPhiOp>(invalid_occurrence.functions[0].blocks[1].insts[0])
      .incoming[1].successor_occurrence = lir::LirSuccessorOccurrenceId{7};
  expect(!bir::lower_lir_to_raw_bir(invalid_occurrence).has_value(),
         "out-of-domain typed PHI occurrence authority must reject transactionally");
  auto incoherent_occurrence = parallel_phi_module(false);
  std::get<lir::LirCondBr>(incoherent_occurrence.functions[0].blocks[0].terminator)
      .false_successor = lir::LirBlockId{2};
  expect(!bir::lower_lir_to_raw_bir(incoherent_occurrence).has_value(),
         "typed PHI occurrence authority to a non-destination edge must reject transactionally");
  auto duplicate_occurrence = parallel_phi_module(true);
  std::get<lir::LirPhiOp>(duplicate_occurrence.functions[0].blocks[1].insts[0])
      .incoming[1].successor_occurrence = lir::LirSuccessorOccurrenceId::switch_default();
  expect(!bir::lower_lir_to_raw_bir(duplicate_occurrence).has_value(),
         "duplicate typed PHI occurrence authority must reject transactionally");
}

void test_typed_phi_loop_and_parallel_edge_occurrences() {
  lir::LirBlock entry;
  entry.id = lir::LirBlockId{1}; entry.label = "entry";
  entry.insts.push_back(lir::LirConstInt{lir::LirValueId{21}, scalar_type(c4c::TB_INT), 1});
  entry.terminator = lir::LirBr{"header", lir::LirBlockId{2}};
  lir::LirBlock header;
  header.id = lir::LirBlockId{2}; header.label = "header";
  header.insts.push_back(lir::LirPhiOp{lir::LirOperand::ssa("%phi", lir::LirValueId{23}), lir::LirTypeRef::integer(32), {
      {lir::LirOperand::ssa("%entry", lir::LirValueId{21}), "entry", lir::LirBlockId{1}, lir::LirSuccessorOccurrenceId::direct_branch()},
      {lir::LirOperand::ssa("%body", lir::LirValueId{22}), "body", lir::LirBlockId{3}, lir::LirSuccessorOccurrenceId::direct_branch()}}});
  header.terminator = lir::LirBr{"body", lir::LirBlockId{3}};
  lir::LirBlock body;
  body.id = lir::LirBlockId{3}; body.label = "body";
  body.insts.push_back(lir::LirConstInt{lir::LirValueId{22}, scalar_type(c4c::TB_INT), 2});
  body.terminator = lir::LirBr{"header", lir::LirBlockId{2}};
  lir::LirFunction loop; loop.name = "phi_loop"; loop.return_type = scalar_type(c4c::TB_INT);
  loop.blocks = {entry, header, body}; loop.entry = lir::LirBlockId{1};
  lir::LirModule loop_module; loop_module.functions.push_back(loop);
  const auto loop_raw = bir::lower_lir_to_raw_bir(loop_module);
  expect(loop_raw.has_value(), "loop PHI must bind the later backedge value by native authority");

}

void test_direct_pointer_body_parameter_gep_receipt_and_rejections() {
  lir::LirModule module;
  auto texts = std::make_shared<c4c::TextTable>();
  module.link_name_texts = texts;
  module.link_names.attach_text_table(texts.get());
  const auto owner = module.link_names.intern("direct_pointer_body_parameter_owner");
  auto pointer = scalar_type(c4c::TB_CHAR);
  pointer.ptr_level = 1;
  pointer.inner_rank = -1;
  lir::LirBlock entry = return_block(0, "entry");
  entry.insts.push_back(lir::LirGepOp{
      lir::LirOperand::ssa("%misleading.result", lir::LirValueId{42}),
      lir::LirTypeRef::integer(8),
      lir::LirOperand::ssa("%not-authoritative-spelling", lir::LirValueId{41}), false,
      {lir::LirGepIndex::typed(lir::LirTypeRef::integer(64),
                               lir::LirOperand::integer("misleading-zero", 0))}});
  lir::LirFunction function = void_definition("direct_pointer_body_parameter_owner", {entry});
  function.link_name_id = owner;
  function.params.emplace_back("%presentation-only", pointer);
  function.signature_params.push_back({"%presentation-only-signature", pointer, false});
  function.signature_param_type_refs.push_back(lir::LirTypeRef(lir::LirBuiltinType::Pointer));
  function.native_body_parameter_definitions.push_back(
      {lir::LirValueId{41}, 0, lir::LirTypeRef(lir::LirBuiltinType::Pointer), owner,
       lir::LirNativeBodyParameterAbi::DirectPointer});
  module.functions.push_back(std::move(function));

  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "selected direct pointer body parameter GEP must publish verified Raw BIR: " +
             (raw.has_value() ? "foundation verifier rejected it" : raw.error().detail));
  const auto view = raw.value().view();
  const auto function_view = view.function(view.functions()[0]).value();
  const auto instruction = function_view.instruction(
      function_view.instructions(function_view.blocks()[0]).value()[0]).value();
  const auto* gep = instruction.get_element_ptr();
  const auto* parameter = gep ? std::get_if<bir::DirectPointerBodyParameterGepBase>(
      &gep->base.authority) : nullptr;
  expect(parameter && parameter->source_value_id == 41 && parameter->parameter_index == 0 &&
             parameter->pointer_type == bir::Type{bir::TypeKind::Pointer} &&
             parameter->owner.valid(),
         "Raw BIR must retain only selected direct-pointer body-parameter authority");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = module;
    mutate(candidate);
    expect(!bir::lower_lir_to_raw_bir(candidate).has_value(), message);
  };
  rejected([](auto& candidate) { candidate.functions[0].native_body_parameter_definitions.clear(); },
           "missing body-parameter authority must reject transactionally");
  rejected([](auto& candidate) { candidate.functions[0].native_body_parameter_definitions[0].value = lir::LirValueId::invalid(); },
           "invalid body-parameter value must reject transactionally");
  rejected([](auto& candidate) { candidate.functions[0].native_body_parameter_definitions[0].owner = candidate.link_names.intern("foreign_parameter_owner"); },
           "foreign body-parameter owner must reject transactionally");
  rejected([](auto& candidate) { candidate.functions[0].native_body_parameter_definitions[0].type = lir::LirTypeRef::integer(64); },
           "type-incoherent body-parameter authority must reject transactionally");
  rejected([](auto& candidate) { candidate.functions[0].native_body_parameter_definitions[0].abi = lir::LirNativeBodyParameterAbi::Invalid; },
           "malformed direct-pointer ABI must reject transactionally");
  rejected([](auto& candidate) { candidate.functions[0].native_body_parameter_definitions.push_back(candidate.functions[0].native_body_parameter_definitions[0]); },
           "duplicate body-parameter authority must reject transactionally");
  rejected([](auto& candidate) { candidate.functions[0].blocks[0].insts.push_back(candidate.functions[0].blocks[0].insts[0]); },
           "duplicate selected body-parameter GEP receipt must reject transactionally");
  rejected([](auto& candidate) { candidate.functions[0].signature_params[0].is_byval = true; },
           "byval body parameter must remain rejected transactionally");
  rejected([](auto& candidate) { candidate.functions[0].signature_is_variadic = true; },
           "variadic body parameter must remain rejected transactionally");
}

void test_return_value_parameter_authority_receipt_and_rejections() {
  lir::LirModule module;
  auto texts = std::make_shared<c4c::TextTable>();
  module.link_name_texts = texts;
  module.link_names.attach_text_table(texts.get());
  const auto owner = module.link_names.intern("return_value_parameter_owner");
  lir::LirBlock entry = return_block(0, "entry");
  entry.terminator = lir::LirRet{
      lir::LirOperand::ssa("%presentation-only", lir::LirValueId{61}),
      lir::LirTypeRef::integer(32),
      lir::LirReturnValueParameterAuthority{
          lir::LirValueId{61}, 0, lir::LirTypeRef::integer(32), owner,
          lir::LirNativeBodyParameterAbi::DirectScalar,
          lir::LirReturnValueParameterRole::ReturnValue}};
  lir::LirFunction function;
  function.name = "return_value_parameter_owner";
  function.link_name_id = owner;
  function.return_type = scalar_type(c4c::TB_INT);
  function.signature_return_type_ref = lir::LirTypeRef::integer(32);
  function.params.emplace_back("%presentation-only", scalar_type(c4c::TB_INT));
  function.signature_params.push_back({"%presentation-only-signature",
                                       scalar_type(c4c::TB_INT), false});
  function.signature_param_type_refs.push_back(lir::LirTypeRef::integer(32));
  function.native_body_parameter_definitions.push_back(
      {lir::LirValueId{61}, 0, lir::LirTypeRef::integer(32), owner,
       lir::LirNativeBodyParameterAbi::DirectScalar});
  function.blocks.push_back(std::move(entry));
  function.entry = lir::LirBlockId{0};
  module.functions.push_back(std::move(function));

  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "selected return-value parameter authority must publish verified Raw BIR: " +
             (raw.has_value() ? "foundation verifier rejected it" : raw.error().detail));
  const auto view = raw.value().view();
  const auto function_view = view.function(view.functions()[0]).value();
  const auto terminator = function_view.terminator(function_view.blocks()[0]).value();
  const auto* returned = std::get_if<bir::ReturnTerm>(&terminator);
  const auto parameters = function_view.parameters();
  expect(returned && returned->value && parameters.size() == 1 &&
             *returned->value == parameters[0],
         "Raw BIR return must preserve the selected parameter ValueId exactly");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = module;
    mutate(candidate);
    expect(!bir::lower_lir_to_raw_bir(candidate).has_value(), message);
  };
  rejected([](auto& candidate) {
    std::get<lir::LirRet>(candidate.functions[0].blocks[0].terminator)
        .return_value_parameter_authority.reset();
  }, "missing selected return-value parameter authority must reject transactionally");
  rejected([](auto& candidate) {
    std::get<lir::LirRet>(candidate.functions[0].blocks[0].terminator)
        .return_value_parameter_authority->owner = candidate.link_names.intern("foreign_return_owner");
  }, "foreign return-value parameter owner must reject transactionally");
  rejected([](auto& candidate) {
    std::get<lir::LirRet>(candidate.functions[0].blocks[0].terminator)
        .return_value_parameter_authority->parameter_index = 1;
  }, "out-of-range return-value parameter index must reject transactionally");
  rejected([](auto& candidate) {
    std::get<lir::LirRet>(candidate.functions[0].blocks[0].terminator)
        .return_value_parameter_authority->type = lir::LirTypeRef::integer(64);
  }, "return-value parameter type mismatch must reject transactionally");
  rejected([](auto& candidate) {
    std::get<lir::LirRet>(candidate.functions[0].blocks[0].terminator)
        .return_value_parameter_authority->abi = lir::LirNativeBodyParameterAbi::Invalid;
  }, "return-value parameter ABI mismatch must reject transactionally");
  rejected([](auto& candidate) {
    std::get<lir::LirRet>(candidate.functions[0].blocks[0].terminator)
        .return_value_parameter_authority->role = lir::LirReturnValueParameterRole::Invalid;
  }, "return-value parameter role mismatch must reject transactionally");
  rejected([](auto& candidate) {
    std::get<lir::LirRet>(candidate.functions[0].blocks[0].terminator).value_str =
        lir::LirOperand::ssa("%different", lir::LirValueId{62});
  }, "return-value parameter operand mismatch must reject transactionally");
  rejected([](auto& candidate) {
    candidate.functions[0].signature_return_type_ref = lir::LirTypeRef::integer(64);
  }, "return-value parameter signature mismatch must reject transactionally");
  rejected([](auto& candidate) {
    candidate.functions[0].native_body_parameter_definitions.push_back(
        candidate.functions[0].native_body_parameter_definitions[0]);
  }, "duplicate native return-value parameter definition must reject transactionally");
  rejected([](auto& candidate) {
    auto duplicate = candidate.functions[0].blocks[0];
    duplicate.id = lir::LirBlockId{1};
    duplicate.label = "duplicate_return_authority";
    candidate.functions[0].blocks.push_back(std::move(duplicate));
  }, "duplicate return-value authority rows must reject transactionally");
}

void test_switch_selector_parameter_authority_receipt_and_rejections() {
  auto make_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    const auto owner = module.link_names.intern("switch_selector_parameter_owner");
    lir::LirFunction function;
    function.name = "switch_selector_parameter_owner";
    function.link_name_id = owner;
    function.return_type = scalar_type(c4c::TB_VOID);
    function.signature_return_type_ref = lir::LirTypeRef("void");
    function.params.emplace_back("%presentation-only", scalar_type(c4c::TB_INT));
    function.signature_params.push_back({"%presentation-only-signature",
                                         scalar_type(c4c::TB_INT), false});
    function.signature_param_type_refs.push_back(lir::LirTypeRef::integer(32));
    function.native_body_parameter_definitions.push_back(
        {lir::LirValueId{61}, 0, lir::LirTypeRef::integer(32), owner,
         lir::LirNativeBodyParameterAbi::DirectScalar});
    lir::LirBlock entry;
    entry.id = lir::LirBlockId{0};
    entry.label = "entry";
    entry.terminator = lir::LirSwitch{
        .selector_name = "%presentation-only",
        .selector_type = "i32",
        .default_label = "default",
        .cases = {{7, "case"}},
        .default_successor = lir::LirBlockId{1},
        .case_successors = {lir::LirBlockId{2}},
        .selector = lir::LirValueId{61},
        .selector_type_ref = lir::LirTypeRef::integer(32),
        .selector_parameter_authority = lir::LirSwitchSelectorParameterAuthority{
            lir::LirValueId{61}, 0, lir::LirTypeRef::integer(32), owner,
            lir::LirNativeBodyParameterAbi::DirectScalar,
            lir::LirSwitchSelectorParameterRole::SwitchSelector}};
    function.blocks.push_back(std::move(entry));
    function.blocks.push_back(return_block(1, "default"));
    function.blocks.push_back(return_block(2, "case"));
    function.entry = lir::LirBlockId{0};
    module.functions.push_back(std::move(function));
    return module;
  };

  const auto module = make_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "selected switch-selector parameter authority must publish verified Raw BIR");
  const auto view = raw.value().view();
  const auto function = view.function(view.functions()[0]).value();
  const auto terminator = function.terminator(function.blocks()[0]).value();
  const auto* sw = std::get_if<bir::SwitchTerm>(&terminator);
  expect(sw && function.parameters().size() == 1 && sw->selector == function.parameters()[0],
         "Raw BIR switch must use the selected parameter ValueId directly");

  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = make_module();
    mutate(candidate);
    const auto raw_rejected = bir::lower_lir_to_raw_bir(candidate);
    expect(!raw_rejected.has_value(), message + " (Raw rollback)");
    expect(!bir::lower_lir_to_canonical_bir(candidate).has_value(),
           message + " (Canonical rollback)");
  };
  rejected([](auto& candidate) {
    std::get<lir::LirSwitch>(candidate.functions[0].blocks[0].terminator)
        .selector_parameter_authority.reset();
  }, "missing switch-selector parameter authority must reject transactionally");
  rejected([](auto& candidate) {
    std::get<lir::LirSwitch>(candidate.functions[0].blocks[0].terminator)
        .selector_parameter_authority->value = lir::LirValueId::invalid();
  }, "invalid switch-selector parameter authority must reject transactionally");
  rejected([](auto& candidate) {
    candidate.functions[0].native_body_parameter_definitions.push_back(
        candidate.functions[0].native_body_parameter_definitions[0]);
  }, "duplicate switch-selector parameter authority must reject transactionally");
  rejected([](auto& candidate) {
    std::get<lir::LirSwitch>(candidate.functions[0].blocks[0].terminator)
        .selector_parameter_authority->owner = candidate.link_names.intern("foreign_switch_owner");
  }, "foreign switch-selector parameter owner must reject transactionally");
  rejected([](auto& candidate) {
    std::get<lir::LirSwitch>(candidate.functions[0].blocks[0].terminator)
        .selector_parameter_authority->parameter_index = 1;
  }, "out-of-range switch-selector parameter index must reject transactionally");
  rejected([](auto& candidate) {
    std::get<lir::LirSwitch>(candidate.functions[0].blocks[0].terminator)
        .selector_parameter_authority->type = lir::LirTypeRef::integer(64);
  }, "switch-selector parameter type mismatch must reject transactionally");
  rejected([](auto& candidate) {
    std::get<lir::LirSwitch>(candidate.functions[0].blocks[0].terminator)
        .selector_parameter_authority->abi = lir::LirNativeBodyParameterAbi::DirectPointer;
  }, "switch-selector parameter ABI mismatch must reject transactionally");
  rejected([](auto& candidate) {
    std::get<lir::LirSwitch>(candidate.functions[0].blocks[0].terminator)
        .selector_parameter_authority->role = lir::LirSwitchSelectorParameterRole::Invalid;
  }, "switch-selector parameter role mismatch must reject transactionally");
  rejected([](auto& candidate) {
    std::get<lir::LirSwitch>(candidate.functions[0].blocks[0].terminator).selector =
        lir::LirValueId{62};
  }, "switch-selector parameter value relation mismatch must reject transactionally");
  rejected([](auto& candidate) {
    std::get<lir::LirSwitch>(candidate.functions[0].blocks[0].terminator)
        .selector_type_ref = lir::LirTypeRef::integer(64);
  }, "switch-selector parameter type relation mismatch must reject transactionally");
}

void test_truthiness_comparison_lhs_parameter_authority_receipt_and_rejections() {
  const auto make_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    const auto owner = module.link_names.intern("truthiness_parameter_owner");
    lir::LirFunction function;
    function.name = "truthiness_parameter_owner";
    function.return_type = scalar_type(c4c::TB_VOID);
    function.signature_return_type_ref = lir::LirTypeRef("void");
    function.link_name_id = owner;
    function.params.emplace_back("%presentation-only", scalar_type(c4c::TB_INT));
    function.signature_params.push_back({"%presentation-only-signature", scalar_type(c4c::TB_INT), false});
    function.signature_param_type_refs.push_back(lir::LirTypeRef::integer(32));
    function.native_body_parameter_definitions.push_back(
        {lir::LirValueId{71}, 0, lir::LirTypeRef::integer(32), owner,
         lir::LirNativeBodyParameterAbi::DirectScalar});
    lir::LirBlock entry = return_block(0, "entry");
    entry.insts.push_back(lir::LirCmpOp{
        lir::LirOperand::ssa("%truthiness-result", lir::LirValueId{72}), false,
        lir::LirCmpPredicate::Ne, lir::LirTypeRef::integer(32),
        lir::LirOperand::ssa("%presentation-only-lhs", lir::LirValueId{71}),
        lir::LirOperand::integer("presentation-only-zero", 0),
        lir::LirTruthinessComparisonLhsParameterAuthority{
            lir::LirValueId{71}, 0, lir::LirTypeRef::integer(32), owner,
            lir::LirNativeBodyParameterAbi::DirectScalar,
            lir::LirTruthinessComparisonLhsParameterRole::TruthinessComparisonLhs}});
    function.blocks.push_back(std::move(entry));
    function.entry = lir::LirBlockId{0};
    module.functions.push_back(std::move(function));
    return module;
  };
  const auto module = make_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "truthiness parameter authority must publish verified Raw BIR");
  const auto view = raw.value().view();
  const auto function = view.function(view.functions()[0]).value();
  const auto compare = function.instruction(function.instructions(function.blocks()[0]).value()[0]).value();
  expect(compare.compare() && compare.compare()->predicate == bir::ComparePredicate::Ne &&
             compare.compare()->type == bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
             compare.compare()->direct_scalar_truthiness_lhs &&
             compare.compare()->direct_scalar_truthiness_lhs->source_value_id == 71 &&
             compare.compare()->direct_scalar_truthiness_lhs->parameter_index == 0 &&
             compare.compare()->direct_scalar_truthiness_lhs->scalar_type ==
                 bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
             compare.operands().size() == 2 && compare.operands()[0] == function.parameters()[0],
         "Raw BIR must retain the exact typed truthiness comparison-LHS parameter authority");
  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = make_module();
    mutate(candidate);
    expect(!bir::lower_lir_to_raw_bir(candidate).has_value(), message + " (Raw rollback)");
    expect(!bir::lower_lir_to_canonical_bir(candidate).has_value(), message + " (Canonical rollback)");
  };
  rejected([](auto& candidate) { std::get<lir::LirCmpOp>(candidate.functions[0].blocks[0].insts[0]).truthiness_lhs_parameter_authority.reset(); }, "missing truthiness authority must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCmpOp>(candidate.functions[0].blocks[0].insts[0]).truthiness_lhs_parameter_authority->value = lir::LirValueId::invalid(); }, "invalid truthiness authority must reject transactionally");
  rejected([](auto& candidate) { candidate.functions[0].native_body_parameter_definitions.push_back(candidate.functions[0].native_body_parameter_definitions[0]); }, "duplicate truthiness definition must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCmpOp>(candidate.functions[0].blocks[0].insts[0]).truthiness_lhs_parameter_authority->owner = candidate.link_names.intern("foreign_truthiness_owner"); }, "foreign truthiness owner must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCmpOp>(candidate.functions[0].blocks[0].insts[0]).truthiness_lhs_parameter_authority->parameter_index = 1; }, "truthiness parameter index mismatch must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCmpOp>(candidate.functions[0].blocks[0].insts[0]).truthiness_lhs_parameter_authority->type = lir::LirTypeRef::integer(64); }, "truthiness parameter type mismatch must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCmpOp>(candidate.functions[0].blocks[0].insts[0]).truthiness_lhs_parameter_authority->abi = lir::LirNativeBodyParameterAbi::DirectPointer; }, "truthiness parameter ABI mismatch must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCmpOp>(candidate.functions[0].blocks[0].insts[0]).truthiness_lhs_parameter_authority->role = lir::LirTruthinessComparisonLhsParameterRole::Invalid; }, "truthiness parameter role mismatch must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCmpOp>(candidate.functions[0].blocks[0].insts[0]).lhs = lir::LirOperand::ssa("%other", lir::LirValueId{73}); }, "truthiness LHS relation mismatch must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCmpOp>(candidate.functions[0].blocks[0].insts[0]).predicate = lir::LirCmpPredicate::Eq; }, "truthiness predicate mismatch must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCmpOp>(candidate.functions[0].blocks[0].insts[0]).is_float = true; }, "floating truthiness comparison must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCmpOp>(candidate.functions[0].blocks[0].insts[0]).rhs = lir::LirOperand::integer("one", 1); }, "truthiness zero RHS mismatch must reject transactionally");
  rejected([](auto& candidate) { candidate.functions[0].blocks[0].insts.push_back(candidate.functions[0].blocks[0].insts[0]); }, "second truthiness authority row must reject transactionally");
}

void test_fixed_direct_call_argument0_parameter_authority_receipt_and_rejections() {
  const auto make_module = [] {
    lir::LirModule module;
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
    const auto caller_owner = module.link_names.intern("fixed_direct_call_parameter_owner");
    const auto callee_owner = module.link_names.intern("fixed_direct_call_target");
    lir::LirFunction callee;
    callee.name = "fixed_direct_call_target";
    callee.link_name_id = callee_owner;
    callee.return_type = scalar_type(c4c::TB_INT);
    callee.signature_return_type_ref = lir::LirTypeRef::integer(32);
    callee.params.emplace_back("%target-param", scalar_type(c4c::TB_INT));
    callee.signature_params.push_back({"%target-param", scalar_type(c4c::TB_INT), false});
    callee.signature_param_type_refs.push_back(lir::LirTypeRef::integer(32));
    callee.is_declaration = true;
    lir::LirFunction caller;
    caller.name = "fixed_direct_call_parameter_owner";
    caller.link_name_id = caller_owner;
    caller.return_type = scalar_type(c4c::TB_VOID);
    caller.signature_return_type_ref = lir::LirTypeRef("void");
    caller.params.emplace_back("%presentation-only", scalar_type(c4c::TB_INT));
    caller.signature_params.push_back({"%presentation-only", scalar_type(c4c::TB_INT), false});
    caller.signature_param_type_refs.push_back(lir::LirTypeRef::integer(32));
    caller.native_body_parameter_definitions.push_back(
        {lir::LirValueId{81}, 0, lir::LirTypeRef::integer(32), caller_owner,
         lir::LirNativeBodyParameterAbi::DirectScalar});
    lir::LirBlock entry = return_block(0, "entry");
    lir::LirCallOp call;
    call.result = lir::LirOperand::ssa("%presentation-result", lir::LirValueId{82});
    call.return_type = lir::LirTypeRef::integer(32);
    call.callee = lir::LirOperand::global("@presentation-target", callee_owner);
    call.direct_callee_link_name_id = callee_owner;
    call.arg_type_refs = {lir::LirTypeRef::integer(32)};
    call.callee_signature = lir::LirCallSignature{
        lir::LirTypeRef::integer(32), lir::LirExtAttr::None, {"i32"},
        {lir::LirTypeRef::integer(32)}, false, false, false};
    call.structured_args.push_back({"i32",
        lir::LirOperand::ssa("%presentation-argument", lir::LirValueId{81}),
        lir::LirTypeRef::integer(32)});
    call.structured_args[0].fixed_direct_call_argument_parameter_authority =
        lir::LirFixedDirectCallArgumentParameterAuthority{
            lir::LirValueId{81}, 0, lir::LirTypeRef::integer(32), caller_owner,
            lir::LirNativeBodyParameterAbi::DirectScalar,
            lir::LirFixedDirectCallArgumentParameterRole::FixedDirectCallArgument0};
    entry.insts.push_back(std::move(call));
    caller.blocks.push_back(std::move(entry));
    caller.entry = lir::LirBlockId{0};
    module.functions = {std::move(callee), std::move(caller)};
    return module;
  };
  const auto module = make_module();
  const auto raw = bir::lower_lir_to_raw_bir(module);
  expect(raw.has_value() && bir::FoundationVerifier::verify(raw.value()).ok(),
         "fixed direct-call parameter authority must publish verified Raw BIR: " +
             (raw.has_value() ? "foundation verifier rejected it" : raw.error().detail));
  const auto view = raw.value().view();
  const auto function = view.function(view.functions()[1]).value();
  const auto call = function.instruction(function.instructions(function.blocks()[0]).value()[0]).value();
  expect(call.call() && call.call()->direct_scalar_argument0 &&
             call.call()->direct_scalar_argument0->source_value_id == 81 &&
             call.call()->direct_scalar_argument0->parameter_index == 0 &&
             call.call()->direct_scalar_argument0->scalar_type ==
                 bir::Type{bir::TypeKind::Integer, 32, "i32"} &&
             call.operands().size() == 1 && call.operands()[0] == function.parameters()[0],
         "Raw BIR must retain the exact typed fixed direct-call argument-0 authority");
  const auto rejected = [&](auto mutate, const std::string& message) {
    auto candidate = make_module();
    mutate(candidate);
    expect(!bir::lower_lir_to_raw_bir(candidate).has_value(), message + " (Raw rollback)");
    expect(!bir::lower_lir_to_canonical_bir(candidate).has_value(), message + " (Canonical rollback)");
  };
  rejected([](auto& candidate) { std::get<lir::LirCallOp>(candidate.functions[1].blocks[0].insts[0]).structured_args[0].fixed_direct_call_argument_parameter_authority.reset(); }, "missing fixed direct-call authority must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCallOp>(candidate.functions[1].blocks[0].insts[0]).structured_args[0].fixed_direct_call_argument_parameter_authority->value = lir::LirValueId::invalid(); }, "invalid fixed direct-call authority must reject transactionally");
  rejected([](auto& candidate) { candidate.functions[1].native_body_parameter_definitions.push_back(candidate.functions[1].native_body_parameter_definitions[0]); }, "duplicate fixed direct-call definition must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCallOp>(candidate.functions[1].blocks[0].insts[0]).structured_args[0].fixed_direct_call_argument_parameter_authority->owner = candidate.link_names.intern("foreign_fixed_direct_call_owner"); }, "foreign fixed direct-call owner must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCallOp>(candidate.functions[1].blocks[0].insts[0]).structured_args[0].fixed_direct_call_argument_parameter_authority->parameter_index = 1; }, "fixed direct-call parameter index mismatch must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCallOp>(candidate.functions[1].blocks[0].insts[0]).structured_args[0].fixed_direct_call_argument_parameter_authority->type = lir::LirTypeRef::integer(64); }, "fixed direct-call parameter type mismatch must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCallOp>(candidate.functions[1].blocks[0].insts[0]).structured_args[0].fixed_direct_call_argument_parameter_authority->abi = lir::LirNativeBodyParameterAbi::DirectPointer; }, "fixed direct-call ABI mismatch must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCallOp>(candidate.functions[1].blocks[0].insts[0]).structured_args[0].fixed_direct_call_argument_parameter_authority->role = lir::LirFixedDirectCallArgumentParameterRole::Invalid; }, "fixed direct-call role mismatch must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCallOp>(candidate.functions[1].blocks[0].insts[0]).structured_args[0].operand = lir::LirOperand::ssa("%other", lir::LirValueId{83}); }, "fixed direct-call argument value mismatch must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCallOp>(candidate.functions[1].blocks[0].insts[0]).structured_args[0].type_ref = lir::LirTypeRef::integer(64); }, "fixed direct-call argument type mismatch must reject transactionally");
  rejected([](auto& candidate) { std::get<lir::LirCallOp>(candidate.functions[1].blocks[0].insts[0]).callee_signature->fixed_param_type_refs[0] = lir::LirTypeRef::integer(64); }, "fixed callee parameter mismatch must reject transactionally");
}

}  // namespace

int main() {
  test_structured_gep_index_adapter();
  test_void_return_rejects_authoritative_value();
  test_supported_import_and_views();
  test_direct_branch_successor_receipt_and_rejections();
  test_generic_inline_asm_ssa_edges();
  test_structured_lir_import_ssa_chain();
  test_structured_lir_import_rejections();
  test_lir_inline_asm_structured_value_contract();
  test_closed_typed_lir_type_receipt();
  test_i32_inline_asm_output_store_receipt_and_rejections();
  test_i64_inline_asm_output_store_receipt_and_rejections();
  test_structured_type_spec_signature_receipt();
  test_direct_scalar_signature_receipt();
  test_direct_scalar_signature_rejections_and_transactionality();
  test_plain_parameter_signature_receipt();
  test_plain_parameter_signature_rejections_and_transactionality();
  test_function_metadata_builder_contract();
  test_function_metadata_import_receipt();
  test_function_metadata_import_rejections_and_transactionality();
  test_typed_lir_type_rejections();
  test_anonymous_aggregate_layout_verifier_rejections();
  test_verifier_rejects_malformed_raw_type();
  test_intrinsic_requirements_receipt();
  test_module_name_and_struct_declaration_receipt();
  test_module_name_and_struct_declaration_rejections();
  test_structured_constant_value_receipt();
  test_constant_value_rejections_and_forward_use();
  test_direct_global_integer_store_receipt();
  test_direct_global_integer_store_builder_contract();
  test_direct_global_integer_store_rejections();
  test_direct_global_integer_load_receipt();
  test_direct_global_integer_load_builder_contract();
  test_direct_global_integer_load_rejections();
  test_selected_global_array_gep_receipt();
  test_indirect_branch_terminator_receipt_and_rejections();
  test_direct_label_address_constant_receipt_and_rejections();
  test_typed_computed_goto_receipt_and_rejections();
  test_switch_selector_type_authority_and_display_mirror();
  test_switch_terminator_receipt_and_rejections();
  test_conditional_branch_terminator_receipt_and_rejections();
  test_selected_global_array_gep_ssa_index_receipt();
  test_selected_global_array_gep_builder_contract();
  test_label_address_gep_base_builder_contract();
  test_selected_global_array_gep_rejections();
  test_selected_vla_stack_save_receipt_and_rejections();
  test_selected_direct_static_local_array_gep_receipt_and_rejections();
  test_scalar_integer_return_receipt();
  test_scalar_integer_ssa_return_receipt();
  test_scalar_integer_return_builder_contract();
  test_scalar_integer_return_rejections();
  test_string_pool_receipt_and_views();
  test_string_pool_rejections_and_transactionality();
  test_external_declaration_receipt_and_views();
  test_external_declaration_rejections_and_transactionality();
  test_global_object_receipt_and_views();
  test_scalar_global_type_authority_without_mirror();
  test_enum_storage_global_receipt_and_rejections();
  test_vrm_register_global_receipt_and_rejections();
  test_complex_storage_global_receipt_and_rejections();
  test_scalar_pointer_global_receipt_and_rejections();
  test_pointer_to_array_global_receipt_and_rejections();
  test_mixed_pointer_array_global_receipt_and_rejections();
  test_fixed_scalar_base_array_global_receipt_and_rejections();
  test_direct_vector_global_receipt_and_rejections();
  test_function_pointer_global_receipt_and_rejections();
  test_va_list_global_receipt_and_rejections();
  test_named_aggregate_global_receipt_and_rejections();
  test_aggregate_store_backed_structured_receiver();
  test_flexible_member_literal_struct_global_receipt_and_rejections();
  test_global_object_rejections_and_transactionality();
  test_specialization_metadata_receipt_and_rejections();
  test_accumulated_module_surface_checkpoint();
  test_inline_asm_shape_rejection();
  test_direct_zero_argument_void_call_receipt();
  test_direct_zero_argument_void_call_builder_contract();
  test_native_floating_call_result_builder_contract();
  test_direct_zero_argument_void_call_rejections();
  test_direct_integer_call_receipt_and_rejections();
  test_direct_native_floating_call_receipt_and_rejections();
  test_downstream_double_fadd_receipt_and_rejections();
  test_downstream_double_olt_compare_receipt_and_rejections();
  test_scalar_double_to_float_fptrunc_receipt_and_rejections();
  test_scalar_float_to_double_fpext_receipt_and_rejections();
  test_scalar_signed_i32_to_double_sitofp_receipt_and_rejections();
  test_scalar_unsigned_i32_to_double_uitofp_receipt_and_rejections();
  test_scalar_double_to_signed_i32_fptosi_receipt_and_rejections();
  test_scalar_double_to_unsigned_i32_fptoui_receipt_and_rejections();
  test_wide_ffs_select_trunc_receipt_and_rejections();
  test_normalized_i32_add_receipt_and_rejections();
  test_normalized_i32_mul_receipt_and_rejections();
  test_native_intrinsic_receipt_and_rejections();
  test_builtin_ffs_cttz_add_one_receipt_and_rejections();
  test_builtin_ctz_call_narrow_final_use_receipt_and_rejections();
  test_builtin_clz_call_narrow_final_use_receipt_and_rejections();
  test_builtin_popcount_call_narrow_final_use_receipt_and_rejections();
  test_builtin_ffs_add_select_false_arm_receipt_and_rejections();
  test_native_intrinsic_i64_trunc_receipt_and_rejections();
  test_scalar_i32_to_i64_sext_receipt_and_rejections();
  test_selected_global_i32_slt_compare_receipt_and_rejections();
  test_selected_global_i32_abs_receipt_and_rejections();
  test_mixed_accepted_row_dispatcher_transactionality();
  test_selected_hoisted_alloca_authority_receipt_and_rejections();
  test_amd64_sysv_overflow_aggregate_memcpy_receipt_and_rejections();
  test_selected_local_scalar_load_authority_receipt_and_rejections();
  test_typed_phi_edge_authority_receipt_and_rejections();
  test_typed_phi_loop_and_parallel_edge_occurrences();
  test_direct_pointer_body_parameter_gep_receipt_and_rejections();
  test_return_value_parameter_authority_receipt_and_rejections();
  test_switch_selector_parameter_authority_receipt_and_rejections();
  test_truthiness_comparison_lhs_parameter_authority_receipt_and_rejections();
  test_fixed_direct_call_argument0_parameter_authority_receipt_and_rejections();
  return 0;
}
