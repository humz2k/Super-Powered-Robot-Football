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
        init_vec();
    }

    ~ScriptingManager() { lua_close(m_L); }

    void init_logger();

    void init_vec();

    void register_function(std::function<int(lua_State*)> func,
                           std::string name);

    // returns non zero on failure
    int run_file(std::string filename);

    int run_string(std::string lua_script);

    lua_State* state() { return m_L; }
};

void l_construct_vec3(lua_State* L, lua_Number x, lua_Number y, lua_Number z);

extern ScriptingManager scripting;

} // namespace SPRF

#endif // _SPRF_SCRIPTING_HPP_