/**
 * @file refboxlistener.cc
 * @brief RefboxListener source file
 */
#include "refboxlistener.h"
#include <libbsmart/referee_percept.h>
#include "commands.h"
//#include "../ConfigFile/ConfigFile.h"
#include "global.h"

// log4cxx
using namespace log4cxx;
LoggerPtr RefboxListener::logger ( Logger::getLogger ( "RefboxListener" ) );

/**
 * @brief Initialize RefboxListener
 * Get IP and port from config file, open socket, init vars
 * @param gamestate_ BSmart::Game_States
 */
RefboxListener::RefboxListener ( BSmart::Game_States* gamestate_ ) :
        gamestate ( gamestate_ )
{
    socket = 0;
    string message = "";

    string refbox_ip = Global::config.read<string> ( "refbox_ip", "224.5.23.1" );
    uint16_t refbox_port = Global::config.read<uint16_t> ( "refbox_port", 10001 );

    std::ostringstream o;
    if ( ! ( o << refbox_port ) )
        LOG4CXX_ERROR ( logger, "Could not convert port to string" );
    message = "IP: ";
    message += refbox_ip;
    message += " port: ";
    message += o.str();
    LOG4CXX_INFO ( logger, message );

    try {
        socket = new BSmart::Multicast_Socket();
        socket->bind ( refbox_ip.c_str(), refbox_port );
        socket->set_non_blocking();
    } catch ( BSmart::IO_Exception e ) {
        message = "Could not open referee socket: ";
        message += e.what();
        LOG4CXX_ERROR ( logger, message );
    }

    buffer = new char[65536];
    buflen = 65536;
    has_new_data = false;
}

RefboxListener::~RefboxListener()
{
}

/**
 * @brief Start listing for refbox commands in endless loop
 */
void RefboxListener::run()
{
    LOG4CXX_DEBUG ( logger, "run()" );
    while ( 1 ) {
        execute();
        msleep ( 20 );
    }
}

/**
 * @brief Try reading refbox command from socket and execute it
 */
void RefboxListener::execute()
{
    ssize_t r_result ( 0 );

    //this was received (if has_new_data is true)
    //initialization is not needed (but compiler will warn).
    GameStatePacket gsp = { 0, 0, 0, 0, 0 };

    // there may be no data available
    try {
        //read all data in network queue
        while ( 1 ) {
            r_result = socket->read ( buffer, buflen );

            // we expect packets with 6 bytes:
            // http://small-size.informatik.uni-bremen.de/referee:protocol
            if ( r_result == 6 ) {
                // parse referee data in buffer here
                gsp.cmd = buffer[0];
                gsp.cmd_counter = buffer[1];
                gsp.goals_blue = buffer[2];
                gsp.goals_yellow = buffer[3];
                gsp.time_remaining = ntohs ( * ( ( unsigned short* ) ( buffer + 4 ) ) );
            }

            // counter grew bigger or there was
            // a overflow after 2^32 referee commands.
            if ( gsp.cmd_counter != gsp_last.cmd_counter ) {
                has_new_data = true;
            }
        }
    } catch ( BSmart::Multicast_Socket::No_Data_Available e ) {
    }

    // counter grew bigger or there was
    // a overflow after 2^32 referee commands.
    if ( has_new_data ) {
        std::ostringstream o;
        o << "got referee command: " << gsp.cmd;
        LOG4CXX_DEBUG ( logger, o.str() );
        new_refbox_cmd ( gsp.cmd );
        gsp_last = gsp;
        has_new_data = false;
    }

}

/**
 * @brief send refbox command comd to gamestate and input_serial
 * @param comd
 */
void RefboxListener::new_refbox_cmd ( char comd )
{
    if ( comd != gsp_last.cmd ) {
        gamestate->set_refbox_cmd ( comd );
        input_serial ( comd );
        gsp_last.cmd = comd;
    }
}

/**
 * @brief Map a cmd from serial to a Referee_Percept command
 * @param cmd command from commands.h
 */
void RefboxListener::input_serial ( const char cmd )
{
    std::ostringstream o;
    o << "byte from serial: " << ( int ) cmd << "  " << cmd;
    LOG4CXX_DEBUG ( logger, o.str() );

    switch ( cmd ) {
        case COMM_START:
            update_game_state ( Referee_Percept::START );
            break;
        case COMM_STOP:
            update_game_state ( Referee_Percept::STOP );
            break;
        case COMM_HALT:
            update_game_state ( Referee_Percept::HALT );
            break;
        case COMM_READY:
            update_game_state ( Referee_Percept::READY );
            break;
        case COMM_CANCEL:
            update_game_state ( Referee_Percept::CANCEL );
            break;
        case COMM_TIMEOUT_YELLOW:
            update_game_state ( Referee_Percept::TIMEOUT_YELLOW );
            break;
        case COMM_TIMEOUT_BLUE:
            update_game_state ( Referee_Percept::TIMEOUT_BLUE );
            break;
        case COMM_KICKOFF_YELLOW:
            update_game_state ( Referee_Percept::KICKOFF_YELLOW );
            break;
        case COMM_KICKOFF_BLUE:
            update_game_state ( Referee_Percept::KICKOFF_BLUE );
            break;
        case COMM_DIRECT_YELLOW:
            update_game_state ( Referee_Percept::FREEKICK_YELLOW );
            break;
        case COMM_DIRECT_BLUE:
            update_game_state ( Referee_Percept::FREEKICK_BLUE );
            break;
        case COMM_PENALTY_YELLOW:
            update_game_state ( Referee_Percept::PENALTY_YELLOW );
            break;
        case COMM_PENALTY_BLUE:
            update_game_state ( Referee_Percept::PENALTY_BLUE );
            break;
        case COMM_GOAL_YELLOW:
            update_game_state ( Referee_Percept::GOAL_YELLOW );
            break;
        case COMM_GOAL_BLUE:
            update_game_state ( Referee_Percept::GOAL_BLUE );
            break;
        case COMM_SUBGOAL_YELLOW:
            update_game_state ( Referee_Percept::DGOAL_YELLOW );
            break;
        case COMM_SUBGOAL_BLUE:
            update_game_state ( Referee_Percept::DGOAL_BLUE );
            break;
        case COMM_YELLOWCARD_YELLOW:
            update_game_state ( Referee_Percept::YELLOW_CARD_YELLOW );
            break;
        case COMM_YELLOWCARD_BLUE:
            update_game_state ( Referee_Percept::YELLOW_CARD_BLUE );
            break;
        case COMM_REDCARD_YELLOW:
            update_game_state ( Referee_Percept::RED_CARD_YELLOW );
            break;
        case COMM_REDCARD_BLUE:
            update_game_state ( Referee_Percept::RED_CARD_BLUE );
            break;
        case COMM_INDIRECT_YELLOW:
            update_game_state ( Referee_Percept::INDIRECT_YELLOW );
            break;
        case COMM_INDIRECT_BLUE:
            update_game_state ( Referee_Percept::INDIRECT_BLUE );
            break;
        case COMM_FIRST_HALF:
            update_game_state ( Referee_Percept::BEGIN_FIRST_HALF );
            break;
        case COMM_HALF_TIME:
            update_game_state ( Referee_Percept::BEGIN_HALF_TIME );
            break;
        case COMM_SECOND_HALF:
            update_game_state ( Referee_Percept::BEGIN_SECOND_HALF );
            break;
        case COMM_OVER_TIME1:
            update_game_state ( Referee_Percept::BEGIN_OVERTIME_HALF_1 );
            break;
        case COMM_OVER_TIME2:
            update_game_state ( Referee_Percept::BEGIN_OVERTIME_HALF_2 );
            break;
        case COMM_PENALTY_SHOOTOUT:
            update_game_state ( Referee_Percept::BEGIN_PENALTY_SHOOTOUT );
            break;
        default:
            std::ostringstream o;
            o << "Unknown byte from serial: " << ( int ) cmd;
            LOG4CXX_DEBUG ( logger, o.str() );
    }

}

/**
 * @brief Handle signals from referee
 * Depending on the referee_signal, actions will be performed
 * @param referee_signal Referee_Percept
 */
void RefboxListener::update_game_state ( const int referee_signal )
{
    switch ( referee_signal ) {
        case Referee_Percept::START:
            gamestate->set_play_state ( BSmart::Game_States::RUNNING );
            if ( !gamestate->timer_game_get_overall() )
                gamestate->timer_game_start();
            else
                gamestate->timer_game_unpause();
            break;

        case Referee_Percept::STOP:
            gamestate->timer_game_unpause();
            if ( gamestate->timer_timeout_yellow_get_status() == BSmart::ACTIVE )
                gamestate->timer_timeout_yellow_pause();
            else if ( gamestate->timer_timeout_blue_get_status() == BSmart::ACTIVE )
                gamestate->timer_timeout_blue_pause();

            gamestate->set_play_state ( BSmart::Game_States::STOPPED );
            break;

        case Referee_Percept::HALT:
            gamestate->set_play_state ( BSmart::Game_States::HALTED );
            gamestate->timer_game_pause();
            break;

        case Referee_Percept::CANCEL:
            break;

        case Referee_Percept::READY:
            switch ( gamestate->get_play_state() ) {
                case BSmart::Game_States::BEFORE_KICKOFF_BLUE:
                    gamestate->set_play_state ( BSmart::Game_States::KICKOFF_BLUE );
                    break;
                case BSmart::Game_States::BEFORE_PENALTY_BLUE:
                    gamestate->set_play_state ( BSmart::Game_States::PENALTY_BLUE );
                    break;
                case BSmart::Game_States::BEFORE_KICKOFF_YELLOW:
                    gamestate->set_play_state ( BSmart::Game_States::KICKOFF_YELLOW );
                    break;
                case BSmart::Game_States::BEFORE_PENALTY_YELLOW:
                    gamestate->set_play_state ( BSmart::Game_States::PENALTY_YELLOW );
                    break;
                default:
                    break;
            }
            if ( !gamestate->timer_game_get_overall() )
                gamestate->timer_game_start();
            break;

        case Referee_Percept::KICKOFF_YELLOW:
            gamestate->set_play_state ( BSmart::Game_States::BEFORE_KICKOFF_YELLOW );
            break;

        case Referee_Percept::KICKOFF_BLUE:
            gamestate->set_play_state ( BSmart::Game_States::BEFORE_KICKOFF_BLUE );
            break;

        case Referee_Percept::FREEKICK_YELLOW:
            gamestate->set_play_state ( BSmart::Game_States::DIRECT_FREEKICK_YELLOW );
            break;

        case Referee_Percept::FREEKICK_BLUE:
            gamestate->set_play_state ( BSmart::Game_States::DIRECT_FREEKICK_BLUE );
            break;

        case Referee_Percept::INDIRECT_YELLOW:
            gamestate->set_play_state ( BSmart::Game_States::INDIRECT_FREEKICK_YELLOW );
            break;

        case Referee_Percept::INDIRECT_BLUE:
            gamestate->set_play_state ( BSmart::Game_States::INDIRECT_FREEKICK_BLUE );
            break;

        case Referee_Percept::PENALTY_YELLOW:
            gamestate->set_play_state ( BSmart::Game_States::BEFORE_PENALTY_YELLOW );
            break;

        case Referee_Percept::PENALTY_BLUE:
            gamestate->set_play_state ( BSmart::Game_States::BEFORE_PENALTY_BLUE );
            break;

        case Referee_Percept::TIMEOUT_YELLOW:
            if ( !gamestate->timer_timeout_yellow_get_overall() )
                gamestate->timer_timeout_yellow_start();
            else
                gamestate->timer_timeout_yellow_unpause();
            gamestate->timer_game_pause();
            break;

        case Referee_Percept::TIMEOUT_BLUE:
            if ( !gamestate->timer_timeout_blue_get_overall() )
                gamestate->timer_timeout_blue_start();
            else
                gamestate->timer_timeout_blue_unpause();
            gamestate->timer_game_pause();
            break;

        case Referee_Percept::YELLOW_CARD_YELLOW:
            gamestate->add_card ( 1, 0, 0, 0 );

        case Referee_Percept::RED_CARD_YELLOW:
            gamestate->add_card ( 0, 1, 0, 0 );

        case Referee_Percept::YELLOW_CARD_BLUE:
            gamestate->add_card ( 0, 0, 1, 0 );

        case Referee_Percept::RED_CARD_BLUE:
            gamestate->add_card ( 0, 0, 0, 1 );

        case Referee_Percept::GOAL_YELLOW:
            gamestate->add_goal ( 1, 0 );

        case Referee_Percept::GOAL_BLUE:
            gamestate->add_goal ( 0, 1 );

        case Referee_Percept::DGOAL_YELLOW:
            gamestate->add_goal ( -1, 0 );

        case Referee_Percept::DGOAL_BLUE:
            gamestate->add_goal ( 0, -1 );

        case Referee_Percept::BEGIN_FIRST_HALF:
            gamestate->set_game_time ( BSmart::Game_States::FIRST_HALF );
            break;

        case Referee_Percept::BEGIN_HALF_TIME:
            gamestate->set_game_time ( BSmart::Game_States::HALF_TIME );
            break;

        case Referee_Percept::BEGIN_SECOND_HALF:
            gamestate->set_game_time ( BSmart::Game_States::SECOND_HALF );
            break;

        case Referee_Percept::BEGIN_OVERTIME_HALF_1:
            gamestate->set_game_time ( BSmart::Game_States::OVERTIME_HALF_1 );
            break;

        case Referee_Percept::BEGIN_OVERTIME_HALF_2:
            gamestate->set_game_time ( BSmart::Game_States::OVERTIME_HALF_2 );
            break;

        case Referee_Percept::BEGIN_PENALTY_SHOOTOUT:
            gamestate->set_game_time ( BSmart::Game_States::PENALTY_SHOOTOUT );
            break;

        default:
            std::ostringstream o;
            o << "Unknown Referee-Signal " << referee_signal << " received!";
            LOG4CXX_DEBUG ( logger, o.str() );
    }
}

