#include "lowering.hpp"
#include "ir.hpp"
#include "canonical_symbol.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace c4c::codegen::lir {

using namespace c4c::hir;
using namespace c4c::codegen::llvm_helpers;

// ── Helpers shared with hir_emitter.cpp (anonymous-namespace copies there) ────

namespace {

int llvm_struct_field_slot(const HirStructDef& sd, int target_llvm_idx) {
  if (sd.is_union) return 0;
  int last_idx = -1;
  int cur_offset = 0;
  int slot = 0;
  for (const auto& f : sd.fields) {
    if (f.llvm_idx == last_idx) continue;
    last_idx = f.llvm_idx;
    if (f.offset_bytes > cur_offset) ++slot;
    if (f.llvm_idx == target_llvm_idx) return slot;
    cur_offset = f.offset_bytes + std::max(0, f.size_bytes);
    ++slot;
  }
  return 0;
}

int llvm_struct_field_slot_by_name(const HirStructDef& sd, const std::string& field_name) {
  for (const auto& f : sd.fields) {
    if (f.name == field_name) return llvm_struct_field_slot(sd, f.llvm_idx);
  }
  return 0;
}

std::string emitted_link_name(const c4c::hir::Module& mod, c4c::LinkNameId id,
                              std::string_view fallback) {
  const std::string_view resolved = mod.link_names.spelling(id);
  return resolved.empty() ? std::string(fallback) : std::string(resolved);
}

bool init_list_has_field_designator(const InitListItem& item) {
  return item.field_designator_text_id != kInvalidText || item.field_designator.has_value();
}

std::string_view init_list_field_designator_text(const InitListItem& item,
                                                 const TextTable* texts) {
  if (item.field_designator_text_id != kInvalidText && texts) {
    return texts->lookup(item.field_designator_text_id);
  }
  if (item.field_designator) return *item.field_designator;
  return {};
}

std::optional<HirRecordOwnerKey> const_init_aggregate_owner_key_from_type(const TypeSpec& ts) {
  if (ts.record_def && ts.record_def->kind == NK_STRUCT_DEF) {
    const TextId declaration_text_id = ts.record_def->unqualified_text_id;
    if (declaration_text_id != kInvalidText) {
      NamespaceQualifier ns_qual;
      ns_qual.context_id = ts.record_def->namespace_context_id;
      ns_qual.is_global_qualified = ts.record_def->is_global_qualified;
      if (ts.record_def->qualifier_text_ids && ts.record_def->n_qualifier_segments > 0) {
        ns_qual.segment_text_ids.assign(
            ts.record_def->qualifier_text_ids,
            ts.record_def->qualifier_text_ids + ts.record_def->n_qualifier_segments);
      }
      const HirRecordOwnerKey owner_key =
          make_hir_record_owner_key(ns_qual, declaration_text_id);
      if (hir_record_owner_key_has_complete_metadata(owner_key)) return owner_key;
    }
  }

  if (ts.tag_text_id == kInvalidText) return std::nullopt;
  NamespaceQualifier ns_qual;
  ns_qual.context_id = ts.namespace_context_id;
  ns_qual.is_global_qualified = ts.is_global_qualified;
  if (ts.qualifier_text_ids && ts.n_qualifier_segments > 0) {
    ns_qual.segment_text_ids.assign(ts.qualifier_text_ids,
                                    ts.qualifier_text_ids + ts.n_qualifier_segments);
  }
  const HirRecordOwnerKey owner_key = make_hir_record_owner_key(ns_qual, ts.tag_text_id);
  if (hir_record_owner_key_has_complete_metadata(owner_key)) return owner_key;
  return std::nullopt;
}

StructNameId const_init_aggregate_structured_name_id(const c4c::hir::Module& mod,
                                                     const lir::LirModule* module,
                                                     const TypeSpec& aggregate_ts) {
  if (!module || (aggregate_ts.base != TB_STRUCT && aggregate_ts.base != TB_UNION) ||
      aggregate_ts.ptr_level != 0 || aggregate_ts.array_rank != 0) {
    return kInvalidStructName;
  }

  const std::optional<HirRecordOwnerKey> owner_key =
      const_init_aggregate_owner_key_from_type(aggregate_ts);
  if (!owner_key) return kInvalidStructName;
  const SymbolName* structured_tag = mod.find_struct_def_tag_by_owner(*owner_key);
  if (!structured_tag || structured_tag->empty()) return kInvalidStructName;

  const StructNameId name_id =
      module->struct_names.find(llvm_struct_type_str(*structured_tag));
  return module->find_struct_decl(name_id) ? name_id : kInvalidStructName;
}

template <typename Matches>
const GlobalVar* select_global_object_by(const Module& mod, Matches matches) {
  const GlobalVar* best = nullptr;
  for (const auto& g : mod.globals) {
    if (!matches(g)) continue;
    if (!best) {
      best = &g;
      continue;
    }
    const TypeSpec& best_ts = best->type.spec;
    const TypeSpec& cand_ts = g.type.spec;
    if (cand_ts.array_rank > 0 && best_ts.array_rank <= 0) {
      best = &g;
    } else if (cand_ts.array_rank > 0 && best_ts.array_rank > 0 &&
               cand_ts.array_size > best_ts.array_size) {
      best = &g;
    } else if (cand_ts.array_rank <= 0 && best_ts.array_rank <= 0 &&
               g.init.index() != 0 && best->init.index() == 0) {
      best = &g;
    }
  }
  return best;
}

}  // namespace

// ── Constructor ───────────────────────────────────────────────────────────────

ConstInitEmitter::ConstInitEmitter(const Module& mod, LirModule& module)
    : mod_(mod), module_(module) {}

// ── Expr lookup ───────────────────────────────────────────────────────────────

const Expr& ConstInitEmitter::get_expr(ExprId id) const {
  for (const auto& e : mod_.expr_pool)
    if (e.id.value == id.value) return e;
  throw std::runtime_error("ConstInitEmitter: expr not found id=" + std::to_string(id.value));
}

// ── Field type reconstruction ─────────────────────────────────────────────────

TypeSpec ConstInitEmitter::field_decl_type(const HirStructField& f) const {
  TypeSpec ts = f.elem_type;
  if (f.array_first_dim >= 0) {
    const int trailing_rank = std::max(0, std::min(ts.array_rank, 7));
    for (int i = trailing_rank; i > 0; --i) ts.array_dims[i] = ts.array_dims[i - 1];
    for (int i = trailing_rank + 1; i < 8; ++i) ts.array_dims[i] = -1;
    ts.array_rank = trailing_rank + 1;
    ts.array_size = f.array_first_dim;
    ts.array_dims[0] = f.array_first_dim;
    ts.is_ptr_to_array = false;
    ts.inner_rank = -1;
  }
  return ts;
}

const HirStructDef* ConstInitEmitter::lookup_const_init_struct_def(const TypeSpec& ts) const {
  const StructNameId structured_name_id =
      const_init_aggregate_structured_name_id(mod_, &module_, ts);
  const char* site = structured_name_id == kInvalidStructName
                         ? "const-init-aggregate-legacy-compat"
                         : "const-init-aggregate";
  const stmt_emitter_detail::StructuredLayoutLookup layout =
      stmt_emitter_detail::lookup_structured_layout(mod_, &module_, ts, site,
                                                    structured_name_id);
  return layout.legacy_decl;
}

// ── Global object lookup ──────────────────────────────────────────────────────

const GlobalVar* ConstInitEmitter::select_global_object(const std::string& name) const {
  return select_global_object_by(mod_, [&](const GlobalVar& g) { return g.name == name; });
}

const GlobalVar* ConstInitEmitter::select_global_object(GlobalId id) const {
  const GlobalVar* gv = mod_.find_global(id);
  if (!gv) return nullptr;
  if (gv->link_name_id != kInvalidLinkName) {
    if (const GlobalVar* best = select_global_object_by(mod_, [&](const GlobalVar& cand) {
          return cand.link_name_id == gv->link_name_id;
        })) {
      return best;
    }
  }
  if (gv->name_text_id != kInvalidText) {
    if (const GlobalVar* best = select_global_object_by(mod_, [&](const GlobalVar& cand) {
          return cand.name_text_id == gv->name_text_id;
        })) {
      return best;
    }
  }
  if (gv->link_name_id == kInvalidLinkName && gv->name_text_id == kInvalidText) {
    if (const GlobalVar* best = select_global_object(gv->name)) return best;
  }
  return gv;
}

const GlobalVar* ConstInitEmitter::select_global_object(const DeclRef& ref) const {
  if (ref.global) return select_global_object(*ref.global);
  if (ref.link_name_id != kInvalidLinkName) {
    if (const GlobalVar* best = select_global_object_by(mod_, [&](const GlobalVar& cand) {
          return cand.link_name_id == ref.link_name_id;
        })) {
      return best;
    }
  }
  if (ref.name_text_id != kInvalidText) {
    if (const GlobalVar* best = select_global_object_by(mod_, [&](const GlobalVar& cand) {
          return cand.name_text_id == ref.name_text_id;
        })) {
      return best;
    }
  }
  if (ref.link_name_id != kInvalidLinkName || ref.name_text_id != kInvalidText) return nullptr;
  return select_global_object(ref.name);
}

// ── String interning ──────────────────────────────────────────────────────────

std::string ConstInitEmitter::intern_str(const std::string& raw_bytes) {
  return module_.intern_str(raw_bytes);
}

// ── Static string/byte helpers ────────────────────────────────────────────────

bool ConstInitEmitter::is_char_like(TypeBase b) {
  return b == TB_CHAR || b == TB_SCHAR || b == TB_UCHAR;
}

TypeSpec ConstInitEmitter::drop_one_array_dim(TypeSpec ts) {
  if (ts.array_rank <= 0) return ts;
  for (int i = 0; i < ts.array_rank - 1; ++i) ts.array_dims[i] = ts.array_dims[i + 1];
  ts.array_dims[ts.array_rank - 1] = -1;
  ts.array_rank--;
  ts.array_size = (ts.array_rank > 0) ? ts.array_dims[0] : -1;
  return ts;
}

std::string ConstInitEmitter::bytes_from_string_literal(const StringLiteral& sl) {
  std::string bytes = sl.raw;
  if (bytes.size() >= 2 && bytes.front() == '"' && bytes.back() == '"') {
    bytes = bytes.substr(1, bytes.size() - 2);
  } else if (bytes.size() >= 3 && bytes[0] == 'L' && bytes[1] == '"' &&
             bytes.back() == '"') {
    bytes = bytes.substr(2, bytes.size() - 3);
  }
  return decode_c_escaped_bytes(bytes);
}

std::vector<long long> ConstInitEmitter::decode_wide_string_values(const std::string& raw) {
  std::vector<long long> out;
  const char* p = raw.c_str();
  while (*p && *p != '"') ++p;
  if (*p == '"') ++p;
  while (*p && *p != '"') {
    if (*p == '\\' && *(p + 1)) {
      ++p;
      switch (*p) {
        case 'n':  out.push_back('\n'); ++p; break;
        case 't':  out.push_back('\t'); ++p; break;
        case 'r':  out.push_back('\r'); ++p; break;
        case '0':  out.push_back(0); ++p; break;
        case '\\': out.push_back('\\'); ++p; break;
        case '\'': out.push_back('\''); ++p; break;
        case '"':  out.push_back('"'); ++p; break;
        case 'a':  out.push_back('\a'); ++p; break;
        case 'x': {
          ++p;
          long long v = 0;
          while (isxdigit((unsigned char)*p)) {
            v = v * 16 + (isdigit((unsigned char)*p) ? *p - '0' : tolower((unsigned char)*p) - 'a' + 10);
            ++p;
          }
          out.push_back(v);
          break;
        }
        default:
          if (*p >= '0' && *p <= '7') {
            long long v = 0;
            for (int k = 0; k < 3 && *p >= '0' && *p <= '7'; ++k, ++p)
              v = v * 8 + (*p - '0');
            out.push_back(v);
          } else {
            out.push_back(static_cast<unsigned char>(*p)); ++p;
          }
          break;
      }
      continue;
    }
    const unsigned char c0 = static_cast<unsigned char>(*p);
    if (c0 < 0x80) {
      out.push_back(c0); ++p;
    } else if ((c0 & 0xE0) == 0xC0 && p[1]) {
      out.push_back(((c0 & 0x1F) << 6) | (static_cast<unsigned char>(p[1]) & 0x3F));
      p += 2;
    } else if ((c0 & 0xF0) == 0xE0 && p[1] && p[2]) {
      out.push_back(((c0 & 0x0F) << 12) | ((static_cast<unsigned char>(p[1]) & 0x3F) << 6) |
                     (static_cast<unsigned char>(p[2]) & 0x3F));
      p += 3;
    } else if ((c0 & 0xF8) == 0xF0 && p[1] && p[2] && p[3]) {
      out.push_back(((c0 & 0x07) << 18) | ((static_cast<unsigned char>(p[1]) & 0x3F) << 12) |
                     ((static_cast<unsigned char>(p[2]) & 0x3F) << 6) |
                     (static_cast<unsigned char>(p[3]) & 0x3F));
      p += 4;
    } else {
      out.push_back(c0); ++p;
    }
  }
  out.push_back(0);
  return out;
}

std::string ConstInitEmitter::escape_llvm_c_bytes(const std::string& raw_bytes) {
  std::string esc;
  for (unsigned char c : raw_bytes) {
    if (c == '"')      { esc += "\\22"; }
    else if (c == '\\') { esc += "\\5C"; }
    else if (c == '\n') { esc += "\\0A"; }
    else if (c == '\r') { esc += "\\0D"; }
    else if (c == '\t') { esc += "\\09"; }
    else if (c < 32 || c >= 127) {
      char buf[8];
      std::snprintf(buf, sizeof(buf), "\\%02X", c);
      esc += buf;
    } else {
      esc += static_cast<char>(c);
    }
  }
  return esc;
}

// ── Constant evaluation ───────────────────────────────────────────────────────

std::optional<long long> ConstInitEmitter::try_const_eval_int(ExprId id) {
  const Expr& e = get_expr(id);
  return std::visit([&](const auto& p) -> std::optional<long long> {
    using T = std::decay_t<decltype(p)>;
    if constexpr (std::is_same_v<T, IntLiteral>) {
      return p.value;
    } else if constexpr (std::is_same_v<T, CharLiteral>) {
      return static_cast<long long>(p.value);
    } else if constexpr (std::is_same_v<T, CastExpr>) {
      return try_const_eval_int(p.expr);
    } else if constexpr (std::is_same_v<T, UnaryExpr>) {
      auto v = try_const_eval_int(p.operand);
      if (!v) return std::nullopt;
      switch (p.op) {
        case UnaryOp::Plus:   return *v;
        case UnaryOp::Minus:  return -*v;
        case UnaryOp::BitNot: return ~*v;
        case UnaryOp::Not:    return static_cast<long long>(!*v);
        default: return std::nullopt;
      }
    } else if constexpr (std::is_same_v<T, BinaryExpr>) {
      auto lv = try_const_eval_int(p.lhs);
      auto rv = try_const_eval_int(p.rhs);
      if (!lv || !rv) return std::nullopt;
      switch (p.op) {
        case BinaryOp::Add:    return *lv + *rv;
        case BinaryOp::Sub:    return *lv - *rv;
        case BinaryOp::Mul:    return *lv * *rv;
        case BinaryOp::Div:    return (*rv != 0) ? *lv / *rv : 0LL;
        case BinaryOp::Mod:    return (*rv != 0) ? *lv % *rv : 0LL;
        case BinaryOp::Shl:    return *lv << *rv;
        case BinaryOp::Shr:    return *lv >> *rv;
        case BinaryOp::BitAnd: return *lv & *rv;
        case BinaryOp::BitOr:  return *lv | *rv;
        case BinaryOp::BitXor: return *lv ^ *rv;
        case BinaryOp::Lt:     return static_cast<long long>(*lv < *rv);
        case BinaryOp::Le:     return static_cast<long long>(*lv <= *rv);
        case BinaryOp::Gt:     return static_cast<long long>(*lv > *rv);
        case BinaryOp::Ge:     return static_cast<long long>(*lv >= *rv);
        case BinaryOp::Eq:     return static_cast<long long>(*lv == *rv);
        case BinaryOp::Ne:     return static_cast<long long>(*lv != *rv);
        case BinaryOp::LAnd:   return static_cast<long long>(*lv && *rv);
        case BinaryOp::LOr:    return static_cast<long long>(*lv || *rv);
        default: return std::nullopt;
      }
    } else if constexpr (std::is_same_v<T, SizeofTypeExpr>) {
      return static_cast<long long>(sizeof_ts(mod_, p.type.spec));
    } else if constexpr (std::is_same_v<T, SizeofExpr>) {
      const Expr& op = get_expr(p.expr);
      if (has_concrete_type(op.type.spec))
        return static_cast<long long>(sizeof_ts(mod_, op.type.spec));
      return std::nullopt;
    } else {
      return std::nullopt;
    }
  }, e.payload);
}

std::optional<double> ConstInitEmitter::try_const_eval_float(ExprId id) {
  const Expr& e = get_expr(id);
  return std::visit([&](const auto& p) -> std::optional<double> {
    using T = std::decay_t<decltype(p)>;
    if constexpr (std::is_same_v<T, FloatLiteral>) {
      return p.value;
    } else if constexpr (std::is_same_v<T, IntLiteral>) {
      return static_cast<double>(p.value);
    } else if constexpr (std::is_same_v<T, CharLiteral>) {
      return static_cast<double>(p.value);
    } else if constexpr (std::is_same_v<T, CastExpr>) {
      return try_const_eval_float(p.expr);
    } else if constexpr (std::is_same_v<T, UnaryExpr>) {
      auto v = try_const_eval_float(p.operand);
      if (!v) return std::nullopt;
      switch (p.op) {
        case UnaryOp::Plus:  return *v;
        case UnaryOp::Minus: return -*v;
        default: return std::nullopt;
      }
    } else if constexpr (std::is_same_v<T, BinaryExpr>) {
      auto lv = try_const_eval_float(p.lhs);
      auto rv = try_const_eval_float(p.rhs);
      if (!lv || !rv) return std::nullopt;
      switch (p.op) {
        case BinaryOp::Add: return *lv + *rv;
        case BinaryOp::Sub: return *lv - *rv;
        case BinaryOp::Mul: return *lv * *rv;
        case BinaryOp::Div: return *lv / *rv;
        default: return std::nullopt;
      }
    } else if constexpr (std::is_same_v<T, CallExpr>) {
      const BuiltinId builtin_id = p.builtin_id;
      switch (builtin_constant_fp_kind(builtin_id)) {
        case BuiltinConstantFpKind::Infinity:
          if (builtin_id == BuiltinId::HugeValF || builtin_id == BuiltinId::InfF) {
            return static_cast<double>(std::numeric_limits<float>::infinity());
          }
          return std::numeric_limits<double>::infinity();
        case BuiltinConstantFpKind::QuietNaN:
          if (builtin_id == BuiltinId::NanF || builtin_id == BuiltinId::NansF) {
            return static_cast<double>(std::numeric_limits<float>::quiet_NaN());
          }
          return std::numeric_limits<double>::quiet_NaN();
        case BuiltinConstantFpKind::None:
          break;
      }
      return std::nullopt;
    } else {
      return std::nullopt;
    }
  }, e.payload);
}

std::optional<std::pair<long long, long long>> ConstInitEmitter::try_const_eval_complex_int(ExprId id) {
  const Expr& e = get_expr(id);
  return std::visit([&](const auto& p) -> std::optional<std::pair<long long, long long>> {
    using T = std::decay_t<decltype(p)>;
    if constexpr (std::is_same_v<T, IntLiteral>) {
      if (e.type.spec.base == TB_COMPLEX_CHAR || e.type.spec.base == TB_COMPLEX_SCHAR ||
          e.type.spec.base == TB_COMPLEX_UCHAR || e.type.spec.base == TB_COMPLEX_SHORT ||
          e.type.spec.base == TB_COMPLEX_USHORT || e.type.spec.base == TB_COMPLEX_INT ||
          e.type.spec.base == TB_COMPLEX_UINT || e.type.spec.base == TB_COMPLEX_LONG ||
          e.type.spec.base == TB_COMPLEX_ULONG || e.type.spec.base == TB_COMPLEX_LONGLONG ||
          e.type.spec.base == TB_COMPLEX_ULONGLONG) {
        return std::pair<long long, long long>{0, p.value};
      }
      return std::pair<long long, long long>{p.value, 0};
    } else if constexpr (std::is_same_v<T, CharLiteral>) {
      return std::pair<long long, long long>{p.value, 0};
    } else if constexpr (std::is_same_v<T, UnaryExpr>) {
      auto v = try_const_eval_complex_int(p.operand);
      if (!v) return std::nullopt;
      switch (p.op) {
        case UnaryOp::Plus: return v;
        case UnaryOp::Minus: return std::pair<long long, long long>{-v->first, -v->second};
        default: return std::nullopt;
      }
    } else if constexpr (std::is_same_v<T, BinaryExpr>) {
      auto lv = try_const_eval_complex_int(p.lhs);
      auto rv = try_const_eval_complex_int(p.rhs);
      if (!lv || !rv) return std::nullopt;
      switch (p.op) {
        case BinaryOp::Add:
          return std::pair<long long, long long>{lv->first + rv->first, lv->second + rv->second};
        case BinaryOp::Sub:
          return std::pair<long long, long long>{lv->first - rv->first, lv->second - rv->second};
        default:
          return std::nullopt;
      }
    } else if constexpr (std::is_same_v<T, CastExpr>) {
      return try_const_eval_complex_int(p.expr);
    } else {
      return std::nullopt;
    }
  }, e.payload);
}

std::optional<std::pair<double, double>> ConstInitEmitter::try_const_eval_complex_fp(ExprId id) {
  const Expr& e = get_expr(id);
  return std::visit([&](const auto& p) -> std::optional<std::pair<double, double>> {
    using T = std::decay_t<decltype(p)>;
    if constexpr (std::is_same_v<T, FloatLiteral>) {
      if (is_complex_base(e.type.spec.base)) return std::pair<double, double>{0.0, p.value};
      return std::pair<double, double>{p.value, 0.0};
    } else if constexpr (std::is_same_v<T, IntLiteral>) {
      if (is_complex_base(e.type.spec.base)) return std::pair<double, double>{0.0, static_cast<double>(p.value)};
      return std::pair<double, double>{static_cast<double>(p.value), 0.0};
    } else if constexpr (std::is_same_v<T, CharLiteral>) {
      return std::pair<double, double>{static_cast<double>(p.value), 0.0};
    } else if constexpr (std::is_same_v<T, UnaryExpr>) {
      auto v = try_const_eval_complex_fp(p.operand);
      if (!v) return std::nullopt;
      switch (p.op) {
        case UnaryOp::Plus: return v;
        case UnaryOp::Minus: return std::pair<double, double>{-v->first, -v->second};
        default: return std::nullopt;
      }
    } else if constexpr (std::is_same_v<T, BinaryExpr>) {
      auto lv = try_const_eval_complex_fp(p.lhs);
      auto rv = try_const_eval_complex_fp(p.rhs);
      if (!lv || !rv) return std::nullopt;
      switch (p.op) {
        case BinaryOp::Add:
          return std::pair<double, double>{lv->first + rv->first, lv->second + rv->second};
        case BinaryOp::Sub:
          return std::pair<double, double>{lv->first - rv->first, lv->second - rv->second};
        case BinaryOp::Mul:
          return std::pair<double, double>{
              lv->first * rv->first - lv->second * rv->second,
              lv->first * rv->second + lv->second * rv->first};
        case BinaryOp::Div: {
          const double denom = rv->first * rv->first + rv->second * rv->second;
          if (denom == 0.0) return std::nullopt;
          return std::pair<double, double>{
              (lv->first * rv->first + lv->second * rv->second) / denom,
              (lv->second * rv->first - lv->first * rv->second) / denom};
        }
        default:
          return std::nullopt;
      }
    } else if constexpr (std::is_same_v<T, CastExpr>) {
      return try_const_eval_complex_fp(p.expr);
    } else {
      return std::nullopt;
    }
  }, e.payload);
}

std::string ConstInitEmitter::emit_const_int_like(long long value, const TypeSpec& expected_ts) {
  if (llvm_ty(expected_ts) == "ptr") {
    if (value == 0) return "null";
    return "inttoptr (i64 " + std::to_string(value) + " to ptr)";
  }
  if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0)
    return fp_literal(expected_ts.base, static_cast<double>(value));
  return std::to_string(value);
}

std::optional<std::string> ConstInitEmitter::try_emit_global_address_expr(ExprId id) {
  struct GlobalAddressExpr {
    std::string global_name;
    std::string root_type_text;
    TypeSpec current_type;
    std::vector<std::string> indices;
  };

  std::function<std::optional<GlobalAddressExpr>(ExprId)> lower_expr =
      [&](ExprId expr_id) -> std::optional<GlobalAddressExpr> {
    const Expr& expr = get_expr(expr_id);
    return std::visit(
        [&](const auto& payload) -> std::optional<GlobalAddressExpr> {
          using T = std::decay_t<decltype(payload)>;
          if constexpr (std::is_same_v<T, DeclRef>) {
            const GlobalVar* gv = select_global_object(payload);
            if (!gv) return std::nullopt;
            const std::string global_name =
                emitted_link_name(mod_, gv->link_name_id, gv->name);
            return GlobalAddressExpr{
                .global_name = global_name,
                .root_type_text = llvm_alloca_ty(gv->type.spec),
                .current_type = gv->type.spec,
                .indices = {},
            };
          } else if constexpr (std::is_same_v<T, IndexExpr>) {
            auto base = lower_expr(payload.base);
            const auto index_value = try_const_eval_int(payload.index);
            if (!base || !index_value || *index_value < 0) return std::nullopt;
            if (base->current_type.array_rank <= 0) return std::nullopt;
            base->indices.push_back("i64 " + std::to_string(*index_value));
            base->current_type = drop_one_array_dim(base->current_type);
            return base;
          } else if constexpr (std::is_same_v<T, MemberExpr>) {
            if (payload.is_arrow) return std::nullopt;
            auto base = lower_expr(payload.base);
            if (!base) return std::nullopt;
            const HirStructDef* sd_ptr = lookup_const_init_struct_def(base->current_type);
            if (!sd_ptr) return std::nullopt;
            const HirStructDef& sd = *sd_ptr;
            const auto fit =
                std::find_if(sd.fields.begin(), sd.fields.end(), [&](const HirStructField& field) {
                  return field.name == payload.field;
                });
            if (fit == sd.fields.end()) return std::nullopt;
            base->indices.push_back(
                "i32 " + std::to_string(llvm_struct_field_slot(sd, fit->llvm_idx)));
            base->current_type = field_decl_type(*fit);
            return base;
          } else {
            return std::nullopt;
          }
        },
        expr.payload);
  };

  auto address = lower_expr(id);
  if (!address.has_value()) {
    return std::nullopt;
  }
  if (address->indices.empty()) {
    return llvm_global_sym(address->global_name);
  }
  std::string gep = "getelementptr inbounds (" + address->root_type_text + ", ptr @" +
                    address->global_name + ", i64 0";
  for (const auto& index : address->indices) gep += ", " + index;
  gep += ")";
  return gep;
}

std::string ConstInitEmitter::emit_const_scalar_expr(ExprId id, const TypeSpec& expected_ts) {
  const Expr& e = get_expr(id);
  const auto resolve_global_decl_name = [&](LinkNameId id,
                                            std::string_view name) -> std::optional<std::string> {
    const GlobalVar* gv =
        (id != kInvalidLinkName)
            ? select_global_object_by(mod_, [&](const GlobalVar& cand) {
                return cand.link_name_id == id;
              })
            : select_global_object(std::string(name));
    if (!gv) return std::nullopt;
    return emitted_link_name(mod_, gv->link_name_id, gv->name);
  };
  const auto resolve_global_decl_ref_name =
      [&](const DeclRef& ref) -> std::optional<std::string> {
    const GlobalVar* gv = select_global_object(ref);
    if (!gv) return std::nullopt;
    return emitted_link_name(mod_, gv->link_name_id, gv->name);
  };
  const auto resolve_function_decl_name = [&](LinkNameId id,
                                              std::string_view name) -> std::optional<std::string> {
    const Function* fn =
        (id != kInvalidLinkName) ? mod_.find_function(id) : mod_.find_function_by_name_legacy(name);
    if (!fn) return std::nullopt;
    return emitted_link_name(mod_, fn->link_name_id, fn->name);
  };
  const auto resolve_function_link_name = [&](LinkNameId id,
                                              std::string_view fallback) -> std::string {
    return emitted_link_name(mod_, id, fallback);
  };
  const auto resolve_decl_name = [&](LinkNameId id, std::string_view name) -> std::string {
    if (auto global_name = resolve_global_decl_name(id, name)) return *global_name;
    if (auto function_name = resolve_function_decl_name(id, name)) return *function_name;
    return std::string(name);
  };
  return std::visit([&](const auto& p) -> std::string {
    using T = std::decay_t<decltype(p)>;
    if (is_complex_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0) {
      const TypeSpec elem_ts = complex_component_ts(expected_ts.base);
      if (is_float_base(elem_ts.base)) {
        if (auto cv = try_const_eval_complex_fp(id)) {
          const auto emit_fp = [&](double value) -> std::string {
            return fp_literal(elem_ts.base, value);
          };
          return "{ " + llvm_ty(elem_ts) + " " + emit_fp(cv->first) +
                 ", " + llvm_ty(elem_ts) + " " + emit_fp(cv->second) + " }";
        }
      }
      if (auto cv = try_const_eval_complex_int(id)) {
        return "{ " + llvm_ty(elem_ts) + " " +
               emit_const_int_like(cv->first, elem_ts) +
               ", " + llvm_ty(elem_ts) + " " +
               emit_const_int_like(cv->second, elem_ts) +
               " }";
      }
      return "zeroinitializer";
    }
    if constexpr (std::is_same_v<T, IntLiteral>) {
      if (llvm_ty(expected_ts) == "ptr") {
        if (p.value == 0) return "null";
        return "inttoptr (i64 " + std::to_string(p.value) + " to ptr)";
      }
      if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0)
        return fp_literal(expected_ts.base, static_cast<double>(p.value));
      return std::to_string(p.value);
    } else if constexpr (std::is_same_v<T, FloatLiteral>) {
      if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0)
        return fp_literal(expected_ts.base, p.value);
      return fp_to_hex(p.value);
    } else if constexpr (std::is_same_v<T, CharLiteral>) {
      return std::to_string(p.value);
    } else if constexpr (std::is_same_v<T, StringLiteral>) {
      const std::string bytes = bytes_from_string_literal(p);
      const std::string gname = intern_str(bytes);
      const size_t len = bytes.size() + 1;
      return "getelementptr inbounds ([" + std::to_string(len) + " x i8], ptr " +
             gname + ", i64 0, i64 0)";
    } else if constexpr (std::is_same_v<T, DeclRef>) {
      if (resolve_function_decl_name(p.link_name_id, p.name).has_value() ||
          resolve_global_decl_ref_name(p).has_value() ||
          llvm_ty(expected_ts) == "ptr" ||
          expected_ts.ptr_level > 0 || expected_ts.is_fn_ptr) {
        if (auto global_name = resolve_global_decl_ref_name(p)) {
          return llvm_global_sym(*global_name);
        }
        return llvm_global_sym(resolve_decl_name(p.link_name_id, p.name));
      }
      return "0";
    } else if constexpr (std::is_same_v<T, UnaryExpr>) {
      if (p.op == UnaryOp::AddrOf) {
        const Expr& op_e = get_expr(p.operand);
        if (const auto* r = std::get_if<DeclRef>(&op_e.payload)) {
          if (auto global_name = resolve_global_decl_ref_name(*r)) {
            return llvm_global_sym(*global_name);
          }
          return llvm_global_sym(resolve_decl_name(r->link_name_id, r->name));
        }
        if (const auto* s = std::get_if<StringLiteral>(&op_e.payload)) {
          const std::string bytes = bytes_from_string_literal(*s);
          const std::string gname = intern_str(bytes);
          const size_t len = bytes.size() + 1;
          return "getelementptr inbounds ([" + std::to_string(len) + " x i8], ptr " +
                 gname + ", i64 0, i64 0)";
        }
        if (const auto global_addr = try_emit_global_address_expr(p.operand); global_addr.has_value()) {
          return *global_addr;
        }
        if (const auto* idx_e = std::get_if<IndexExpr>(&op_e.payload)) {
          std::vector<long long> indices;
          const Expr* cur_e = &op_e;
          while (const auto* ie = std::get_if<IndexExpr>(&cur_e->payload)) {
            indices.push_back(try_const_eval_int(ie->index).value_or(0));
            cur_e = &get_expr(ie->base);
          }
          std::reverse(indices.begin(), indices.end());
          if (const auto* dr = std::get_if<DeclRef>(&cur_e->payload)) {
            const GlobalVar* gv = select_global_object(*dr);
            if (gv && gv->type.spec.array_rank > 0) {
              const TypeSpec& resolved = gv->type.spec;
              const std::string aty = llvm_alloca_ty(resolved);
              const std::string global_name =
                  emitted_link_name(mod_, gv->link_name_id, gv->name);
              std::string gep =
                  "getelementptr inbounds (" + aty + ", ptr @" + global_name + ", i64 0";
              for (auto idx : indices) gep += ", i64 " + std::to_string(idx);
              gep += ")";
              return gep;
            }
          }
          if (const auto* sl = std::get_if<StringLiteral>(&cur_e->payload)) {
            const std::string bytes = bytes_from_string_literal(*sl);
            const std::string gname = intern_str(bytes);
            const size_t len = bytes.size() + 1;
            const std::string aty = "[" + std::to_string(len) + " x i8]";
            std::string gep = "getelementptr inbounds (" + aty + ", ptr " + gname + ", i64 0";
            for (auto idx : indices) gep += ", i64 " + std::to_string(idx);
            gep += ")";
            return gep;
          }
        }
        if (const auto* mem_e = std::get_if<MemberExpr>(&op_e.payload)) {
          auto aggregate_final_type_text = [](const TypeSpec& ts) -> std::optional<std::string> {
            const std::optional<std::string> spelling = typespec_aggregate_final_spelling(ts);
            if (!spelling) return std::nullopt;
            return llvm_struct_type_str(*spelling);
          };
          if (!mem_e->is_arrow) {
            {
              std::vector<std::string> field_path;
              const Expr* walk = &op_e;
              while (const auto* me = std::get_if<MemberExpr>(&walk->payload)) {
                if (me->is_arrow) break;
                field_path.push_back(me->field);
                walk = &get_expr(me->base);
              }
              std::reverse(field_path.begin(), field_path.end());
              if (field_path.size() >= 2) {
                if (const auto* dr = std::get_if<DeclRef>(&walk->payload)) {
                  const GlobalVar* gv = select_global_object(*dr);
                  if (gv && is_named_aggregate_value(gv->type.spec)) {
                    long long total_offset = 0;
                    TypeSpec cur_ts = gv->type.spec;
                    bool ok = true;
                    for (size_t field_idx = 0; field_idx < field_path.size(); ++field_idx) {
                      const std::string& fname = field_path[field_idx];
                      const auto* sd = lookup_const_init_struct_def(cur_ts);
                      if (!sd) {
                        ok = false;
                        break;
                      }
                      bool found = false;
                      for (const auto& f : sd->fields) {
                        if (f.name == fname) {
                          total_offset += f.offset_bytes;
                          TypeSpec ft = f.elem_type;
                          ft.inner_rank = -1;
                          if (field_idx + 1 < field_path.size() && is_named_aggregate_value(ft)) {
                            cur_ts = ft;
                          } else if (field_idx + 1 < field_path.size()) {
                            ok = false;
                          }
                          found = true;
                          break;
                        }
                      }
                      if (!found) {
                        ok = false;
                        break;
                      }
                    }
                    if (ok) {
                      const std::string global_name =
                          emitted_link_name(mod_, gv->link_name_id, gv->name);
                      if (total_offset == 0) return "@" + global_name;
                      return "getelementptr (i8, ptr @" + global_name +
                             ", i64 " + std::to_string(total_offset) + ")";
                    }
                  }
                }
              }
            }
            const Expr& base_e = get_expr(mem_e->base);
            if (const auto* dr = std::get_if<DeclRef>(&base_e.payload)) {
              if (const GlobalVar* gv = select_global_object(*dr)) {
                const std::optional<std::string> type_text =
                    aggregate_final_type_text(gv->type.spec);
                if (type_text) {
                  const auto* sd = lookup_const_init_struct_def(gv->type.spec);
                  if (sd) {
                    const int fi = llvm_struct_field_slot_by_name(*sd, mem_e->field);
                    const std::string global_name =
                        emitted_link_name(mod_, gv->link_name_id, gv->name);
                    return "getelementptr inbounds (" + *type_text +
                           ", ptr " + llvm_global_sym(global_name) +
                           ", i32 0, i32 " + std::to_string(fi) + ")";
                  }
                }
              }
            }
            if (const auto* idx_e2 = std::get_if<IndexExpr>(&base_e.payload)) {
              std::vector<long long> indices;
              const Expr* cur_e = &base_e;
              while (const auto* ie = std::get_if<IndexExpr>(&cur_e->payload)) {
                indices.push_back(try_const_eval_int(ie->index).value_or(0));
                cur_e = &get_expr(ie->base);
              }
              std::reverse(indices.begin(), indices.end());
              if (const auto* dr = std::get_if<DeclRef>(&cur_e->payload)) {
                if (const GlobalVar* gv = select_global_object(*dr)) {
                  if (gv && gv->type.spec.array_rank > 0) {
                    TypeSpec elem_ts = gv->type.spec;
                    for (int i = 0; i < (int)indices.size() && elem_ts.array_rank > 0; ++i) {
                      elem_ts.array_rank--;
                      if (elem_ts.array_rank > 0) {
                        for (int j = 0; j < 7; ++j) elem_ts.array_dims[j] = elem_ts.array_dims[j+1];
                        elem_ts.array_dims[7] = -1;
                        elem_ts.array_size = elem_ts.array_dims[0];
                      }
                    }
                    if (const auto* sd = lookup_const_init_struct_def(elem_ts)) {
                      const int fi = llvm_struct_field_slot_by_name(*sd, mem_e->field);
                      const std::string aty = llvm_alloca_ty(gv->type.spec);
                      const std::string global_name =
                          emitted_link_name(mod_, gv->link_name_id, gv->name);
                      std::string gep =
                          "getelementptr inbounds (" + aty + ", ptr @" + global_name + ", i64 0";
                      for (auto idx : indices) gep += ", i64 " + std::to_string(idx);
                      gep += ", i32 " + std::to_string(fi) + ")";
                      return gep;
                    }
                  }
                }
              }
            }
          } else {
            const Expr& base_e = get_expr(mem_e->base);
            if (const auto* dr = std::get_if<DeclRef>(&base_e.payload)) {
              if (const GlobalVar* gv = select_global_object(*dr)) {
                if (gv && gv->type.spec.array_rank > 0) {
                  if (const auto* sd = lookup_const_init_struct_def(gv->type.spec)) {
                    const int fi = llvm_struct_field_slot_by_name(*sd, mem_e->field);
                    const std::string aty = llvm_alloca_ty(gv->type.spec);
                    const std::string global_name =
                        emitted_link_name(mod_, gv->link_name_id, gv->name);
                    return "getelementptr inbounds (" + aty + ", ptr @" + global_name +
                           ", i64 0, i64 0, i32 " + std::to_string(fi) + ")";
                  }
                }
              }
            }
            if (const auto* bin_e = std::get_if<BinaryExpr>(&base_e.payload)) {
              if (bin_e->op == BinaryOp::Add) {
                const Expr& lhs_e = get_expr(bin_e->lhs);
                const Expr& rhs_e = get_expr(bin_e->rhs);
                const DeclRef* dr2 = std::get_if<DeclRef>(&lhs_e.payload);
                auto rhs_val = try_const_eval_int(bin_e->rhs);
                if (!dr2) {
                  dr2 = std::get_if<DeclRef>(&rhs_e.payload);
                  rhs_val = try_const_eval_int(bin_e->lhs);
                }
                if (dr2 && rhs_val) {
                  if (const GlobalVar* gv = select_global_object(*dr2)) {
                    if (gv && gv->type.spec.array_rank > 0) {
                      if (const auto* sd = lookup_const_init_struct_def(gv->type.spec)) {
                        const int fi = llvm_struct_field_slot_by_name(*sd, mem_e->field);
                        const std::string aty = llvm_alloca_ty(gv->type.spec);
                        const std::string global_name =
                            emitted_link_name(mod_, gv->link_name_id, gv->name);
                        return "getelementptr inbounds (" + aty + ", ptr @" + global_name +
                               ", i64 0, i64 " + std::to_string(*rhs_val) +
                               ", i32 " + std::to_string(fi) + ")";
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      if (p.op == UnaryOp::Plus) {
        return emit_const_scalar_expr(p.operand, expected_ts);
      }
      if (p.op == UnaryOp::Minus) {
        const Expr& op_e = get_expr(p.operand);
        if (const auto* i = std::get_if<IntLiteral>(&op_e.payload)) {
          if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0)
            return fp_literal(expected_ts.base, static_cast<double>(-i->value));
          return std::to_string(-i->value);
        }
        if (const auto* f = std::get_if<FloatLiteral>(&op_e.payload)) {
          if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0)
            return fp_literal(expected_ts.base, -f->value);
          return fp_to_hex(-f->value);
        }
      }
      if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0) {
        auto fval = try_const_eval_float(id);
        if (fval) return fp_literal(expected_ts.base, *fval);
      }
      return (llvm_ty(expected_ts) == "ptr") ? "null" : "0";
    } else if constexpr (std::is_same_v<T, CastExpr>) {
      const TypeSpec& cast_ts = p.to_type.spec;
      if (is_any_int(cast_ts.base) && cast_ts.ptr_level == 0 &&
          is_any_int(expected_ts.base) && expected_ts.ptr_level == 0) {
        auto val = try_const_eval_int(p.expr);
        if (val) {
          long long v = *val;
          const int bits = int_bits(cast_ts.base);
          if (bits < 64) {
            unsigned long long mask = (1ULL << bits) - 1;
            unsigned long long uv = static_cast<unsigned long long>(v) & mask;
            if (is_signed_int(cast_ts.base) && (uv >> (bits - 1))) {
              v = static_cast<long long>(uv | ~mask);
            } else {
              v = static_cast<long long>(uv);
            }
          }
          if (is_float_base(expected_ts.base))
            return fp_literal(expected_ts.base, static_cast<double>(v));
          return std::to_string(v);
        }
      }
      return emit_const_scalar_expr(p.expr, expected_ts);
    } else if constexpr (std::is_same_v<T, BinaryExpr>) {
      if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0) {
        auto fval = try_const_eval_float(id);
        if (fval) return fp_literal(expected_ts.base, *fval);
      }
      auto val = try_const_eval_int(id);
      if (val) {
        if (llvm_ty(expected_ts) == "ptr") {
          if (*val == 0) return "null";
          return "inttoptr (i64 " + std::to_string(*val) + " to ptr)";
        }
        return std::to_string(*val);
      }
      if (llvm_ty(expected_ts) == "ptr" && p.op == BinaryOp::Add) {
        const Expr& lhs_e = get_expr(p.lhs);
        const Expr& rhs_e = get_expr(p.rhs);
        if (const auto* sl = std::get_if<StringLiteral>(&lhs_e.payload)) {
          auto offset = try_const_eval_int(p.rhs);
          if (offset) {
            const std::string bytes = bytes_from_string_literal(*sl);
            const std::string gname = intern_str(bytes);
            const size_t len = bytes.size() + 1;
            return "getelementptr inbounds ([" + std::to_string(len) + " x i8], ptr " +
                   gname + ", i64 0, i64 " + std::to_string(*offset) + ")";
          }
        }
        const DeclRef* dr = std::get_if<DeclRef>(&lhs_e.payload);
        auto rhs_val = try_const_eval_int(p.rhs);
        if (!dr) {
          dr = std::get_if<DeclRef>(&rhs_e.payload);
          rhs_val = try_const_eval_int(p.lhs);
        }
        if (dr && rhs_val) {
          if (const GlobalVar* gv = select_global_object(*dr)) {
            const std::string global_name =
                emitted_link_name(mod_, gv->link_name_id, gv->name);
            if (gv->type.spec.array_rank > 0) {
              const std::string aty = llvm_alloca_ty(gv->type.spec);
              return "getelementptr inbounds (" + aty + ", ptr @" + global_name +
                     ", i64 0, i64 " + std::to_string(*rhs_val) + ")";
            }
            return "getelementptr inbounds (i8, ptr @" + global_name +
                   ", i64 " + std::to_string(*rhs_val) + ")";
          }
        }
      }
      if (p.op == BinaryOp::Sub || p.op == BinaryOp::Add) {
        auto emit_label_ce = [&](auto& self, ExprId eid) -> std::optional<std::string> {
          const Expr& ex = get_expr(eid);
          return std::visit([&](const auto& q) -> std::optional<std::string> {
            using U = std::decay_t<decltype(q)>;
            if constexpr (std::is_same_v<U, LabelAddrExpr>) {
              return "ptrtoint (ptr blockaddress(@" +
                     resolve_function_link_name(q.fn_link_name_id, q.fn_name) +
                     ", %ulbl_" + q.label_name + ") to i64)";
            } else if constexpr (std::is_same_v<U, CastExpr>) {
              return self(self, q.expr);
            } else if constexpr (std::is_same_v<U, BinaryExpr>) {
              auto lstr = self(self, q.lhs);
              auto rstr = self(self, q.rhs);
              if (!lstr || !rstr) return std::nullopt;
              const char* opname = (q.op == BinaryOp::Sub) ? "sub" : "add";
              return std::string(opname) + " (i64 " + *lstr + ", i64 " + *rstr + ")";
            } else if constexpr (std::is_same_v<U, IntLiteral>) {
              return std::to_string(q.value);
            } else {
              return std::nullopt;
            }
          }, ex.payload);
        };
        auto lstr = emit_label_ce(emit_label_ce, p.lhs);
        auto rstr = emit_label_ce(emit_label_ce, p.rhs);
        if (lstr && rstr) {
          const char* opname = (p.op == BinaryOp::Sub) ? "sub" : "add";
          std::string ce = std::string(opname) + " (i64 " + *lstr + ", i64 " + *rstr + ")";
          const std::string tgt_ty = llvm_ty(expected_ts);
          if (tgt_ty != "i64" && tgt_ty != "ptr") {
            ce = "trunc (i64 " + ce + " to " + tgt_ty + ")";
          }
          return ce;
        }
      }
      return (llvm_ty(expected_ts) == "ptr") ? "null" : "0";
    } else if constexpr (std::is_same_v<T, LabelAddrExpr>) {
      return "blockaddress(@" + resolve_function_link_name(p.fn_link_name_id, p.fn_name) +
             ", %ulbl_" + p.label_name + ")";
    } else if constexpr (std::is_same_v<T, CallExpr>) {
      const BuiltinId builtin_id = p.builtin_id;
      switch (builtin_constant_fp_kind(builtin_id)) {
        case BuiltinConstantFpKind::Infinity:
          if (builtin_id == BuiltinId::HugeValF || builtin_id == BuiltinId::InfF) {
            if (expected_ts.base == TB_FLOAT && expected_ts.ptr_level == 0)
              return fp_to_float_literal(std::numeric_limits<float>::infinity());
            return fp_to_hex(static_cast<double>(std::numeric_limits<float>::infinity()));
          }
          return fp_literal(expected_ts.base, std::numeric_limits<double>::infinity());
        case BuiltinConstantFpKind::QuietNaN:
          if (builtin_id == BuiltinId::NanF || builtin_id == BuiltinId::NansF) {
            if (expected_ts.base == TB_FLOAT && expected_ts.ptr_level == 0)
              return fp_to_float_literal(std::numeric_limits<float>::quiet_NaN());
            return fp_to_hex(static_cast<double>(std::numeric_limits<float>::quiet_NaN()));
          }
          return fp_literal(expected_ts.base, std::numeric_limits<double>::quiet_NaN());
        case BuiltinConstantFpKind::None:
          break;
      }
      return (llvm_ty(expected_ts) == "ptr") ? "null" : "0";
    } else if constexpr (std::is_same_v<T, SizeofTypeExpr>) {
      return std::to_string(sizeof_ts(mod_, p.type.spec));
    } else if constexpr (std::is_same_v<T, SizeofExpr>) {
      auto val = try_const_eval_int(id);
      if (val) return std::to_string(*val);
      return "0";
    } else {
      auto val = try_const_eval_int(id);
      if (val) return std::to_string(*val);
      return (llvm_ty(expected_ts) == "ptr") ? "null" : "0";
    }
  }, e.payload);
}

// ── Aggregate init helpers ────────────────────────────────────────────────────

GlobalInit ConstInitEmitter::child_init_of(const InitListItem& item) const {
  return std::visit([&](const auto& v) -> GlobalInit {
    using V = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<V, InitScalar>) return GlobalInit(v);
    else return GlobalInit(*v);
  }, item.value);
}

std::string ConstInitEmitter::emit_const_vector(const TypeSpec& ts, const GlobalInit& init) {
  TypeSpec elem_ts = ts;
  elem_ts.is_vector = false;
  elem_ts.vector_lanes = 0;
  elem_ts.vector_bytes = 0;
  const long long n = ts.vector_lanes > 0 ? ts.vector_lanes : 1;
  std::vector<std::string> elems(static_cast<size_t>(n),
                                 emit_const_init(elem_ts, GlobalInit(std::monostate{})));
  if (const auto* scalar = std::get_if<InitScalar>(&init)) {
    if (n > 0) elems[0] = emit_const_scalar_expr(scalar->expr, elem_ts);
  } else if (const auto* list = std::get_if<InitList>(&init)) {
    for (size_t i = 0; i < list->items.size() && i < elems.size(); ++i) {
      elems[i] = emit_const_init(elem_ts, child_init_of(list->items[i]));
    }
  }
  std::string out = "<";
  for (size_t i = 0; i < elems.size(); ++i) {
    if (i) out += ", ";
    out += llvm_ty(elem_ts) + " " + elems[i];
  }
  out += ">";
  return out;
}

std::string ConstInitEmitter::emit_const_array(const TypeSpec& ts, const GlobalInit& init) {
  const long long n = ts.array_size;
  if (n <= 0) return "zeroinitializer";
  TypeSpec elem_ts = drop_one_array_dim(ts);

  if (const auto* scalar = std::get_if<InitScalar>(&init)) {
    const Expr& e = get_expr(scalar->expr);
    if (const auto* sl = std::get_if<StringLiteral>(&e.payload);
        sl && ts.array_rank == 1 && is_char_like(elem_ts.base) && elem_ts.ptr_level == 0) {
      std::string bytes = bytes_from_string_literal(*sl);
      if ((long long)bytes.size() > n) bytes.resize(static_cast<size_t>(n));
      if ((long long)bytes.size() < n) bytes.resize(static_cast<size_t>(n), '\0');
      return "c\"" + escape_llvm_c_bytes(bytes) + "\"";
    }
    if (const auto* sl = std::get_if<StringLiteral>(&e.payload);
        sl && sl->is_wide && ts.array_rank == 1 && elem_ts.ptr_level == 0) {
      std::vector<long long> vals = decode_wide_string_values(sl->raw);
      if ((long long)vals.size() > n) vals.resize(static_cast<size_t>(n));
      while ((long long)vals.size() < n) vals.push_back(0);
      std::string out = "[";
      const std::string elem_ty = llvm_alloca_ty(elem_ts);
      for (size_t i = 0; i < vals.size(); ++i) {
        if (i) out += ", ";
        out += elem_ty + " " + std::to_string(vals[i]);
      }
      out += "]";
      return out;
    }
    return format_array_literal(elem_ts, std::vector<std::string>(static_cast<size_t>(n), "zeroinitializer"));
  }

  std::vector<std::string> elems(static_cast<size_t>(n), "zeroinitializer");
  if (const auto* list = std::get_if<InitList>(&init)) {
    if (!is_indexed_list(*list)) return format_array_literal(elem_ts, elems);
    long long next_idx = 0;
    const bool elem_is_scalar_ptr = (elem_ts.ptr_level > 0 && outer_array_rank(elem_ts) == 0);
    for (const auto& item : list->items) {
      const auto maybe_idx = find_array_index(item, next_idx, n);
      if (!maybe_idx) continue;
      size_t idx = static_cast<size_t>(*maybe_idx);
      GlobalInit child = child_init_of(item);
      if (elem_is_scalar_ptr) {
        std::function<void(const GlobalInit&, size_t&)> collect_scalars;
        collect_scalars = [&](const GlobalInit& gi, size_t& out_idx) {
          if (const auto* s = std::get_if<InitScalar>(&gi)) {
            if (out_idx < static_cast<size_t>(n))
              elems[out_idx++] = emit_const_init(elem_ts, gi);
          } else if (const auto* cl = std::get_if<InitList>(&gi)) {
            for (const auto& inner_item : cl->items)
              collect_scalars(child_init_of(inner_item), out_idx);
          }
        };
        if (std::holds_alternative<InitList>(child)) {
          collect_scalars(child, idx);
          next_idx = static_cast<long long>(idx);
          continue;
        }
      }
      elems[idx] = emit_const_init(elem_ts, child);
      next_idx = *maybe_idx + 1;
    }
  }
  return format_array_literal(elem_ts, elems);
}

std::optional<std::string> ConstInitEmitter::try_emit_ptr_from_char_init(const GlobalInit& init) {
  auto make_ptr_to_bytes = [&](const std::string& bytes) -> std::string {
    const std::string gname = intern_str(bytes);
    const size_t len = bytes.size() + 1;
    return "getelementptr inbounds ([" + std::to_string(len) + " x i8], ptr " +
           gname + ", i64 0, i64 0)";
  };

  if (const auto* scalar = std::get_if<InitScalar>(&init)) {
    const Expr& e = get_expr(scalar->expr);
    if (const auto* sl = std::get_if<StringLiteral>(&e.payload)) {
      return make_ptr_to_bytes(bytes_from_string_literal(*sl));
    }
    return std::nullopt;
  }

  const auto* list = std::get_if<InitList>(&init);
  if (!list) return std::nullopt;

  std::string bytes;
  long long next_idx = 0;
  for (const auto& item : list->items) {
    const auto maybe_idx = find_array_index(item, next_idx, -1);
    if (!maybe_idx) return std::nullopt;
    const size_t idx = static_cast<size_t>(*maybe_idx);
    if (idx > bytes.size()) bytes.resize(idx, '\0');

    GlobalInit child_init = child_init_of(item);
    const auto* child_scalar = std::get_if<InitScalar>(&child_init);
    if (!child_scalar) return std::nullopt;
    const Expr& ce = get_expr(child_scalar->expr);
    int ch = 0;
    if (const auto* c = std::get_if<CharLiteral>(&ce.payload)) ch = c->value;
    else if (const auto* i = std::get_if<IntLiteral>(&ce.payload)) ch = static_cast<int>(i->value);
    else return std::nullopt;

    if (idx == bytes.size()) bytes.push_back(static_cast<char>(ch & 0xFF));
    else bytes[idx] = static_cast<char>(ch & 0xFF);
    next_idx = *maybe_idx + 1;
  }
  return make_ptr_to_bytes(bytes);
}

TypeSpec ConstInitEmitter::resolve_flexible_array_field_ts(const HirStructField& f,
                                                           const InitListItem* item,
                                                           const GlobalInit& init) {
  TypeSpec ts = field_decl_type(f);
  if (!f.is_flexible_array) return ts;
  long long deduced = -1;
  if (item && item->resolved_array_bound) deduced = *item->resolved_array_bound;
  if (deduced <= 0) deduced = deduce_array_size_from_init(init);
  if (deduced <= 0) return ts;
  ts.array_rank = 1;
  ts.array_size = deduced;
  ts.array_dims[0] = deduced;
  return ts;
}

std::optional<size_t> ConstInitEmitter::find_union_field_index(const HirStructDef& union_sd,
                                                               const InitListItem& item) const {
  const std::string_view designator =
      init_list_field_designator_text(item, mod_.link_name_texts.get());
  if (!designator.empty()) {
    const auto fit = std::find_if(
        union_sd.fields.begin(), union_sd.fields.end(),
        [&](const HirStructField& f) { return f.name == designator; });
    if (fit == union_sd.fields.end()) return std::nullopt;
    return static_cast<size_t>(std::distance(union_sd.fields.begin(), fit));
  }
  if (item.index_designator && *item.index_designator >= 0) {
    const size_t idx = static_cast<size_t>(*item.index_designator);
    if (idx < union_sd.fields.size()) return idx;
  }
  return std::nullopt;
}

std::optional<long long> ConstInitEmitter::find_array_index(const InitListItem& item,
                                                             long long next_idx,
                                                             long long bound) const {
  long long idx = next_idx;
  if (item.index_designator && *item.index_designator >= 0) idx = *item.index_designator;
  if (idx < 0) return std::nullopt;
  if (bound >= 0 && idx >= bound) return std::nullopt;
  return idx;
}

bool ConstInitEmitter::is_explicitly_mapped_item(const InitListItem& item) const {
  return init_list_has_field_designator(item) || item.index_designator.has_value();
}

bool ConstInitEmitter::is_explicitly_mapped_list(const InitList& list) const {
  return std::all_of(list.items.begin(), list.items.end(), [&](const InitListItem& item) {
    return is_explicitly_mapped_item(item);
  });
}

bool ConstInitEmitter::is_indexed_list(const InitList& list) const {
  return std::all_of(list.items.begin(), list.items.end(), [](const InitListItem& item) {
    return item.index_designator.has_value() && *item.index_designator >= 0;
  });
}

std::optional<size_t> ConstInitEmitter::find_struct_field_index(const HirStructDef& sd,
                                                                const InitListItem& item,
                                                                size_t next_idx) const {
  size_t idx = next_idx;
  const std::string_view designator =
      init_list_field_designator_text(item, mod_.link_name_texts.get());
  if (!designator.empty()) {
    const auto fit = std::find_if(
        sd.fields.begin(), sd.fields.end(),
        [&](const HirStructField& f) { return f.name == designator; });
    if (fit == sd.fields.end()) return std::nullopt;
    idx = static_cast<size_t>(std::distance(sd.fields.begin(), fit));
  } else if (item.index_designator && *item.index_designator >= 0) {
    idx = static_cast<size_t>(*item.index_designator);
  }
  if (idx >= sd.fields.size()) return std::nullopt;
  return idx;
}

std::optional<std::pair<size_t, GlobalInit>> ConstInitEmitter::try_select_canonical_union_field_init(
    const HirStructDef& union_sd, const GlobalInit& union_init) const {
  if (const auto* list = std::get_if<InitList>(&union_init)) {
    if (list->items.empty()) return std::nullopt;
    const auto& item0 = list->items.front();
    const auto maybe_idx = find_union_field_index(union_sd, item0);
    if (maybe_idx) return std::pair<size_t, GlobalInit>(*maybe_idx, child_init_of(item0));
  }
  return std::nullopt;
}

std::vector<std::string> ConstInitEmitter::emit_const_struct_fields_impl(const TypeSpec&,
                                                                          const HirStructDef& sd,
                                                                          const GlobalInit& init,
                                                                          std::vector<TypeSpec>* out_field_types) {
  std::vector<TypeSpec> field_types;
  field_types.reserve(sd.fields.size());
  for (const auto& f : sd.fields) field_types.push_back(field_decl_type(f));

  std::vector<std::string> field_vals;
  field_vals.reserve(sd.fields.size());
  for (size_t i = 0; i < sd.fields.size(); ++i) {
    field_vals.push_back(emit_const_init(field_types[i], GlobalInit(std::monostate{})));
  }
  auto update_field_type =
      [&](size_t idx, const InitListItem* item, const GlobalInit& child_init) -> const TypeSpec& {
    if (idx + 1 == sd.fields.size() && sd.fields[idx].is_flexible_array) {
      field_types[idx] = resolve_flexible_array_field_ts(sd.fields[idx], item, child_init);
    }
    return field_types[idx];
  };
  auto emit_field_mapped_item = [&](size_t idx, const InitListItem& item) {
    GlobalInit child_init = child_init_of(item);
    const TypeSpec& field_ts = update_field_type(idx, &item, child_init);
    if (llvm_field_ty(sd.fields[idx]) == "ptr") {
      if (auto ptr_init = try_emit_ptr_from_char_init(child_init)) {
        field_vals[idx] = *ptr_init;
        return;
      }
    }
    field_vals[idx] = emit_const_init(field_ts, child_init);
  };

  if (const auto* list = std::get_if<InitList>(&init)) {
    if (!is_explicitly_mapped_list(*list)) {
      if (out_field_types) *out_field_types = std::move(field_types);
      return field_vals;
    }
    size_t next_idx = 0;
    for (const auto& item : list->items) {
      const auto maybe_idx = find_struct_field_index(sd, item, next_idx);
      if (!maybe_idx) continue;
      const size_t idx = *maybe_idx;
      emit_field_mapped_item(idx, item);
      next_idx = idx + 1;
    }
  }

  if (out_field_types) *out_field_types = std::move(field_types);
  return field_vals;
}

std::string ConstInitEmitter::emit_const_struct(const TypeSpec& ts, const GlobalInit& init) {
  const HirStructDef* sd_ptr = lookup_const_init_struct_def(ts);
  if (!sd_ptr) return "zeroinitializer";
  const HirStructDef& sd = *sd_ptr;
  if (sd.is_union) return emit_const_union(ts, sd, init);
  return format_struct_literal(sd, emit_const_struct_fields_impl(ts, sd, init, nullptr));
}

std::string ConstInitEmitter::emit_const_union(const TypeSpec& ts, const HirStructDef& sd,
                                                const GlobalInit& init) {
  if (sd.fields.empty()) return "zeroinitializer";
  const int sz = sd.size_bytes;
  auto zero_union = [&]() -> std::string {
    if (sz == 0) return "zeroinitializer";
    const char* open = sd.pack_align > 0 ? "<{ " : "{ ";
    const char* close = sd.pack_align > 0 ? " }>" : " }";
    return std::string(open) + "[" + std::to_string(sz) + " x i8] zeroinitializer" +
           close;
  };
  auto copy_bytes = [](std::vector<unsigned char>& dst, size_t dst_off,
                       const std::vector<unsigned char>& src, size_t max_copy) -> void {
    if (dst_off >= dst.size() || src.empty() || max_copy == 0) return;
    const size_t copy_n = std::min({dst.size() - dst_off, src.size(), max_copy});
    if (copy_n == 0) return;
    std::memcpy(dst.data() + dst_off, src.data(), copy_n);
  };

  std::function<bool(const TypeSpec&, const GlobalInit&, std::vector<unsigned char>&)> encode_bytes;
  encode_bytes = [&](const TypeSpec& cur_ts, const GlobalInit& cur_init,
                     std::vector<unsigned char>& out) -> bool {
    const int cur_sz = std::max(1, sizeof_ts(mod_, cur_ts));
    out.assign(static_cast<size_t>(cur_sz), 0);
    if (cur_ts.ptr_level > 0 || cur_ts.is_fn_ptr) return false;
    const bool is_aggregate_target =
        cur_ts.array_rank > 0 ||
        ((cur_ts.base == TB_STRUCT || cur_ts.base == TB_UNION) && cur_ts.ptr_level == 0);
    if (!is_aggregate_target && std::holds_alternative<InitList>(cur_init)) return true;

    if (cur_ts.array_rank > 0) {
      const long long n = cur_ts.array_size;
      if (n <= 0) return true;
      TypeSpec elem_ts = drop_one_array_dim(cur_ts);
      const int elem_sz = std::max(1, sizeof_ts(mod_, elem_ts));
      if (const auto* scalar = std::get_if<InitScalar>(&cur_init)) {
        const Expr& e = get_expr(scalar->expr);
        if (const auto* sl = std::get_if<StringLiteral>(&e.payload)) {
          if (elem_ts.ptr_level == 0 && is_char_like(elem_ts.base)) {
            const std::string bytes = bytes_from_string_literal(*sl);
            const size_t max_n = static_cast<size_t>(n);
            for (size_t i = 0; i < max_n && i < bytes.size(); ++i) {
              out[i * static_cast<size_t>(elem_sz)] = static_cast<unsigned char>(bytes[i]);
            }
            if (bytes.size() < max_n) {
              out[bytes.size() * static_cast<size_t>(elem_sz)] = 0;
            }
            return true;
          }
        }
        return true;
      }
      if (const auto* list = std::get_if<InitList>(&cur_init)) {
        if (!is_indexed_list(*list)) return true;
        long long next_idx = 0;
        for (const auto& item : list->items) {
          const auto maybe_idx = find_array_index(item, next_idx, n);
          if (!maybe_idx) continue;
          std::vector<unsigned char> elem;
          if (encode_bytes(elem_ts, child_init_of(item), elem)) {
            const size_t off = static_cast<size_t>(*maybe_idx) * static_cast<size_t>(elem_sz);
            copy_bytes(out, off, elem, static_cast<size_t>(elem_sz));
          }
          next_idx = *maybe_idx + 1;
        }
        return true;
      }
      return true;
    }

    if (is_named_aggregate_value(cur_ts)) {
      const HirStructDef* cur_sd_ptr = lookup_const_init_struct_def(cur_ts);
      if (!cur_sd_ptr) return false;
      const HirStructDef& cur_sd = *cur_sd_ptr;
      if (cur_sd.is_union) {
        if (const auto canonical = try_select_canonical_union_field_init(cur_sd, cur_init)) {
          if (canonical->first >= cur_sd.fields.size()) return true;
          std::vector<unsigned char> fb;
          if (!encode_bytes(field_decl_type(cur_sd.fields[canonical->first]), canonical->second, fb))
            return false;
          copy_bytes(out, 0, fb, out.size());
          return true;
        }
        return true;
      }

      if (const auto* list = std::get_if<InitList>(&cur_init)) {
        if (!is_explicitly_mapped_list(*list)) return true;
        size_t next_field = 0;
        for (const auto& item : list->items) {
          const auto maybe_field = find_struct_field_index(cur_sd, item, next_field);
          if (!maybe_field) continue;
          const size_t fi = *maybe_field;
          std::vector<unsigned char> fb;
          if (encode_bytes(field_decl_type(cur_sd.fields[fi]), child_init_of(item), fb)) {
            const size_t off = static_cast<size_t>(cur_sd.fields[fi].offset_bytes);
            copy_bytes(out, off, fb, out.size() - off);
          }
          next_field = fi + 1;
        }
        return true;
      }
      return true;
    }

    if (const auto* scalar = std::get_if<InitScalar>(&cur_init)) {
      const Expr& e = get_expr(scalar->expr);
      unsigned long long v = 0;
      if (const auto* il = std::get_if<IntLiteral>(&e.payload)) v = static_cast<unsigned long long>(il->value);
      else if (const auto* cl = std::get_if<CharLiteral>(&e.payload)) v = static_cast<unsigned long long>(cl->value);
      else return false;
      for (int i = 0; i < cur_sz; ++i) {
        out[static_cast<size_t>(i)] = static_cast<unsigned char>((v >> (8 * i)) & 0xFF);
      }
      return true;
    }
    return true;
  };

  auto emit_union_from_field =
      [&](size_t field_idx, const GlobalInit& child_init) -> std::optional<std::string> {
    if (field_idx >= sd.fields.size()) return std::nullopt;
    std::vector<unsigned char> bytes(static_cast<size_t>(sz), 0);
    std::vector<unsigned char> field_bytes;
    if (!encode_bytes(field_decl_type(sd.fields[field_idx]), child_init, field_bytes)) {
      return std::nullopt;
    }
    if (field_bytes.empty()) return std::nullopt;
    copy_bytes(bytes, 0, field_bytes, bytes.size());
    const bool all_zero = std::all_of(bytes.begin(), bytes.end(), [](unsigned char b) { return b == 0; });
    if (all_zero) return zero_union();
    std::string arr = "[";
    for (size_t i = 0; i < bytes.size(); ++i) {
      if (i) arr += ", ";
      arr += "i8 " + std::to_string(static_cast<unsigned int>(bytes[i]));
    }
    arr += "]";
    const char* open = sd.pack_align > 0 ? "<{ " : "{ ";
    const char* close = sd.pack_align > 0 ? " }>" : " }";
    return std::string(open) + "[" + std::to_string(sz) + " x i8] " + arr + close;
  };

  if (const auto selected = try_select_canonical_union_field_init(sd, init)) {
    if (auto out = emit_union_from_field(selected->first, selected->second)) return *out;
  }
  return zero_union();
}

std::string ConstInitEmitter::format_array_literal(const TypeSpec& elem_ts,
                                                    const std::vector<std::string>& elems) const {
  std::string out = "[";
  const std::string elem_ty = llvm_alloca_ty(elem_ts);
  const bool elem_is_scalar = elem_ts.array_rank == 0 && elem_ts.ptr_level == 0 &&
                              !elem_ts.is_fn_ptr && elem_ts.base != TB_STRUCT &&
                              elem_ts.base != TB_UNION && elem_ts.base != TB_VA_LIST &&
                              !elem_ts.is_vector && !is_complex_base(elem_ts.base);
  const std::string zero_val = elem_is_scalar
      ? (is_float_base(elem_ts.base) ? "0.0" : "0")
      : "zeroinitializer";
  for (size_t i = 0; i < elems.size(); ++i) {
    if (i) out += ", ";
    const std::string& val = (elems[i] == "zeroinitializer" && elem_is_scalar) ? zero_val : elems[i];
    out += elem_ty + " " + val;
  }
  out += "]";
  return out;
}

std::string ConstInitEmitter::format_struct_literal(const HirStructDef& sd,
                                                     const std::vector<std::string>& field_vals) const {
  std::string out = sd.pack_align > 0 ? "<{ " : "{ ";
  bool first = true;
  int cur_offset = 0;
  int last_idx = -1;
  for (size_t i = 0; i < sd.fields.size();) {
    const auto& field = sd.fields[i];
    if (field.llvm_idx == last_idx) {
      ++i;
      continue;
    }
    last_idx = field.llvm_idx;
    if (field.offset_bytes > cur_offset) {
      if (!first) out += ", ";
      first = false;
      out += "[" + std::to_string(field.offset_bytes - cur_offset) +
             " x i8] zeroinitializer";
      cur_offset = field.offset_bytes;
    }
    if (!first) out += ", ";
    first = false;
    out += llvm_field_ty(field) + " ";
    if (field.bit_width < 0) {
      out += field_vals[i];
      cur_offset = field.offset_bytes + std::max(0, field.size_bytes);
      ++i;
      continue;
    }
    unsigned long long packed = 0;
    int storage_size = std::max(0, field.size_bytes);
    while (i < sd.fields.size() && sd.fields[i].llvm_idx == last_idx) {
      const auto& bf = sd.fields[i];
      if (bf.bit_width > 0) {
        long long val = 0;
        try { val = std::stoll(field_vals[i]); } catch (...) {}
        const unsigned long long mask =
            (bf.bit_width >= 64) ? ~0ULL : ((1ULL << bf.bit_width) - 1);
        packed |= (static_cast<unsigned long long>(val) & mask) << bf.bit_offset;
      }
      storage_size = std::max(storage_size, bf.size_bytes);
      ++i;
    }
    out += std::to_string(static_cast<long long>(packed));
    cur_offset = field.offset_bytes + storage_size;
  }
  if (sd.size_bytes > cur_offset) {
    if (!first) out += ", ";
    out += "[" + std::to_string(sd.size_bytes - cur_offset) + " x i8] zeroinitializer";
  }
  out += sd.pack_align > 0 ? " }>" : " }";
  return out;
}

long long ConstInitEmitter::deduce_array_size_from_init(const GlobalInit& init) const {
  if (const auto* list = std::get_if<InitList>(&init)) {
    long long max_idx = -1;
    long long next = 0;
    for (const auto& item : list->items) {
      long long idx = next;
      if (item.index_designator && *item.index_designator >= 0) idx = *item.index_designator;
      if (idx > max_idx) max_idx = idx;
      next = idx + 1;
    }
    return max_idx + 1;
  }
  if (const auto* scalar = std::get_if<InitScalar>(&init)) {
    const Expr& e = get_expr(scalar->expr);
    if (const auto* sl = std::get_if<StringLiteral>(&e.payload)) {
      return static_cast<long long>(bytes_from_string_literal(*sl).size()) + 1;
    }
  }
  return -1;
}

// ── Public entry points ───────────────────────────────────────────────────────

std::string ConstInitEmitter::emit_const_init(const TypeSpec& ts, const GlobalInit& init) {
  if (ts.array_rank > 0 && !(ts.ptr_level > 0 && outer_array_rank(ts) == 0))
    return emit_const_array(ts, init);
  if (ts.is_vector && ts.vector_lanes > 0) return emit_const_vector(ts, init);
  if (ts.base == TB_VA_LIST && ts.ptr_level == 0) return "zeroinitializer";
  if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0)
    return emit_const_struct(ts, init);
  if (const auto* s = std::get_if<InitScalar>(&init))
    return emit_const_scalar_expr(s->expr, ts);
  return (llvm_ty(ts) == "ptr") ? "null" : "zeroinitializer";
}

std::vector<std::string> ConstInitEmitter::emit_const_struct_fields(
    const TypeSpec& ts,
    const HirStructDef& sd,
    const GlobalInit& init,
    std::vector<TypeSpec>* out_field_types) {
  return emit_const_struct_fields_impl(ts, sd, init, out_field_types);
}

}  // namespace c4c::codegen::lir
