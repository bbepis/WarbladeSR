#pragma once
// support.h: Shared set-up for the game-logic tests.
#include "test.h"
#include "fake_engine.h"
#include "globals.h"
#include "game.h"

// Runs the game's real start-up (GameMain: settings, profiles, graphics, sounds, tables, the
// splash screens, the title menu) against the fake engine, and returns when GameMain reaches
// its main loop; the game is then on the title screen. No game data: images and sounds are
// the fake's, and no levels exist unless the test added them (FakePacAdd) first.
void BootGame(void);
// Runs `n` iterations of the main loop's body (GameMain): mouse, GameFrame, render, flip.
void RunFrames(int n);
// RunFrames until `done()` holds, at most `max` frames; returns whether it held.
bool RunFramesUntil(bool (*done)(void), int max);
// Presses `key` for `frames` frames, then releases it for `frames` frames.
void TapKey(enum EKeyboardLayout key, int frames);
