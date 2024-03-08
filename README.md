For the official Endless-Sky project and readme visit [https://github.com/endless-sky/endless-sky/](https://github.com/endless-sky/endless-sky/).
This repository is for a demonstration purposes only with the purpose of showing how a project could use dolt to manage
its data such as configuration. This demo currently only works on Intel macs.

## Building

Start by cloning the dolthub endless-sky fork:

`git clone git@github.com:dolthub/endless-sky.git`

Then clone the endless-sky database from dolthub. I like to have the source and data together so I clone the database
into the newly created endless-sky folder.  The database is expected to be named `datadb` so you can clone it like so:

`dolt clone dolthub/endless-sky endless-sky/datadb`

Then build the game:

```bash
cmake . --preset macos
cmake --build . --preset macos-debug
```

## Running
Start a dolt sql-server instance serving the database cloned into the `endless-sky/datadb` folder with the user `dolt` (no password):

```bash
dolt sql-server -H127.0.0.1 -udolt
```

Then run the game:

```bash
./build/macos/Debug/endless-sky
```


    