#ifndef _SPRF_SCRIPTING_HPP_
#define _SPRF_SCRIPTING_HPP_

#include <functional>
#include <lua/lua.hpp>
#include <string>
#include <vector>

namespace SPRF {

class ScriptingManager {
  private:
    lua_State* m_L;

  public:
    ScriptingManager() : m_L(luaL_newstate()) {
        luaL_openlibs(m_L);
        lua_newtable(m_L);
        lua_setglobal(m_L, "sprf");
        init_logger();
    }

    ~ScriptingManager() { lua_close(m_L); }

    void init_logger();

    void register_function(std::function<int(lua_State*)> func,
                           std::string name);

    // returns non zero on failure
    int run_file(std::string filename) {
        if (luaL_dofile(m_L, filename.c_str())) {
            fprintf(stderr, "%s", lua_tostring(m_L, -1));
            lua_pop(m_L, 1);
            return 1;
        }
        return 0;
    }

    lua_State* state() { return m_L; }
};

} // namespace SPRF

#endif // _SPRF_SCRIPTING_HPP_