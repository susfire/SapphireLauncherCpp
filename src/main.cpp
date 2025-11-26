#include "Utf8Ini.h"
#include "args.hpp"
#include "httplib.h"
#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif// _WIN32

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
languageIndex = 1
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
  auto languageIndex = ini.GetValue( "sapphire", "languageIndex" );
  if( languageIndex.empty() )
  {
    throw std::runtime_error( "Bad sapphire.ini: languageIndex is empty" );
  }

  nlohmann::json request = {
          { "username", username },
          { "pass", password },
  };
  auto path = args.create ? "/sapphire-api/lobby/createAccount" : "/sapphire-api/lobby/login";

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


  std::vector< std::string > commandArgs;
  auto debugger = ini.GetValue( "sapphire", "debugger" );
  if( !debugger.empty() )
  {
    bool ctrlDown = false;
#ifdef _WIN32
    // Sleep a little bit to allow the user to hold down the control key
    Sleep( 500 );
    ctrlDown = ( GetAsyncKeyState( VK_CONTROL ) & 0x8000 ) != 0;
#endif// _WIN32

    if( args.debug || !ini.GetValue( "sapphire", "debug" ).empty() || ctrlDown )
    {
      commandArgs.emplace_back( debugger );
    }
  }
  commandArgs.emplace_back( executable );
  commandArgs.emplace_back( "DEV.TestSID=" + sId );
  commandArgs.emplace_back( "DEV.UseSqPack=1" );
  commandArgs.emplace_back( "DEV.DataPathType=1" );
  for( int i = 0; i < 8; i++ )
  {
    auto index = std::to_string( i + 1 );
    commandArgs.emplace_back( "DEV.LobbyHost0" + index + "=" + lobbyHost );
    commandArgs.emplace_back( "DEV.LobbyPort0" + index + "=" + std::to_string( lobbyPort ) );
  }
  commandArgs.emplace_back( "SYS.Region=3" );
  commandArgs.emplace_back( "language=" + languageIndex );
  commandArgs.emplace_back( "version=1.0.0.0" );
  commandArgs.emplace_back( "DEV.MaxEntitledExpansionID=1" );
  commandArgs.emplace_back( "DEV.GMServerHost=" + frontierHost );

  std::string command;
  for( size_t i = 0; i < commandArgs.size(); i++ )
  {
    if( i > 0 )
    {
      command += " ";
    }
    if( commandArgs[ i ].find( ' ' ) != std::string::npos )
    {
      command += "\"" + commandArgs[ i ] + "\"";
    }
    else
    {
      command += commandArgs[ i ];
    }
  }
  std::cout << command << std::endl;
  if( args.nostart || !ini.GetValue( "sapphire", "nostart" ).empty() )
  {
    return EXIT_SUCCESS;
  }

#ifdef _WIN32
  if( !fs::exists( executable ) )
  {
    throw std::runtime_error( "Bad sapphire.ini: executable '" + executable + "' does not exist" );
  }
  return system( command.c_str() );
#else
  return EXIT_SUCCESS;
#endif// _WIN32
} catch( std::exception& e )
{
  std::cout << e.what() << std::endl;
  return EXIT_FAILURE;
}
