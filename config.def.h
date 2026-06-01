/* Taken from https://github.com/djpohly/dwl/issues/466 */
#define COLOR(hex)    { ((hex >> 24) & 0xFF) / 255.0f, \
                        ((hex >> 16) & 0xFF) / 255.0f, \
                        ((hex >> 8) & 0xFF) / 255.0f, \
                        (hex & 0xFF) / 255.0f }
/* appearance */
static const int sloppyfocus               = 1;  /* focus follows mouse */
static const int bypass_surface_visibility = 0;  /* 1 means idle inhibitors will disable idle tracking even if it's surface isn't visible  */
static const int smartgaps                 = 0;  /* 1 means no outer gap when there is only one window */
static int gaps                            = 1;  /* 1 means gaps between windows are added */
static const unsigned int gappx            = 5; /* gap pixel between windows */
static const unsigned int borderpx         = 2;  /* border pixel of windows */
static const float rootcolor[]             = COLOR(0x222222ff);
static const float bordercolor[]           = COLOR(0x444444ff);
static const float focuscolor[]            = COLOR(0xffffffff);
static const float urgentcolor[]           = COLOR(0xff0000ff);
/* This conforms to the xdg-protocol. Set the alpha to zero to restore the old behavior */
static const float fullscreen_bg[]         = {0.1f, 0.1f, 0.1f, 1.0f}; /* You can also use glsl colors */
static const float resize_factor           = 0.0002f; /* Resize multiplier for mouse resizing, depends on mouse sensivity. */
static const uint32_t resize_interval_ms   = 16; /* Resize interval depends on framerate and screen refresh rate. */

enum Direction { DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN };
enum {
    VIEW_L = -1,
    VIEW_R = 1,
    SHIFT_L = -2,
    SHIFT_R = 2,
} RotateTags;

#define SCRATCHPAD_COUNT 3
/* tagging - TAGCOUNT must be no greater than 31 */
#define TAGCOUNT (9)

/* logging */
static int log_level = WLR_ERROR;

static const Rule rules[] = {
	/* app_id             title       tags mask     isfloating   monitor */
	{ "Gimp_EXAMPLE",     NULL,       0,            1,           -1 }, /* Start on currently visible tags floating, not tiled */
	{ "firefox_EXAMPLE",  NULL,       1 << 8,       0,           -1 }, /* Start on ONLY tag "9" */
    /* default/example rule: can be changed but cannot be eliminated; at least one rule must exist */
};

/* layout(s) */
static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "|w|",      btrtile },
	{ "[]=",      tile },
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ "[M]",      monocle },
};

/* monitors */
/* (x=-1, y=-1) is reserved as an "autoconfigure" monitor position indicator
 * WARNING: negative values other than (-1, -1) cause problems with Xwayland clients due to
 * https://gitlab.freedesktop.org/xorg/xserver/-/issues/899 */
static const MonitorRule monrules[] = {
   /* name        mfact  nmaster scale layout       rotate/reflect                x    y
    * example of a HiDPI laptop monitor:
    { "eDP-1",    0.5f,  1,      2,    &layouts[0], WL_OUTPUT_TRANSFORM_NORMAL,   -1,  -1 }, */
    { NULL,       0.55f, 1,      1,    &layouts[0], WL_OUTPUT_TRANSFORM_NORMAL,   -1,  -1 },
	/* default monitor rule: can be changed but cannot be eliminated; at least one monitor rule must exist */
};

/* keyboard */
static const struct xkb_rule_names xkb_rules = {
	/* can specify fields: rules, model, layout, variant, options */
	/* example:
	.options = "ctrl:nocaps",
	*/
	.options = NULL,
};

static const int repeat_rate = 25;
static const int repeat_delay = 600;

/* Trackpad */
static const int tap_to_click = 1;
static const int tap_and_drag = 1;
static const int drag_lock = 1;
static const int natural_scrolling = 0;
static const int disable_while_typing = 1;
static const int left_handed = 0;
static const int middle_button_emulation = 0;
/* You can choose between:
LIBINPUT_CONFIG_SCROLL_NO_SCROLL
LIBINPUT_CONFIG_SCROLL_2FG
LIBINPUT_CONFIG_SCROLL_EDGE
LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN
*/
static const enum libinput_config_scroll_method scroll_method = LIBINPUT_CONFIG_SCROLL_2FG;

/* You can choose between:
LIBINPUT_CONFIG_CLICK_METHOD_NONE
LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS
LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER
*/
static const enum libinput_config_click_method click_method = LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS;

/* You can choose between:
LIBINPUT_CONFIG_SEND_EVENTS_ENABLED
LIBINPUT_CONFIG_SEND_EVENTS_DISABLED
LIBINPUT_CONFIG_SEND_EVENTS_DISABLED_ON_EXTERNAL_MOUSE
*/
static const uint32_t send_events_mode = LIBINPUT_CONFIG_SEND_EVENTS_ENABLED;

/* You can choose between:
LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT
LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE
*/
static const enum libinput_config_accel_profile accel_profile = LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT;
static const double accel_speed = 0.0;

/* You can choose between:
LIBINPUT_CONFIG_TAP_MAP_LRM -- 1/2/3 finger tap maps to left/right/middle
LIBINPUT_CONFIG_TAP_MAP_LMR -- 1/2/3 finger tap maps to left/middle/right
*/
static const enum libinput_config_tap_button_map button_map = LIBINPUT_CONFIG_TAP_MAP_LRM;

/* If you want to use the windows key for MODKEY, use WLR_MODIFIER_LOGO */
#define MODKEY WLR_MODIFIER_LOGO

#define TAGKEYS(KEY,SKEY,TAG) \
	{ MODKEY,                    KEY,            view,            {.ui = 1 << TAG} }, \
	{ MODKEY|WLR_MODIFIER_CTRL,  KEY,            toggleview,      {.ui = 1 << TAG} }, \
	{ MODKEY|WLR_MODIFIER_SHIFT, SKEY,           tag,             {.ui = 1 << TAG} }, \
	{ MODKEY|WLR_MODIFIER_CTRL|WLR_MODIFIER_SHIFT,SKEY,toggletag, {.ui = 1 << TAG} }

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
/*static const char *termcmd[] = { "foot", NULL };*/
static const char *termcmd[] = { "kitty", NULL };
/*static const char *menucmd[] = { "wmenu-run", "-b" };*/
static const char *menucmd[] = { "rofi", "-show", "run", NULL };
static const char *screenshot[] = { "/bin/sh", "-c", "~/.dotfiles/scripts/sscb.sh", NULL };
static const char *partialss[] = { "/bin/sh", "-c", "~/.dotfiles/scripts/psscb.sh", NULL };

static const char *mediaplaypause[] = { "playerctl", "play-pause", NULL };
static const char *mediaprev[] = { "playerctl", "previous", NULL };
static const char *medianext[] = { "playerctl", "next", NULL };

static const Key keys[] = {
	/* Note that Shift changes certain key codes: c -> C, 2 -> at, etc. */
	/* modifier                  key                 function        argument */

        /* Open application launcher (rofi) */
	{ MODKEY,                    XKB_KEY_space,      spawn,          {.v = menucmd} },

        /* Take partial screenshot based on manual drag selection */
	{ MODKEY|WLR_MODIFIER_ALT,   XKB_KEY_p,          spawn,          {.v = screenshot} },

        /* Take screenshot of one screen, selected by clicking */
	{ MODKEY,                    XKB_KEY_p,          spawn,          {.v = partialss} },

        /* Open terminal (kitty) */
	{ MODKEY,		     XKB_KEY_Return,     spawn,          {.v = termcmd} },

        /* Shift to Previous/Next client on active tag */
	{ MODKEY,                    XKB_KEY_e,          focusstack,     {.i = +1} },
	{ MODKEY,                    XKB_KEY_r,          focusstack,     {.i = -1} },
        /* Shift to Left/Right/Up/Down client on active tag */
	{ MODKEY,                    XKB_KEY_h,          focusdir,       {.ui = 0} },
	{ MODKEY,                    XKB_KEY_k,          focusdir,       {.ui = 1} },
	{ MODKEY,                    XKB_KEY_u,          focusdir,       {.ui = 2} },
	{ MODKEY,                    XKB_KEY_j,          focusdir,       {.ui = 3} },

        /* No idea */
	{ MODKEY,                    XKB_KEY_i,          incnmaster,     {.i = +1} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_I,          incnmaster,     {.i = -1} },

        /* Shift to Previous/Next tag on active monitor */
	{ MODKEY,                    XKB_KEY_a,          rotatetags,     {.i = VIEW_L} },
	{ MODKEY,                    XKB_KEY_d,          rotatetags,     {.i = VIEW_R} },
        /* Move active client to Previous/Next tag on active monitor */
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_A,          rotatetags,     {.i = SHIFT_L} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_D,          rotatetags,     {.i = SHIFT_R} },

        /* Not sure */
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_H,          setmfact,       {.f = -0.05f} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_K,          setmfact,       {.f = +0.05f} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_Return,     zoom,           {0} },

        /* Toggle the gaps around clients */
	{ MODKEY,                    XKB_KEY_g,          togglegaps,     {0} },

        /* Kill active client */
	{ MODKEY, 		     XKB_KEY_x,          killclient,     {0} },

        /* Add/Remove active client to/from scratchpad (think special workspace in Hyprland) */
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_S,          addscratchpad,    {0} },
	{ MODKEY|WLR_MODIFIER_CTRL,  XKB_KEY_s,          removescratchpad, {0} },
        /* Toggle show/hide scratchpad */
	{ MODKEY,                    XKB_KEY_s,          togglescratchpad, {0} },

        /* btrtile layout; allows you to resize and such. If you can't manipulate clients, use this */
	{ MODKEY,                    XKB_KEY_t,          setlayout,      {.v = &layouts[0]} },

        /* Other layouts that I can't be bothered with */
	{ MODKEY,                    XKB_KEY_f,          setlayout,      {.v = &layouts[1]} },
	{ MODKEY,                    XKB_KEY_m,          setlayout,      {.v = &layouts[2]} },
        /* Not sure */
	{ MODKEY,                    XKB_KEY_o,      setlayout,      {0} },

        /* Toggle floating tile for active client */
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_space,      togglefloating, {0} },

        /* Toggle fullscreen for active client */
	{ MODKEY,                    XKB_KEY_b,         togglefullscreen, {0} },

        /* Combine/Restore all clients on all tags for active monitor */
	{ MODKEY,                    XKB_KEY_0,          view,           {.ui = ~0} },
	{ MODKEY,                    XKB_KEY_Tab,        view,           {0} },
        /* Not sure, but likely has to do with the top of the two above commands */
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_parenright, tag,            {.ui = ~0} },

        /* Shift active monitor to Previous/Next monitor */
	{ MODKEY,                    XKB_KEY_q,      focusmon,       {.i = WLR_DIRECTION_LEFT} },
	{ MODKEY,                    XKB_KEY_w,     focusmon,       {.i = WLR_DIRECTION_RIGHT} },
        /* Shift active client to Previous/Next monitor */
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_Q,       tagmon,         {.i = WLR_DIRECTION_LEFT} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_W,    tagmon,         {.i = WLR_DIRECTION_RIGHT} },

        /* Move active client Up/Down/Right/Left on active monitor */
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_Up,         swapclients,    {.i = DIR_UP} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_Down,       swapclients,    {.i = DIR_DOWN} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_Right,      swapclients,    {.i = DIR_RIGHT} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_Left,       swapclients,    {.i = DIR_LEFT} },
        /* Resize active client Right/Left/Up/Down */
	{ MODKEY|WLR_MODIFIER_CTRL,  XKB_KEY_Right,      setratio_h,     {.f = +0.025f} },
	{ MODKEY|WLR_MODIFIER_CTRL,  XKB_KEY_Left,       setratio_h,     {.f = -0.025f} },
	{ MODKEY|WLR_MODIFIER_CTRL,  XKB_KEY_Up,         setratio_v,     {.f = -0.025f} },
	{ MODKEY|WLR_MODIFIER_CTRL,  XKB_KEY_Down,       setratio_v,     {.f = +0.025f} },

        /* Control playing audio/media, using standard media pause/prev/next keys */
	{ 0,                         XKB_KEY_XF86AudioPlay,  spawn,     {.v = mediaplaypause} },
	{ 0,                         XKB_KEY_XF86AudioPrev,   spawn,     {.v = mediaprev} },
	{ 0,                         XKB_KEY_XF86AudioNext,  spawn,     {.v = medianext} },

        /* Allows you to switch tags on active monitor using Mod + numrow number
         * or move active client to specific numbered tag using Mod + Shift + numrow number */
	TAGKEYS(          XKB_KEY_1, XKB_KEY_exclam,                     0),
	TAGKEYS(          XKB_KEY_2, XKB_KEY_at,                         1),
	TAGKEYS(          XKB_KEY_3, XKB_KEY_numbersign,                 2),
	TAGKEYS(          XKB_KEY_4, XKB_KEY_dollar,                     3),
	TAGKEYS(          XKB_KEY_5, XKB_KEY_percent,                    4),
	TAGKEYS(          XKB_KEY_6, XKB_KEY_asciicircum,                5),
	TAGKEYS(          XKB_KEY_7, XKB_KEY_ampersand,                  6),
	TAGKEYS(          XKB_KEY_8, XKB_KEY_asterisk,                   7),
	TAGKEYS(          XKB_KEY_9, XKB_KEY_parenleft,                  8),

        /* Shutdown dwl */
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_X,          quit,           {0} },

	/* Ctrl-Alt-Backspace and Ctrl-Alt-Fx used to be handled by X server */
	{ WLR_MODIFIER_CTRL|WLR_MODIFIER_ALT,XKB_KEY_Terminate_Server, quit, {0} },
	/* Ctrl-Alt-Fx is used to switch to another VT, if you don't know what a VT is
	 * do not remove them.
	 */
#define CHVT(n) { WLR_MODIFIER_CTRL|WLR_MODIFIER_ALT,XKB_KEY_XF86Switch_VT_##n, chvt, {.ui = (n)} }
	CHVT(1), CHVT(2), CHVT(3), CHVT(4), CHVT(5), CHVT(6),
	CHVT(7), CHVT(8), CHVT(9), CHVT(10), CHVT(11), CHVT(12),
};

static const Button buttons[] = {
	{ MODKEY, BTN_LEFT,   moveresize,     {.ui = CurMove} },
	{ MODKEY, BTN_MIDDLE, togglefloating, {0} },
	{ MODKEY, BTN_RIGHT,  moveresize,     {.ui = CurResize} },
};
