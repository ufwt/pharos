#ifndef Pharos_Demangle_H
#define Pharos_Demangle_H

#include <string>

// Thrown for errors encountered while demangling names.
class DemanglerError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

enum class SymbolType {
  Unspecified,
  Namespace,
  StaticClassMember,
  GlobalObject,
  GlobalFunction,
  ClassMethod,
  GlobalThing1,
  GlobalThing2
};

enum class Scope {
  Unspecified,
  Private,
  Protected,
  Public
};

enum class MethodProperty {
  Unspecified,
  Ordinary,
  Static,
  Virtual,
  Thunk
};

enum class Distance {
  Unspecified,
  Near,
  Far,
  Huge
};


// Forward declaration of the core "type" definition.
class DemangledType;

using DemangledTypePtr = std::shared_ptr<DemangledType>;

// Vectors of demangled types are used for several purposes.  Arguments to a function, the
// terms in a fully qualified name, and a stack of names or types for numbered references.
// While the underlying types are identical in practice, I'm going to attempt to keep them
// separate logically in case they ever need to diverge.
typedef std::vector<DemangledTypePtr> FunctionArgs;
typedef std::vector<DemangledTypePtr> FullyQualifiedName;
typedef std::vector<DemangledTypePtr> ReferenceStack;

// The classes describing the demangled results are demangler independent, but strictly
// speaking the boolean match" flag on the str() methods is specific to the Visual Studio
// Demangler.  Solutions could include overridding the str() implementation for each demangler,
// or just ignoring this problem since every demangler implementation ought to be emitting a
// consistent C++ representation.

// Template parameters can be either a type or a constant.
class DemangledTemplateParameter {
 public:
  // If the type pointer is NULL, then the constant value is used.
  DemangledTypePtr type;
  int64_t constant_value;

  // If pointer is true, the parameter is a constant pointer to type.
  bool pointer = false;

  DemangledTemplateParameter(DemangledTypePtr t);
  DemangledTemplateParameter(int64_t c);
  std::string str(bool match = false) const;
};

using DemangledTemplateParameterPtr = std::shared_ptr<DemangledTemplateParameter>;

typedef std::vector<DemangledTemplateParameterPtr> DemangledTemplate;

class DemangledType {
  // The the type a pointer, reference, refref, etc. to a function?
  bool is_func_ptr() const;

  std::string str_class_properties(bool match = false) const;
  std::string str_storage_properties(bool match = false, bool is_retval = false) const;
  std::string str_distance(bool match = false) const;
  std::string str_pointer_punctuation(bool match = false) const;
  std::string str_simple_type(bool match = false) const;
  std::string str_template_parameters(bool match = false) const;
  std::string str_function_arguments(bool match = false) const;
  std::string str_class_name(bool match = false) const;

 public:

  // Laziness. :-(
  std::string str_name_qualifiers(const FullyQualifiedName& the_name, bool match = false) const;

  bool is_const;
  bool is_volatile;
  bool is_reference;
  bool is_pointer;

  // Hacky thing for complex types that can't get rendered any better than putting them inside
  // a pair of single quotes.  e.g. ?X@??Y@@9@9 demangles to "`Y'::X".  The extra quotes aren't
  // present if this is the outermost symbol, but are if it's part of a namespace? ...
  bool is_embedded;

  // Currently used for signaling between functions, but might be useful in general.
  bool is_func;

  // Poorly understood features involving storage classes, see update_storage_class()...
  bool is_based;
  bool is_member;

  // This really just means that we were a term in a fully qualified
  // name.  We can't actually tell from the demangling whether we were
  bool is_namespace;

  // True if the namespace is anonymous.  The simple_type string then contains the unique
  // identifier name that's not typically shown for anonymous namespaces.
  bool is_anonymous;

  // This is handled horribly by Microsoft, and equally horribly by me.  I want to think some
  // more about the correct approach after I know more about the other $$ cases.  For this
  // particular one, I would expect the correct answer to something more like a reference to a
  // reference to a type (although we may still need come custom outputing to avoid getting a
  // space between the references.)  Or maybe is_reference, is_pointer, and is_refref should be
  // an enum?  Apparently the correct name for this is "rvalue reference"?
  bool is_refref;

  // Enum controlling how to interpret this type.
  // 1=namespace, 2=static class member, 3=global object, 4=global function, 5=class method
  SymbolType symbol_type;

  // Really an enum: 0=near, 1=far, 2=huge
  Distance distance;

  // Really an enum: 0=segment relative, 1=absolute (64-bit mode), 2=__based (64-bit mode)
  int pointer_base;

  // The type pointed to or referenced.
  DemangledTypePtr inner_type;

  // The real type of an enum (Usually int, and often coded assuch regardless).
  DemangledTypePtr enum_real_type;

  // Simple type is currently a bit of a hodge-podge...  It contains the string representing
  // simple types (e.g. unsigned int).  But it also contains the names of name spaces, the
  // names of templated types, simple class names (which are indistinguishable from classes),
  // and occasionally other keywords like class or struct.
  std::string simple_type;

  // The fully qualified name of a complex type (e.g. a templated class).
  FullyQualifiedName name;

  // I'm not sure that I've named this correctly.  Set by symbol types 6 & 7.
  DemangledTypePtr com_interface;

  // If the class was templated, these are the parameters.
  DemangledTemplate template_parameters;

  // Scope (private, protected, public) of class method. Only applicable to class methods.
  Scope scope;

  // Class method property (static, virtual, thunk). Only applicable to class methods.
  MethodProperty method_property;

  // Will eventually be looked up in Pharos calling convention map and
  // be a pointer to an actual calling convention object.
  std::string calling_convention;

  // Was this symbol exported?
  bool is_exported;

  // Names and name like things...
  bool is_ctor;
  bool is_dtor;
  std::string method_name;
  FullyQualifiedName class_name;

  // The fully qualified name of a exported variable.   Names are still messy. :-(
  FullyQualifiedName instance_name;

  // Return value type.  Applicable only to functions and class methods.
  DemangledTypePtr retval;

  // Function arguments.  Applicable only to functions and class methods.
  FunctionArgs args;

  // And then the really obscure values (like parameters for RTTI data structures).
  int64_t n1;
  int64_t n2;
  int64_t n3;
  int64_t n4;

  DemangledType();
  std::string get_class_name() const;
  std::string get_method_name() const;
  std::string str(bool match = false, bool is_retval = false) const;
  void debug_type(bool match = false, size_t indent = 0, std::string label = "") const;
};

// Main entry point to demangler
DemangledTypePtr visual_studio_demangle(const std::string & mangled, bool debug = false);

#endif

/* Local Variables:   */
/* mode: c++          */
/* fill-column:    95 */
/* comment-column: 0  */
/* End:               */
