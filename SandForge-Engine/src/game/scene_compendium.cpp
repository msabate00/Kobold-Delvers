#include "scene_compendium.h"
#include "scene_manager.h"
#include "app/app.h"
#include "core/input.h"
#include "ui/ui.h"
#include "ui/ui_anim.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

void Scene_Compendium::CompendiumEntries()
{
    entries.clear();

    entries = {
        {
            "Kobold", Material::NpcCell,
            "Small kobolds that walk through the cave. They are the main objective: protect them,\nmake paths for them and guide enough of them to the exit.",
            0,
            {
                { "Kobolds walk over solid floors and turn around when the path is blocked.",
                    {
                        { MiniCell{1, 1, Material::NpcCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{2, 1, Material::NpcCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::NpcCell}, MiniCell{2, 1, Material::Stone}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                },
                { "Water, lava, fire, acid, smoke or gas can kill them. Build safe paths first.",
                    {
                        { MiniCell{1, 1, Material::NpcCell}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Fire}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                }
            }
        },
        {
            "Player", Material::PlayerCell,
            "You play as a kobold in special levels. You can move, jump and use the available\nmaterials to solve the room yourself.",
            4,
            {
                { "Walk left and right over solid terrain.",
                    {
                        { MiniCell{1, 1, Material::PlayerCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{2, 1, Material::PlayerCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                },
                { "Jump to climb small gaps or reach higher platforms.",
                    {
                        { MiniCell{1, 1, Material::PlayerCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 0, Material::PlayerCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{2, 0, Material::PlayerCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                },
                { "Special levels finish when the player reaches the exit.",
                    {
                        { MiniCell{0, 1, Material::PlayerCell}, MiniCell{2, 1, Material::NpcGoalCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::PlayerCell}, MiniCell{2, 1, Material::NpcGoalCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{2, 1, Material::PlayerCell}, MiniCell{2, 1, Material::NpcGoalCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                }
            }
        },
        {
            "Bonus", Material::NpcBonusCell,
            "Optional collectible. Reaching it is not mandatory, but it can give an extra star\nif the level asks for it.",
            0,
            {
                { "Send a kobold through the bonus before going to the exit to secure the extra star.",
                    {
                        { MiniCell{0, 1, Material::NpcCell}, MiniCell{1, 1, Material::NpcBonusCell}, MiniCell{2, 1, Material::NpcGoalCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::NpcCell}, MiniCell{1, 1, Material::NpcBonusCell}, MiniCell{2, 1, Material::NpcGoalCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{2, 1, Material::NpcCell}, MiniCell{2, 1, Material::NpcGoalCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                }
            }
        },
        {
            "Spawn", Material::NpcSpawnerCell,
            "Start point used in regular levels. Kobolds appear here and immediately begin to walk.",
            0,
            {
                { "The spawner creates the first kobold on the path.",
                    {
                        { MiniCell{0, 1, Material::NpcSpawnerCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{0, 1, Material::NpcSpawnerCell}, MiniCell{1, 1, Material::NpcCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{0, 1, Material::NpcSpawnerCell}, MiniCell{2, 1, Material::NpcCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                }
            }
        },
        {
            "Exit", Material::NpcGoalCell,
            "Goal zone. Regular levels need enough kobolds inside the exit to complete the level.",
            0,
            {
                { "Regular levels finish when enough kobolds reach this tile alive.",
                    {
                        { MiniCell{0, 1, Material::NpcCell}, MiniCell{2, 1, Material::NpcGoalCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::NpcCell}, MiniCell{2, 1, Material::NpcGoalCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{2, 1, Material::NpcCell}, MiniCell{2, 1, Material::NpcGoalCell}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                }
            }
        },
        {
            "Erase", Material::Empty,
            "Tool used to remove painted cells. It does not remove protected level cells like\nspawns, goals or fixed terrain.",
            0,
            {
                { "Erase clears normal painted materials and leaves an empty cell again.",
                    {
                        { MiniCell{1, 1, Material::Sand}, MiniCell{1, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Empty}, MiniCell{1, 2, Material::Stone} }
                    }
                }
            }
        },
        {
            "Sand", Material::Sand,
            "Basic falling material. It piles up, slides diagonally and can sink through liquids.\nUseful for ramps, weights and quick terrain changes.",
            1,
            {
                { "Sand falls until it finds support, then starts making a pile.",
                    {
                        { MiniCell{1, 0, Material::Sand}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Sand}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Sand}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Sand}, MiniCell{1, 0, Material::Sand},MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Sand}, MiniCell{2, 1, Material::Sand},MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Sand}, MiniCell{2, 1, Material::Sand}, MiniCell{1, 0, Material::Sand},MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                    }
                },
                { "Sand sinks through liquids by swapping places with them.",
                    {
                        { MiniCell{1, 0, Material::Sand}, MiniCell{0, 1, Material::Water},MiniCell{1, 1, Material::Water},MiniCell{2, 1, Material::Water}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Sand}, MiniCell{1, 0, Material::Water},MiniCell{0, 1, Material::Water},MiniCell{2, 1, Material::Water}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 0, Material::Sand}, MiniCell{0, 1, Material::Lava},MiniCell{1, 1, Material::Lava},MiniCell{2, 1, Material::Lava}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Sand}, MiniCell{1, 0, Material::Lava},MiniCell{0, 1, Material::Lava},MiniCell{2, 1, Material::Lava}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                }
                
            }
        },
        {
            "Water", Material::Water,
            "Heavy liquid. It falls, spreads sideways and reacts with hot materials. It can cool\nlava, steam hot coal and help vines grow.",
            2,
            {
                { "Water falls down first, then spreads sideways when blocked.",
                    {
                        { MiniCell{1, 0, Material::Water}, MiniCell{1, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Water}, MiniCell{1, 2, Material::Stone} },
                        { MiniCell{0, 2, Material::Water}, MiniCell{1, 2, Material::Stone} }
                    }
                },
                { "Water touching lava cools both cells into stone and may create steam above.",
                    {
                        { MiniCell{0, 1, Material::Water}, MiniCell{1, 1, Material::Lava}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{0, 1, Material::Stone}, MiniCell{1, 1, Material::Stone}, MiniCell{1, 0, Material::Steam}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                },
                { "Water cools hot coal back into coal and releases steam.",
                    {
                        { MiniCell{1, 0, Material::Water}, MiniCell{1, 1, Material::HotCoal}, MiniCell{1, 2, Material::Stone} },
                        { MiniCell{1, 0, Material::Steam}, MiniCell{1, 1, Material::Coal}, MiniCell{1, 2, Material::Stone} }
                    }
                }
            }
        },
        {
            "Lava", Material::Lava,
            "Hot liquid. It falls slowly, spreads and ignites flammable materials around it.\nDangerous, but useful for creating stone bridges with water.",
            2,
            {
                { "Lava plus water creates stone, blocking paths or making new platforms.",
                    {
                        { MiniCell{0, 1, Material::Lava}, MiniCell{1, 1, Material::Water}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{0, 1, Material::Stone}, MiniCell{1, 1, Material::Stone}, MiniCell{1, 0, Material::Steam}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                },
                { "Lava near wood, vine, oil, coal, gas or dynamite starts heat reactions.",
                    {
                        { MiniCell{1, 1, Material::Lava}, MiniCell{2, 1, Material::Wood}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Lava}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Lava}, MiniCell{2, 1, Material::Smoke}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                }
            }
        },
        {
            "Stone", Material::Stone,
            "Solid wall material. It does not move and works as a safe barrier, floor or support.\nIt can also appear after lava and water touch.",
            3,
            {
                { "Stone blocks liquids, gases and kobolds, so it is the safest terrain piece.",
                    {
                        { MiniCell{1, 0, Material::Water}, MiniCell{0, 1, Material::Stone}, MiniCell{2, 1, Material::Stone}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Water}, MiniCell{0, 1, Material::Stone}, MiniCell{2, 1, Material::Stone}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Water}, MiniCell{0, 1, Material::Stone}, MiniCell{2, 1, Material::Stone}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                    }
                },
                { "Acid can dissolve stone, opening a passage.",
                    {
                        { MiniCell{1, 0, Material::Acid}, MiniCell{1, 1, Material::Stone}, MiniCell{1, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Acid}, MiniCell{1, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Empty}, MiniCell{1, 2, Material::Stone} }
                    }
                }
            }
        },
        {
            "Steam", Material::Steam,
            "Light gas made by hot reactions. It rises, can condense back into water and slowly\ndisappears if it has nothing else to do.",
            5,
            {
                { "Steam rises because it is lighter than the materials around it.",
                    {
                        { MiniCell{1, 2, Material::Steam} },
                        { MiniCell{1, 1, Material::Steam} },
                        { MiniCell{1, 0, Material::Steam} }
                    }
                },
                { "Sometimes steam condenses into water before vanishing.",
                    {
                        { MiniCell{1, 1, Material::Steam} },
                        { MiniCell{1, 0, Material::Water} }
                    }
                }
            }
        },
        {
            "Wood", Material::Wood,
            "Solid flammable block. It can hold terrain or close a path, but heat will burn it\naway sooner or later.",
            6,
            {
                { "Wood works as a temporary wall or bridge.",
                    {
                        { MiniCell{1, 1, Material::Wood}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Wood}, MiniCell{1, 0, Material::Sand}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                },
                { "Wood plus fire, lava or hot coal turns into fire and then smoke.",
                    {
                        { MiniCell{1, 1, Material::Wood}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Fire}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 0, Material::Smoke}, MiniCell{2, 0, Material::Smoke}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                }
            }
        },
        {
            "Fire", Material::Fire,
            "Short-lived heat. It spreads through flammable materials, creates smoke and can turn\nwater into steam.",
            7,
            {
                { "Fire plus water creates steam and removes the flame.",
                    {
                        { MiniCell{1, 1, Material::Fire}, MiniCell{2, 1, Material::Water}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Steam}, MiniCell{2, 1, Material::Steam}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                    }
                },
                { "Fire spreads to wood, vines, oil, gas and dynamite.",
                    {
                        { MiniCell{1, 1, Material::Fire}, MiniCell{2, 1, Material::Vine}, MiniCell{2, 0, Material::Vine}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Fire}, MiniCell{2, 1, Material::Fire}, MiniCell{2, 0, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                    }
                }
            }
        },
        {
            "Smoke", Material::Smoke,
            "Dark gas created by fire. It rises and fades out. It is dangerous for kobolds if a\nlevel forces them through it.",
            7,
            {
                { "Smoke rises and slowly disappears.",
                    {
                        { MiniCell{1, 2, Material::Smoke} },
                        { MiniCell{1, 1, Material::Smoke} },
                        { MiniCell{1, 0, Material::Smoke} }
                    }
                }
            }
        },
        {
            "Snow", Material::Snow,
            "Cold falling material. It behaves like a soft solid, but heat melts it into water.",
            10,
            {
                { "Snow falls and piles up, similar to sand but colder.",
                    {
                        { MiniCell{1, 0, Material::Snow}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Snow}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                },
                { "Snow plus heat melts into water, which can then keep flowing.",
                    {
                        { MiniCell{1, 1, Material::Snow}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Water}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{0, 1, Material::Water}, MiniCell{1, 1, Material::Water}, MiniCell{2, 1, Material::Steam}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                }
            }
        },
        {
            "Vine", Material::Vine,
            "Plant material. It grows when touching water, can fill gaps and burns very quickly\nwhen heat gets close.",
            9,
            {
                { "Vines grow from water into nearby empty or water cells.",
                    {
                        { MiniCell{1, 1, Material::Vine}, MiniCell{2, 1, Material::Water}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Vine}, MiniCell{2, 1, Material::Vine}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 0, Material::Vine}, MiniCell{1, 1, Material::Vine}, MiniCell{2, 1, Material::Vine}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                },
                { "Vines can create a bridge, but they are not safe near lava or fire.",
                    {
                        { MiniCell{0, 1, Material::Stone}, MiniCell{1, 1, Material::Vine}, MiniCell{2, 1, Material::Stone}, MiniCell{0, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{0, 1, Material::Stone}, MiniCell{1, 1, Material::Vine}, MiniCell{2, 1, Material::Stone}, MiniCell{1, 0, Material::NpcCell}, MiniCell{0, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                },
                { "Vines can be used as ladders to climb up.",
                    {
                        { MiniCell{1, 1, Material::Vine},MiniCell{1, 2, Material::Vine}, MiniCell{0, 2, Material::NpcCell}, },
                        { MiniCell{1, 1, Material::Vine},MiniCell{1, 2, Material::Vine}, MiniCell{1, 2, Material::NpcCell}, },
                        { MiniCell{1, 1, Material::Vine},MiniCell{1, 2, Material::Vine}, MiniCell{1, 1, Material::NpcCell}, },
                        { MiniCell{1, 1, Material::Vine},MiniCell{1, 2, Material::Vine}, MiniCell{1, 0, Material::NpcCell}, },
                    }
                },
                { "Vine plus fire, lava or hot coal becomes fire.",
                    {
                        { MiniCell{1, 1, Material::Vine}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Fire}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 0, Material::Smoke}, MiniCell{2, 0, Material::Smoke}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                }
            }
        },
        {
            "Oil", Material::Oil,
            "Flammable liquid. It flows like water, but it catches fire when heat touches it.\nGood for chain reactions and traps.",
            10,
            {
                { "Oil flows through low areas and can carry fire reactions across the level.",
                    {
                        { MiniCell{1, 0, Material::Oil}, MiniCell{1, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Oil}, MiniCell{1, 2, Material::Stone} },
                        { MiniCell{0, 2, Material::Oil}, MiniCell{1, 2, Material::Stone} }
                    }
                },
                { "Oil plus fire, lava or hot coal becomes fire.",
                    {
                        { MiniCell{1, 1, Material::Oil}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Fire}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                }
            }
        },
        {
            "Coal", Material::Coal,
            "Solid fuel. It does not burn immediately like wood; heat turns it into hot coal,\nwhich then keeps producing fire around it.",
            10,
            {
                { "Coal plus heat becomes hot coal.",
                    {
                        { MiniCell{1, 1, Material::Coal}, MiniCell{2, 1, Material::Lava}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::HotCoal}, MiniCell{2, 1, Material::Lava}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                },
                { "Hot coal can ignite nearby coal and extend the heat source.",
                    {
                        { MiniCell{1, 1, Material::HotCoal}, MiniCell{2, 1, Material::Coal}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::HotCoal}, MiniCell{2, 1, Material::HotCoal}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::HotCoal},MiniCell{1, 0, Material::Fire}, MiniCell{2, 1, Material::HotCoal}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                }
            }
        },
        {
            "Hot Coal", Material::HotCoal,
            "Ignited coal. It spreads heat, lights nearby coal and can keep creating fire for a\nlong time unless water cools it.",
            11,
            {
                { "Water cools hot coal back into normal coal and releases steam.",
                    {
                        { MiniCell{1, 0, Material::Water}, MiniCell{1, 1, Material::HotCoal}, MiniCell{1, 2, Material::Stone} },
                        { MiniCell{1, 0, Material::Steam}, MiniCell{1, 1, Material::Coal}, MiniCell{1, 2, Material::Stone} }
                    }
                },
                { "Hot coal lights flammable neighbours without needing moving lava.",
                    {
                        { MiniCell{1, 1, Material::HotCoal}, MiniCell{2, 1, Material::Wood}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::HotCoal}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                },
                { "Hot coal plus dynamite or gas triggers an explosion.",
                    {
                        { MiniCell{1, 1, Material::HotCoal}, MiniCell{2, 1, Material::Dynamite}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{0, 1, Material::Fire}, MiniCell{1, 1, Material::Fire}, MiniCell{2, 1, Material::Fire}, MiniCell{1, 0, Material::Fire}, MiniCell{1, 2, Material::Fire} },
                        { MiniCell{0, 0, Material::Smoke}, MiniCell{1, 0, Material::Smoke}, MiniCell{2, 0, Material::Smoke}, MiniCell{0, 1, Material::Fire}, MiniCell{2, 1, Material::Fire} }
                    }
                }
            }
        },
        {
            "Gas", Material::FlammableGas,
            "Light flammable gas. It rises, spreads upwards and explodes when fire, lava or hot\ncoal gets close.",
            10,
            {
                { "Gas rises instead of falling, so it can fill ceilings and upper pockets.",
                    {
                        { MiniCell{1, 2, Material::FlammableGas} },
                        { MiniCell{1, 1, Material::FlammableGas} },
                        { MiniCell{0, 0, Material::FlammableGas}, MiniCell{1, 0, Material::FlammableGas}, MiniCell{2, 0, Material::FlammableGas} }
                    }
                },
                { "Gas plus heat explodes into a burst of fire.",
                    {
                        { MiniCell{1, 1, Material::FlammableGas}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Fire}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{0, 0, Material::Smoke}, MiniCell{1, 0, Material::Smoke}, MiniCell{2, 0, Material::Smoke}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                }
            }
        },
        {
            "Fragile", Material::FragilePlatform,
            "Breakable solid platform. It can hold kobolds, but it is flammable and weak against\nacid, so it is not reliable forever.",
            12,
            {
                { "Fragile platforms can be used as temporary floor for materials.",
                    {

                        { MiniCell{1, 0, Material::Sand}, MiniCell{0, 2, Material::FragilePlatform}, MiniCell{1, 2, Material::FragilePlatform}, MiniCell{2, 2, Material::FragilePlatform} },
                        { MiniCell{1, 1, Material::Sand}, MiniCell{0, 2, Material::FragilePlatform}, MiniCell{1, 2, Material::FragilePlatform}, MiniCell{2, 2, Material::FragilePlatform} },
                        { MiniCell{1, 1, Material::Sand}, MiniCell{0, 2, Material::FragilePlatform}, MiniCell{1, 2, Material::FragilePlatform}, MiniCell{2, 2, Material::FragilePlatform} },
                    }
                },
                { "But they cannot support the weight of a kobold in motion.",
                    {
                        { MiniCell{0, 0, Material::NpcCell}, MiniCell{0, 1, Material::Stone}, MiniCell{1, 1, Material::FragilePlatform}, MiniCell{2, 1, Material::FragilePlatform} },
                        { MiniCell{1, 0, Material::NpcCell}, MiniCell{0, 1, Material::Stone}, MiniCell{1, 1, Material::FragilePlatform}, MiniCell{2, 1, Material::FragilePlatform} },
                        { MiniCell{1, 0, Material::NpcCell}, MiniCell{0, 1, Material::Stone}, MiniCell{1, 1, Material::Empty}, MiniCell{2, 1, Material::FragilePlatform} },
                        { MiniCell{1, 1, Material::NpcCell}, MiniCell{0, 1, Material::Stone},  MiniCell{2, 1, Material::FragilePlatform} },
                        { MiniCell{1, 2, Material::NpcCell}, MiniCell{0, 1, Material::Stone},  MiniCell{2, 1, Material::FragilePlatform} },
                    }
                },
                { "Fire burns fragile blocks away.",
                    {
                        { MiniCell{1, 1, Material::FragilePlatform}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Fire}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 0, Material::Smoke}, MiniCell{2, 0, Material::Smoke}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                },
                { "Acid can also dissolve fragile platforms and open a drop.",
                    {
                        { MiniCell{1, 0, Material::Acid}, MiniCell{1, 1, Material::FragilePlatform}, MiniCell{1, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Acid}, MiniCell{1, 2, Material::Stone} },
                        { MiniCell{1, 2, Material::Stone} }
                    }
                }
            }
        },
        {
            "Acid", Material::Acid,
            "Corrosive liquid. It eats stone, wood, vines, coal, dynamite, snow and fragile blocks.\nUse it to open paths, but keep kobolds away from it.",
            12,
            {
                { "Acid falls and spreads like a dangerous liquid.",
                    {
                        { MiniCell{1, 0, Material::Acid}, MiniCell{1, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Acid}, MiniCell{1, 2, Material::Stone} },
                        { MiniCell{0, 1, Material::Acid}, MiniCell{1, 1, Material::Acid}, MiniCell{2, 1, Material::Acid}, MiniCell{1, 2, Material::Stone} }
                    }
                },
                { "Acid dissolves many solid targets and may consume itself while doing it.",
                    {
                        { MiniCell{1, 0, Material::Acid}, MiniCell{1, 1, Material::Stone}, MiniCell{1, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Acid}, MiniCell{1, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Empty}, MiniCell{1, 2, Material::Stone} }
                    }
                },
                { "Acid is also lethal for kobolds and the player.",
                    {
                        { MiniCell{1, 1, Material::NpcCell}, MiniCell{2, 1, Material::Acid}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Acid}, MiniCell{2, 1, Material::Acid}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                }
            }
        },
        {
            "Dynamite", Material::Dynamite,
            "Explosive solid. It detonates when touched by heat and fills a large area with fire.\nVery useful for opening paths, very bad near kobolds.",
            8,
            {
                { "Dynamite stays still until a heat source reaches it.",
                    {
                        { MiniCell{1, 1, Material::Dynamite}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{1, 1, Material::Dynamite}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} }
                    }
                },
                { "Heat detonates dynamite into a fire burst.",
                    {
                        { MiniCell{1, 1, Material::Dynamite}, MiniCell{2, 1, Material::Fire}, MiniCell{0, 2, Material::Stone}, MiniCell{1, 2, Material::Stone}, MiniCell{2, 2, Material::Stone} },
                        { MiniCell{0, 1, Material::Fire}, MiniCell{1, 1, Material::Fire}, MiniCell{2, 1, Material::Fire}, MiniCell{1, 0, Material::Fire}, MiniCell{1, 2, Material::Fire} },
                        { MiniCell{0, 0, Material::Smoke}, MiniCell{1, 0, Material::Smoke}, MiniCell{2, 0, Material::Smoke}, MiniCell{0, 1, Material::Fire}, MiniCell{2, 1, Material::Fire} }
                    }
                }
            }
        }
    };
}
void Scene_Compendium::OnEnter()
{
    uiIntroTimer = 0.0f;
    backHoverT = 0.0f;
    settingsHoverT = 0.0f;
    listScroll = 0.0f;
    infoScroll = 0.0f;

    if (entries.empty()) CompendiumEntries();
    selectedEntry = FindFirstUnlocked();
    entryHoverT.assign(entries.size(), 0.0f);
}

void Scene_Compendium::Update(float dt)
{
    uiIntroTimer += dt;

    if (app->input->KeyDown(GLFW_KEY_ESCAPE)) {
        mgr->Request(SCENE_MAINMENU);
        return;
    }
}
bool Scene_Compendium::EntryUnlocked(const CompendiumEntry& e) const
{
    if (e.unlockLevel <= 0) return true;
    return app->progress.StarsFor(e.unlockLevel - 1) >= 1;
}

int Scene_Compendium::FindFirstUnlocked() const
{
    for (int i = 0; i < (int)entries.size(); ++i) {
        if (EntryUnlocked(entries[i])) return i;
    }
    return 0;
}

bool Scene_Compendium::DrawTopButton(float x, float y, float w, float h, const char* text, uint32 color, float& hoverT)
{
    UI* ui = app->ui;

    float mx = (float)app->input->MouseX();
    float my = (float)app->input->MouseY();

    bool hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);
    hoverT = UIAnim::Hover(hoverT, hover, 8.0f, app->dt);

    float scale = 1.0f + hoverT * 0.08f;
    float drawW = w * scale;
    float drawH = h * scale;
    float drawX = x - (drawW - w) * 0.5f;
    float drawY = y - (drawH - h) * 0.5f - hoverT * 2.0f;

    if (ui->ImageButton(ui->interfaceTex, drawX, drawY, drawW, drawH, ui->buttonUp, ui->buttonDown, ui->buttonDown, color, 5))
        return true;

    ui->TextCentered(drawX + 2, drawY - 2, drawW, drawH, text, RGBAu32(250, 250, 250, 245), 0.75f + hoverT * 0.03f);
    return false;
}

void Scene_Compendium::DrawHolder(float x, float y, float size, uint32 tint)
{
    UI* ui = app->ui;
    ui->Image(ui->interfaceTex, x, y, size, size, ui->hudMaterialBackgroundRect, tint, 4);
}

void Scene_Compendium::DrawMatIcon(Material mat, float x, float y, float size, uint32 tint)
{
    UI* ui = app->ui;

    DrawHolder(x, y, size, RGBAu32(255, 255, 255, 245));

    const AtlasRectPx& r = matProps((uint8)mat).rect;
    if (mat == PlayerCell) {
        tint = RGBAu32(255, 150, 215, 255);
    }
    ui->Image(ui->matAtlas, x + size * 0.14f, y + size * 0.14f, size * 0.72f, size * 0.72f, r, tint, 5);
}

void Scene_Compendium::DrawGrid(float x, float y, float size, const std::vector<MiniCell>& cells, uint32 tint)
{
    UI* ui = app->ui;
    const float cell = size / 3.0f;
    const float pad = cell * 0.10f;

    for (int yy = 0; yy < 3; ++yy) {
        for (int xx = 0; xx < 3; ++xx) {
            DrawHolder(x + xx * cell, y + yy * cell, cell - 2.0f, tint);
        }
    }

    for (const MiniCell& c : cells) {
        if (c.x < 0 || c.x > 2 || c.y < 0 || c.y > 2) continue;
        if (c.mat == Material::Null) continue;

        const float ix = x + c.x * cell + pad;
        const float iy = y + c.y * cell + pad;
        const float is = cell - pad * 2.0f - 2.0f;

        if (c.mat == Material::Empty) {
            ui->TextCentered(ix, iy - is * 0.10f, is, is, "X", RGBAu32(250, 235, 220, 230), is / 38.0f);
        }
        else {
            if (c.mat == PlayerCell) {
                ui->Image(ui->matAtlas, ix, iy, is, is, matProps((uint8)c.mat).rect, RGBAu32(255, 150, 215,255), 6);
            }
            else {
                ui->Image(ui->matAtlas, ix, iy, is, is, matProps((uint8)c.mat).rect, RGBAu32(255, 255, 255, 245), 6);
            }
            
        }
    }
}

void Scene_Compendium::DrawInteraction(const MatInteraction& it, float x, float y, float w, float scale)
{
    UI* ui = app->ui;

    ui->Text(x, y, it.text, RGBAu32(236, 230, 218, 225), 0.66f * scale);

    if (it.steps.empty()) return;

    const int n = (int)it.steps.size();
    const float arrowW = 34.0f * scale;
    const float gap = 10.0f * scale;
    const float maxGrid = 92.0f * scale;
    const float minGrid = 52.0f * scale;
    const float gridsSpace = w - (float)(n - 1) * (arrowW + gap * 2.0f);
    const float grid = std::fmax(minGrid, std::fmin(maxGrid, gridsSpace / (float)n));
    const float totalW = grid * (float)n + (float)(n - 1) * (arrowW + gap * 2.0f);
    const float gx = x + std::fmax(0.0f, (w - totalW) * 0.5f);
    const float gy = y + 30.0f * scale;

    float cx = gx;
    for (int i = 0; i < n; ++i) {
        DrawGrid(cx, gy, grid, it.steps[i], RGBAu32(255, 255, 255, 240));
        cx += grid;

        if (i + 1 < n) {
            ui->TextCentered(cx + gap, gy + grid * 0.34f, arrowW, grid * 0.30f, "->", RGBAu32(240, 220, 160, 240), 0.86f * scale);
            cx += arrowW + gap * 2.0f;
        }
    }
}
void Scene_Compendium::DrawEntryButton(int index, float x, float y, float w, float h, float scale)
{
    UI* ui = app->ui;
    const CompendiumEntry& e = entries[index];
    const bool unlocked = EntryUnlocked(e);
    const bool selected = selectedEntry == index;

    if ((int)entryHoverT.size() != (int)entries.size()) entryHoverT.assign(entries.size(), 0.0f);

    float mx = (float)app->input->MouseX();
    float my = (float)app->input->MouseY();
    const bool hover = unlocked && (mx >= x && mx <= x + w && my >= y && my <= y + h);
    entryHoverT[index] = UIAnim::Hover(entryHoverT[index], hover, 8.0f, app->dt);

    const float ht = entryHoverT[index];
    const uint32 base = unlocked
        ? (selected ? RGBAu32(82, 68, 52, 245) : RGBAu32(42, 38, 34, 230))
        : RGBAu32(4, 4, 5, 245);
    const uint32 over = selected ? RGBAu32(104, 80, 55, 250) : RGBAu32(58, 50, 42, 240);
    const uint32 act = RGBAu32(74, 60, 45, 245);

    if (unlocked) {
        if (ui->Button(x, y, w, h, base, over, act)) {
            if (selectedEntry != index) infoScroll = 0.0f;
            selectedEntry = index;
        }
    }
    else {
        ui->Rect(x, y, w, h, base);
    }

    ui->RectBorders(x, y, w, h, selected ? 2.0f * scale : 1.0f * scale,
        unlocked ? (selected ? RGBAu32(220, 186, 120, 210) : RGBAu32(80, 70, 58, 160)) : RGBAu32(18, 18, 20, 255));

    const float iconS = h - 12.0f * scale;
    const float ix = x + 8.0f * scale;
    const float iy = y + (h - iconS) * 0.5f - ht * 1.0f * scale;

    if (unlocked) {
        DrawMatIcon(e.mat, ix, iy, iconS, RGBAu32(255, 255, 255, 245));
        ui->Text(x + h + 4.0f * scale, y + 11.0f * scale - ht * 1.0f * scale, e.name,
            RGBAu32(240, 235, 225, 235), (0.72f + ht * 0.03f) * scale);
    }
    else {
        DrawHolder(ix, iy, iconS, RGBAu32(0, 0, 0, 255));
        ui->Text(x + h + 4.0f * scale, y + 11.0f * scale, "???", RGBAu32(24, 24, 26, 255), 0.72f * scale);
    }
}

void Scene_Compendium::DrawInfoPanel(const CompendiumEntry& e, float x, float y, float w, float h, float scale)
{
    UI* ui = app->ui;

    ui->Rect(x, y, w, h, RGBAu32(33, 30, 27, 236));
    ui->RectBorders(x, y, w, h, 2.0f * scale, RGBAu32(92, 78, 58, 180));

    const float pad = 28.0f * scale;
    const float iconS = 92.0f * scale;
    const float titleX = x + pad + iconS + 22.0f * scale;
    const float titleY = y + pad + 8.0f * scale;

    DrawMatIcon(e.mat, x + pad, y + pad, iconS, RGBAu32(255, 255, 255, 255));

    ui->Text(titleX, titleY, e.name, RGBAu32(250, 240, 225, 245), 1.30f * scale);

    char unlockText[64];
    if (e.unlockLevel <= 0) std::snprintf(unlockText, sizeof(unlockText), "Unlocked by default");
    else std::snprintf(unlockText, sizeof(unlockText), "Unlocked after Level %d", e.unlockLevel);
    ui->Text(titleX, titleY + 34.0f * scale, unlockText, RGBAu32(210, 195, 170, 210), 0.66f * scale);

    ui->Text(x + pad, y + pad + iconS + 20.0f * scale, e.desc, RGBAu32(236, 230, 218, 230), 0.75f * scale);

    float iy = y + pad + iconS + 86.0f * scale;
    ui->Rect(x + pad, iy, w - pad * 2.0f, 2.0f * scale, RGBAu32(92, 78, 58, 120));
    iy += 22.0f * scale;

    const float blockH = 138.0f * scale;
    const float blockGap = 6.0f * scale;
    const float viewY = iy + 28.0f * scale;
    const float viewH = y + h - pad - viewY;
    const float contentH = e.interactions.empty()
        ? 0.0f
        : (float)e.interactions.size() * (blockH + blockGap) - blockGap;
    const float maxScroll = std::fmax(0.0f, contentH - viewH);
    infoScroll = std::clamp(infoScroll, 0.0f, maxScroll);

    ui->Text(x + pad, iy, maxScroll > 0.0f ? "INTERACTIONS  (use mouse wheel to scroll)" : "INTERACTIONS", RGBAu32(245, 220, 160, 240), 0.78f * scale);

    if (e.interactions.empty()) {
        ui->Text(x + pad, viewY, "No special reaction for now.", RGBAu32(210, 205, 195, 180), 0.72f * scale);
        return;
    }

    float drawY = viewY - infoScroll;
    for (int i = 0; i < (int)e.interactions.size(); ++i) {
        if (drawY >= viewY - 1.0f && drawY + blockH <= viewY + viewH + 1.0f) {
            ui->Rect(x + pad, drawY - 8.0f * scale, w - pad * 2.0f, blockH, RGBAu32(24, 22, 20, 165));
            ui->RectBorders(x + pad, drawY - 8.0f * scale, w - pad * 2.0f, blockH, 1.0f * scale, RGBAu32(70, 60, 48, 135));
            DrawInteraction(e.interactions[i], x + pad + 14.0f * scale, drawY, w - pad * 2.0f - 28.0f * scale, scale);
        }
        drawY += blockH + blockGap;
    }

    if (maxScroll > 0.0f) {
        const float trackX = x + w - pad * 0.55f;
        const float trackY = viewY;
        const float trackH = viewH;
        const float knobH = std::fmax(28.0f * scale, trackH * (viewH / contentH));
        const float knobY = trackY + (trackH - knobH) * (infoScroll / maxScroll);
        ui->Rect(trackX, trackY, 3.0f * scale, trackH, RGBAu32(20, 18, 16, 170));
        ui->Rect(trackX - 1.0f * scale, knobY, 5.0f * scale, knobH, RGBAu32(150, 120, 82, 190));
    }
}
void Scene_Compendium::DrawUI(int&, Material&)
{
    if (entries.empty()) CompendiumEntries();

    UI* ui = app->ui;

    float vw = (float)app->framebufferSize.x;
    float vh = (float)app->framebufferSize.y;

    ui->Cursor();

    float s = std::fmin(vw / 1280.0f, vh / 720.0f);
    if (s < 0.82f) s = 0.82f;
    if (s > 1.18f) s = 1.18f;
    auto S = [&](float v) { return v * s; };

    ui->Rect(0, 0, vw, vh, RGBAu32(45, 42, 38, 255));

    const float topH = S(76.0f);
    ui->Rect(0, 0, vw, topH, RGBAu32(34, 31, 28, 255));
    ui->Rect(0, topH, vw, S(2.0f), RGBAu32(64, 52, 38, 180));
    ui->TextCentered(0, S(12.0f), vw, S(44.0f), "COMPENDIUM", RGBAu32(245, 240, 232, 245), 1.15f * s);

    if (DrawTopButton(S(16.0f), S(22.0f), S(100.0f), S(42.0f), "BACK", RGBAu32(220, 170, 170, 220), backHoverT))
        mgr->Request(SCENE_MAINMENU);

    if (DrawTopButton(vw - S(16.0f) - S(100.0f), S(22.0f), S(100.0f), S(42.0f), "SETTINGS", RGBAu32(102, 161, 255, 220), settingsHoverT))
        mgr->OpenSettings(SCENE_COMPENDIUM);

    const float margin = S(38.0f);
    const float gap = S(24.0f);
    const float bodyY = topH + S(26.0f);
    const float bodyH = vh - bodyY - S(34.0f);
    const float leftW = std::fmin(S(330.0f), vw * 0.34f);
    const float rightW = vw - margin * 2.0f - gap - leftW;
    const float leftX = margin;
    const float rightX = leftX + leftW + gap;

    ui->Rect(leftX, bodyY, leftW, bodyH, RGBAu32(30, 28, 25, 232));
    ui->RectBorders(leftX, bodyY, leftW, bodyH, S(2.0f), RGBAu32(84, 70, 52, 170));
    ui->Text(leftX + S(18.0f), bodyY + S(14.0f), "DISCOVERED", RGBAu32(245, 220, 160, 235), 0.72f * s);

    int unlockedCount = 0;
    for (const CompendiumEntry& e : entries) if (EntryUnlocked(e)) ++unlockedCount;

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%d/%d", unlockedCount, (int)entries.size());
    ui->Text(leftX + leftW - S(62.0f), bodyY + S(14.0f), buf, RGBAu32(220, 210, 195, 200), 0.72f * s);

    const float listX = leftX + S(12.0f);
    const float listY = bodyY + S(48.0f);
    const float listW = leftW - S(24.0f);
    const float itemH = S(48.0f);
    const float itemGap = S(8.0f);
    const float listH = bodyH - S(64.0f);
    const float contentH = (float)entries.size() * (itemH + itemGap) - itemGap;
    const float maxScroll = std::fmax(0.0f, contentH - listH);

    const float mx = (float)app->input->MouseX();
    const float my = (float)app->input->MouseY();
    const int scrollSteps = app->input->ScrollSteps();
    if (scrollSteps != 0) {
        const bool overLeft = mx >= leftX && mx <= leftX + leftW && my >= bodyY && my <= bodyY + bodyH;
        const bool overRight = mx >= rightX && mx <= rightX + rightW && my >= bodyY && my <= bodyY + bodyH;

        if (overRight) infoScroll -= (float)scrollSteps * S(162.0f);
        else if (overLeft) listScroll -= (float)scrollSteps * S(42.0f);
        else listScroll -= (float)scrollSteps * S(42.0f);
    }

    listScroll = std::clamp(listScroll, 0.0f, maxScroll);

    if (selectedEntry < 0 || selectedEntry >= (int)entries.size() || !EntryUnlocked(entries[selectedEntry])) {
        selectedEntry = FindFirstUnlocked();
        infoScroll = 0.0f;
    }

    for (int i = 0; i < (int)entries.size(); ++i) {
        const float iy = listY + (float)i * (itemH + itemGap) - listScroll;
        if (iy + itemH < listY || iy > listY + listH) continue;
        DrawEntryButton(i, listX, iy, listW, itemH, s);
    }

    if (maxScroll > 0.0f) {
        const float trackX = leftX + leftW - S(8.0f);
        const float trackY = listY;
        const float trackH = listH;
        const float knobH = std::fmax(S(28.0f), trackH * (listH / contentH));
        const float knobY = trackY + (trackH - knobH) * (listScroll / maxScroll);
        ui->Rect(trackX, trackY, S(3.0f), trackH, RGBAu32(20, 18, 16, 170));
        ui->Rect(trackX - S(1.0f), knobY, S(5.0f), knobH, RGBAu32(150, 120, 82, 190));
    }

    DrawInfoPanel(entries[selectedEntry], rightX, bodyY, rightW, bodyH, s);

    ui->Text(leftX, vh - S(24.0f), "ESC: Back  |  Mouse wheel: Scroll", RGBAu32(226, 220, 210, 170), 0.68f * s);
}
