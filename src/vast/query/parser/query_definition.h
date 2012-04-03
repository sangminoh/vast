#ifndef VAST_QUERY_PARSER_CLAUSE_DEFINITION_H
#define VAST_QUERY_PARSER_CLAUSE_DEFINITION_H

#include "vast/query/parser/query.h"

namespace vast {
namespace query {
namespace parser {

template <typename Iterator>
query<Iterator>::query(util::parser::error_handler<Iterator>& error_handler)
  : query::base_type(qry)
  , expr(error_handler)
{
    qi::_1_type _1;
    qi::_2_type _2;
    qi::_3_type _3;
    qi::_4_type _4;

    qi::raw_type raw;
    qi::lexeme_type lexeme;
    qi::alpha_type alpha;
    qi::alnum_type alnum;

    using qi::on_error;
    using qi::fail;

    binary_query_op.add
        ("||", ast::logical_or)
        ("&&", ast::logical_and)
        ;

    unary_query_op.add
        ("!", ast::logical_not)
        ;

    binary_clause_op.add
        ("~",  ast::match)
        ("!~", ast::not_match)
        ("==", ast::equal)
        ("!=", ast::not_equal)
        ("<",  ast::less)
        ("<=", ast::less_equal)
        (">",  ast::greater)
        (">=", ast::greater_equal)
        ;

    type.add
        ("bool", ze::bool_type)
        ("int", ze::int_type)
        ("uint", ze::uint_type)
        ("double", ze::double_type)
        ("duration", ze::duration_type)
        ("timepoint", ze::timepoint_type)
        ("string", ze::string_type)
        ("vector", ze::vector_type)
        ("set", ze::set_type)
        ("table", ze::table_type)
        ("record", ze::record_type)
        ("address", ze::address_type)
        ("prefix", ze::prefix_type)
        ("port", ze::port_type)
        ;

    qry
        =   unary_clause
        >>  *(binary_query_op > unary_clause)
        ;

    unary_clause
        =   event_clause
        |   type_clause
        |   (unary_query_op > unary_clause)
        ;

    event_clause
        =   identifier > '.' > identifier
        >   binary_clause_op
        >   expr;

    type_clause
        =   lexeme['@' > type]
        >   binary_clause_op > expr;
        ;

    identifier
        =   raw[lexeme[(alpha | '_') >> *(alnum | '_')]]
        ;

    BOOST_SPIRIT_DEBUG_NODES(
        (qry)
        (unary_clause)
        (type_clause)
        (event_clause)
        (identifier)
    );

    on_error<fail>(qry, error_handler.functor()(_4, _3));

    binary_query_op.name("binary query operator");
    unary_query_op.name("unary query operator");
    binary_clause_op.name("binary clause operator");
    type.name("type");
    qry.name("query");
    unary_clause.name("unary clause");
    event_clause.name("event clause");
    type_clause.name("type clause");
    identifier.name("identifier");
}

} // namespace ast
} // namespace query
} // namespace vast

#endif
