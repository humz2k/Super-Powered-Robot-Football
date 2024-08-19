#ifndef _SPRF_SCRIPTING_HPP_
#define _SPRF_SCRIPTING_HPP_

#include <lua/lua.hpp>
#include <string>
#include "engine/engine.hpp"

namespace SPRF{

static int l_tracelog(lua_State* L){
    int log_type = luaL_checkinteger(L,1);
    int nargs = lua_gettop(L);
    std::string out = "";
    for (int i = 2; i <= nargs; i++){
        out += std::string(luaL_tolstring(L,i,NULL));
        lua_pop(L,1);
    }
    TraceLog(log_type,"LUA: %s",out.c_str());
    return 0;
}

class ScriptingManager{
    private:
        lua_State* m_L;
    public:
        ScriptingManager() : m_L(luaL_newstate()){
            luaL_openlibs(m_L);
            lua_newtable(m_L);
            lua_setglobal(m_L,"sprf");
            init_logger();
        }

        ~ScriptingManager(){
            lua_close(m_L);
        }

        void init_logger(){
            lua_getglobal(m_L,"sprf");
            lua_pushinteger(m_L,LOG_INFO);
            lua_setfield(m_L,-2,"log_info");
            lua_pushinteger(m_L,LOG_CONSOLE);
            lua_setfield(m_L,-2,"log_console");
            lua_pushinteger(m_L,LOG_ERROR);
            lua_setfield(m_L,-2,"log_error");
            lua_pushinteger(m_L,LOG_DEBUG);
            lua_setfield(m_L,-2,"log_debug");
            lua_pushinteger(m_L,LOG_WARNING);
            lua_setfield(m_L,-2,"log_warning");
            lua_pop(m_L, 1);
            register_function(l_tracelog,"tracelog");
        }

        void register_function(lua_CFunction func, std::string name){
            lua_getglobal(m_L,"sprf");
            lua_pushcfunction(m_L,func);
            lua_setfield(m_L,-2,name.c_str());
            lua_pop(m_L, 1);
        }

        // returns non zero on failure
        int run_file(std::string filename){
            if (luaL_dofile(m_L,filename.c_str())){
                fprintf(stderr, "%s", lua_tostring(m_L, -1));
                lua_pop(m_L, 1);
                return 1;
            }
            return 0;
        }

        lua_State* state(){return m_L;}
};

} // namespace SPRF

#endif // _SPRF_SCRIPTING_HPP_