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
        out += std::string(luaL_tolstring(L, i, NULL)) + " ";
        lua_pop(L, 1);
    }
    TraceLog(log_type, "LUA: %s", out.c_str());
    return 0;
}

static int l_wrapper(lua_State* L) {
    int index = lua_tointeger(L, lua_upvalueindex(1));
    return funcs[index](L);
}

static int newvec3(lua_State* L) {
    int nargs = lua_gettop(L);
    TraceLog(LOG_INFO, "nargs = %d", nargs);
    if (!((nargs == 0) || (nargs == 3))) {
        lua_error(L);
        return 0;
    }
    lua_newtable(L);
    lua_Number x, y, z;
    x = y = z = 0;
    if (nargs == 3) {
        x = luaL_checknumber(L, 1);
        y = luaL_checknumber(L, 2);
        z = luaL_checknumber(L, 3);
    }

    lua_pushnumber(L, x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, y);
    lua_setfield(L, -2, "y");
    lua_pushnumber(L, z);
    lua_setfield(L, -2, "z");
    return 1;
}

void l_construct_vec3(lua_State* L, lua_Number x, lua_Number y, lua_Number z) {
    lua_newtable(L);
    lua_pushnumber(L, x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, y);
    lua_setfield(L, -2, "y");
    lua_pushnumber(L, z);
    lua_setfield(L, -2, "z");
}

void ScriptingManager::init_vec() { register_function(newvec3, "vec3"); }

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
        TraceLog(LOG_ERROR, "LUA: %s", lua_tostring(m_L, -1));
        lua_pop(m_L, 1);
        return 1;
    }
    return 0;
}

int ScriptingManager::run_string(std::string script) {
    std::lock_guard<std::mutex> guard(script_mutex);
    if (luaL_dostring(m_L, script.c_str())) {
        TraceLog(LOG_ERROR, "LUA: %s", lua_tostring(m_L, -1));
        lua_pop(m_L, 1);
        return 1;
    }
    return 0;
}

void ScriptingManager::refresh(){
    funcs.clear();
    funcs.resize(0);
    lua_close(m_L);
    m_L = luaL_newstate();
    luaL_openlibs(m_L);
    lua_newtable(m_L);
    lua_setglobal(m_L, "sprf");
    init_logger();
    init_vec();
}

ScriptingManager scripting;

}; // namespace SPRF