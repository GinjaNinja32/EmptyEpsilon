#include "gameGlobalInfo.h"

P<GameGlobalInfo> gameGlobalInfo;

REGISTER_MULTIPLAYER_CLASS(GameGlobalInfo, "GameGlobalInfo")
GameGlobalInfo::GameGlobalInfo()
: MultiplayerObject("GameGlobalInfo")
{
    assert(!gameGlobalInfo);

    callsign_counter = 0;
    victory_faction = -1;
    gameGlobalInfo = this;
    for(int n=0; n<max_player_ships; n++)
    {
        playerShipId[n] = -1;
        registerMemberReplication(&playerShipId[n]);
    }
    for(int n=0; n<max_nebulas; n++)
    {
        nebula_info[n].vector = sf::Vector3f(random(-1, 1), random(-1, 1), random(-1, 1));
        nebula_info[n].textureName = "Nebula" + string(irandom(1, 3));
        registerMemberReplication(&nebula_info[n].vector);
        registerMemberReplication(&nebula_info[n].textureName);
    }

    global_message_timeout = 0.0;
    player_warp_jump_drive_setting = PWJ_ShipDefault;
    scanning_complexity = SC_Normal;
    long_range_radar_range = 30000;
    use_beam_shield_frequencies = true;
    use_system_damage = true;
    allow_main_screen_tactical_radar = true;
    allow_main_screen_long_range_radar = true;

    registerMemberReplication(&scanning_complexity);
    registerMemberReplication(&global_message);
    registerMemberReplication(&global_message_timeout, 1.0);
    registerMemberReplication(&victory_faction);
    registerMemberReplication(&long_range_radar_range);
    registerMemberReplication(&use_beam_shield_frequencies);
    registerMemberReplication(&use_system_damage);
    registerMemberReplication(&allow_main_screen_tactical_radar);
    registerMemberReplication(&allow_main_screen_long_range_radar);

    for(unsigned int n=0; n<factionInfo.size(); n++)
        reputation_points.push_back(0);
    registerMemberReplication(&reputation_points, 1.0);
}

P<PlayerSpaceship> GameGlobalInfo::getPlayerShip(int index)
{
    assert(index >= 0 && index < max_player_ships);
    if (game_server)
        return game_server->getObjectById(playerShipId[index]);
    return game_client->getObjectById(playerShipId[index]);
}

void GameGlobalInfo::setPlayerShip(int index, P<PlayerSpaceship> ship)
{
    assert(index >= 0 && index < max_player_ships);
    assert(game_server);

    if (ship)
        playerShipId[index] = ship->getMultiplayerId();
    else
        playerShipId[index] = -1;
}

int GameGlobalInfo::findPlayerShip(P<PlayerSpaceship> ship)
{
    for(int n=0; n<max_player_ships; n++)
        if (getPlayerShip(n) == ship)
            return n;
    return -1;
}

int GameGlobalInfo::insertPlayerShip(P<PlayerSpaceship> ship)
{
    for(int n=0; n<max_player_ships; n++)
    {
        if (!getPlayerShip(n))
        {
            setPlayerShip(n, ship);
            return n;
        }
    }
    return -1;
}

void GameGlobalInfo::update(float delta)
{
    if (global_message_timeout > 0.0)
    {
        global_message_timeout -= delta;
    }
}

string GameGlobalInfo::getNextShipCallsign()
{
    callsign_counter += 1;
    switch(irandom(0, 9))
    {
    case 0: return "S" + string(callsign_counter);
    case 1: return "NC" + string(callsign_counter);
    case 2: return "CV" + string(callsign_counter);
    case 3: return "SS" + string(callsign_counter);
    case 4: return "VS" + string(callsign_counter);
    case 5: return "BR" + string(callsign_counter);
    case 6: return "CSS" + string(callsign_counter);
    case 7: return "UTI" + string(callsign_counter);
    case 8: return "VK" + string(callsign_counter);
    case 9: return "CCN" + string(callsign_counter);
    }
    return "SS" + string(callsign_counter);
}

void GameGlobalInfo::addScript(P<Script> script)
{
    script_list.update();
    script_list.push_back(script);
}

void GameGlobalInfo::reset()
{
    gm_callback_functions.clear();
    
    foreach(GameEntity, e, entityList)
        e->destroy();
    foreach(SpaceObject, o, space_object_list)
        o->destroy();
    if (engine->getObject("scenario"))
        engine->getObject("scenario")->destroy();

    foreach(Script, s, script_list)
    {
        s->destroy();
    }
    for(unsigned int n=0; n<reputation_points.size(); n++)
        reputation_points[n] = 0;
    callsign_counter = 0;
    victory_faction = -1;
}

void GameGlobalInfo::startScenario(string filename)
{
    reset();
    
    P<ScriptObject> script = new ScriptObject();
    script->run(filename);
    engine->registerObject("scenario", script);
}

void GameGlobalInfo::destroy()
{
    reset();
    MultiplayerObject::destroy();
}

string playerWarpJumpDriveToString(EPlayerWarpJumpDrive player_warp_jump_drive)
{
    switch(player_warp_jump_drive)
    {
    case PWJ_ShipDefault:
        return "Ship default";
    case PWJ_WarpDrive:
        return "Warp-drive";
    case PWJ_JumpDrive:
        return "Jump-drive";
    case PWJ_WarpAndJumpDrive:
        return "Both";
    default:
        return "?";
    }
}

string getSectorName(sf::Vector2f position)
{
    constexpr float sector_size = 20000;
    int sector_x = floorf(position.x / sector_size) + 5;
    int sector_y = floorf(position.y / sector_size) + 5;
    string y;
    string x;
    if (sector_y >= 0)
        y = string(char('A' + (sector_y)));
    else
        y = string(char('z' + 1 + sector_y)) + string(char('z' + 1 + sector_y));
    if (sector_x >= 0)
        x = string(sector_x);
    else
        x = string(100 + sector_x);
    return y + x;
}

static int victory(lua_State* L)
{
    gameGlobalInfo->setVictory(luaL_checkstring(L, 1));
    if (engine->getObject("scenario"))
        engine->getObject("scenario")->destroy();
    engine->setGameSpeed(0.0);
    return 0;
}
/// victory(string)
/// Called with a faction name as parameter, sets a certain faction as victor and ends the game.
REGISTER_SCRIPT_FUNCTION(victory);

static int globalMessage(lua_State* L)
{
    gameGlobalInfo->global_message = luaL_checkstring(L, 1);
    gameGlobalInfo->global_message_timeout = 5.0;
    return 0;
}
/// globalMessage(string)
/// Show a global message on the main screens of all active player ships.
REGISTER_SCRIPT_FUNCTION(globalMessage);

static int getPlayerShip(lua_State* L)
{
    int index = luaL_checkinteger(L, 1);
    if (index == -1)
    {
        for(index = 0; index<GameGlobalInfo::max_player_ships; index++)
        {
            P<PlayerSpaceship> ship = gameGlobalInfo->getPlayerShip(index);
            if (ship)
                return convert<P<PlayerSpaceship> >::returnType(L, ship);
        }
        return 0;
    }
    if (index < 1 || index > GameGlobalInfo::max_player_ships)
        return 0;
    P<PlayerSpaceship> ship = gameGlobalInfo->getPlayerShip(index - 1);
    if (!ship)
        return 0;
    return convert<P<PlayerSpaceship> >::returnType(L, ship);
}
/// getPlayerShip(index)
/// Return the player's ship, use -1 to get the first active player ship.
REGISTER_SCRIPT_FUNCTION(getPlayerShip);

static int getObjectsInRadius(lua_State* L)
{
    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float r = luaL_checknumber(L, 3);
    
    sf::Vector2f position(x, y);
    
    PVector<SpaceObject> objects;
    PVector<Collisionable> objectList = CollisionManager::queryArea(position - sf::Vector2f(r, r), position + sf::Vector2f(r, r));
    foreach(Collisionable, obj, objectList)
    {
        P<SpaceObject> sobj = obj;
        if (sobj && (sobj->getPosition() - position) < r)
            objects.push_back(sobj);
    }
    
    return convert<PVector<SpaceObject> >::returnType(L, objects);
}
/// getObjectsInRadius(x, y, radius)
/// Return a list of all space objects at the x,y location within a certain radius.
REGISTER_SCRIPT_FUNCTION(getObjectsInRadius);
