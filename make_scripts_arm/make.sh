#!/bin/bash

echo "---------------------------------------"
echo "QVM Compiler"
echo "Compile: game.qvm, cgame.qvm, ui.qvm"
echo "---------------------------------------"

cd ..

mkdir -p linux/build
mkdir -p linux/game/qvm
mkdir -p linux/build/game

cp -p make_scripts_arm/asm linux/build/game/
cp -p make_scripts_arm/lcc linux/build/game/
cp -p make_scripts_arm/cpp linux/build/game/
cp -p make_scripts_arm/rcc linux/build/game/
cp -p make_scripts_arm/game.q3asm linux/build/

LIBRARY=""
INCLUDE=""

cc="./lcc -DGAME -DQVM -S -I../../../code/game -I../../../code/qcommon $1"

cd linux/build/game

# ########################################
# Files to compile to           game.qvm
# ########################################

$cc ../../../code/game/ai_main.c
$cc ../../../code/game/bg_misc.c
$cc ../../../code/game/bg_pmove.c
$cc ../../../code/game/bg_slidemove.c
$cc ../../../code/game/g_active.c
$cc ../../../code/game/g_alloc.c
$cc ../../../code/game/g_bot.c
$cc ../../../code/game/g_client.c
$cc ../../../code/game/g_cmds.c
$cc ../../../code/game/g_combat.c
$cc ../../../code/game/g_items.c
$cc ../../../code/game/g_main.c
$cc ../../../code/game/g_misc.c
$cc ../../../code/game/g_physics.c
$cc ../../../code/game/g_sandbox.c
$cc ../../../code/game/g_session.c
$cc ../../../code/game/g_spawn.c
$cc ../../../code/game/g_svcmds.c
$cc ../../../code/game/g_utils.c
$cc ../../../code/game/g_weapon.c

cp ../../../code/game/g_syscalls.asm ..
cp ../../../code/q_sharedsyscalls.asm ..

$cc ../../../code/shared/javascript.c
$cc ../../../code/shared/javascript_func.c
$cc ../../../code/shared/system.c
$cc ../../../code/shared/engine.c
$cc ../../../code/shared/core.c


echo "-----------------"
echo "game.qvm compiled"
echo "-----------------"

./asm -f ../game

# ########################################
# End of compilation files, add yours here
# ########################################

cd ../../..
cd make_scripts_arm
cd ..

mkdir -p linux/build
mkdir -p linux/game/qvm
mkdir -p linux/build/cgame

cp -p make_scripts_arm/asm linux/build/cgame/
cp -p make_scripts_arm/lcc linux/build/cgame/
cp -p make_scripts_arm/cpp linux/build/cgame/
cp -p make_scripts_arm/rcc linux/build/cgame/
cp -p make_scripts_arm/cgame.q3asm linux/build/

LIBRARY=""
INCLUDE=""

cc="./lcc -DCGAME -DQVM -S -I..\..\..\code\cgame -I..\..\..\code\ui -I..\..\..\code\qcommon $1"

cd linux/build/cgame

# ########################################
# Files to compile to            сgame.qvm
# ########################################

$cc ../../../code/cgame/cg_consolecmds.c
$cc ../../../code/cgame/cg_draw.c
$cc ../../../code/cgame/cg_drawtools.c
$cc ../../../code/cgame/cg_effects.c
$cc ../../../code/cgame/cg_ents.c
$cc ../../../code/cgame/cg_event.c
$cc ../../../code/cgame/cg_info.c
$cc ../../../code/cgame/cg_localents.c
$cc ../../../code/cgame/cg_main.c
$cc ../../../code/cgame/cg_marks.c
$cc ../../../code/cgame/cg_players.c
$cc ../../../code/cgame/cg_playerstate.c
$cc ../../../code/cgame/cg_predict.c
$cc ../../../code/cgame/cg_scoreboard.c
$cc ../../../code/cgame/cg_servercmds.c
$cc ../../../code/cgame/cg_snapshot.c
$cc ../../../code/cgame/cg_unlagged.c
$cc ../../../code/cgame/cg_view.c
$cc ../../../code/cgame/cg_weapons.c

$cc ../../../code/game/bg_misc.c
$cc ../../../code/game/bg_pmove.c
$cc ../../../code/game/bg_slidemove.c

cp ../../../code/cgame/cg_syscalls.asm ..
cp ../../../code/q_sharedsyscalls.asm ..
cp ../../../code/q_sharedsyscalls_client.asm ..

$cc ../../../code/shared/javascript.c
$cc ../../../code/shared/javascript_func.c
$cc ../../../code/shared/system.c
$cc ../../../code/shared/engine.c
$cc ../../../code/shared/core.c

echo "-----------------"
echo "cgame.qvm compiled"
echo "-----------------"

./asm -f ../cgame

# ########################################
# End of compilation files, add yours here
# ########################################

cd ../../..
cd make_scripts_arm
cd ..

mkdir -p linux/build
mkdir -p linux/game/qvm
mkdir -p linux/build/ui

cp -p make_scripts_arm/asm linux/build/ui/
cp -p make_scripts_arm/lcc linux/build/ui/
cp -p make_scripts_arm/cpp linux/build/ui/
cp -p make_scripts_arm/rcc linux/build/ui/
cp -p make_scripts_arm/ui.q3asm linux/build/

LIBRARY=""
INCLUDE=""

cc="./lcc -DUI -DQVM -S -I..\..\..\code\ui -I..\..\..\code\qcommon $1"

cd linux/build/ui

# ########################################
# Files to compile to               ui.qvm
# ########################################

$cc ../../../code/ui/ui_atoms.c
$cc ../../../code/ui/ui_connect.c
$cc ../../../code/ui/ui_gameinfo.c
$cc ../../../code/ui/ui_main.c
$cc ../../../code/ui/ui_menu.c
$cc ../../../code/ui/ui_qmenu.c

cp ../../../code/ui/ui_syscalls.asm ..
cp ../../../code/q_sharedsyscalls.asm ..
cp ../../../code/q_sharedsyscalls_client.asm ..

$cc ../../../code/game/bg_misc.c

$cc ../../../code/shared/javascript.c
$cc ../../../code/shared/javascript_func.c
$cc ../../../code/shared/system.c
$cc ../../../code/shared/engine.c
$cc ../../../code/shared/core.c

echo "-----------------"
echo "ui.qvm compiled"
echo "-----------------"

./asm -f ../ui

# ########################################
# End of compilation files, add yours here
# ########################################

cd ../../..
cd make_scripts_arm
cd ../..

# ##############################################################
# Replace "default" here with the name of your mod
# Replace "opensandbox-SDK" here with the name of your SDK folder
# Replace "opensandbox" here with the name of your game folder
# ##############################################################

cp opensandbox-SDK/linux/game/qvm/game.qvm opensandbox/game/core.default/qvm/
cp opensandbox-SDK/linux/game/qvm/cgame.qvm opensandbox/game/core.default/qvm/
cp opensandbox-SDK/linux/game/qvm/ui.qvm opensandbox/game/core.default/qvm/
