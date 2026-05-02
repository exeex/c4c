#pragma once

// AST node definitions for the tiny-c2ll C++ frontend.
//
// Design:
//  - Tagged-union style: NodeKind enum + flat Node struct.
//  - All string data stored as const char* into an Arena (no heap allocation
//    per node).  Directly backportable to C.
//  - Child pointers are raw Node* or Node** (arena-allocated arrays).
//  - TypeSpec is a flat struct covering all C type specifier combinations.
//
// Pure-C backport note: this header can be #included from C after removing
// the namespace wrapper, the class keyword, and default member initializers.

#include <cctype>
#include <cstddef>
#include <cstring>
#include <string>

#include "../builtin.hpp"
#include "../../shared/text_id_table.hpp"

namespace c4c {

enum class ExecutionDomain : uint8_t {
    Host = 0,
    Device = 1,
    HostDevice = 2,
};

// Forward declaration
struct Node;

// ── TypeSpec ──────────────────────────────────────────────────────────────────
// Represents a C type, including qualifiers, pointer levels, array bounds,
// and struct/union/enum tags.
//
// Pure-C backport note: plain struct, no changes needed.

enum TypeBase {
    TB_VOID,
    TB_CHAR,        // char (signed)
    TB_UCHAR,       // unsigned char
    TB_SCHAR,       // signed char
    TB_SHORT,       // short (signed)
    TB_USHORT,      // unsigned short
    TB_INT,         // int (signed)
    TB_UINT,        // unsigned int
    TB_LONG,        // long (signed)
    TB_ULONG,       // unsigned long
    TB_LONGLONG,    // long long (signed)
    TB_ULONGLONG,   // unsigned long long
    TB_FLOAT,       // float
    TB_DOUBLE,      // double
    TB_LONGDOUBLE,  // long double
    TB_BOOL,        // _Bool
    TB_STRUCT,      // struct
    TB_UNION,       // union
    TB_ENUM,        // enum
    TB_TYPEDEF,     // typedef name
    TB_INT128,      // __int128
    TB_UINT128,     // __uint128_t
    TB_VA_LIST,     // __builtin_va_list / __va_list
    TB_FUNC_PTR,    // function pointer type (from cast/sizeof)
    TB_COMPLEX_FLOAT,      // __complex__ float / float _Complex
    TB_COMPLEX_DOUBLE,     // __complex__ double / double _Complex
    TB_COMPLEX_LONGDOUBLE, // __complex__ long double
    // GCC extension: _Complex applied to integer types
    TB_COMPLEX_CHAR,       // _Complex char (signed)
    TB_COMPLEX_SCHAR,      // _Complex signed char
    TB_COMPLEX_UCHAR,      // _Complex unsigned char
    TB_COMPLEX_SHORT,      // _Complex short
    TB_COMPLEX_USHORT,     // _Complex unsigned short
    TB_COMPLEX_INT,        // _Complex int
    TB_COMPLEX_UINT,       // _Complex unsigned int
    TB_COMPLEX_LONG,       // _Complex long
    TB_COMPLEX_ULONG,      // _Complex unsigned long
    TB_COMPLEX_LONGLONG,   // _Complex long long
    TB_COMPLEX_ULONGLONG,  // _Complex unsigned long long
    TB_AUTO,               // C++ auto type deduction placeholder
};

enum class TemplateArgKind : uint8_t {
    Type = 0,
    Value = 1,
};

struct TemplateArgRef;

struct TemplateArgRefList {
    TemplateArgRef* data;
    int size;
};

struct TypeSpec {
    // Cross-stage type contract: semantic type identity consumed after parsing.
    TypeBase base;
    TypeBase enum_underlying_base; // fixed underlying base for TB_ENUM, or TB_VOID when unknown/default

    // Compatibility/display spelling: source names that may still bridge
    // parser, Sema, HIR, and diagnostics where no TextId-backed authority
    // exists yet.
    const char* tag;         // struct/union/enum tag or typedef name (may be null)
    TextId tag_text_id;      // parser-owned identity for tag/typedef names when available
    int template_param_owner_namespace_context_id = -1; // owning template namespace for type params
    TextId template_param_owner_text_id = kInvalidText; // owning function/template TextId for type params
    int template_param_index = -1; // owning template parameter index, or -1 when not a type param
    TextId template_param_text_id = kInvalidText; // parser-owned identity for the template parameter

    // Cross-stage type contract: parser-owned record identity for structured
    // type names.
    Node* record_def;        // concrete parser-owned NK_STRUCT_DEF for struct/union types, or null

    // Compatibility/display spelling: qualified source name path attached to
    // tagged/typedef names until all consumers use structured identity.
    const char** qualifier_segments; // structured qualifier path for tagged/typedef names
    TextId* qualifier_text_ids;       // parser-owned text identity for qualifier_segments
    int n_qualifier_segments;        // qualifier segment count (excludes base name)
    bool is_global_qualified;        // true when the source type started with ::
    int namespace_context_id = -1;    // resolved namespace context for qualified type names

    // Cross-stage type contract: declarator shape attached to the base type.
    int ptr_level;           // 0 = not a pointer; 1 = *; 2 = **; ...
    bool is_lvalue_ref;      // true for C++ lvalue references (T&)
    bool is_rvalue_ref;      // true for C++ rvalue references (T&&)

    // Parser-produced layout hint from parsed attributes.
    int align_bytes;         // explicit alignment from __attribute__((aligned)) / aligned(N)

    // Cross-stage type contract: array/function-pointer declarator shape.
    long long array_size;    // -1 = not array; -2 = [] (unsized); >= 0 = size
    int array_rank;          // number of array dimensions (0 = not array)
    long long array_dims[8]; // outer-to-inner dimensions; each is -2 (unsized) or >= 0
    bool is_ptr_to_array;    // true for declarators like (*p)[N] (pointer outside array)
    int  inner_rank;         // >= 0: array_dims split: inner_rank trailing dims belong to
                             // pointed-to typedef array; outer (array_rank-inner_rank) are
                             // declarator dims → llvm_ty generates [outer x ptr]

    // Parser-produced layout and lowering hints; later stages should treat
    // these as parsed attributes, not independent type identity.
    bool is_vector;          // GCC vector type from __attribute__((vector_size(N)))
    long long vector_lanes;  // number of vector elements when is_vector=true
    long long vector_bytes;  // total storage size in bytes when is_vector=true

    // Debug/recovery payload: preserves the unevaluated bound expression when
    // the array size is not a resolved integer at parse time.
    Node* array_size_expr;   // non-null when array size is a computed expression

    // Parser-produced qualifier, declarator, and attribute hints.
    bool is_const;
    bool is_volatile;
    bool is_fn_ptr;      // true when this TypeSpec was derived from a (*)(params) declarator
    bool is_packed;      // __attribute__((packed)) — struct layout uses alignment 1
    bool is_noinline;    // __attribute__((noinline))
    bool is_always_inline; // __attribute__((always_inline))

    // Cross-stage contract payload for unresolved template struct
    // instantiation. Non-null when this struct type depends on unresolved
    // template type params (e.g., Pair<T> inside a template function body).
    // The HIR resolves it during instantiation.
    const char* tpl_struct_origin;     // original template struct name (e.g., "Pair")
    TemplateArgRefList tpl_struct_args; // structured template arg payload

    // Legacy bridge spelling for dependent member typedefs; this is a pending
    // `StructLike::type`-style name until a structured member-type reference
    // replaces the string handoff.
    const char* deferred_member_type_name; // pending `StructLike::type`-style member typedef
    TextId deferred_member_type_text_id =
        kInvalidText; // parser-owned identity for deferred_member_type_name
};

struct TemplateArgRef {
    // Cross-stage template argument contract.
    TemplateArgKind kind;
    TypeSpec type;
    long long value;

    // Debug/recovery/display spelling. Helpers may use this to preserve or
    // search original argument text, but structured fields above remain the
    // primary semantic payload.
    const char* debug_text;
};

// ── OperatorKind ─────────────────────────────────────────────────────────────
// Identifies which C++ overloaded operator a method implements.
// OP_NONE means the method is an ordinary (non-operator) method.
enum OperatorKind {
    OP_NONE = 0,
    OP_SUBSCRIPT,   // operator[]
    OP_DEREF,       // operator*  (unary dereference)
    OP_ARROW,       // operator->
    OP_PRE_INC,     // operator++ (prefix)
    OP_POST_INC,    // operator++(int) (postfix)
    OP_EQ,          // operator==
    OP_NEQ,         // operator!=
    OP_BOOL,        // operator bool
    OP_PLUS,        // operator+
    OP_MINUS,       // operator-
    OP_ASSIGN,      // operator=
    OP_CALL,        // operator()
    OP_LT,          // operator<
    OP_GT,          // operator>
    OP_LE,          // operator<=
    OP_GE,          // operator>=
    OP_SPACESHIP,   // operator<=>
};

// Return a canonical mangled suffix for an operator kind (e.g. "operator_subscript").
const char* operator_kind_mangled_name(OperatorKind ok);

enum LambdaCaptureDefault {
    LCD_NONE = 0,
    LCD_BY_REFERENCE,
    LCD_BY_COPY,
};

// ── NodeKind ──────────────────────────────────────────────────────────────────

enum NodeKind {
    // Literals
    NK_INT_LIT,         // integer literal
    NK_FLOAT_LIT,       // floating-point literal
    NK_STR_LIT,         // string literal
    NK_CHAR_LIT,        // character literal

    // Primary expressions
    NK_VAR,             // identifier (variable / enum constant reference)

    // Binary / unary expressions
    NK_BINOP,           // binary operation: left op right
    NK_UNARY,           // prefix unary: op operand  (left = operand)
    NK_POSTFIX,         // postfix op: operand (left), op = "++" or "--"
    NK_ADDR,            // &expr  (left = operand)
    NK_DEREF,           // *expr  (left = operand)

    // Assignment
    NK_ASSIGN,          // left = right
    NK_COMPOUND_ASSIGN, // left op= right

    // Complex expressions
    NK_PACK_EXPANSION,  // expr... : left=pattern expression
    NK_CALL,            // func(...) : func=left, args=children[0..n_children-1]
    NK_BUILTIN_CALL,    // builtin(...) : func=left, args=children, builtin_id identifies semantic
    NK_INDEX,           // array[index] : left=array, right=index
    NK_MEMBER,          // expr.field or expr->field
    NK_CAST,            // (type)expr : left=expr
    NK_TERNARY,         // cond ? then_ : else_
    NK_SIZEOF_EXPR,     // sizeof(expr) : left=expr
    NK_SIZEOF_TYPE,     // sizeof(type)
    NK_SIZEOF_PACK,     // sizeof...(pack) : left=pack expr, sval=raw pack text
    NK_ALIGNOF_TYPE,    // _Alignof(type) / __alignof__(type)
    NK_ALIGNOF_EXPR,    // __alignof__(expr) : left=expr
    NK_COMMA_EXPR,      // left, right
    NK_STMT_EXPR,       // ({ ... }) : body=block node
    NK_COMPOUND_LIT,    // (type){...} : type + children=init items
    NK_VA_ARG,          // __builtin_va_arg(ap, type) : left=ap
    NK_GENERIC,         // _Generic(expr, type1: val1, ...) : left=ctrl_expr,
    NK_REAL_PART,       // __real__ expr : left=expr
    NK_IMAG_PART,       // __imag__ expr : left=expr
    NK_LAMBDA,          // [capture] [()] { ... } : body=block
                        // children[i]=NK_CAST{type=assoc_type,left=val,ival=1 if default}

    // Initializer
    NK_INIT_LIST,       // { ... } initializer list: children=items
    NK_INIT_ITEM,       // [desig] = value or .field = value

    // Statements
    NK_BLOCK,           // { stmts... } : children=stmts
    NK_EXPR_STMT,       // expr; : left=expr (may be null for empty stmt)
    NK_RETURN,          // return [expr]; : left=expr (nullable)
    NK_IF,              // if(cond) then [else else_]
    NK_WHILE,           // while(cond) body
    NK_FOR,             // for(init;cond;update) body
    NK_RANGE_FOR,       // for(decl : range_expr) body  (C++11 range-based for)
    NK_DO_WHILE,        // do body while(cond);
    NK_SWITCH,          // switch(cond) body
    NK_CASE,            // case val: body (next stmt)
    NK_CASE_RANGE,      // case lo ... hi: body (GCC extension)
    NK_DEFAULT,         // default: body
    NK_BREAK,           // break;
    NK_CONTINUE,        // continue;
    NK_GOTO,            // goto label;
    NK_LABEL,           // name: body
    NK_ASM,             // asm("...":...): left=template, children=clobber strings
    NK_EMPTY,           // ; (empty statement)
    NK_STATIC_ASSERT,   // static_assert/_Static_assert: left=condition, right=message

    // Declarations
    NK_DECL,            // local variable / parameter declaration
    NK_FUNCTION,        // function definition
    NK_GLOBAL_VAR,      // global variable declaration/definition
    NK_STRUCT_DEF,      // struct/union type definition
    NK_ENUM_DEF,        // enum type definition

    NK_OFFSETOF,        // __builtin_offsetof(type, member): type=base_type, name=field_path

    NK_PRAGMA_WEAK,     // #pragma weak symbol: name = symbol
    NK_PRAGMA_EXEC,     // #pragma c4 exec host/device: name = "host" or "device"

    // C++ new/delete expressions
    NK_NEW_EXPR,        // new T / new T(args) / new T[n] / ::new (p) T(args)
                        // type=allocated type, children=ctor args, left=placement arg (nullable)
                        // right=array size expr (for new[]), ival: 0=scalar, 1=array
                        // is_global_qualified=true for ::new
    NK_DELETE_EXPR,     // delete p / delete[] p
                        // left=operand, ival: 0=scalar delete, 1=array delete[]

    // Error recovery placeholders
    NK_INVALID_EXPR,    // placeholder for a malformed expression
    NK_INVALID_STMT,    // placeholder for a malformed statement

    // Top-level
    NK_PROGRAM,         // root node: children = top-level items
};

// ── Node ──────────────────────────────────────────────────────────────────────
// Flat struct covering all node kinds.  Fields not used by a given kind are
// zero-initialized by the arena allocator.
//
// Pure-C backport note: this is a plain C struct with no methods.
// The dump() free function provides debug printing.

struct Node {
    // Cross-stage semantic payload: node kind and source/provenance metadata
    // carried from parsing into later diagnostics and lowering.
    NodeKind kind;
    int line;
    int column;
    const char* file;
    int namespace_context_id; // owning namespace for declarations / resolved namespace for refs

    // Cross-stage semantic payload: type contract attached to declarations,
    // return types, casts, sizeof/alignof, compound literals, and params.
    TypeSpec type;

    // Name/text identity bridge: source and canonical names for identifiers,
    // declarations, labels, members, records, and enums.
    const char* name;
    const char* unqualified_name; // source spelling base name before namespace canonicalization
    TextId unqualified_text_id;   // parser-owned text identity for unqualified_name
    const char** qualifier_segments; // structured qualifier path from source spelling
    TextId* qualifier_text_ids;      // parser-owned text identity for qualifier_segments
    int n_qualifier_segments;        // qualifier segment count (excludes base name)
    bool is_global_qualified;        // true when the source spelling started with ::
    TextId* using_value_alias_target_qualifier_text_ids;
    int n_using_value_alias_target_qualifier_segments;
    TextId using_value_alias_target_text_id;
    int using_value_alias_target_namespace_context_id;
    bool using_value_alias_target_is_global_qualified;
    BuiltinId builtin_id;

    // Compatibility/display spelling: operator token text still used by
    // expression consumers and diagnostics for operator-shaped nodes.
    const char* op;

    // Cross-stage semantic payload plus debug/display spelling for scalar
    // literals. Integer and character literals carry resolved values; floats
    // preserve both parsed value and raw lexeme spelling.
    long long ival;     // NK_INT_LIT, NK_CHAR_LIT
    double    fval;     // NK_FLOAT_LIT (parsed value; raw text in sval)
    const char* sval;   // NK_STR_LIT (raw lexeme), NK_FLOAT_LIT (raw lexeme)
    bool is_imaginary;  // NK_INT_LIT, NK_FLOAT_LIT: GCC imaginary literal (200i, 1.0fi)

    // Cross-stage semantic payload: primary expression/statement child links.
    Node* left;         // primary child / operand / condition / func / lo
    Node* right;        // secondary child / index / hi
    Node* cond;         // NK_IF, NK_WHILE, NK_FOR, NK_DO_WHILE, NK_SWITCH, NK_TERNARY
    Node* then_;        // NK_IF, NK_TERNARY
    Node* else_;        // NK_IF (nullable), NK_TERNARY
    Node* body;         // NK_WHILE, NK_FOR, NK_DO_WHILE, NK_SWITCH, NK_FUNCTION,
                        // NK_LABEL, NK_CASE, NK_DEFAULT, NK_STMT_EXPR
    Node* init;         // NK_FOR init clause; NK_DECL / NK_GLOBAL_VAR initializer
    Node* update;       // NK_FOR update expression

    // Cross-stage semantic payload: arena-owned child arrays for variable-size
    // syntactic lists.
    // NK_BLOCK stmts / NK_CALL args / NK_PROGRAM items / NK_INIT_LIST items /
    // NK_COMPOUND_LIT items
    Node** children;
    int    n_children;

    // NK_FUNCTION params
    Node** params;
    int    n_params;

    // Cross-stage semantic payload and name/text identity bridge: C++ subset
    // template metadata carried on declarations and references.
    const char** template_param_names;
    TextId*      template_param_name_text_ids; // parallel: parser-owned identity for template_param_names
    bool*        template_param_is_nttp;  // parallel: true if non-type template param
    bool*        template_param_is_pack;  // parallel: true if template parameter is a pack
    bool*        template_param_has_default; // parallel: true if param has a default
    TypeSpec*    template_param_default_types; // parallel: default type for type params
    long long*   template_param_default_values; // parallel: default value for NTTP params
    const char** template_param_default_exprs; // parallel: deferred NTTP default expression text
    int          n_template_params;
    TypeSpec*    template_arg_types;
    bool*        template_arg_is_value;   // parallel: true if arg is a constant value
    long long*   template_arg_values;     // parallel: NTTP arg values (valid when is_value)
    const char** template_arg_nttp_names; // parallel: forwarded NTTP name (null if literal)
    TextId*      template_arg_nttp_text_ids; // parallel: parser-owned identity for forwarded NTTP names
    Node**       template_arg_exprs;      // parallel: parsed NTTP expression node when available
    int          n_template_args;
    bool         has_template_args;       // true if <...> was parsed (distinguishes f() from f<>())
    // Legacy bridge field: string spelling of the primary template until all
    // specialization paths use structured template identity.
    const char*  template_origin_name;    // primary template name for specialization patterns / instantiations

    // Cross-stage semantic payload: function-pointer declarator prototypes
    // attached to declarations, globals, functions, and params.
    Node** fn_ptr_params;
    int    n_fn_ptr_params;
    bool   fn_ptr_variadic;

    // For nested fn-ptr declarators like int (* (*p)(int a, int b))(int c, int d):
    // ret_fn_ptr_params stores the RETURN type's fn_ptr params (int c, int d),
    // while fn_ptr_params stores the declaration's own fn_ptr params (int a, int b).
    Node** ret_fn_ptr_params;
    int    n_ret_fn_ptr_params;
    bool   ret_fn_ptr_variadic;

    // Cross-stage semantic payload: record fields, member aliases, bases, and
    // layout hints carried by NK_STRUCT_DEF.
    Node** fields;
    int    n_fields;
    const char** member_typedef_names; // struct-scope typedef/using alias names
    TypeSpec* member_typedef_types;    // parallel alias target types
    int    n_member_typedefs;
    TypeSpec* base_types; // C++ base classes for NK_STRUCT_DEF
    int    n_bases;
    int    pack_align;   // #pragma pack alignment (0 = default, >0 = packed to N bytes)
    int    struct_align; // __attribute__((aligned(N))) on struct (0 = default)

    // Cross-stage semantic payload plus name/text identity bridge for
    // NK_ENUM_DEF variants.
    const char** enum_names;  // arena-allocated array of variant names
    TextId*      enum_name_text_ids; // parallel parser-owned identity for enum_names
    long long*   enum_vals;   // arena-allocated array of variant values
    int          n_enum_variants;
    int          enum_has_explicit;   // bitmask: bit i = variant i has explicit value

    // Parser recovery/debug payload: inline asm operand bookkeeping that keeps
    // parsed constraint lists available to later lowering and diagnostics.
    const char** asm_constraints; // outputs first, then inputs
    int          asm_n_constraints;
    int          asm_num_outputs;
    int          asm_num_inputs;
    int          asm_num_clobbers;

    // Semantic hints/flags: parser-derived declaration, expression, layout,
    // and C++ subset attributes consumed by later stages.
    bool variadic;      // NK_FUNCTION
    bool is_static;     // NK_FUNCTION, NK_DECL, NK_GLOBAL_VAR
    bool is_extern;     // NK_FUNCTION, NK_GLOBAL_VAR
    bool is_inline;     // NK_FUNCTION
    bool is_constexpr;  // NK_IF, NK_FUNCTION, NK_DECL, NK_GLOBAL_VAR
    bool is_consteval;  // NK_FUNCTION
    bool is_const_method; // NK_FUNCTION: const-qualified member function
    bool is_lvalue_ref_method; // NK_FUNCTION: member function ref-qualified with &
    bool is_rvalue_ref_method; // NK_FUNCTION: member function ref-qualified with &&
    bool is_concept_id;   // NK_VAR: parsed as a C++ concept-id expression
    bool is_explicit_specialization; // NK_FUNCTION: template<> explicit specialization
    uint8_t visibility; // 0=default, 1=hidden, 2=protected (from #pragma GCC visibility)
    bool is_volatile_asm; // NK_ASM
    bool is_arrow;      // NK_MEMBER (-> = true, . = false)
    bool is_union;      // NK_STRUCT_DEF
    bool is_anon_field; // NK_DECL: synthetic field for anonymous sub-struct/union
    bool is_designated; // NK_INIT_ITEM (has a designator)
    bool is_index_desig;// NK_INIT_ITEM (designator is index [N])
    long long desig_val;// NK_INIT_ITEM index designator value
    const char* desig_field; // NK_INIT_ITEM field designator name
    const char* linkage_spec; // C++ subset linkage string, e.g. "C"
    OperatorKind operator_kind; // C++ operator overloading: which operator this method implements
    bool is_constructor;  // NK_FUNCTION: this method is a constructor (name == struct tag)
    bool is_destructor;   // NK_FUNCTION: this method is a destructor (~ClassName)
    bool is_ctor_init;    // NK_DECL: initialized via constructor call  Type var(args)
    bool is_deleted;      // NK_FUNCTION: = delete (deleted function)
    bool is_defaulted;    // NK_FUNCTION: = default (defaulted special member)
    bool is_parameter_pack; // NK_DECL: function parameter pack / declarator pack
    ExecutionDomain execution_domain; // declaration execution domain (default Host)

    // Cross-stage semantic payload plus name bridge for constructor
    // initializer lists.
    const char** ctor_init_names;  // arena-allocated array of member names
    Node***      ctor_init_args;   // arena-allocated: per-init array of arg nodes
    int*         ctor_init_nargs;  // arena-allocated: per-init argument count
    int          n_ctor_inits;     // number of initializer list items

    // Semantic hints/flags for NK_LAMBDA capture and parameter-list spelling.
    LambdaCaptureDefault lambda_capture_default;
    bool lambda_has_parameter_list;
};

inline bool is_primary_template_struct_def(const Node* node) {
    return node &&
           node->kind == NK_STRUCT_DEF &&
           node->n_template_params > 0 &&
           node->n_template_args == 0 &&
           (!node->template_origin_name || !node->template_origin_name[0] ||
            (node->name && node->template_origin_name &&
             std::strcmp(node->name, node->template_origin_name) == 0));
}

inline bool node_has_template_param_name(const Node* node, const char* name) {
    if (!node || !name || !name[0] || !node->template_param_names) return false;
    for (int i = 0; i < node->n_template_params; ++i) {
        const char* param_name = node->template_param_names[i];
        if (param_name && std::strcmp(param_name, name) == 0) return true;
    }
    return false;
}

inline bool node_has_template_param_text_id(const Node* node, TextId text_id) {
    if (!node || text_id == kInvalidText || !node->template_param_name_text_ids) {
        return false;
    }
    for (int i = 0; i < node->n_template_params; ++i) {
        if (node->template_param_name_text_ids[i] == text_id) return true;
    }
    return false;
}

inline bool text_mentions_template_param(const Node* node, const char* text) {
    if (!node || !text || !text[0]) return false;
    const char* cur = text;
    while (*cur) {
        if (std::isalpha(static_cast<unsigned char>(*cur)) || *cur == '_') {
            const char* start = cur;
            ++cur;
            while (std::isalnum(static_cast<unsigned char>(*cur)) || *cur == '_')
                ++cur;
            const size_t len = static_cast<size_t>(cur - start);
            for (int i = 0; i < node->n_template_params; ++i) {
                const char* param_name = node->template_param_names[i];
                if (!param_name) continue;
                if (std::strlen(param_name) == len &&
                    std::strncmp(start, param_name, len) == 0) {
                    return true;
                }
            }
            continue;
        }
        ++cur;
    }
    return false;
}

inline bool typespec_mentions_template_param(const TypeSpec& ts, const Node* node);

inline bool template_arg_list_mentions_template_param(const TypeSpec& ts,
                                                      const Node* node) {
    if (!node || ts.tpl_struct_args.size <= 0 || !ts.tpl_struct_args.data) return false;
    for (int i = 0; i < ts.tpl_struct_args.size; ++i) {
        const TemplateArgRef& arg = ts.tpl_struct_args.data[i];
        if (arg.kind == TemplateArgKind::Type &&
            typespec_mentions_template_param(arg.type, node)) {
            return true;
        }
        if (text_mentions_template_param(node, arg.debug_text)) {
            return true;
        }
    }
    return false;
}

inline std::string encode_template_arg_debug_ref(const TypeSpec& ts) {
    std::string out;
    out += "base=" + std::to_string(static_cast<int>(ts.base));
    out += ",tag=";
    out += ts.tag ? ts.tag : "";
    out += ",ptr=" + std::to_string(ts.ptr_level);
    out += ",arr=" + std::to_string(ts.array_rank);
    return out;
}

inline std::string encode_template_arg_debug_list(const TypeSpec& ts) {
    std::string out;
    if (!ts.tpl_struct_args.data) return out;
    for (int i = 0; i < ts.tpl_struct_args.size; ++i) {
        const TemplateArgRef& arg = ts.tpl_struct_args.data[i];
        if (i > 0) out += ",";
        if (arg.debug_text && arg.debug_text[0]) {
            out += arg.debug_text;
            continue;
        }
        if (arg.kind == TemplateArgKind::Value) {
            out += std::to_string(arg.value);
            continue;
        }
        if (arg.kind == TemplateArgKind::Type) {
            out += "{";
            out += encode_template_arg_debug_ref(arg.type);
            out += "}";
        } else {
            out += "?";
        }
    }
    return out;
}

inline std::string template_arg_debug_text_at(const TypeSpec& ts, int index) {
    if (index < 0 || index >= ts.tpl_struct_args.size || !ts.tpl_struct_args.data) {
        return {};
    }
    const TemplateArgRef& arg = ts.tpl_struct_args.data[index];
    if (arg.debug_text && arg.debug_text[0]) {
        return arg.debug_text;
    }
    if (arg.kind == TemplateArgKind::Value) {
        return std::to_string(arg.value);
    }
    if (arg.kind == TemplateArgKind::Type) return encode_template_arg_debug_ref(arg.type);
    return {};
}

inline bool typespec_mentions_template_param(const TypeSpec& ts, const Node* node) {
    if (node_has_template_param_name(node, ts.tag) ||
        node_has_template_param_text_id(node, ts.tag_text_id) ||
        node_has_template_param_text_id(node, ts.template_param_text_id) ||
        template_arg_list_mentions_template_param(ts, node)) {
        return true;
    }
    if (ts.deferred_member_type_text_id != kInvalidText) {
        return node_has_template_param_text_id(
            node, ts.deferred_member_type_text_id);
    }
    return text_mentions_template_param(node, ts.deferred_member_type_name);
}

inline bool is_dependent_template_struct_specialization(const Node* node) {
    if (!node ||
        node->kind != NK_STRUCT_DEF ||
        node->n_template_params <= 0 ||
        !node->template_origin_name ||
        !node->template_origin_name[0] ||
        node->n_template_args <= 0) {
        return false;
    }

    for (int i = 0; i < node->n_template_args; ++i) {
        const bool is_value_arg =
            node->template_arg_is_value && node->template_arg_is_value[i];
        if (is_value_arg) {
            if (node->template_arg_nttp_names &&
                node_has_template_param_name(node, node->template_arg_nttp_names[i])) {
                return true;
            }
            continue;
        }
        if (node->template_arg_types &&
            typespec_mentions_template_param(node->template_arg_types[i], node)) {
            return true;
        }
    }

    return false;
}


// ── Free functions ────────────────────────────────────────────────────────────

// Print a debug dump of the AST rooted at n (indent 0 = top-level).
void ast_dump(const Node* n, int indent);

// Return a short debug name for a NodeKind.
const char* node_kind_name(NodeKind k);

}  // namespace c4c
