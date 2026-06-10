#include "bir.hpp"

#include <utility>

namespace c4c::backend::bir {

Value Value::immediate_i1(bool value) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::I1;
  result.immediate = value ? 1 : 0;
  result.immediate_bits = value ? 1u : 0u;
  return result;
}

Value Value::immediate_i8(std::int8_t value) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::I8;
  result.immediate = value;
  return result;
}

Value Value::immediate_i16(std::int16_t value) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::I16;
  result.immediate = value;
  return result;
}

Value Value::immediate_i32(std::int32_t value) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::I32;
  result.immediate = value;
  return result;
}

Value Value::immediate_i64(std::int64_t value) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::I64;
  result.immediate = value;
  return result;
}

Value Value::immediate_f32_bits(std::uint32_t bits) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::F32;
  result.immediate = static_cast<std::int64_t>(bits);
  result.immediate_bits = bits;
  return result;
}

Value Value::immediate_f64_bits(std::uint64_t bits) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::F64;
  result.immediate = static_cast<std::int64_t>(bits);
  result.immediate_bits = bits;
  return result;
}

Value Value::immediate_f128_bits(std::uint64_t low_bits, std::uint64_t high_bits) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::F128;
  result.immediate = static_cast<std::int64_t>(low_bits);
  result.immediate_bits = low_bits;
  result.f128_payload = F128Payload{
      .low_bits = low_bits,
      .high_bits = high_bits,
  };
  return result;
}

Value Value::named(TypeKind type, std::string value_name) {
  Value result;
  result.kind = Kind::Named;
  result.type = type;
  result.name = std::move(value_name);
  return result;
}

Value Value::named_symbol_pointer(std::string value_name, LinkNameId link_name_id) {
  Value result = named(TypeKind::Ptr, std::move(value_name));
  result.pointer_symbol_link_name_id = link_name_id;
  return result;
}

const StructuredTypeDeclSpelling* StructuredTypeSpellingContext::find_struct_decl(
    std::string_view name) const {
  for (const auto& declaration : declarations) {
    if (declaration.name == name) {
      return &declaration;
    }
  }
  return nullptr;
}

std::string render_type(TypeKind type) {
  switch (type) {
    case TypeKind::Void:
      return "void";
    case TypeKind::I1:
      return "i1";
    case TypeKind::I8:
      return "i8";
    case TypeKind::I16:
      return "i16";
    case TypeKind::I32:
      return "i32";
    case TypeKind::I64:
      return "i64";
    case TypeKind::Ptr:
      return "ptr";
    case TypeKind::F32:
      return "float";
    case TypeKind::F64:
      return "double";
    case TypeKind::F128:
      return "f128";
  }
  return "<unknown>";
}

std::string render_binary_opcode(BinaryOpcode opcode) {
  switch (opcode) {
    case BinaryOpcode::Add:
      return "add";
    case BinaryOpcode::Sub:
      return "sub";
    case BinaryOpcode::Mul:
      return "mul";
    case BinaryOpcode::And:
      return "and";
    case BinaryOpcode::Or:
      return "or";
    case BinaryOpcode::Xor:
      return "xor";
    case BinaryOpcode::Shl:
      return "shl";
    case BinaryOpcode::LShr:
      return "lshr";
    case BinaryOpcode::AShr:
      return "ashr";
    case BinaryOpcode::SDiv:
      return "sdiv";
    case BinaryOpcode::UDiv:
      return "udiv";
    case BinaryOpcode::SRem:
      return "srem";
    case BinaryOpcode::URem:
      return "urem";
    case BinaryOpcode::Eq:
      return "eq";
    case BinaryOpcode::Ne:
      return "ne";
    case BinaryOpcode::Slt:
      return "slt";
    case BinaryOpcode::Sle:
      return "sle";
    case BinaryOpcode::Sgt:
      return "sgt";
    case BinaryOpcode::Sge:
      return "sge";
    case BinaryOpcode::Ult:
      return "ult";
    case BinaryOpcode::Ule:
      return "ule";
    case BinaryOpcode::Ugt:
      return "ugt";
    case BinaryOpcode::Uge:
      return "uge";
  }
  return "<unknown>";
}

std::string render_cast_opcode(CastOpcode opcode) {
  switch (opcode) {
    case CastOpcode::SExt:
      return "sext";
    case CastOpcode::ZExt:
      return "zext";
    case CastOpcode::Trunc:
      return "trunc";
    case CastOpcode::FPTrunc:
      return "fptrunc";
    case CastOpcode::FPExt:
      return "fpext";
    case CastOpcode::FPToSI:
      return "fptosi";
    case CastOpcode::FPToUI:
      return "fptoui";
    case CastOpcode::SIToFP:
      return "sitofp";
    case CastOpcode::UIToFP:
      return "uitofp";
    case CastOpcode::PtrToInt:
      return "ptrtoint";
    case CastOpcode::IntToPtr:
      return "inttoptr";
    case CastOpcode::Bitcast:
      return "bitcast";
  }
  return "<unknown>";
}

bool call_argument_binary_source_producer_opcode_is_materializable(
    BinaryOpcode opcode) {
  switch (opcode) {
    case BinaryOpcode::Add:
    case BinaryOpcode::Sub:
    case BinaryOpcode::And:
    case BinaryOpcode::Or:
    case BinaryOpcode::Xor:
    case BinaryOpcode::Mul:
    case BinaryOpcode::SDiv:
    case BinaryOpcode::SRem:
      return true;
    case BinaryOpcode::UDiv:
    case BinaryOpcode::URem:
    case BinaryOpcode::Shl:
    case BinaryOpcode::LShr:
    case BinaryOpcode::AShr:
    case BinaryOpcode::Eq:
    case BinaryOpcode::Ne:
    case BinaryOpcode::Slt:
    case BinaryOpcode::Sle:
    case BinaryOpcode::Sgt:
    case BinaryOpcode::Sge:
    case BinaryOpcode::Ult:
    case BinaryOpcode::Ule:
    case BinaryOpcode::Ugt:
    case BinaryOpcode::Uge:
      return false;
  }
  return false;
}

const CallArgumentSourceRelationship* find_call_argument_source_relationship(
    const CallInst& call,
    std::size_t arg_index) {
  if (arg_index >= call.args.size()) {
    return nullptr;
  }

  const CallArgumentSourceRelationship* result = nullptr;
  for (const auto& relationship : call.arg_sources) {
    if (relationship.arg_index != arg_index) {
      continue;
    }
    if (result != nullptr) {
      return nullptr;
    }
    result = &relationship;
  }
  return result;
}

const CallInst* indexed_call_inst(const Block& block,
                                  const CallInst& call,
                                  std::size_t call_instruction_index) {
  if (call_instruction_index >= block.insts.size()) {
    return nullptr;
  }
  const auto* block_call =
      std::get_if<CallInst>(&block.insts[call_instruction_index]);
  return block_call == &call ? block_call : nullptr;
}

CallArgumentSourceProducerMaterialization
find_call_argument_source_producer_materialization(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index,
    std::size_t arg_index) {
  if (call_instruction_index >= block.insts.size() ||
      arg_index >= call.args.size() ||
      find_call_argument_source_relationship(call, arg_index) == nullptr) {
    return {};
  }
  if (indexed_call_inst(block, call, call_instruction_index) == nullptr) {
    return {};
  }

  const auto& source_value = call.args[arg_index];
  if (source_value.kind != Value::Kind::Named || source_value.name.empty()) {
    return {};
  }
  for (std::size_t inst_index = call_instruction_index; inst_index-- > 0;) {
    const auto& inst = block.insts[inst_index];
    if (const auto* load_local = std::get_if<LoadLocalInst>(&inst);
        load_local != nullptr &&
        load_local->result.kind == Value::Kind::Named &&
        load_local->result.name == source_value.name &&
        load_local->result.type == source_value.type) {
      return CallArgumentSourceProducerMaterialization{
          .available = true,
          .arg_index = arg_index,
          .producer_kind = CallArgumentSourceProducerKind::LoadLocal,
          .producer_instruction = &inst,
          .producer_instruction_index = inst_index,
          .produced_value = &load_local->result,
          .materializable = true,
      };
    }
    if (const auto* binary = std::get_if<BinaryInst>(&inst);
        binary != nullptr &&
        binary->result.kind == Value::Kind::Named &&
        binary->result.name == source_value.name &&
        binary->result.type == source_value.type) {
      return CallArgumentSourceProducerMaterialization{
          .available = true,
          .arg_index = arg_index,
          .producer_kind = CallArgumentSourceProducerKind::Binary,
          .producer_instruction = &inst,
          .producer_instruction_index = inst_index,
          .produced_value = &binary->result,
          .materializable =
              call_argument_binary_source_producer_opcode_is_materializable(
                  binary->opcode),
      };
    }
  }
  return {};
}

CallResultSourceIdentity find_call_result_source_identity(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index) {
  if (indexed_call_inst(block, call, call_instruction_index) == nullptr ||
      !call.result.has_value() ||
      call.result->kind != Value::Kind::Named ||
      call.result->name.empty()) {
    return {};
  }
  return CallResultSourceIdentity{
      .available = true,
      .call_instruction_index = call_instruction_index,
      .result_value = &*call.result,
  };
}

CallResultLaneSourceIdentity find_call_result_lane_source_identity(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index,
    const Value& value) {
  const auto result_identity =
      find_call_result_source_identity(block, call, call_instruction_index);
  if (!result_identity.available ||
      value.kind != Value::Kind::Named ||
      value.name.empty()) {
    return {};
  }

  CallResultLaneSourceIdentity result;
  if (call.result->kind == value.kind &&
      call.result->name == value.name &&
      call.result->type == value.type) {
    result = CallResultLaneSourceIdentity{
        .available = true,
        .call_instruction_index = call_instruction_index,
        .lane_index = 0,
        .lane_value = &*call.result,
        .aliases_primary_result = true,
    };
  }

  for (std::size_t lane_index = 0; lane_index < call.result_lanes.size();
       ++lane_index) {
    const auto& lane = call.result_lanes[lane_index];
    if (lane.kind != value.kind ||
        lane.name != value.name ||
        lane.type != value.type) {
      continue;
    }
    const bool aliases_primary_result =
        lane_index == 0 &&
        call.result->kind == lane.kind &&
        call.result->name == lane.name &&
        call.result->type == lane.type;
    if (result.available && result.aliases_primary_result &&
        aliases_primary_result) {
      continue;
    }
    if (result.available) {
      return {};
    }
    result = CallResultLaneSourceIdentity{
        .available = true,
        .call_instruction_index = call_instruction_index,
        .lane_index = lane_index,
        .lane_value = &lane,
        .aliases_primary_result = aliases_primary_result,
    };
  }
  return result;
}

CallArgumentPublicationSourceRouting find_call_argument_publication_source_routing(
    const CallInst& call,
    std::size_t arg_index) {
  const auto* relationship =
      find_call_argument_source_relationship(call, arg_index);
  if (relationship == nullptr) {
    return {};
  }

  const auto* selection =
      relationship->source_selection.has_value() &&
              call_argument_source_selection_available(
                  *relationship->source_selection)
          ? &*relationship->source_selection
          : nullptr;
  const auto* dependency =
      relationship->direct_global_select_chain_dependency.has_value() &&
              call_argument_direct_global_select_chain_dependency_available(
                  *relationship->direct_global_select_chain_dependency)
          ? &*relationship->direct_global_select_chain_dependency
          : nullptr;
  const bool available =
      relationship->source_encoding != CallArgumentSourceEncodingKind::None ||
      selection != nullptr ||
      dependency != nullptr;
  if (!available) {
    return {};
  }

  return CallArgumentPublicationSourceRouting{
      .available = true,
      .arg_index = relationship->arg_index,
      .source_encoding = relationship->source_encoding,
      .source_value_id = relationship->source_value_id,
      .source_value_name = relationship->source_value_name,
      .source_base_value_id = relationship->source_base_value_id,
      .source_base_value_name = relationship->source_base_value_name,
      .source_pointer_byte_delta = relationship->source_pointer_byte_delta,
      .source_selection = selection,
      .direct_global_select_chain_dependency = dependency,
  };
}

}  // namespace c4c::backend::bir
