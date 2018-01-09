//
// LexerGenerator.cpp
// Copyright (c) Charles Baker. All rights reserved.
//

#include "stdafx.hpp"
#include "LexerGenerator.hpp"
#include "Error.hpp"
#include "LexerToken.hpp"
#include "LexerItem.hpp"
#include "LexerState.hpp"
#include "LexerAction.hpp"
#include "LexerErrorPolicy.hpp"
#include "RegexNode.hpp"
#include "RegexParser.hpp"
#include <sweet/utility/shared_ptr_less.hpp>
#include <sweet/assert/assert.hpp>
#include <limits.h>

using std::pair;
using std::vector;
using std::make_pair;
using namespace sweet;
using namespace sweet::lexer;

/**
// Constructor.
//
// @param token
//  The token to generate a LexerStateMachine from.
*/
LexerGenerator::LexerGenerator( const LexerToken& token, LexerErrorPolicy* event_sink )
: event_sink_( event_sink ),
  actions_(),
  states_(),
  whitespace_states_(),
  start_state_( NULL ),
  whitespace_start_state_( NULL ),
  ranges_()
{
    generate_states( RegexParser(token, this), &states_, &start_state_ );
}

/**
// Constructor.
//
// @param symbols
//  The regular expressions and addresses to generate a LexerStateMachine to recognize.
*/
LexerGenerator::LexerGenerator( const std::vector<LexerToken>& tokens, const std::vector<LexerToken>& whitespace_tokens, LexerErrorPolicy* event_sink )
: event_sink_( event_sink ),
  actions_(),
  states_(),
  whitespace_states_(),
  start_state_( NULL ),
  whitespace_start_state_( NULL ),
  ranges_()
{    
    generate_states( RegexParser(tokens, this), &states_, &start_state_ );
    generate_states( RegexParser(whitespace_tokens, this), &whitespace_states_, &whitespace_start_state_ );
}

/**
// Get the actions in generated by this LexerGenerator.
//
// @return
//  The actions.
*/
std::vector<std::shared_ptr<LexerAction> >& LexerGenerator::actions()
{
    return actions_;
}

/**
// Get the generated states.
//
// @return
//  The generated states.
*/
std::set<std::shared_ptr<LexerState>, shared_ptr_less<LexerState>>& LexerGenerator::states()
{
    return states_;
}

/**
// Get the generated whitespace states.
//
// @return
//  The generated whitespace states.
*/
std::set<std::shared_ptr<LexerState>, shared_ptr_less<LexerState>>& LexerGenerator::whitespace_states()
{
    return whitespace_states_;
}

/**
// Get the start state.
//
// @return
//  The start state.
*/
const LexerState* LexerGenerator::start_state() const
{
    return start_state_;
}

/**
// Get the whitespace start state.
//
// @return
//  The whitespace start state.
*/
const LexerState* LexerGenerator::whitespace_start_state() const
{
    return whitespace_start_state_;
}

/**
// Add a new or retrieve an existing LexerAction.
//
// If the parser already has a LexerAction whose identifier 
// matches \e identifier then that LexerAction is returned.  Otherwise 
// a new LexerAction is created, added to this ParserGenerator so that it 
// can be returned later if necessary, and returned from this call.
//
// @param identifier
//  The identifier of the LexerAction to add or retrieve.
//
// @return
//  The LexerAction whose identifier matches \e identifier or null if 
//  \e identifier is empty.
*/
const lexer::LexerAction* LexerGenerator::add_lexer_action( const std::string& identifier )
{
    SWEET_ASSERT( !identifier.empty() );
    
    std::shared_ptr<lexer::LexerAction> lexer_action;

    if ( !identifier.empty() )
    {    
        std::vector<std::shared_ptr<lexer::LexerAction> >::const_iterator i = actions_.begin();
        while ( i != actions_.end() && (*i)->get_identifier() != identifier )
        {
            ++i;
        }
        
        if ( i != actions_.end() )
        {
            lexer_action = *i;
        }
        else
        {
            lexer_action.reset( new lexer::LexerAction(actions_.size(), identifier) );
            actions_.push_back( lexer_action );
        }
    }

    return lexer_action.get();
}

/**
// Fire an error from this generator.
//
// @param line
//  The line number that the error occured on.
//
// @param error
//  The error to fire.
*/
void LexerGenerator::fire_error( int line, const error::Error& error ) const
{
    if ( event_sink_ )
    {
        event_sink_->lexer_error( line, error );
    }
}


/**
// Fire a message to be printed from this generator.
//
// @param format
//  A printf-style format string that desribes the message to print.
//
// @param ...
//  Parameters as described by \e format.
*/
void LexerGenerator::fire_printf( const char* format, ... ) const
{
    if ( event_sink_ )
    {
        SWEET_ASSERT( format );
        va_list args;
        va_start( args, format );
        event_sink_->lexer_vprintf( format, args );
        va_end( args );
    }
}


/**
// Generate the state that results from accepting any character in the range
// [\e begin, \e end) from \e state.
//
// @param state
//  The state to generate from.
//
// @param begin
//  The begin character in the range to accept to generate the goto state.
//
// @param end
//  The end character in the range to accept to generate the goto state.
//
// @return
//  The state generated when accepting [\e begin, \e end) from \e state.
*/
std::shared_ptr<LexerState> LexerGenerator::goto_( const LexerState* state, int begin, int end )
{
    SWEET_ASSERT( state );
    SWEET_ASSERT( begin != INVALID_BEGIN_CHARACTER && begin != INVALID_END_CHARACTER );
    SWEET_ASSERT( begin <= end );

    std::shared_ptr<LexerState> goto_state( new LexerState() );

    const std::set<LexerItem>& items = state->get_items();
    for ( std::set<LexerItem>::const_iterator item = items.begin(); item != items.end(); ++item )
    {
        std::set<RegexNode*, RegexNodeLess> next_nodes = item->next_nodes( begin, end );
        if ( !next_nodes.empty() )
        {
            goto_state->add_item( next_nodes );
        }
    }

    return goto_state;
}

/**
// Generate the states for a LexerStateMachine from \e regex_parser.
//
// @param regex_parser
//  The RegexParser to get the RegexNodes from that are then used to generate 
//  states.
//
// @param states
//  The set of states to populate from the output of the RegexParser (assumed
//  not null).
//
// @param start_state
//  A variable to receive the starting state for the lexical analyzer
//  (assumed not null).
*/
void LexerGenerator::generate_states( const RegexParser& regex_parser, std::set<std::shared_ptr<LexerState>, shared_ptr_less<LexerState>>* states, const LexerState** start_state )
{
    SWEET_ASSERT( states );
    SWEET_ASSERT( states->empty() );
    SWEET_ASSERT( start_state );
    SWEET_ASSERT( !*start_state );

    if ( !regex_parser.empty() && regex_parser.errors() == 0 )
    {
        std::shared_ptr<LexerState> state( new LexerState() );    
        state->add_item( regex_parser.node()->get_first_positions() );
        generate_symbol_for_state( state.get() );
        states->insert( state );
        *start_state = state.get();

        int added = 1;
        while ( added > 0 )
        {
            added = 0;
            for ( std::set<std::shared_ptr<LexerState>, shared_ptr_less<LexerState>>::const_iterator i = states->begin(); i != states->end(); ++i )
            {
                LexerState* state = i->get();
                SWEET_ASSERT( state );

                if ( !state->is_processed() )
                {
                    state->set_processed( true );

                //
                // Create the distinct ranges of characters that can be 
                // transitioned on from the current state.
                //
                    clear();                
                    const std::set<LexerItem>& items = state->get_items();
                    for ( std::set<LexerItem>::const_iterator item = items.begin(); item != items.end(); ++item )
                    {
                        const std::set<RegexNode*, RegexNodeLess>& next_nodes = item->get_next_nodes();
                        for ( std::set<RegexNode*, RegexNodeLess>::const_iterator j = next_nodes.begin(); j != next_nodes.end(); ++j )
                        {
                            const RegexNode* next_node = *j;
                            SWEET_ASSERT( next_node );
                            if ( !next_node->is_end() )
                            {
                                insert( next_node->get_begin_character(), next_node->get_end_character() );
                            }
                        }
                    }
                    
                //
                // Create a goto state and a transition from the current 
                // state for each distinct range.
                //                    
                    vector<pair<int, bool> >::const_iterator j = ranges_.begin();
                    while ( j != ranges_.end() )
                    {               
                        int begin = (j + 0)->first;
                        int end = (j + 1)->first;
                        SWEET_ASSERT( begin < end );
                        
                        std::shared_ptr<LexerState> goto_state = goto_( state, begin, end );
                        if ( !goto_state->get_items().empty() )
                        {                    
                            std::shared_ptr<LexerState> actual_goto_state = *states->insert( goto_state ).first;
                            if ( goto_state == actual_goto_state )
                            {
                                added += 1;
                                generate_symbol_for_state( goto_state.get() );
                            }                            
                            state->add_transition( begin, end, actual_goto_state.get() );
                        }                    

                        ++j;
                        if ( !j->second )
                        {
                            ++j;
                            SWEET_ASSERT( j == ranges_.end() || j->second );
                        }
                    }
                }
            }
        }
    }

    generate_indices_for_states();
}

/**
// Generate indices for the generated states.
*/
void LexerGenerator::generate_indices_for_states()
{
    int index = 0;
    
    for ( std::set<std::shared_ptr<LexerState>, shared_ptr_less<LexerState>>::iterator i = states_.begin(); i != states_.end(); ++i )
    {
        LexerState* state = i->get();
        SWEET_ASSERT( state );
        state->set_index( index );
        ++index;
    }

    for ( std::set<std::shared_ptr<LexerState>, shared_ptr_less<LexerState>>::iterator i = whitespace_states_.begin(); i != whitespace_states_.end(); ++i )
    {
        LexerState* state = i->get();
        SWEET_ASSERT( state );
        state->set_index( index );
        ++index;
    }
}

/**
// Generate the matching symbol for \e state if it has one.
//
// @param state
//  The state to generate a matching symbol for.
*/
void LexerGenerator::generate_symbol_for_state( LexerState* state ) const
{
    SWEET_ASSERT( state );

    int line = INT_MAX;
    LexerTokenType type = TOKEN_NULL;
    const LexerToken* token = NULL;

    const std::set<LexerItem>& items = state->get_items();
    for ( std::set<LexerItem>::const_iterator item = items.begin(); item != items.end(); ++item )
    {
        std::set<RegexNode*, RegexNodeLess>::const_iterator i = item->get_next_nodes().begin();
        while ( i != item->get_next_nodes().end() )
        {
            const RegexNode* node = *i;
            SWEET_ASSERT( node );

            if ( node->is_end() && node->get_token() )
            {
                if ( node->get_token()->type() > type )
                {
                    line = node->get_token()->line();
                    type = node->get_token()->type();
                    token = node->get_token();
                }
                else if ( node->get_token()->type() == type && node->get_token()->line() < line )
                {
                    line = node->get_token()->line();
                    type = node->get_token()->type();
                    token = node->get_token();
                }
                else if ( node->get_token()->type() == type && node->get_token()->line() == line )
                {
                    SWEET_ASSERT( type != TOKEN_NULL );
                    SWEET_ASSERT( line != INT_MAX );
                    SWEET_ASSERT( token );
                    fire_error( token->line(), LexerSymbolConflictError("0x%08x and 0x%08x conflict but are both defined on the same line", token, node->get_token()) );
                }
            }
            
            ++i;
        }
    }    

    state->set_symbol( token ? token->symbol() : NULL );
}

/**
// Clear the current distinct ranges maintained by this LexerGenerator.
*/
void LexerGenerator::clear()
{
    ranges_.clear();
}


/**
// Insert the range [\e begin, \e end) into the current distinct ranges for 
// this LexerGenerator.
//
// The ranges are stored as a vector of pair<int, bool>.  The first element of
// the pair represents the character and the second element represents whether
// or not that character is considered in or out.
//
// This is done so that transitions can be efficiently calculated for 
// independent ranges of characters.  For example if a state has three next 
// nodes that represent characters in the the ranges [0, 256), [0, 32), and 
// [0, 64) then three goto states should be generated with transitions on 
// [0, 32), [32, 64), and [64, 256) respectively.
//
// @param begin
//  The begin character in the range of characters to insert.
//
// @param end
//  The end character in the range of characters to insert.
*/
void LexerGenerator::insert( int begin, int end )
{
    bool in = false;        

    vector<pair<int, bool> >::iterator i = ranges_.begin();
    while ( i != ranges_.end() && i->first < begin )
    {
        in = i->second;
        ++i;
    }        

    if ( i == ranges_.end() || i->first != begin )
    {
        i = ranges_.insert( i, make_pair(begin, true) );
        ++i;
    }
                   
    while ( i != ranges_.end() && i->first < end )
    {
        in = i->second;
        i->second = true;
        ++i;
    }        
    
    if ( i == ranges_.end() || i->first != end )
    {
        ranges_.insert( i, make_pair(end, in) );
    }
}
