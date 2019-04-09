#ifndef GAME_SERVER_ENTITIES_DUMMY_H
#define GAME_SERVER_ENTITIES_DUMMY_H

//dummymode 32 vars (BlmapChill police guard)
int m_LovedX;
int m_LovedY;
int m_LowerPanic;
int m_DummySpeed;
int m_HelpMode; //0=off 1=right side 2=right_side_extreme
bool m_HelpHook;
bool m_ClosestPolice;

int m_DummyGrenadeJump;
bool m_DummyTouchedGround;
bool m_DummyAlreadyBeenHere;
bool m_DummyStartGrenade;
bool m_DummyUsedDJ;
int m_DummySpawnTeleporter; // 1 = left, 2 = middle, 3 = right (the totele 9 at spawn)
bool m_ReachedCinemaEntrance;

#endif