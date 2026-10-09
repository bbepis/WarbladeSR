#pragma once
// Named values for the game's magic numbers: state/mode/kind ids, array sizes, bit flags.
// Every name here is a plain integer constant (an enumerator), so using it instead of the
// literal compiles to the same bytes. Values the original compares or assigns but whose
// meaning is unknown keep an UNKNOWN/UNUSED name rather than a guess.
//
// Integer constants are enumerators. A float value may be named with a #define (a textual
// substitution, so the same literal), never a `const float` (that would change the code), and variable/field types stay as declared in types.h
// and globals.h (an enum-typed field would change its size).


// ============================================================================
// Game flow
// ============================================================================

// g_state (and g_savedState): the top-level state GameFrame() dispatches on.
enum GameState {
    STATE_PAUSED            = 0,    // paused: quit/profile windows open, timers frozen
    STATE_UNUSED_1          = 1,    // never set; only compared (render.c border width)
    STATE_PLAYING           = 2,    // normal gameplay
    STATE_TITLE             = 5,    // title / main menu
    STATE_HISCORE_TABLE     = 6,    // hiscore/tally table after a run
    STATE_POST_ROUND_IDLE   = 7,    // attract-mode countdown on the tally screen
    STATE_SHOP              = 9,    // in the shop between levels
    STATE_BONUS_RACE        = 10,   // bonus race stage (entered by ITEM_METEOR_STORM)
    STATE_MEMORY_STATION    = 11,   // memory-station bonus stage (card grid)
    STATE_RESPAWN           = 12,   // after death / NewGame, before the "get ready"
    STATE_MALFUNCTION_DEATH = 13,   // warp-malfunction death sequence (PLAYER_DIED)
    STATE_ENTER_HISCORE     = 15,   // entering a hiscore name
    STATE_MALFUNCTION       = 16,   // gameplay with the warp-malfunction alarm ticking
    STATE_METEOR_STORM      = 17,   // meteor storm bonus stage / its results
    STATE_GEM_DROP          = 18,   // Gem Drop bonus stage (falling gems)
    STATE_UNUSED_19         = 19,   // never set; GameMain skips rendering in it
    STATE_SHOP_GATE         = 20,   // end-of-round rank popup, decides shop vs next level
    STATE_UNUSED_21         = 21,   // never set; in MsgHook's no-auto-pause list
    STATE_GET_READY         = 22,   // "get ready" countdown of a freshly started level
    STATE_INPUT_CONFIG      = 23,   // input-configuration menu
    STATE_END_SEQUENCE      = 25    // EndSequence(): the ending slideshow
};

// g_gameMode / g_pendingGameMode. Names follow CrashHandler's log text.
enum GameMode {
    MODE_SINGLE          = 0,   // "SINGLE PLAYER GAME" (F1)
    MODE_TWO_PLAYER      = 1,   // "TWO PLAYER GAME" (F2): players take turns, each on their own level
    MODE_DUAL            = 2,   // "DUAL PLAYER GAME" (F3, "2 players duel"): both ships on one level
    MODE_TEAM            = 3,   // "TEAM PLAYER GAME": never started from the menus
    MODE_UNUSED_4        = 4,   // never set; empty case in several switches
    MODE_ACE_TOURNAMENT  = 5,   // "ACE_TURNAMENT GAME": never started from the menus
    MODE_TIME_TRIAL      = 6    // "TIME TRIAL GAME" (F5)
};

// g_cfg.difficulty.
enum Difficulty {
    DIFF_EASY   = 0,
    DIFF_NORMAL = 1,
    DIFF_HARD   = 2,
    DIFF_ACE    = 3,
    NUM_DIFFICULTIES = 4
};

// g_hofMode: which Hall of Fame / hiscore table is shown or written.
// 0..3 are the difficulties (table[difficulty]), 4 is table[4], 5 is table5.
enum HallOfFameMode {
    HOF_EASY        = 0,
    HOF_NORMAL      = 1,
    HOF_HARD        = 2,
    HOF_ACE         = 3,
    HOF_METEORSTORM = 4,
    HOF_TIME_TRIAL  = 5
};

// g_attractScreen: the title-screen page cycle.
enum AttractScreen {
    ATTRACT_INTRO         = 0,  // IntroFrame
    ATTRACT_ABOUT         = 1,  // AboutScreen
    ATTRACT_MISSION       = 2,  // MissionScreen
    ATTRACT_HELP_CONTROLS = 3,  // HelpControls
    ATTRACT_HELP_BONUSES  = 4,  // HelpBonuses
    ATTRACT_HALL_OF_FAME  = 5   // HallOfFame
};

// g_curLevelData.type.
enum LevelType {
    LEVEL_WAVE       = 1,   // normal wave; aliens that finish their path hover in formation
    LEVEL_BONUS_WAVE = 2,   // bonus wave; aliens drift and escape upwards at the end of their path
    LEVEL_RACE       = 3,   // aliens that finish their path escape (counted)
    LEVEL_BOSS       = 4,   // a single boss enemy
    LEVEL_WAVE_AIMED = 6    // like LEVEL_WAVE, but enemy fire always aims at the player
};


// ============================================================================
// Settings (g_cfg)
// ============================================================================

// g_cfg.device0 / device1 (indexed as (&g_cfg.device0)[player]).
enum InputDevice {
    DEVICE_KEYBOARD  = 0,
    DEVICE_JOYSTICK1 = 1,
    DEVICE_JOYSTICK2 = 2
};

// g_cfg.musicFormat.
enum MusicFormat {
    MUSIC_FMT_MOD      = 0,     // data\music\<song>.mus (BASS module)
    MUSIC_FMT_MP3      = 1,     // data\music\<song>.mp3
    MUSIC_FMT_PLAYLIST = 2      // random line of playlist.m3u
};

// g_cfg.renderer: the SDL render driver (a button on the settings page). The byte was PTK's
// DirectX (1) / OpenGL (0) choice, so 0 and 1 both mean auto, and the original exe reads every
// value but 0 as DirectX. Anything out of range is auto too (RendererChoiceOf).
enum RendererChoice {
    RENDERER_AUTO_OLD_OPENGL = 0,   // an original WarBlade.inf that chose OpenGL
    RENDERER_AUTO      = 1,         // SDL picks (no SDL_HINT_RENDER_DRIVER)
    RENDERER_DIRECTX9  = 2,         // "direct3d"
    RENDERER_DIRECTX11 = 3,         // "direct3d11"
    RENDERER_DIRECTX12 = 4,         // "direct3d12"
    RENDERER_OPENGL    = 5,         // "opengl"
    RENDERER_VULKAN    = 6,         // "vulkan"
    RENDERER_COUNT
};

// g_cfg.borderMode ("SCROLLING BORDER").
enum BorderMode {
    BORDER_ON    = 0,
    BORDER_OFF   = 1,
    BORDER_BLACK = 2
};

// g_cfg.bulletIntensity.
enum BulletIntensity {
    BULLETS_NORMAL   = 0,
    BULLETS_BRIGHT   = 1,
    BULLETS_FLARE_FX = 2
};

// g_cfg.collisionDetail ("COLLISION DETECTION").
enum CollisionDetail {
    COLLISION_SIMPLE = 0,
    COLLISION_NORMAL = 3
};

// g_bgTint: the colour tint of the level background.
enum BgTint {
    BGTINT_RED   = 0,
    BGTINT_GREEN = 1,
    BGTINT_BLUE  = 2
};


// ============================================================================
// Player
// ============================================================================

// Player::ship: the ship types (g_shipDefs), picked at random for each new game.
enum { NUM_SHIPS = 10 };

// Player::weapon.
enum WeaponType {
    WEAPON_SINGLE       = 0,
    WEAPON_DOUBLE       = 1,
    WEAPON_TRIPLE       = 2,
    WEAPON_QUAD         = 3,
    WEAPON_SUPER_TRIPLE = 4,
    WEAPON_PLASMA       = 5,
    WEAPON_FIREBALLS    = 6,
    WEAPON_LASER        = 7,
    WEAPON_WAR_PLASMA   = 8     // "WAR.I.PLASMA"
};

// Player::rank (also HiscoreEntry::rank). The ADMIRAL_n_m / GRANDMASTER_n ranks
// share their title and add pips to the rank badge.
enum PlayerRank {
    RANK_ENSIGN        = 0,
    RANK_LIEUTENANT    = 1,
    RANK_COMMANDER     = 2,
    RANK_CAPTAIN       = 3,
    RANK_ADMIRAL       = 4,
    RANK_ADMIRAL_1_1   = 5,
    RANK_ADMIRAL_1_2   = 6,
    RANK_ADMIRAL_1_3   = 7,
    RANK_ADMIRAL_2_1   = 8,
    RANK_ADMIRAL_2_2   = 9,
    RANK_ADMIRAL_2_3   = 10,
    RANK_ADMIRAL_3_1   = 11,
    RANK_ADMIRAL_3_2   = 12,
    RANK_ADMIRAL_3_3   = 13,
    RANK_KNIGHT        = 14,    // "WARBLADE KNIGHT"
    RANK_LORD          = 15,
    RANK_OVERLORD      = 16,
    RANK_GRANDMASTER   = 17,
    RANK_GRANDMASTER_1 = 18,
    RANK_GRANDMASTER_2 = 19,
    RANK_GRANDMASTER_3 = 20,
    RANK_CHAMPION      = 21,
    RANK_GOD           = 22,
    RANK_GOD_PLUTO     = 23,
    RANK_GOD_NEPTUNE   = 24,
    RANK_GOD_URANUS    = 25,
    RANK_GOD_SATURN    = 26,
    RANK_GOD_JUPITER   = 27,
    RANK_GOD_MARS      = 28,
    RANK_GOD_TELLUS    = 29,
    RANK_GOD_VENUS     = 30,
    RANK_GOD_MERCURY   = 31,
    RANK_GOD_SOL       = 32,
    MAX_RANK           = 32
};

// Player::marks: the six rank-marker gems (bought in the shop, or collected
// in order as ITEM_RANK_GEM_1..6).
enum RankMarks {
    MARK_1    = 0x01,
    MARK_2    = 0x02,
    MARK_3    = 0x04,
    MARK_4    = 0x08,
    MARK_5    = 0x10,
    MARK_6    = 0x20,
    MARKS_ALL = 0x3f
};

// g_acc.medals (AwardMedal / ClearMedalsMask / GetMedals).
enum Medal {
    MEDAL_DRUNK_FINISH = 0x01,  // level finished at full speed in drunk mode
    MEDAL_SPEED_STREAK = 0x02,  // five full-speed finishes in a row
    MEDAL_BONUS_RATIO  = 0x04,  // perfect bonus-round ratio (CheckRatioMedal)
    MEDAL_ALL_LEVELS   = 0x08,  // every level's secret found (CheckAllLevelsMedal)
    MEDAL_OVERALL      = 0x10,  // all profile toggles done; cleared by every profile reset
    MEDAL_EXACT_MONEY  = 0x20,  // money exactly at the magic amount
    MEDALS_ALL         = 0x3f
};


// ============================================================================
// Objects
// ============================================================================

// g_items[].type and Pickup()'s argument.
enum ItemType {
    ITEM_LETTER_E          = 0,
    ITEM_LETTER_X          = 1,
    ITEM_LETTER_T          = 2,
    ITEM_LETTER_R          = 3,
    ITEM_LETTER_A          = 4,
    ITEM_RANDOM_BONUS      = 5,     // rolls a weighted type from g_itemTypePool
    ITEM_MEMORY_STATION    = 6,     // enters STATE_MEMORY_STATION
    ITEM_TIMES2            = 7,
    ITEM_TIMES5            = 8,
    ITEM_EXTRA_BULLET      = 9,
    ITEM_EXTRA_SPEED       = 10,
    ITEM_SHIELD            = 11,
    ITEM_WEAPON_SINGLE     = 12,
    ITEM_WEAPON_DOUBLE     = 13,
    ITEM_WEAPON_TRIPLE     = 14,
    ITEM_WARP              = 15,    // finishes the level
    ITEM_SCOOP             = 16,
    ITEM_WEAPON_QUAD       = 17,
    ITEM_AUTOFIRE          = 18,
    ITEM_GEM_BOMB          = 19,
    ITEM_METEOR_STORM      = 20,    // enters STATE_BONUS_RACE
    ITEM_ARMOUR            = 21,
    ITEM_SUCKER_BLUE_MONEY = 22,    // sucker traps: secrets counters
    ITEM_SUCKER_GEMS       = 23,
    ITEM_SUCKER_MULTIPLIER = 24,
    ITEM_MIRROR            = 25,
    ITEM_MONEY_BOMB        = 26,
    ITEM_EXTRA_LIFE        = 27,
    ITEM_EXTRA_TIME        = 28,
    ITEM_MONEY_SMALL       = 29,
    ITEM_MONEY_MEDIUM      = 30,
    ITEM_MONEY_LARGE       = 31,
    ITEM_MONEY_BLUE        = 32,
    ITEM_MONEY_DOUBLER     = 33,
    ITEM_DRUNK             = 34,
    ITEM_FREEZE            = 35,
    ITEM_EXTRA_BULLET_SPEED = 36,
    ITEM_MONEY_SMALL_BURST  = 38,   // money flung out by a burst; turns into ITEM_MONEY_SMALL
    ITEM_MONEY_MEDIUM_BURST = 39,
    ITEM_MONEY_LARGE_BURST  = 40,
    ITEM_MONEY_BLUE_BURST   = 41,
    ITEM_MONEY_SMALL_TALLY  = 42,   // scooped money flying to the score counter
    ITEM_MONEY_MEDIUM_TALLY = 43,
    ITEM_MONEY_LARGE_TALLY  = 44,
    ITEM_MONEY_BLUE_TALLY   = 45,
    ITEM_GEM               = 46,
    ITEM_STAR              = 47,    // drifting starfield item (UpdateStarItems)
    ITEM_RANK_GEM_1        = 48,    // sets MARK_1 ... ITEM_RANK_GEM_6 sets MARK_6
    ITEM_RANK_GEM_2        = 49,
    ITEM_RANK_GEM_3        = 50,
    ITEM_RANK_GEM_4        = 51,
    ITEM_RANK_GEM_5        = 52,
    ITEM_RANK_GEM_6        = 53,
    ITEM_BAD_GEM           = 64     // "OH NO": clears the rank marks
};

// Shop(): g_shopSelItem - 1 (g_shopSelItem 0 is the exit row).
enum ShopItem {
    SHOP_EXTRA_SPEED    = 0,
    SHOP_EXTRA_BULLET   = 1,
    SHOP_DOUBLE_SHOT    = 2,
    SHOP_LESS_SPEED     = 3,
    SHOP_TRIPLE_SHOT    = 4,
    SHOP_QUAD_SHOT      = 5,
    SHOP_AUTO_FIRE      = 6,
    SHOP_SUPER_TRIPLE   = 7,
    SHOP_SHIP_ARMOUR    = 8,
    SHOP_PLASMA         = 9,
    SHOP_EXTRA_LIFE     = 10,
    SHOP_FIRE_BALLS     = 11,
    SHOP_GAME_SECRET    = 12,
    SHOP_RANK_MARKER    = 13,
    SHOP_EXTRA_TIME     = 14,
    SHOP_LASER_BEAM     = 15,
    SHOP_WAR_I_PLASMA   = 16,
    SHOP_ROCKET_PACK    = 17,
    SHOP_ALIEN_LOCK     = 18,
    SHOP_SUPER_AUTOFIRE = 19,
    SHOP_CLEAR_SHIELDS  = 20
};

// Enemy::type (g_enemies[player][i].type).
enum EnemyType {
    ENEMY_PATTERNED    = 1,     // follows its group's path in g_curLevelData
    ENEMY_HOVER        = 2,     // holds a formation slot, sometimes dives
    ENEMY_DIVING       = 3,     // diving attack run (from ENEMY_HOVER)
    ENEMY_FLYBY        = 4,     // zig-zags down, wraps to the top
    ENEMY_DEBRIS       = 5,     // falling debris of a destroyed enemy
    ENEMY_WRAPPER      = 6,     // roams and wraps around all screen edges, aimed shots
    ENEMY_UNKNOWN_7    = 7,     // drawn like type 1, no update case
    ENEMY_CAPTURED     = 8,     // grabbed by the player's shield, rides beside the ship
    ENEMY_MOTHERSHIP   = 9,     // HurryUp() mothership, fires rockets
    ENEMY_ESCAPER      = 10,    // flies off the top of the screen
    ENEMY_MONEY_SUCKER = 11,    // drains the player's money
    ENEMY_MONEY_SHIP   = 12,    // every 8th HurryUp()
    ENEMY_BOSS         = 13,
    ENEMY_GUARD        = 18     // g_gfxGuard; fires beam walls
};

// LevelObj::type (g_levelObj[i].type): enemy shots and hazards.
enum LevelObjType {
    LOBJ_AIMED_SHOT  = 6,       // slow aimed shot (ENEMY_WRAPPER, boss guns)
    LOBJ_SHOT        = 7,       // ordinary straight-down enemy bullet
    LOBJ_ROCKET      = 9,       // mothership rocket with a fuse
    LOBJ_BOSS_BURST  = 14,      // boss radial burst shot
    LOBJ_BOSS_SHOT   = 15,      // shot from a boss gun hardpoint
    LOBJ_BEAM        = 18,      // beam-wall shot of ENEMY_GUARD
    LOBJ_ROCKET_B    = 200      // drawn like LOBJ_ROCKET, not updated like it
};

// Memory-station cards: g_cards[r][c].type.
enum CardType {
    CARD_LOSE_TIME          = 1,
    CARD_SCORE_X2           = 2,
    CARD_SCORE_X5           = 3,
    CARD_SCORE_100          = 4,  // score 100 (times the multiplier)
    CARD_SCORE_1000         = 5,  // score 1000
    CARD_SCORE_10000        = 6,  // score 10000
    CARD_MONEY_DOUBLER      = 7,
    CARD_MONEY_50           = 8,  // money 50
    CARD_MONEY_100          = 9,  // money 100
    CARD_MONEY_200          = 10, // money 200
    CARD_EXTRA_TIME         = 11,
    CARD_EXTRA_LIFE         = 12,
    CARD_MARK_6             = 13, // LETTER(MARK_6) ... CARD_MARK_1 = LETTER(MARK_1)
    CARD_MARK_5             = 14,
    CARD_MARK_4             = 15,
    CARD_MARK_3             = 16,
    CARD_MARK_2             = 17,
    CARD_MARK_1             = 18,
    CARD_EXTRA_BULLET       = 19,
    CARD_GEM_A              = 24, // 24..30: gem cards
    CARD_GEM_B              = 25,
    CARD_GEM_C              = 26,
    CARD_GEM_D              = 27,
    CARD_GEM_E              = 28,
    CARD_GEM_F              = 29,
    CARD_GEM_G              = 30,
    CARD_LETTER_E           = 31,
    CARD_LETTER_X           = 32,
    CARD_LETTER_T           = 33,
    CARD_LETTER_R           = 34,
    CARD_LETTER_A           = 35,
    CARD_EXTRA_SPEED        = 36,
    CARD_EXTRA_BUFF_TIME    = 37,
    CARD_SECRET_BIRD        = 38
};

// g_bonusMeteors[i].type.
enum BonusMeteorType {
    METEOR_HAZARD = 0,
    METEOR_MONEY  = 1
};

// g_musicMode: which track PlayNextMusic() restarts.
enum MusicMode {
    MUSIC_TITLE     = 0,
    MUSIC_CUSTOM    = 1,    // g_songNames[] list
    MUSIC_TIMETRIAL = 2,
    MUSIC_BOSS      = 3,
    MUSIC_MEMORY    = 4,
    MUSIC_METEOR    = 5,
    MUSIC_PROMOTED  = 6,
    MUSIC_GEM_DROP  = 7,
    MUSIC_HISCORE   = 8,
    MUSIC_SHOP      = 9,
    MUSIC_END       = 10
};


// ============================================================================
// Sizes (array capacities; "MAX_X - 1" is the last index)
// ============================================================================

enum {
    NUM_PLAYERS             = 4,        // g_save.players[]
    MAX_ENEMIES             = 150,      // g_enemies[2][150]
    MAX_ITEMS               = 150,      // g_items[]
    MAX_LEVEL_OBJS          = 100,      // g_levelObj[]
    MAX_MAP_OBJS            = 100,      // g_mapObjs[]
    MAX_EXPLOSIONS          = 50,       // g_explosions[]
    MAX_EXPLOSION_PARTICLES = 500,      // g_explosionParticles[]
    MAX_PARTICLES           = 1000,     // g_particles[]
    MAX_SPARKS              = 2000,     // g_sparks[]
    MAX_SLOTS               = 1000,     // g_slots[] (burst sparks)
    MAX_SPARKLE_FLASHES     = 10,       // g_sparkleFlashes[]
    MAX_LOGO_FLASHES        = 49,       // g_logoFlashes[]
    MAX_BEAMS               = 12,       // g_beams[]
    MAX_FLASH_RINGS         = 4,        // g_flash[]
    MAX_FX                  = 10,       // g_fx[]
    MAX_BONUS_METEORS       = 30,       // g_bonusMeteors[]
    MAX_FALLING_GEMS        = 10,       // g_fallingGems[]
    MAX_POPUPS              = 10,       // g_popups[]
    MAX_SCOOP               = 15,       // g_scoop[]
    NUM_STARS               = 3000,     // g_stars[]
    MAX_ALIEN_GFX_SLOTS     = 200,      // g_alienGfxSlots[]
    NUM_HAZARD_GFX          = 6,        // g_alienGfxCache[]
    MAX_BOSS_GUNS           = 10,       // Enemy::bossGunASlot[] ...
    MAX_PATTERNS            = 50,       // g_patterns[] (att%03d.swd)
    MAX_PATTERN_ENTRIES     = 150,      // Pattern::entries[]
    MAX_ENEMY_GROUPS        = 255,      // g_groupEnemyCount[] ...
    NUM_BONUS_WEIGHTS       = 37,       // g_bonusWeight[]
    NUM_METEOR_VARIANTS     = 46,       // g_bonusGfxArea[]
    MAX_LEVEL_RECS          = 4000,     // g_levelRecs[], g_replayRecs[][]
    NUM_HISCORE_TABLES      = 5,        // HiscoreData::table[][] (+ table5)
    MAX_HISCORES            = 20,       // entries per hiscore table
    MAX_SOUND_QUEUE         = 10,       // g_soundQueue[]
    MAX_PLAYLIST_LINES      = 10000,    // g_playlistLines[]
    MAX_LINKS               = 30,       // g_links[]
    MAX_MENU_ENTRIES        = 85,       // g_menuEntries[]
    MAX_WINDOWS             = 10,       // g_windows[]
    MAX_WINDOW_IMAGE_RECTS  = 5,        // Window::imageRects[]
    MAX_WINDOW_MENU_ITEMS   = 40,       // Window::menuItems[]
    MAX_BLITS               = 2000,     // g_blit[], g_blit2[]
    MAX_STRETCH_ROT         = 1000,     // g_stretchRot[]
    MAX_STRETCH_ROT2        = 20,       // g_stretchRot2[]
    MAX_STRETCH_I           = 150,      // g_stretchI[]
    MAX_STRETCH_F           = 1500,     // g_stretchF[]
    NAME_LEN                = 30,       // Account::name[], g_name[]
    NUM_LEVEL_SECRETS       = 50,       // Account::levelDone[]
    MAX_CLASSIC_LEVELS      = 500,      // classic_level_NNN.lvd probed
    MAX_TIME_TRIAL_LEVELS   = 50        // timetrial_NN.lvd probed
};


// ============================================================================
// Shared tuning values and sentinels
// ============================================================================

enum {
    // x (sometimes y) argument of DrawMenuText/DrawTinyText/WinOpen/WinAddText/AddMenuText...:
    // centre the text or window on the screen instead of placing it.
    POS_CENTERED            = -5000,

    // Added to g_time into g_transitionLockUntil (with g_transitionLock = 1) after a click or
    // key, so one press doesn't trigger twice. The short lock is for press-and-hold repeating
    // controls (volume, star sliders).
    TRANSITION_LOCK_MS      = 500,
    TRANSITION_LOCK_FAST_MS = 50,

    // How long a hotkey banner (g_optionMsg / g_msgTime) stays on screen.
    MSG_DURATION_MS         = 2000,

    // g_menuIdleTimeout = g_time + MENU_IDLE_MS: menu inactivity before the attract cycle.
    MENU_IDLE_MS            = 120000,
    // g_idleTimeoutMs default: how long an attract screen stays up.
    ATTRACT_SCREEN_MS       = 60000,

    // End-of-stage delay: a finished bonus stage waits this long before handing back.
    DONE_DELAY_MS           = 3000,

    // g_cfg.fps: game speed per difficulty. Note the two different ACE values in the original:
    // the menu's ACE hotkey sets 200, restoring a suspended ACE game sets 80.
    FPS_EASY                = 50,
    FPS_NORMAL              = 60,
    FPS_HARD                = 70,
    FPS_ACE_MENU            = 200,
    FPS_ACE_RESTORE         = 80,

    // "No best time recorded yet" (Account best times, compared with <).
    NO_TIME_RECORDED        = 999999999,

    // Score reached at a level-100 milestone that unlocks secret 26, the marathon score and
    // the secret-bird icons.
    MARATHON_MILESTONE_SCORE = 200000000,
    // Levels per background/theme cycle and per marathon milestone (level % 100); the first
    // milestone counts only after level 5.
    LEVEL_THEME_CYCLE       = 100,
    MARATHON_START_LEVEL    = 5,

    // Fixed frame size of the hazard/portal sprites (g_hazardNGfxW/H).
    HAZARD_GFX_W            = 0x240,
    HAZARD_GFX_H            = 0x60,

    // g_cfg.bgTint: background brightness, a 6-step cycle 40, 55, ... 115 (not the
    // BgTint colour above). The quality presets use 70.
    BG_BRIGHTNESS_MIN       = 40,
    BG_BRIGHTNESS_STEP      = 15,
    BG_BRIGHTNESS_MAX       = 115,
    BG_BRIGHTNESS_DEFAULT   = 55,
    BG_BRIGHTNESS_PRESET    = 70,

    // Player::bullets cap.
    MAX_BULLETS             = 50,

    // WinOpen() mode: tiled frame, or the sliding popup style every menu popup uses.
    WIN_MODE_TILED          = 1,
    WIN_MODE_SLIDING        = 2,

    // g_mapObjs[].state of a homing missile (vs. plain debris). Unrelated to LOBJ_ROCKET_B.
    MAPOBJ_STATE_HOMING     = 200,

    // Money-bag drop roll (1-100): <= SMALL_MAX is ITEM_MONEY_SMALL, <= MEDIUM_MAX
    // ITEM_MONEY_MEDIUM, <= LARGE_MAX ITEM_MONEY_LARGE, above that ITEM_MONEY_BLUE. The money
    // sucker's steal uses the same bands.
    MONEY_ROLL_SMALL_MAX    = 40,
    MONEY_ROLL_MEDIUM_MAX   = 67,
    MONEY_ROLL_LARGE_MAX    = 87,

    // Spawn x of a hazard enemy / money ship: RandRange(0, (g_screenW - SPAN) >> 1) + MARGIN.
    SPAWN_X_MARGIN          = 100,
    SPAWN_X_SPAN            = 200,

    // SoundPlay / SoundQueueAdd volume and pan.
    SFX_VOL_FULL            = 255,
    SFX_PAN_CENTER          = 127
};

// Money the player needs for the shop to open / stay open (compared with the float money).
#define SHOP_MIN_MONEY 50.0


// ============================================================================
// Menu ids (built and handled in menu.c)
// ============================================================================

// Ids for g_clicked (a main-menu / options-page entry built by AddMenuText/AddMenuItem) and
// g_clickItem (a popup-window button built by WinAddMenuItem). The two use separate globals,
// so the same numeric value can mean two different things; each member below says which one it
// is. Only entries whose meaning was checked against InitMenu()/the popup that builds them and
// against MenuUpdate()/MenuHandler() are named; a few auto-numbered ids (blank spacer rows,
// stray unused ids like 151) are left as comments rather than guessed at.
enum MenuId {
    // -- g_clicked: main-menu / attract-screen top level (InitMenu, id 10-91) --
    MENUID_START             = 10,   // "START" top-level label: same as START_1P
    MENUID_START_1P          = 11,
    MENUID_START_2P          = 12,
    MENUID_START_2P_DUEL     = 13,
    // 14: blank spacer row between START 2P DUEL and START TIME TRIAL
    MENUID_START_TIME_TRIAL  = 15,
    MENUID_DEMO_GAME         = 16,   // also fires when the menu has been idle past g_menuIdleTimeout
    MENUID_ABOUT             = 20,
    MENUID_STORY             = 30,   // "STORY": opens ATTRACT_MISSION
    MENUID_SETTINGS          = 40,   // "SETTINGS" top-level label: opens ATTRACT_HELP_CONTROLS
    MENUID_INPUT             = 41,   // "INPUT" submenu of SETTINGS: opens STATE_INPUT_CONFIG
    MENUID_BONUSES           = 50,
    MENUID_HISCORE           = 60,   // "HISCORE" top-level label: hall of fame at the current difficulty
    MENUID_HOF_EASY          = 61,
    MENUID_HOF_NORMAL        = 62,
    MENUID_HOF_HARD          = 63,
    MENUID_HOF_ACE           = 64,
    MENUID_HOF_METEORSTORM   = 65,
    MENUID_HOF_TIME_TRIAL    = 66,
    MENUID_HELP              = 70,
    MENUID_FAQ               = 80,
    MENUID_USER_PROFILES     = 90,   // opens the "USER PROFILES" list window directly
    MENUID_USER_MANUAL       = 91,
    MENUID_QUIT_YES          = 100,  // "YES" on the top-level QUIT confirm: goodbye sound + terminate

    // -- g_clicked: options-page sliders/toggles (InitMenu, id 101-136 and 700-703) --
    MENUID_DIFFICULTY_PREV       = 101,
    MENUID_DIFFICULTY_NEXT       = 102,
    MENUID_MUSIC_FORMAT_PREV     = 103,  // CycleMusicFormat(); both arrows do the same thing
    MENUID_MUSIC_FORMAT_NEXT     = 104,
    MENUID_MUSICVOLUME_MUTE      = 105,  // g_cfg.musicVolume / ApplyMusicVolume()
    MENUID_MUSICVOLUME_DOWN      = 106,
    MENUID_MUSICVOLUME_UP        = 107,
    MENUID_MUSICVOLUME_MAX       = 108,
    MENUID_SFXVOL_MUTE           = 109,  // g_cfg.sfxVol / SetSfxVolume()
    MENUID_SFXVOL_DOWN           = 110,
    MENUID_SFXVOL_UP             = 111,
    MENUID_SFXVOL_MAX            = 112,
    MENUID_SFX_TOGGLE_PREV       = 113,  // g_cfg.sfxOn on/off toggle; both arrows do the same thing
    MENUID_SFX_TOGGLE_NEXT       = 114,
    MENUID_BORDER_MODE_PREV      = 115,  // cycles BORDER_BLACK/ON/OFF; both arrows do the same thing
    MENUID_BORDER_MODE_NEXT      = 116,
    MENUID_BG_ENABLED_PREV       = 117,  // g_cfg.bgEnabled toggle; both arrows do the same thing
    MENUID_BG_ENABLED_NEXT       = 118,
    MENUID_BG_BRIGHTNESS_PREV    = 119,  // cycles g_cfg.bgTint down the BG_BRIGHTNESS_* ladder
    MENUID_BG_BRIGHTNESS_NEXT    = 120,
    // 121-126: unused (121-124 were skipped in the original; 125-126 were the colour-depth arrows)
    MENUID_NUM_STARS_DOWN        = 127,
    MENUID_NUM_STARS_UP          = 128,
    MENUID_COLLISION_DETAIL_PREV = 129,  // COLLISION_SIMPLE/NORMAL toggle; both arrows do the same
    MENUID_COLLISION_DETAIL_NEXT = 130,
    MENUID_PARTICLES_PREV        = 131,  // g_cfg.particlesOn toggle; both arrows do the same thing
    MENUID_PARTICLES_NEXT        = 132,
    MENUID_BULLET_INTENSITY_PREV = 133,  // cycles g_cfg.bulletIntensity; both arrows do the same
    MENUID_BULLET_INTENSITY_NEXT = 134,
    MENUID_BG_STARS_PREV         = 135,  // g_cfg.bgStars (starfield style) toggle; both arrows same
    MENUID_BG_STARS_NEXT         = 136,
    MENUID_JUKEBOX                = 137,
    MENUID_CONFIG                 = 139,  // opens STATE_INPUT_CONFIG, same as MENUID_INPUT/F9
    MENUID_VOICE_PACK_NEXT        = 150,  // "NEXT VOICE PACK"; 151 is an unused twin, never wired up
    MENUID_SPARKS_DOWN            = 152,  // "E" + shift decreases g_cfg.sparks
    MENUID_SPARKS_UP              = 153,  // "E" increases g_cfg.sparks
    MENUID_TOGGLE_RENDERER        = 172,  // cycles g_cfg.renderer
    MENUID_TOGGLE_SHUFFLE         = 173,  // playlist random/sequential
    MENUID_TOGGLE_PROFILE_LIST    = 174,  // "A": same list window as MENUID_USER_PROFILES
    MENUID_TOGGLE_INPUT_SWAP      = 175,  // SwapKeyBindings()
    MENUID_TOGGLE_WINDOWED        = 176,
    // 177 was "TOGGLE SOUND MIXER" (BASS hardware/software mixing), gone with the SDL port
    MENUID_TOGGLE_VSYNC           = 710,  // g_cfg.vsyncOff (SDL port)
    MENUID_TOGGLE_INTERPOLATION   = 711,  // cycles g_cfg.interpolation (SDL port)
    MENUID_MUSICVOL_MUTE          = 700,  // second volume slider: g_cfg.musicVol / SetMusicVolTable()
    MENUID_MUSICVOL_DOWN          = 701,
    MENUID_MUSICVOL_UP            = 702,
    MENUID_MUSICVOL_MAX           = 703,
    MENUID_ALIEN_BUFFER_DOWN      = 0x409,
    MENUID_ALIEN_BUFFER_UP        = 0x40a,
    MENUID_ONLINE_HALL_OF_FAME    = 555,    // hiscore screen: opens the archived online hall of fame

    // -- g_clickItem: window-close and generic dialog sentinels --
    MENUID_CLOSE             = 0xff,   // generic close/cancel/no; used by almost every popup
    MENUID_CANCEL_ALL        = 0xfc,   // close + WinCloseAll() + clear g_profileWinOpen
    MENUID_LOGIN_OK          = 0xfd,
    MENUID_LOGIN_CANCEL      = 0x9f6,
    MENUID_NEW_PROFILE_OK    = 0xfe,   // "NORMAL PROFILE" in the new-profile window
    MENUID_NEW_PROFILE_EASY  = 0x9ec,  // "EASY PROFILE": opens the easy-profile warning
    MENUID_EASY_PROFILE_YES  = 0x9ed,  // confirms creating the easy profile

    // -- g_clickItem: profile menu (logout, select, reset, backup/restore, name/password) --
    MENUID_LOGOUT                 = 0x106,
    MENUID_TOGGLE_PROFILE_SEL     = 0x107,  // set/clear g_cfg.profileSel to the current profile
    MENUID_OPEN_RESET_CATEGORY    = 0x109,  // opens the "RESET PROFILE" category picker
    MENUID_OPEN_BACKUP_PROFILE    = 0x10a,
    MENUID_OPEN_RESTORE_WARNING   = 0x10b,
    MENUID_RESTORE_PROFILE_YES    = 0xa6e,
    MENUID_OPEN_CHANGE_USERNAME   = 0x10c,
    MENUID_CHANGE_USERNAME_OK     = 0xa78,
    MENUID_OPEN_CHANGE_PASSWORD   = 0x10d,
    MENUID_CHANGE_PASSWORD_OK     = 0xa82,
    MENUID_RESET_TIMETRIAL        = 0xa5a,
    MENUID_RESET_SECRETS          = 0xa5b,
    MENUID_RESET_PERCENTAGES      = 0xa5c,
    MENUID_RESET_BONUS_ROUNDS     = 0xa5d,
    MENUID_RESET_TIMES            = 0xa5e,
    MENUID_RESET_ALL_SCORES       = 0xa5f,
    MENUID_RESET_ALL              = 0xa60,

    // -- g_clickItem: quit-game / quit-to-Windows confirmations (QuitGameDialog/QuitToWindowsDialog) --
    MENUID_QUIT_GAME_CONFIRM      = 0xc8,   // 200: "QUIT CURRENT GAME"
    MENUID_RETIRE_CONFIRM         = 200001, // "RETIRE FROM GAME"
    MENUID_QUIT_GAME_TO_WINDOWS   = 0xc9,   // 201: "QUIT TO WINDOWS" (from the in-game quit dialog)
    MENUID_QUIT_GAME_CONTINUE     = 0xca,   // 202: "CONTINUE GAME"
    MENUID_QUIT_TO_WINDOWS_YES    = 0xcb,   // 203
    MENUID_QUIT_TO_WINDOWS_NO     = 0xcc,   // 204
    MENUID_JUKEBOX_LAUNCH_CONFIRM = 0x108,

    // -- g_clickItem: generic dialog buttons --
    MENUID_DIALOG_DISMISS_A       = 10003,  // plain close, no side effect
    MENUID_DIALOG_DISMISS_B       = 4999,   // plain close, no side effect (different dialog)
    MENUID_DIALOG_NO              = 7980,   // OpenCreateProfileWin()'s "NO"

    // -- g_clickItem: profile list / login windows (also PROFILE_USE_ID_BASE/VIEW_ID_BASE + slot) --
    MENUID_PROFILE_NEW            = 100,    // "NEW" in the USER PROFILES list window
    MENUID_CREATE_PROFILE_YES     = 49992,  // OpenCreateProfileWin()'s "YES"
    MENUID_QUICK_START_1P         = 79802,  // NewGame(true) from a first-run quick-start prompt
    MENUID_QUICK_START_TIME_TRIAL = 499921,

    // -- g_clickItem: first-run quality presets (offered before the first profile is created) --
    MENUID_QUALITY_PRESET_LOW     = 4000,
    MENUID_QUALITY_PRESET_MEDIUM  = 4001,
    MENUID_QUALITY_PRESET_HIGH    = 4002,
    MENUID_QUALITY_PRESET_ULTRA   = 4003,
};
