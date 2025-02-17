/*****************************************************************************
 * fsm.hpp
 *****************************************************************************
 * Copyright (C) 2003 the VideoLAN team
 * $Id: fsm.hpp 14187 2006-02-07 16:37:40Z courmisch $
 *
 * Authors: Cyril Deguet     <asmax@via.ecp.fr>
 *          Olivier Teulière <ipkiss@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef FSM_HPP
#define FSM_HPP

#include "../src/skin_common.hpp"
#include <string>
#include <map>
#include <set>

class EvtGeneric;
class CmdGeneric;


/// This class implements a Finite State Machine (FSM)
class FSM: public SkinObject
{
    public:
        FSM( intf_thread_t *pIntf ): SkinObject( pIntf ) {}
        virtual ~FSM() {}

        /// Add a state to the machine
        void addState( const string &state );

        /// Add a transition to the machine
        void addTransition( const string &state1, const string &event,
                            const string &state2,
                            CmdGeneric *pCmd = NULL );

        /// Retrieve the current state
        const string &getState() const { return m_currentState; }

        /// Set the current state, without bothering about transitions
        void setState( const string &state );

        /// Find a transition from the current state with the input event,
        /// change the state, and call the associated callback (if any).
        void handleTransition( const string &event );

    private:
        /// A Key_t contains the initial state of a transition, and a string
        /// characterizing an event (for example: "mouse:left:down:ctrl")
        typedef pair<string, string> Key_t;

        /// A Data_t contains the final state of a transition, and a callback
        /// to execute when the transition is applied
        typedef pair<string, CmdGeneric*> Data_t;

        /// Current state of the machine
        string m_currentState;

        /// Set containing the different states
        set<string> m_states;

        /// Map containing the different transitions between defined types
        /// It associates a final state (and potentially a callback)
        /// with a couple of the form: (currentState, triggerEvent)
        map<Key_t, Data_t> m_transitions;
};


#endif
