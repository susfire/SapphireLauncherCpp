#include "Utf8Ini.h"
#include "args.hpp"
#include "httplib.h"
#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

static std::string readFile( const char* path )
{
  std::ifstream file( path );
  if( !file.is_open() )
  {
    throw std::runtime_error( "Error opening file " + std::string( path ) );
  }
  std::string content( ( std::istreambuf_iterator< char >( file ) ), std::istreambuf_iterator< char >() );
  file.close();
  return content;
}

struct Arguments : public ArgumentParser {
  bool create = false;
  bool nostart = false;
  bool debug = false;

  Arguments( int argc, char** argv )
      : ArgumentParser( "Sapphire Launcher" )
  {
    addBool( "-create", create, "Create a new user." );
    addBool( "-nostart", nostart, "Don't start the game (Windows only)" );
    addBool( "-debug", debug, "Start under debugger" );
    parseOrExit( argc, argv );
  }
};

int main( int argc, char** argv )
try
{
  Arguments args( argc, argv );
  if( !fs::exists( "sapphire.ini" ) )
  {
    std::ofstream file( "sapphire.ini" );
    file << R"([sapphire]
api = http://127.0.0.1:80
username = user
password = pass
executable = ffxiv_dx11.exe
debugger =
)";
    std::cout << "Example sapphire.ini created" << std::endl;
    return EXIT_FAILURE;
  }
  Utf8Ini ini;
  int errorLine = 0;
  if( !ini.Deserialize( readFile( "sapphire.ini" ), errorLine ) )
  {
    std::cout << "Error deserializing sapphire.ini at line " << errorLine << std::endl;
    return EXIT_FAILURE;
  }
  auto api = ini.GetValue( "sapphire", "api" );
  while( !api.empty() && api.back() == '/' )
  {
    api.pop_back();
  }
  if( api.empty() )
  {
    throw std::runtime_error( "Bad sapphire.ini: api is empty" );
  }
  auto username = ini.GetValue( "sapphire", "username" );
  if( username.empty() )
  {
    throw std::runtime_error( "Bad sapphire.ini:username is empty" );
  }
  auto password = ini.GetValue( "sapphire", "password" );
  if( password.empty() )
  {
    throw std::runtime_error( "Bad sapphire.ini: password is empty" );
  }
  auto executable = ini.GetValue( "sapphire", "executable" );
  if( executable.empty() )
  {
    throw std::runtime_error( "Bad sapphire.ini: executable is empty" );
  }

  nlohmann::json request = {
          { "username", username },
          { "pass", password },
  };
  auto path = args.create ? "/sapphire-api/lobby/createUser" : "/sapphire-api/lobby/login";

  httplib::Client client( api );
  std::cout << "POST " << api << path << std::endl;
  httplib::Result res = client.Post( path, request.dump(), "application/json" );
  if( !res )
  {
    throw std::runtime_error( "Error posting to " + api + path + ": " + httplib::to_string( res.error() ) );
  }
  if( res->status != 200 )
  {
    if( args.create )
    {
      throw std::runtime_error( "Error creating user (already exists?)" );
    }
    else
    {
      throw std::runtime_error( "Error logging in (bad username or password?). Run sapphire create to create the user." );
    }
  }
  const auto response = nlohmann::json::parse( res->body );
  auto frontierHost = response.at( "frontierHost" ).get< std::string >();
  auto lobbyHost = response.at( "lobbyHost" ).get< std::string >();
  auto lobbyPort = response.at( "lobbyPort" ).get< int >();
  auto sId = response.at( "sId" ).get< std::string >();


  std::string commandLine;
  auto debugger = ini.GetValue( "sapphire", "debugger" );
  if( !debugger.empty() )
  {
    // Sleep a little bit to allow the user to hold down the control key
    Sleep( 500 );
  }
  if( args.debug || !ini.GetValue( "sapphire", "debug" ).empty() || GetAsyncKeyState( VK_CONTROL ) & 0x8000 )
  {
    commandLine = debugger + " ";
  }
  commandLine += executable + " ";
  commandLine += "DEV.TestSID=" + sId + " DEV.UseSqPack=1 DEV.DataPathType=1 ";
  for( int i = 0; i < 8; i++ )
  {
    auto index = std::to_string( i + 1 );
    commandLine += "DEV.LobbyHost0" + index + "=" + lobbyHost + " ";
    commandLine += "DEV.LobbyPort0" + index + "=" + std::to_string( lobbyPort ) + " ";
  }
  commandLine += "SYS.Region=3 language=1 version=1.0.0.0 ";
  commandLine += "DEV.MaxEntitledExpansionID=1 ";
  commandLine += "DEV.GMServerHost=" + frontierHost;
  std::cout << commandLine << std::endl;
  if( args.nostart || !ini.GetValue( "sapphire", "nostart" ).empty() )
  {
    return EXIT_SUCCESS;
  }

#ifdef _WIN32
  if( !fs::exists( executable ) )
  {
    throw std::runtime_error( "Bad sapphire.ini: executable '" + executable + "' does not exist" );
  }
  return system( commandLine.c_str() );
#else
  return EXIT_SUCCESS;
#endif// _WIN32
} catch( std::exception& e )
{
  std::cout << e.what() << std::endl;
  return EXIT_FAILURE;
}
