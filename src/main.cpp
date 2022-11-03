//   Copyright 2022 light - caozhanhao
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
extern const int LIGHT_AUDIO_READ_BUFFER_SIZE = 65536;

#include <atomic>

std::atomic<bool> light_is_running = true;
bool light_output = true;

#include "light.hpp"
#include <string>
#include <signal.h>

using namespace std;
using namespace light;

bool is_http(const std::string &str)
{
  std::string a = str.substr(0, 7);
  for (auto &r: a)
  {
    r = std::tolower(r);
  }
  return a == "https://" || a == "http://";
}

static void signal_handle(int sig)
{
  light_is_running = false;
  LIGHT_NOTICE("Quitting.");
  std::exit(-1);
}

int main(int argc, char *argv[])
{
  signal(SIGINT, signal_handle);
  Player player;
  Option option(argc, argv);
  option.add(argv[0],
             [&player](Option::CallbackArgType args)
             {
               std::thread([&player]
                           {
                             while (true)
                             {
                               if (light::term::kbhit())
                               {
                                 switch (light::term::getch())
                                 {
                                   case 'q':
                                     light_is_running = false;
                                     player.go();
                                     LIGHT_NOTICE("Quitting.");
                                     return;
                                   case ' ':
                                     if (player.is_paused())
                                     {
                                       player.go();
                                       LIGHT_NOTICE("Continue.");
                                     }
                                     else
                                     {
                                       player.pause();
                                       LIGHT_NOTICE("Paused.");
                                     }
                                     break;
                                 }
                               }
                             }
                           }).detach();
               for (auto &r: args)
               {
                 if (is_http(r))
                   player.push_online(r);
                 else
                   player.push_local(r);
                 player.output();
               }
             });
  option.add("s", "server",
             [&player](Option::CallbackArgType args)
             {
               if (args.size() == 0)
               {
                 std::cout << "--server need one argument.\n";
               }
               player.set_audio_server(args[0]);
             }, 10);
  option.add("c", "cache",
             [&player](Option::CallbackArgType args)
             {
               if (args.size() == 0)
               {
                 player.enable_cache();
               }
               else
               {
                 player.enable_cache(args[0]);
               }
             }, 9);
  option.add("i", "input",
             [&player](Option::CallbackArgType args)
             {
               if (args.size() == 0)
               {
                 std::cout << "--local-input need at least one argument.\n";
               }
               for (auto &r: args)
               {
                 if (is_http(r))
                 {
                   player.push_online(r);
                 }
                 else
                 {
                   player.push_local(r);
                 }
               }
             });
  option.add("f", "file-output",
             [&player](Option::CallbackArgType args)
             {
               if (args.size() != 1)
               {
                 std::cout << "--file-output need exactly one argument.\n";
               }
               player.output_to_file(args[0]);
             });
  option.add("o", "output",
             [&player](Option::CallbackArgType args)
             {
               if (args.size() == 0)
               {
                 player.output();
               }
               else if (args.size() == 1)
               {
                 player.output(std::stoi(args[0]));
               }
               else
               {
                 std::cout << "--output has too many arguments.\n";
               }
             });
  option.add("h", "help",
             [](Option::CallbackArgType args)
             {
               std::cout <<
                         "light - A simple music player by caozhanhao\n"
                         "Usage: light[options...] <arguments>\n"
                         "-s, --server        <PulseAudio server> Set PulseAudio server.\n"
                         "                    (default: PULSE_SERVER)\n"
                         "-i, --input         <music urls/paths>  Push local songs into list.\n"
                         "-c, --cache         <cache path>        Cache the music before\n"
                         "                    (default:cache/)    playing online music.\n"
                         "-o, --output                            Output songs from list in order.\n"
                         "-f, --file-output   <filename>          Output will be a wav file \n"
                         "                                        instead of playing\n"
                         "--no-bar                                With no bar.\n"
                         "--example                               See some examples.\n"
                         "-h, --help                              Get this help.\n"
                         "\n"
                         "While light is playing songs, you can use space key to pause or continue.\n"
                         "Use 'q' or Ctrl-C to quit.\n"
                         << std::endl;
             });
  option.add("example",
             [](Option::CallbackArgType args)
             {
               std::cout <<
                         "light -i a.mp3 -f a.wav -o            a.mp3 -> a.wav\n"
                         "light a.mp3 -s xxx.xxx.xxx\n"
                         "or light -io a.mp3 -s xxx.xxx.xxx     Play a.mp3 in server xxx.xxx.xxx\n"
                         << std::endl;
             });
  option.parse();
  option.run();
  return 0;
}