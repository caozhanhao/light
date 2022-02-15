extern const int LIGHT_AUDIO_READ_BUFFER_SIZE = 65536;
#include "light.h"
#include <string>
using namespace std;
using namespace light;
int main(int argc, char* argv[])
{
  Player player;
  Option option(argc, argv);
  option.add("s", "server",
    [&player](Option::CallbackArgType args)
    {
      if (args.size() == 0)
        std::cout << "--server need one argument.\n";
      player.set_audio_server(args[0]);
    }, 10);
  option.add("c", "cache",
    [&player](Option::CallbackArgType args)
    {
      if (args.size() == 0)
        player.enable_cache();
      else
        player.enable_cache(args[0]);
    }, 9);
  option.add("i", "local-input",
    [&player](Option::CallbackArgType args)
    {
      if (args.size() == 0)
        std::cout << "--local-input need at least one argument.\n";
      for(auto& r : args)
        player.push_local(r);
    });
  option.add("n", "online-input",
    [&player](Option::CallbackArgType args)
    {
      if (args.size() == 0)
        std::cout << "--online-input need at least one argument.\n";
      for (auto& r : args)
        player.push_online(r);
    });
  option.add("o", "ordered-play",
    [&player](Option::CallbackArgType args)
    {
      if (args.size() == 0)
        player.ordered_play();
      else if (args.size() == 1)
        player.ordered_play(std::stoi(args[0]));
    });
  option.add("l", "loop-play",
    [&player](Option::CallbackArgType args)
    {
      if (args.size() == 0)
        player.repeated_play();
      else if (args.size() == 1)
        player.repeated_play(std::stoi(args[0]));
    });
  option.add("r", "random-play",
    [&player](Option::CallbackArgType args)
    {
      if (args.size() == 0)
        player.random_play();
      else if (args.size() == 1)
        player.random_play(std::stoi(args[0]));
    });
  option.add("no-bar",
    [&player](Option::CallbackArgType args)
    {
      player.no_bar();
    }, 8);
  option.add("h","help",
    [](Option::CallbackArgType args)
    {
      std::cout <<
        R"(Light
Usage: light[options...] <arguments>
-s, --server        <PulseAudio server> Set PulseAudio server. 
                (default: PULSE_SERVER)                    
-i, --local-input   <music paths>       Push local musics into list.
-n, --online-input  <urls>              Push online musics into list.
-c, --cache         <cache path(opt)>   Cache the music before 
                    (default:cache/)    playing online music.
-o, --ordered-play  <num(opt)>          Play songs from list in order.
-l, --loop-play     <times(opt)>        Play a song from list repeatedly.
-r, --random-play   <num(opt)>          Play random songs from list.
    --no-bar                            With no bar.
-h, --help                              Get this help.)" << std::endl;
    });
  option.parse();
  option.run();
  return 0;
}