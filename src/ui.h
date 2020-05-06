#pragma once
#ifndef CATA_SRC_UI_H
#define CATA_SRC_UI_H

#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "color.h"
#include "cursesdef.h"
#include "point.h"
#include "string_formatter.h"

class translation;

////////////////////////////////////////////////////////////////////////////////////
/**
 * uilist constants
 */
const int UILIST_ERROR = -1024;
const int UILIST_WAIT_INPUT = -1025;
const int UILIST_UNBOUND = -1026;
const int UILIST_CANCEL = -1027;
const int UILIST_TIMEOUT = -1028;
const int UILIST_ADDITIONAL = -1029;
const int MENU_WIDTH_ENTRIES = -2;
const int MENU_AUTOASSIGN = -1;
// NOLINTNEXTLINE(cata-use-named-point-constants)
constexpr point MENU_AUTOASSIGN_POS( MENU_AUTOASSIGN, MENU_AUTOASSIGN );

class input_context;
class string_input_popup;
class ui_adaptor;
struct input_event;

catacurses::window new_centered_win( int nlines, int ncols );

/**
 * mvwzstr: line of text with horizontal offset and color
 */

struct mvwzstr {
    int left = 0;
    nc_color color = c_unset;
    std::string txt;
    int sym = 0;
};

/**
 * uilist_entry: entry line for uilist
 */
struct uilist_entry {
    int retval;                 // return this int
    bool enabled;               // darken, and forbid scrolling if hilight_disabled is false
    bool force_color = false;   // Never darken this option
    int hotkey;                 // keycode from (int)getch(). -1: automagically pick first free character: 1-9 a-z A-Z
    std::string txt;            // what it says on the tin
    std::string desc;           // optional, possibly longer, description
    std::string ctxt;           // second column text
    nc_color hotkey_color;
    nc_color text_color;
    mvwzstr extratxt;

    //std::string filtertxt; // possibly useful
    uilist_entry( std::string T ) : retval( -1 ), enabled( true ), hotkey( -1 ), txt( T ) {
        text_color = c_red_red;
    }
    uilist_entry( std::string T, std::string D ) : retval( -1 ), enabled( true ), hotkey( -1 ),
        txt( T ), desc( D ) {
        text_color = c_red_red;
    }
    uilist_entry( std::string T, int K ) : retval( -1 ), enabled( true ), hotkey( K ), txt( T ) {
        text_color = c_red_red;
    }
    uilist_entry( int R, bool E, int K, std::string T ) : retval( R ), enabled( E ), hotkey( K ),
        txt( T ) {
        text_color = c_red_red;
    }
    uilist_entry( int R, bool E, int K, std::string T, std::string D ) : retval( R ), enabled( E ),
        hotkey( K ), txt( T ), desc( D ) {
        text_color = c_red_red;
    }
    uilist_entry( int R, bool E, int K, std::string T, std::string D, std::string C ) : retval( R ),
        enabled( E ),
        hotkey( K ), txt( T ), desc( D ), ctxt( C ) {
        text_color = c_red_red;
    }
    uilist_entry( int R, bool E, int K, std::string T, nc_color H, nc_color C ) : retval( R ),
        enabled( E ), hotkey( K ), txt( T ),
        hotkey_color( H ), text_color( C ) {}
};

/**
 * Generic multi-function callback for highlighted items, key presses, and window control. Example:
 *
 * class monmenu_cb: public uilist_callback {
 *   public:
 *   bool key(int ch, int num, uilist * menu) {
 *     if ( ch == 'k' && num > 0 ) {
 *       std::vector<monster> * game_z=static_cast<std::vector<monster>*>(myptr);
 *       game_z[num]->dead = true;
 *     }
 *   }
 *   void refresh( uilist *menu ) {
 *       if( menu->selected >= 0 && static_cast<size_t>( menu->selected ) < game_z.size() ) {
 *           mvwprintz( menu->window, 0, 0, c_red, "( %s )",game_z[menu->selected]->name() );
 *           wrefresh( menu->window );
 *       }
 *   }
 * }
 * uilist monmenu;
 * for( size_t i = 0; i < z.size(); ++i ) {
 *   monmenu.addentry( z[i].name );
 * }
 * monmenu_cb * cb;
 * cb->setptr( &g->z );
 * monmenu.callback = cb
 * monmenu.query();
 *
 */
class uilist;

/**
* uilist::query() handles most input events first,
* and then passes the event to the callback if it can't handle it.
*
* The callback returninig a boolean false signifies that the callback can't "handle the
* event completely". This is unchanged before or after the PR.
* @{
*/
class uilist_callback
{
    public:
        virtual void select( uilist * ) {}
        virtual bool key( const input_context &, const input_event &/*key*/, int /*entnum*/,
                          uilist * ) {
            return false;
        }
        virtual void refresh( uilist * ) {}
        virtual ~uilist_callback() = default;
};
/*@}*/
/**
 * uilist: scrolling vertical list menu
 */

class uilist // NOLINT(cata-xy)
{
    public:
        uilist();
        uilist( const std::string &hotkeys_override );
        // query() will be called at the end of these convenience constructors
        uilist( const std::string &msg, const std::vector<uilist_entry> &opts );
        uilist( const std::string &msg, const std::vector<std::string> &opts );
        uilist( const std::string &msg, std::initializer_list<const char *const> opts );
        uilist( const point &start, int width, const std::string &msg,
                const std::vector<uilist_entry> &opts );
        uilist( const point &start, int width, const std::string &msg,
                const std::vector<std::string> &opts );
        uilist( const point &start, int width, const std::string &msg,
                std::initializer_list<const char *const> opts );

        ~uilist();

        void init();
        void setup();
        // initialize the window or reposition it after screen size change.
        void reposition( ui_adaptor &ui );
        void show();
        bool scrollby( int scrollby );
        void query( bool loop = true, int timeout = -1 );
        void filterlist();
        void addentry( const std::string &str );
        void addentry( int r, bool e, int k, const std::string &str );
        // K is templated so it matches a `char` literal and a `int` value.
        // Using a fixed type (either `char` or `int`) will lead to ambiguity with the
        // other overload when called with the wrong type.
        template<typename K, typename ...Args>
        void addentry( const int r, const bool e, K k, const char *const format, Args &&... args ) {
            return addentry( r, e, k, string_format( format, std::forward<Args>( args )... ) );
        }
        void addentry_desc( const std::string &str, const std::string &desc );
        void addentry_desc( int r, bool e, int k, const std::string &str, const std::string &desc );
        void addentry_col( int r, bool e, int k, const std::string &str, const std::string &column,
                           const std::string &desc = "" );
        void settext( const std::string &str );

        void reset();

        operator int() const;

    private:
        int scroll_amount_from_key( int key );
        int scroll_amount_from_action( const std::string &action );
        void apply_scrollbar();
        // This function assumes it's being called from `query` and should
        // not be made public.
        void inputfilter();

    public:
        // Parameters
        // TODO change to setters
        std::string title;
        std::string text;
        // basically the same as desc, except it doesn't change based on selection
        std::string footer_text;
        std::vector<uilist_entry> entries;

        std::string input_category;
        std::vector<std::pair<std::string, translation>> additional_actions;

        nc_color border_color;
        nc_color text_color;
        nc_color title_color;
        nc_color hilight_color;
        nc_color hotkey_color;
        nc_color disabled_color;

        uilist_callback *callback;

        int textwidth;

        int pad_left = 0;
        int pad_right = 0;

        int desc_lines;
        bool desc_enabled;

        bool filtering;
        bool filtering_nocase;

        // return on selecting disabled entry, default false
        bool allow_disabled = false;
        // return UILIST_UNBOUND on keys unbound & unhandled by callback, default false
        bool allow_anykey = false;
        // return UILIST_CANCEL on "QUIT" action, default true
        bool allow_cancel = true;
        // return UILIST_ADDITIONAL if the input action is inside `additional_actions`
        // and unhandled by callback, default false.
        bool allow_additional = false;
        bool hilight_disabled = false;

    private:
        std::string hotkeys;

    public:
        // Iternal states
        // TODO make private
        std::vector<std::string> textformatted;

        catacurses::window window;
        int w_x;
        int w_y;
        int w_width;
        int w_height;

        int vshift = 0;

        int fselected = 0;

    private:
        std::vector<int> fentries;
        std::map<int, int> keymap;

        std::unique_ptr<string_input_popup> filter_popup;
        std::string filter;

        int max_entry_len;
        int max_column_len;

        int vmax = 0;

        bool w_x_autoassigned = false;
        bool w_y_autoassigned = false;

        bool started = false;

    public:
        // Results
        // TODO change to getters
        std::string ret_act;
        int ret;
        int keypress;

        int selected;
};

/**
 * Callback for uilist that pairs menu entries with points
 * When an entry is selected, view will be centered on the paired point
 */
class pointmenu_cb : public uilist_callback
{
    private:
        const std::vector< tripoint > &points;
        int last; // to suppress redrawing
        tripoint last_view; // to reposition the view after selecting
    public:
        pointmenu_cb( const std::vector< tripoint > &pts );
        ~pointmenu_cb() override = default;
        void refresh( uilist *menu ) override;
};

#endif // CATA_SRC_UI_H
