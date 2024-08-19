#include "scripting.hpp"
#include "engine/engine.hpp"
#include <mutex>

namespace SPRF {
static std::vector<std::function<int(lua_State*)>> funcs;

static std::mutex script_mutex;

static int l_tracelog(lua_State* L) {
    int log_type = luaL_checkinteger(L, 1);
    int nargs = lua_gettop(L);
    std::string out = "";
    for (int i = 2; i <= nargs; i++) {
        out += std::string(luaL_tolstring(L, i, NULL));
        lua_pop(L, 1);
    }
    TraceLog(log_type, "LUA: %s", out.c_str());
    return 0;
}

static int l_wrapper(lua_State* L) {
    int index = lua_tointeger(L, lua_upvalueindex(1));
    return funcs[index](L);
}

void ScriptingManager::init_logger() {
    lua_getglobal(m_L, "sprf");
    lua_pushinteger(m_L, LOG_INFO);
    lua_setfield(m_L, -2, "log_info");
    lua_pushinteger(m_L, LOG_CONSOLE);
    lua_setfield(m_L, -2, "log_console");
    lua_pushinteger(m_L, LOG_ERROR);
    lua_setfield(m_L, -2, "log_error");
    lua_pushinteger(m_L, LOG_DEBUG);
    lua_setfield(m_L, -2, "log_debug");
    lua_pushinteger(m_L, LOG_WARNING);
    lua_setfield(m_L, -2, "log_warning");
    lua_pop(m_L, 1);
    register_function(l_tracelog, "tracelog");
}

void ScriptingManager::register_function(std::function<int(lua_State*)> func,
                                         std::string name) {
    std::lock_guard<std::mutex> guard(script_mutex);
    lua_getglobal(m_L, "sprf");
    int idx = funcs.size();
    funcs.push_back(func);
    lua_pushinteger(m_L, idx);
    lua_pushcclosure(m_L, l_wrapper, 1);
    lua_setfield(m_L, -2, name.c_str());
    lua_pop(m_L, 1);
}

int ScriptingManager::run_file(std::string filename) {
    std::lock_guard<std::mutex> guard(script_mutex);
    if (luaL_dofile(m_L, filename.c_str())) {
        fprintf(stderr, "%s", lua_tostring(m_L, -1));
        lua_pop(m_L, 1);
        return 1;
    }
    return 0;
}

}; // namespace SPRF