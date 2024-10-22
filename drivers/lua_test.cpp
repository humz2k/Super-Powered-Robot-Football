#include "engine/engine.hpp"
#include "testing.hpp"
#include <lua/lua.hpp>
#include <stdio.h>
#include <string.h>

namespace SPRF {

class LuaCommand : public DevConsoleCommand {
  public:
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) {
        if (args.size() == 1) {
            scripting.run_file(args[0]);
        }
    }
};

class CallLuaCommand : public DevConsoleCommand {
  public:
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) {
        if (args.size() == 0)
            return;
        std::string input = "";
        for (auto& i : args) {
            input += i;
        }
        scripting.run_string(input);
    }
};

class MyScene : public TestScene {
  public:
    MyScene(Game* game) : TestScene(game, false) {
        dev_console()->add_command<LuaCommand>("lua");
        dev_console()->add_command<CallLuaCommand>("exec_lua");
        simple_map()->load_editor(this);
    }
};

} // namespace SPRF

int main(void) {
    SPRF::game =
        new SPRF::Game(600, 600, "lua_test", 600, 600, 200, false, 1.0);

    rlImGuiSetup(true);

    SPRF::game->load_scene<SPRF::MyScene>();

    while (SPRF::game->running()) {
        SPRF::game->draw();
    }

    delete SPRF::game;
    // ImGui::PopFont();
    rlImGuiShutdown();
    // lua_close(SPRF::L);
    enet_deinitialize();

    return 0;
}