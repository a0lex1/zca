## zca - Cross-platform PKI-aware modular-based C&C framework written in C++ using boost      

Zca helps to control remote machines in PKI-aware maneer so that the private key (which signs commands
and is used to validate SSL certs of forwarded ports) is only available at the frontend part.
Therefore, backend takeover won't lead to taking over the controlled agents.
All forwarded traffic is multiplexed into the one connection.

Zca has the following abilities:
  - executing shell commands on agents
  - forwarding any ports and pipes from agent though backend to frontend
  - sending/receiving files
  - viewing detailed pretty-printed list of agents
  - all this is done by piping several base commands toghether (pipe, start-bounce, etc.)
  - after every command execution, bot changes salt to mitigate replay attack ('sequential command concept')

### Physical components and their interaction
```
      has admin public key                                      has admin private key
         _________                  _________                   ______________    ssl-unwrapped
        |         |                |         |<--tcp port map--|              |<--tcp port map--  _____
        |         |                |         |                 |              |                  |
PARTS:  |  AGENT  | <--CC[ vlan ]>-| BACKEND |<-~-~-~-~-~-~-~-~|   FRONTEND   |<-----------------|ADMIN
        |         |       (salt)   |         |parallel netshell|              |  serial netshell |
        |         |                |         |                 |              |                  |_____
        |_________|                |_________|                 |______________|                  
             |
         ____v____
        |         |
        |  tcp:// | forwarded ports&pipes
        |  pipe:/ |
          
```

### Commands
   
```   
MODULES   |                                        |                                             |
==============================================--[CLASSES AND THEIR COMMANDS]=====================+(END USER INTERFACE)===============
   |parts->Agent                                   | Backend                                     | Frontend
   |      |AgentCore                               | BackendCore                                 | FrontendCore
   |      |AgentConfig                             | BackendConfig                               | FrontendConfig
   v      |                                        |                                             |
 _________+________________________________________+_____________________________________________+_____________________________________
          |                                        |                                             |                                     
dummy     |DummyAgentModule                        |DummyBackendModule                           |DummyFrontendModule                  
basecmd   |BasecmdAgentModule                      |BasecmdBackendModule                         |BasecmdFrontendModule                
          |  echo-args                             |  cmd-exec <bid> echo-args 1 2 3             |  cmd-exec <bid> [-W] echo-args 1 2 3
          |                                        |  cmd-exec <bid> shell [-tT] <cmdline>       |  *cmd-exec <bid> <cmdline> ^^ SIG   
          |  shell [-tT] <cmdline>                 |  bot-count                                  |                                     
          |                                        |  bot-kill <bid>                             |  *sync                              
          |                                        |  exit                                       |  *exit                              
          |                                        |  bot_list [-n -c -wW -hH -iI -eE -aA -bB]   |  *bot-list [-n -c -wW -hH -iI -eE -aA -bB]
          |  cmd-exec <bid> echo-args              |                                             |                                      
          |  cmd-exec <bid>                        |                                             |                                      
          |                                        |                                             |  *foreach-bot [-wW -hH] <cmd>        
          |                                        |                                             |                                      
net       |NetAgentModule                          |NetBackendModule                             |NetFrontendModule                     
          |  *pipe                                 |  *pipe                                      |  *pipe                               
          |  *start-bounce                         |  *start-bounce                              |  *start-bounce                       
          |                                        |                                             |                                      
          |                                        |                                             |  *map                                
          |                                        |                                             |                                      
filemgr   |FilemgrAgentModule                      |FilemgrBackendModule                         |FilemgrFrontendModule                 
          |<adds storage >                         |                                             |  *file-get                           
          |                                        |                                             |  *file-put                           
          |                                        |                                             |  *file-cat                           
          |                                        |                                             |  *list-file-hashes                   
          |                                        |                                             |  *update-file-hashes                 
          |

-----------------------------------------------------------------------------------------
```


## Patterns

Builder

[./libs/zca/src/zca/buildable.h](./libs/zca/src/zca/buildable.h)

Core + Modules

[./libs/zca/src/zca/core_facade.h](./libs/zca/src/zca/core_facade.h)

[./libs/zca/src/zca/core/ag/agent_core.h](./libs/zca/src/zca/core/ag/agent_core.h)

[./libs/zca/src/zca/core/back/backend_core.h](./libs/zca/src/zca/core/back/backend_core.h)

[./libs/zca/src/zca/core/front/frontend_core.h](./libs/zca/src/zca/core/front/frontend_core.h)



### Building

Zca is a crossplatform CMake project. Just use QtCreator (Linux) or Visual Studio (Windows) to build.


### Testing

  - all unit tests: ./ztests


### Testing shell command execution

First, execute backend:

```./zbackend.exe --adminuri=tcp://127.0.0.1:10000 --ccuri=tcp://127.0.0.1:20000
[INFO ];(TID 19776) Log initialized
[INFO ];(TID 19776) Parsing parameters
[INFO ];(TID 19776) Initializing configs
[INFO ];(TID 19776) Local admin address  : [tcp://] 127.0.0.1:10000
[INFO ];(TID 19776) Local bot address    : [tcp://] 127.0.0.1:20000
[INFO ];(TID 19776) Num threads default  : 24
[INFO ];(TID 19776) Adding modules
[INFO ];(TID 19776) Starting backend...
[INFO ];(TID 19776) Running thread model...
```

Next, connect one agent:
```./zagent --remuri=tcp://127.0.0.1:20000

[INFO ];(TID 17996) Log initialized
[INFO ];(TID 17996) Parsing parameters
[INFO ];(TID 17996) Initializing configs
[INFO ];(TID 17996) Remote address      : [tcp://] 127.0.0.1:20000
[INFO ];(TID 17996) Num threads default : 24
[INFO ];(TID 17996) Adding modules
[INFO ];(TID 17996) Starting agent...
[INFO ];(TID 17996) Running thread model...
```


Next, connect frontend to backend:
```
zfront.exe --back-bot-addr=tcp://127.0.0.1:10000 --front-admin-addr=tcp://127.0.0.1:30000
[INFO ];(TID 8076)  Using 24 threads
[INFO ];(TID 8076)  Backend CC addr:    127.0.0.1:10000
[INFO ];(TID 8076)  Frontend admin addr:    127.0.0.1:30000
[INFO ] (TID 8076)  [RunLoop] Doing DoWrappedStart()
```

Now you can connect with netcat: `nc localhost 30000`

Type `bot-list`


```
$ nc localhost 30000
bot-list
200,CMD_OK,0,TEXT,4
| bid                              | ip              | ver   | cmdstate | cmd   | cmdres | postcmd | postres | neterr | netshellerr | salt  |
| :----                            | :----           | :---- | :----    | :---- | :----  | :----   | :----   | :----  | :----       | :---- |
| 06f9a3cc2b6bd63f8675e0e944a7f52a | 127.0.0.1:52658 | 0.1   | NONE     |       | -      |         | -       | -      | --          | MA==  |
```

Now execute a command:

```cmd-exec 06f9a3cc2b6bd63f8675e0e944a7f52a shell whoami
200,CMD_OK,0,TEXT,2
desktop-7sg30e9\user
```

You can now see the result of the last command in `bot-list` table:

```
bot-list
200,CMD_OK,0,TEXT,6
| bid                              | ip              | ver   | cmdstate     | cmd          | cmdres                | postcmd | postres | neterr | netshellerr | salt  |
| :----                            | :----           | :---- | :----        | :----        | :----                 | :----   | :----   | :----  | :----       | :---- |
| 06f9a3cc2b6bd63f8675e0e944a7f52a | 127.0.0.1:52658 | 0.1   | DONE_NO_POST | shell whoami | 200,CMD_OK,0,TEXT,2   |         | -       | -      | --          | MQ==  |
|                                  |                 |       |              |              | desktop-7sg30e9\user  |         |         |        |             |       |
|                                  |                 |       |              |              |                       |         |         |        |             |       |
```

## Other commands are not available in public version of zca
