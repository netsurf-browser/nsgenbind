/*
 * This is a bison parser for web IDL derived from the the grammar in
 * apendix A of W3C WEB IDL - http://www.w3.org/TR/WebIDL/
 *
 */

%{

#include <stdio.h>
#include <string.h>

#include "webidl-ast.h"

#include "webidl-parser.h"
#include "webidl-lexer.h"

char *errtxt;

static void webidl_error(const char *str)
{
    errtxt = strdup(str);
}

int webidl_wrap()
{
    return 1;
}

%}

%locations
%define api.pure
%error-verbose

 /* the w3c grammar results in 10 shift/reduce, 2 reduce/reduce conflicts 
  * The reduce/reduce error are both the result of empty sequences
  */
 /* %expect 10 */
 /* %expect-rr 2 */

%union
{
  int attr;
  char* text;
  long value;
  struct ifmember_s **ifmember;
}


%token TOK_ANY
%token TOK_ATTRIBUTE
%token TOK_BOOLEAN
%token TOK_BYTE
%token TOK_CALLBACK
%token TOK_LEGACYCALLER
%token TOK_CONST
%token TOK_CREATOR
%token TOK_DATE
%token TOK_DELETER
%token TOK_DICTIONARY
%token TOK_DOUBLE
%token TOK_ELLIPSIS
%token TOK_ENUM
%token TOK_EOL
%token TOK_EXCEPTION
%token TOK_FALSE
%token TOK_FLOAT
%token TOK_GETRAISES
%token TOK_GETTER
%token TOK_IMPLEMENTS
%token TOK_IN
%token TOK_INFINITY
%token TOK_INHERIT
%token TOK_INTERFACE
%token TOK_LONG
%token TOK_MODULE
%token TOK_NAN
%token TOK_NATIVE
%token TOK_NULL_LITERAL
%token TOK_OBJECT
%token TOK_OCTET
%token TOK_OMITTABLE
%token TOK_OPTIONAL
%token TOK_OR
%token TOK_PARTIAL
%token TOK_RAISES
%token TOK_READONLY
%token TOK_SETRAISES
%token TOK_SETTER
%token TOK_SEQUENCE
%token TOK_SHORT
%token TOK_STATIC
%token TOK_STRING
%token TOK_STRINGIFIER
%token TOK_TRUE
%token TOK_TYPEDEF
%token TOK_UNRESTRICTED
%token TOK_UNSIGNED
%token TOK_VOID

%token TOK_POUND_SIGN

%token <text>       TOK_IDENTIFIER
%token <value>      TOK_INT_LITERAL
%token <text>       TOK_FLOAT_LITERAL
%token <text>       TOK_STRING_LITERAL
%token <text>       TOK_OTHER_LITERAL
%token <text>       TOK_JAVADOC

%type <text> Inheritance
%type <ifmember> InterfaceMembers

%%

 /* [1] start with definitions  */
Definitions:
        /* empty */
        |
        ExtendedAttributeList Definition Definitions
        | 
        error ';' 
        { 
            fprintf(stderr, "%d: %s\n", yylloc.first_line, errtxt);
            free(errtxt);
            YYABORT ;
        }
        ;
        
 /* [2] */
Definition:  
        CallbackOrInterface
        |
        Partial 
        | 
        Dictionary 
        | 
        Exception 
        | 
        Enum 
        | 
        Typedef 
        | 
        ImplementsStatement
        ;

 /* [3] */
CallbackOrInterface:
        TOK_CALLBACK CallbackRestOrInterface
        |
        Interface
        ;

 /* [4] */
CallbackRestOrInterface:
        CallbackRest
        |
        Interface
        ;

 /* [5] */
Interface:
        TOK_INTERFACE TOK_IDENTIFIER Inheritance '{' InterfaceMembers '}' ';'
        {
        }
        ;

 /* [6] */
Partial:
        TOK_PARTIAL PartialDefinition
        ;

 /* [7] */
PartialDefinition:
        PartialInterface 
        | 
        PartialDictionary
        ;

 /* [8] */
PartialInterface:
        TOK_INTERFACE TOK_IDENTIFIER '{' InterfaceMembers '}' ';'
        ;

 /* [9] */
InterfaceMembers:
        /* empty */
        {
          $$ = NULL;
        }
        |
        ExtendedAttributeList InterfaceMember InterfaceMembers
        {
          $$ = NULL;
        }
        ;

 /* [10] */
InterfaceMember:
        Const
        | 
        AttributeOrOperation
        ;

 /* [11] */
Dictionary:
        TOK_DICTIONARY TOK_IDENTIFIER Inheritance '{' DictionaryMembers '}' ';'
        ;

 /* [12] */
DictionaryMembers:
        /* empty */
        |
        ExtendedAttributeList DictionaryMember DictionaryMembers
        ;

 /* [13] */
DictionaryMember:
        Type TOK_IDENTIFIER Default ";"
        ;

 /* [14] */
PartialDictionary:
        TOK_DICTIONARY TOK_IDENTIFIER '{' DictionaryMembers '}' ';'

 /* [15] */
Default:
        /* empty */
        |
        '=' DefaultValue 
        ;


 /* [16] */
DefaultValue:
        ConstValue 
        |
        TOK_STRING_LITERAL
        ;

 /* [17] */
Exception:
        /* empty */
        |
        TOK_EXCEPTION TOK_IDENTIFIER Inheritance '{' ExceptionMembers '}' ';'
        ;

 /* [18] */
ExceptionMembers:
        /* empty */
        |
        ExtendedAttributeList ExceptionMember ExceptionMembers
        ; 

 /* [19] returns a string */
Inheritance:
        /* empty */
        {
          $$ = NULL;
        }
        |
        ':' TOK_IDENTIFIER
        {
          $$ = $2;
        }
        ;

/* [20] */
Enum:
        TOK_ENUM TOK_IDENTIFIER '{' EnumValueList '}' ';'
        ;

/* [21] */
EnumValueList:
        TOK_STRING_LITERAL EnumValues
        ;

/* [22] */
EnumValues:
        /* empty */
        |
        ',' TOK_STRING_LITERAL EnumValues
        ;

 /* [23] - bug in w3c grammar? it doesnt list the equals as a terminal  */
CallbackRest:
        TOK_IDENTIFIER '=' ReturnType '(' ArgumentList ')' ';'

 /* [24] */
Typedef:
        TOK_TYPEDEF ExtendedAttributeList Type TOK_IDENTIFIER ';'
        ;

 /* [25] */
ImplementsStatement:
        TOK_IDENTIFIER TOK_IMPLEMENTS TOK_IDENTIFIER ';'
        ;

 /* [26] */
Const:
        TOK_CONST ConstType TOK_IDENTIFIER '=' ConstValue ';'
        ;

 /* [27] */
ConstValue:
        BooleanLiteral 
        | 
        FloatLiteral 
        | 
        TOK_INT_LITERAL 
        | 
        TOK_NULL_LITERAL
        ;

 /* [28] */
BooleanLiteral:
        TOK_TRUE 
        | 
        TOK_FALSE
        ;

 /* [29] */
FloatLiteral:
        TOK_FLOAT_LITERAL
        | 
        '-' TOK_INFINITY
        | 
        TOK_INFINITY
        |
        TOK_NAN
        ;

 /* [30] */	
AttributeOrOperation:
        TOK_STRINGIFIER StringifierAttributeOrOperation 
        | 
        Attribute 
        | 
        Operation
        ;

 /* [31] */	
StringifierAttributeOrOperation:
        Attribute 
        | 
        OperationRest 
        | 
        ';'
        ;

 /* [32] */	
Attribute:
        Inherit ReadOnly TOK_ATTRIBUTE Type TOK_IDENTIFIER ';'
        ;

 /* [33] */	
Inherit:
        /* empty */
        |
        TOK_INHERIT
        ; 

 /* [34] */	
ReadOnly:
        /* empty */
        |
        TOK_READONLY
        ;

 /* [35] */
Operation:
        Qualifiers OperationRest
        ;

 /* [36] */
Qualifiers:
        TOK_STATIC
        | 
        Specials
        ;

 /* [37] */
Specials:
        /* empty */
        |
        Special Specials 
        ;

 /* [38] */
Special:
        TOK_GETTER
        |
        TOK_SETTER
        |
        TOK_CREATOR
        |
        TOK_DELETER
        |
        TOK_LEGACYCALLER
        ;

 /* [39] */
OperationRest:
        ReturnType OptionalIdentifier '(' ArgumentList ')' ';'
        ;

 /* [40] */
OptionalIdentifier:
        /* empty */
        |
        TOK_IDENTIFIER
        ;


 /* [41] */
ArgumentList:
        /* empty */
        |
        Argument Arguments 
        ;

 /* [42] */
Arguments:
        /* empty */
        |
        ',' Argument Arguments 
        ;


 /* [43] */
Argument:
        ExtendedAttributeList OptionalOrRequiredArgument
        ;

 /* [44] */
OptionalOrRequiredArgument:
        TOK_OPTIONAL Type ArgumentName Default 
        | 
        Type Ellipsis ArgumentName
        ;

 /* [45] */
ArgumentName:
        ArgumentNameKeyword 
        | 
        TOK_IDENTIFIER
        ;

 /* [46] */
Ellipsis:
        /* empty */
        |
        TOK_ELLIPSIS
        ;

 /* [47] */
ExceptionMember:
        Const 
        | 
        ExceptionField
        ;

 /* [48] */
ExceptionField:
        Type TOK_IDENTIFIER ';'
        ;

 /* [49] extended attribute list inside square brackets */
ExtendedAttributeList:
        /* empty */
        |
        '[' ExtendedAttribute ExtendedAttributes ']'
        ;

 /* [50] extended attributes are separated with a comma */
ExtendedAttributes:
        /* empty */
        |
        ',' ExtendedAttribute ExtendedAttributes
        ;

 /* [51] extended attributes are nested with normal, square and curly braces */
ExtendedAttribute:
        '(' ExtendedAttributeInner ')' ExtendedAttributeRest
        |
        '[' ExtendedAttributeInner ']' ExtendedAttributeRest
        |
        '{' ExtendedAttributeInner '}' ExtendedAttributeRest
        |
        Other ExtendedAttributeRest 
        ;

 /* [52] extended attributes can be space separated too */
ExtendedAttributeRest:
        /* empty */
        |
        ExtendedAttribute
        ;

 /* [53] extended attributes are nested with normal, square and curly braces */
ExtendedAttributeInner:
        /* empty */
        |
        '(' ExtendedAttributeInner ')' ExtendedAttributeInner
        |
        '[' ExtendedAttributeInner ']' ExtendedAttributeInner
        |
        '{' ExtendedAttributeInner '}' ExtendedAttributeInner
        |
        OtherOrComma ExtendedAttributeInner
        ;

 /* [54] */
Other:
        TOK_INT_LITERAL
        |
        TOK_FLOAT_LITERAL
        |
        TOK_IDENTIFIER
        |
        TOK_STRING_LITERAL
        |
        TOK_OTHER_LITERAL 
        |
        '-'
        |
        '.'
        |
        TOK_ELLIPSIS
        |
        ':'
        |
        ';'
        |
        '<'
        |
        '='
        |
        '>'
        |
        '?'
        |
        TOK_DATE
        |
        TOK_STRING
        |
        TOK_INFINITY
        |
        TOK_NAN
        |
        TOK_ANY
        |
        TOK_BOOLEAN
        |
        TOK_BYTE
        |
        TOK_DOUBLE
        |
        TOK_FALSE
        |
        TOK_FLOAT
        |
        TOK_LONG
        |
        TOK_NULL_LITERAL
        |
        TOK_OBJECT
        |
        TOK_OCTET
        |
        TOK_OR
        |
        TOK_OPTIONAL
        |
        TOK_SEQUENCE
        |
        TOK_SHORT
        |
        TOK_TRUE
        |
        TOK_UNSIGNED
        |
        TOK_VOID
        |
        ArgumentNameKeyword
        ;

 /* [55] */
ArgumentNameKeyword:
        TOK_ATTRIBUTE
        |
        TOK_CALLBACK
        |
        TOK_CONST
        |
        TOK_CREATOR
        |
        TOK_DELETER
        |
        TOK_DICTIONARY
        |
        TOK_ENUM
        |
        TOK_EXCEPTION
        |
        TOK_GETTER
        |
        TOK_IMPLEMENTS
        |
        TOK_INHERIT
        |
        TOK_INTERFACE
        |
        TOK_LEGACYCALLER
        |
        TOK_PARTIAL
        |
        TOK_SETTER
        |
        TOK_STATIC
        |
        TOK_STRINGIFIER
        |
        TOK_TYPEDEF
        |
        TOK_UNRESTRICTED
        ;

 /* [56] as it says an other element or a comma */
OtherOrComma:
        Other
        |
        ','
        ;

 /* [57] */
Type:
        SingleType 
        | 
        UnionType TypeSuffix
        ;

 /* [58] */
SingleType:
        NonAnyType 
        | 
        TOK_ANY TypeSuffixStartingWithArray
        ;

 /* [59] */
UnionType:
        '(' UnionMemberType TOK_OR UnionMemberType UnionMemberTypes ')'
        ;

 /* [60] */
UnionMemberType:
        NonAnyType 
        | 
        UnionType TypeSuffix 
        | 
        TOK_ANY '[' ']' TypeSuffix
        ;

 /* [61] */
UnionMemberTypes:
        /* empty */
        |
        TOK_OR UnionMemberType UnionMemberTypes 
        ;

 /* [62] */
NonAnyType:
        PrimitiveType TypeSuffix 
        | 
        TOK_STRING TypeSuffix 
        | 
        TOK_IDENTIFIER TypeSuffix 
        | 
        TOK_SEQUENCE '<' Type '>' Null 
        | 
        TOK_OBJECT TypeSuffix 
        | 
        TOK_DATE TypeSuffix
        ;

 /* [63] */
ConstType:
        PrimitiveType Null 
        | 
        TOK_IDENTIFIER Null
        ;

 /* [64] */
PrimitiveType:
        UnsignedIntegerType 
        | 
        UnrestrictedFloatType 
        | 
        TOK_BOOLEAN
        | 
        TOK_BYTE
        | 
        TOK_OCTET
        ;

 /* [65] */
UnrestrictedFloatType:
        TOK_UNRESTRICTED FloatType 
        | 
        FloatType
        ;

 /* [66] */
FloatType:
        TOK_FLOAT
        |
        TOK_DOUBLE
        ;

 /* [67] */
UnsignedIntegerType:
        TOK_UNSIGNED IntegerType 
        | 
        IntegerType
        ;

 /* [68] */
IntegerType:
        TOK_SHORT
        | 
        TOK_LONG OptionalLong
        ;

 /* [69] */
OptionalLong:
        /* empty */
        |
        TOK_LONG
        ; 

 /* [70] */
TypeSuffix:
        /* empty */
        |
        '[' ']' TypeSuffix 
        | 
        '?' TypeSuffixStartingWithArray 
        ;

 /* [71] */	
TypeSuffixStartingWithArray:
        /* empty */
        |
        '[' ']' TypeSuffix
        ;

 /* [72] */
Null:
        /* empty */
        |
        '?'
        ;

 /* [73] */	
ReturnType:
        Type 
        | 
        TOK_VOID
        ;

%%
