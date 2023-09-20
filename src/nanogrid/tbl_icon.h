#ifndef DEFINE_ICON
#define NONDEF
#define DEFINE_ICON(icon, bank, val, set) icon set,
#define DEFINE_NO_ICON(icon, set) icon set,
enum {

#else

#ifndef DEFINE_NO_ICON
#define DEFINE_NO_ICON(icon, set)
#endif

#endif


DEFINE_NO_ICON(ICON_NONE,         = 0)

DEFINE_ICON(ICON_TEXTURE,       'a',  2, = 3)
DEFINE_ICON(ICON_CAMERA,        'a',  24,   )
DEFINE_ICON(ICON_FS_BOOKMARK,   'd',  11,   )
DEFINE_ICON(ICON_FS_NEW_FOLDER, 'd',  14,   )
DEFINE_ICON(ICON_FS_REFRESH,    'd',  17,   )
DEFINE_ICON(ICON_FS_FOLDER,     'd',  18,   )
DEFINE_ICON(ICON_FS_FILE,       'd',  19,   )
DEFINE_ICON(ICON_WIND,          'q',  2,    )
DEFINE_ICON(ICON_EYE,           'u',  21,   )
DEFINE_ICON(ICON_SCENE,         'x',  1,    )
DEFINE_ICON(ICON_LIGHT,         'x',  9,    )
DEFINE_ICON(ICON_UNLOCKED,      'y',  17,   )
DEFINE_ICON(ICON_LOCKED,        'y',  18,   )
DEFINE_ICON(ICON_TERMINAL,      'z',  18,   )
DEFINE_ICON(ICON_GHOST,         'ba', 2,    )
DEFINE_ICON(ICON_ARROW_R,       'ca', 20,   )
DEFINE_ICON(ICON_ARROW_D,       'ca', 21,   )
DEFINE_ICON(ICON_INFO,          'da', 2,   )
DEFINE_ICON(ICON_WARNING,       'da', 3,   )
DEFINE_ICON(ICON_ERROR,         'da', 4,   )
DEFINE_ICON(ICON_CLOSE,         'da', 20,   )
DEFINE_ICON(ICON_TRASH,         'da', 22,   )

DEFINE_NO_ICON(ICON_MAX,)

#ifdef NONDEF
};
#endif

#undef NONDEF
#undef DEFINE_ICON
#undef DEFINE_NO_ICON
